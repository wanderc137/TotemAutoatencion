#include <windows.h>
#include <winspool.h>
#include <stdio.h>
#include <string.h>
#include "printer.h"

/*
 * printer.c — Impresion ESC/POS directa a la Masung MS800
 *
 * En vez de usar GDI (que requiere driver instalado), enviamos
 * comandos ESC/POS directamente al puerto COM/USB virtual.
 * Esto funciona SIEMPRE que la impresora este conectada por USB.
 *
 * La MS800 aparece como puerto COM virtual cuando se conecta por USB.
 * Buscamos automaticamente el puerto COM correcto.
 */

/* ═══════════════════════════════════════════════════
   Buscar puerto COM de la impresora Masung
   ═══════════════════════════════════════════════════ */
static HANDLE FindPrinterPort(void)
{
    /* Intentar puertos COM del 1 al 20 */
    for (int i = 1; i <= 20; i++) {
        char portName[16];
        snprintf(portName, sizeof(portName), "\\\\.\\COM%d", i);

        HANDLE h = CreateFileA(portName, GENERIC_READ | GENERIC_WRITE,
                               0, NULL, OPEN_EXISTING, 0, NULL);
        if (h != INVALID_HANDLE_VALUE) {
            /* Configurar COM: 9600 baud, 8N1 */
            DCB dcb;
            ZeroMemory(&dcb, sizeof(dcb));
            dcb.DCBlength = sizeof(dcb);
            dcb.BaudRate = CBR_9600;
            dcb.ByteSize = 8;
            dcb.Parity = NOPARITY;
            dcb.StopBits = ONESTOPBIT;
            dcb.fDtrControl = DTR_CONTROL_ENABLE;
            dcb.fRtsControl = RTS_CONTROL_ENABLE;

            if (SetCommState(h, &dcb)) {
                COMMTIMEOUTS ct;
                ZeroMemory(&ct, sizeof(ct));
                ct.ReadIntervalTimeout = 100;
                ct.ReadTotalTimeoutConstant = 100;
                SetCommTimeouts(h, &ct);
                return h;
            }
            CloseHandle(h);
        }
    }

    /* Intentar impresora predeterminada via OpenPrinter */
    char printerName[256];
    DWORD size = sizeof(printerName);
    if (GetDefaultPrinterA(printerName, &size)) {
        HANDLE hPrinter;
        if (OpenPrinterA(printerName, &hPrinter, NULL)) {
            /* Obtener informacion del puerto */
            DWORD needed = 0;
            GetPrinterA(hPrinter, 2, NULL, 0, &needed);
            if (needed > 0) {
                BYTE* buffer = (BYTE*)malloc(needed);
                if (GetPrinterA(hPrinter, 2, buffer, needed, &needed)) {
                    PRINTER_INFO_2A* pi = (PRINTER_INFO_2A*)buffer;
                    if (pi->pPortName) {
                        /* Intentar abrir el puerto directamente */
                        char fullPort[256];
                        snprintf(fullPort, sizeof(fullPort), "\\\\.\\%s", pi->pPortName);
                        ClosePrinter(hPrinter);
                        free(buffer);

                        HANDLE hPort = CreateFileA(fullPort, GENERIC_READ | GENERIC_WRITE,
                                                   0, NULL, OPEN_EXISTING, 0, NULL);
                        if (hPort != INVALID_HANDLE_VALUE) {
                            DCB dcb;
                            ZeroMemory(&dcb, sizeof(dcb));
                            dcb.DCBlength = sizeof(dcb);
                            dcb.BaudRate = CBR_9600;
                            dcb.ByteSize = 8;
                            dcb.Parity = NOPARITY;
                            dcb.StopBits = ONESTOPBIT;
                            dcb.fDtrControl = DTR_CONTROL_ENABLE;
                            SetCommState(hPort, &dcb);

                            COMMTIMEOUTS ct;
                            ZeroMemory(&ct, sizeof(ct));
                            ct.ReadIntervalTimeout = 100;
                            ct.ReadTotalTimeoutConstant = 100;
                            SetCommTimeouts(hPort, &ct);
                            return hPort;
                        }
                    }
                    free(buffer);
                }
                ClosePrinter(hPrinter);
            } else {
                ClosePrinter(hPrinter);
            }
        }
    }

    return INVALID_HANDLE_VALUE;
}

/* ═══════════════════════════════════════════════════
   Enviar datos crudos al puerto
   ═══════════════════════════════════════════════════ */
static int SendRaw(HANDLE hPort, const BYTE* data, int len)
{
    DWORD written = 0;
    return WriteFile(hPort, data, (DWORD)len, &written, NULL) ? (int)written : -1;
}

/* SendStr disponible si se necesita en el futuro */

/* ═══════════════════════════════════════════════════
   IMPRIMIR TICKET — Comandos ESC/POS para Masung MS800
   ═══════════════════════════════════════════════════ */
int Printer_PrintTicket(const char* text)
{
    HANDLE hPort = FindPrinterPort();
    if (hPort == INVALID_HANDLE_VALUE)
        return 0;

    int ok = 1;

    /* ─── 1. Inicializar impresora ─── */
    BYTE init[] = { 0x1B, 0x40 };  /* ESC @ = Initialize */
    SendRaw(hPort, init, sizeof(init));

    /* Pequeña pausa para que la impresora responda */
    Sleep(50);

    /* ─── 2. Configurar impresora ─── */
    BYTE linespace[] = { 0x1B, 0x33, 0x18 };  /* ESC 3 x18 = line spacing 3mm */
    SendRaw(hPort, linespace, sizeof(linespace));

    BYTE codepage[] = { 0x1B, 0x74, 0x0F };   /* ESC t 15 = PC1252 (Latin) */
    SendRaw(hPort, codepage, sizeof(codepage));

    /* ─── 3. Imprimir cada linea del ticket ─── */
    const char* line = text;

    while (*line) {
        const char* end = strchr(line, '\n');
        int len = end ? (int)(end - line) : (int)strlen(line);

        if (len == 0) {
            /* Linea vacia: feed 1 linea */
            BYTE feed[] = { 0x1B, 0x64, 0x01 };
            SendRaw(hPort, feed, sizeof(feed));
            if (!end) break;
            line = end + 1;
            continue;
        }

        /* Detectar tipo de linea */
        BOOL isSeparator = (strstr(line, "===") != NULL || strstr(line, "---") != NULL);
        BOOL isTitle = (strstr(line, "TOTEM") != NULL || strstr(line, "TICKET") != NULL);
        BOOL isTicketNum = (len >= 3 && len <= 6 &&
                           (line[0] == 'A' || line[0] == 'B' || line[0] == 'C') &&
                           strchr(line, '-') != NULL);

        /* ── Configurar formato por tipo de linea ── */
        if (isTitle || isSeparator) {
            /* Centrado + doble ancho + negrita */
            BYTE align[] = { 0x1B, 0x61, 0x01 };
            BYTE boldOn[] = { 0x1B, 0x47, 0x01 };
            BYTE sizeDouble[] = { 0x1D, 0x21, 0x11 };  /* GS ! = width2 + height2 */
            SendRaw(hPort, align, sizeof(align));
            SendRaw(hPort, boldOn, sizeof(boldOn));
            SendRaw(hPort, sizeDouble, sizeof(sizeDouble));
        } else if (isTicketNum) {
            /* Centrado + triple alto + negrita */
            BYTE align[] = { 0x1B, 0x61, 0x01 };
            BYTE boldOn[] = { 0x1B, 0x47, 0x01 };
            BYTE sizeBig[] = { 0x1D, 0x21, 0x22 };  /* width3 + height3 */
            SendRaw(hPort, align, sizeof(align));
            SendRaw(hPort, boldOn, sizeof(boldOn));
            SendRaw(hPort, sizeBig, sizeof(sizeBig));
        } else {
            /* Normal: izquierda, sin negrita, tamano normal */
            BYTE align[] = { 0x1B, 0x61, 0x00 };
            BYTE boldOff[] = { 0x1B, 0x47, 0x00 };
            BYTE sizeNormal[] = { 0x1D, 0x21, 0x00 };
            SendRaw(hPort, align, sizeof(align));
            SendRaw(hPort, boldOff, sizeof(boldOff));
            SendRaw(hPort, sizeNormal, sizeof(sizeNormal));
        }

        /* ── Enviar texto ── */
        SendRaw(hPort, (const BYTE*)line, len);

        /* ── Salto de linea ── */
        BYTE lf[] = { 0x0A };
        SendRaw(hPort, lf, sizeof(lf));

        /* Reset a normal despues de linea especial */
        if (isSeparator || isTitle || isTicketNum) {
            BYTE sizeReset[] = { 0x1D, 0x21, 0x00 };
            BYTE alignLeft[] = { 0x1B, 0x61, 0x00 };
            SendRaw(hPort, sizeReset, sizeof(sizeReset));
            SendRaw(hPort, alignLeft, sizeof(alignLeft));
        }

        if (!end) break;
        line = end + 1;
    }

    /* ─── 4. Feed final + corte ─── */
    BYTE feedFinal[] = { 0x1B, 0x64, 0x03 };  /* Feed 3 lineas */
    SendRaw(hPort, feedFinal, sizeof(feedFinal));
    Sleep(50);

    BYTE cut[] = { 0x1B, 0x69 };  /* ESC i = Full cut */
    SendRaw(hPort, cut, sizeof(cut));

    Sleep(100); /* Esperar corte */

    CloseHandle(hPort);
    return ok;
}

/* ═══════════════════════════════════════════════════
   Verificar si hay impresora disponible
   ═══════════════════════════════════════════════════ */
int Printer_IsAvailable(void)
{
    HANDLE hPort = FindPrinterPort();
    if (hPort == INVALID_HANDLE_VALUE)
        return 0;
    CloseHandle(hPort);
    return 1;
}
