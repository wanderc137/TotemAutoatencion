#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "ticket.h"
#include "config.h"

typedef struct {
    char fecha[9];
    int  ultimoA;
    int  ultimoB;
    int  ultimoC;
} Contadores;

static const char* CONTADORES_FILE = "contadores.dat";

void Ticket_CheckDailyReset(void)
{
    char hoy[9];
    GetDateFormatA(LOCALE_SYSTEM_DEFAULT, 0, NULL, "yyyyMMdd", hoy, sizeof(hoy));

    Contadores cont;
    FILE* f = fopen(CONTADORES_FILE, "rb");
    if (f) {
        fread(&cont, sizeof(Contadores), 1, f);
        fclose(f);

        if (strcmp(cont.fecha, hoy) != 0) {
            memset(&cont, 0, sizeof(Contadores));
            memcpy(cont.fecha, hoy, 8);
            cont.fecha[8] = '\0';
            f = fopen(CONTADORES_FILE, "wb");
            if (f) {
                fwrite(&cont, sizeof(Contadores), 1, f);
                fclose(f);
            }
        }
    } else {
        memset(&cont, 0, sizeof(Contadores));
        memcpy(cont.fecha, hoy, 8);
        cont.fecha[8] = '\0';
        f = fopen(CONTADORES_FILE, "wb");
        if (f) {
            fwrite(&cont, sizeof(Contadores), 1, f);
            fclose(f);
        }
    }
}

void Ticket_Generate(const char* prefix, char* out, int maxlen)
{
    char hoy[9];
    GetDateFormatA(LOCALE_SYSTEM_DEFAULT, 0, NULL, "yyyyMMdd", hoy, sizeof(hoy));

    Contadores cont;
    FILE* f = fopen(CONTADORES_FILE, "rb");
    if (f) {
        fread(&cont, sizeof(Contadores), 1, f);
        fclose(f);
    } else {
        memset(&cont, 0, sizeof(Contadores));
    }

    if (strcmp(cont.fecha, hoy) != 0) {
        memset(&cont, 0, sizeof(Contadores));
        memcpy(cont.fecha, hoy, 8);
        cont.fecha[8] = '\0';
    }

    int* contador = NULL;
    if (strcmp(prefix, "A") == 0)
        contador = &cont.ultimoA;
    else if (strcmp(prefix, "B") == 0)
        contador = &cont.ultimoB;
    else if (strcmp(prefix, "C") == 0)
        contador = &cont.ultimoC;
    else
        return;

    (*contador)++;

    snprintf(out, maxlen, "%s-%03d", prefix, *contador);

    f = fopen(CONTADORES_FILE, "wb");
    if (f) {
        fwrite(&cont, sizeof(Contadores), 1, f);
        fclose(f);
    }
}

void Ticket_FormatOutput(char* out, int maxlen,
                          const char* numero, const char* criterio)
{
    SYSTEMTIME st;
    GetLocalTime(&st);

    char fecha[64];
    char hora[64];
    GetDateFormatA(LOCALE_SYSTEM_DEFAULT, DATE_LONGDATE, &st, NULL, fecha, sizeof(fecha));
    GetTimeFormatA(LOCALE_SYSTEM_DEFAULT, TIME_NOSECONDS, &st, NULL, hora, sizeof(hora));

    snprintf(out, maxlen,
        "================================\r\n"
        "    TOTEM DE AUTOATENCION\r\n"
        "================================\r\n"
        "\r\n"
        "  Numero: %s\r\n"
        "  Criterio: %s\r\n"
        "\r\n"
        "  Fecha: %s\r\n"
        "  Hora: %s\r\n"
        "\r\n"
        "  Espere su turno.\r\n"
        "================================\r\n",
        numero, criterio, fecha, hora);
}
