#include <windows.h>
#include <math.h>
#include <stdio.h>
#include "ui.h"
#include "config.h"

/* ─── Bitmaps globales (cargados en main.c) ─── */
HBITMAP g_bmpVerde = NULL;
HBITMAP g_bmpAzul  = NULL;
HBITMAP g_bmpRojo  = NULL;

/* ─── Datos de botones ─── */
static const char* BUTTON_NAMES[BUTTON_COUNT] = {
    "Atencion General",
    "Creditos",
    "Preferencial"
};
static const char* BUTTON_DESCS[BUTTON_COUNT] = {
    "Atencion al cliente y tramites generales",
    "Solicitud y consulta de creditos",
    "Atencion preferencial para adultos mayores\ny personas con discapacidad"
};

/* ─── Fuente ─── */
static HFONT CrearFuente(int altura, int peso, BOOL italic, const char* font)
{
    return CreateFontA(altura, 0, 0, 0, peso,
                       italic ? TRUE : FALSE, FALSE, FALSE,
                       DEFAULT_CHARSET,
                       OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                       CLEARTYPE_QUALITY, DEFAULT_PITCH, font);
}

/* ═══════════════════════════════════════════════════
   FONDO
   ═══════════════════════════════════════════════════ */
void UI_DrawBackground(HDC hdc)
{
    RECT rc = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
    HBRUSH hBrush = CreateSolidBrush(COLOR_BG_DARK);
    FillRect(hdc, &rc, hBrush);
    DeleteObject(hBrush);

    TRIVERTEX vert[2];
    vert[0].x = 0;              vert[0].y = SCREEN_HEIGHT;
    vert[0].Red = 0x1A00; vert[0].Green = 0x2A00; vert[0].Blue = 0x4A00; vert[0].Alpha = 0;
    vert[1].x = SCREEN_WIDTH;   vert[1].y = 0;
    vert[1].Red = 0x0A00; vert[1].Green = 0x1600; vert[1].Blue = 0x2800; vert[1].Alpha = 0;
    GRADIENT_RECT gr = { 0, 1 };
    GradientFill(hdc, vert, 2, &gr, 1, GRADIENT_FILL_RECT_V);

    HPEN hPen = CreatePen(PS_SOLID, 2, COLOR_GOLD);
    HGDIOBJ oldPen = SelectObject(hdc, hPen);
    MoveToEx(hdc, 60, HALF_HEIGHT, NULL);
    LineTo(hdc, SCREEN_WIDTH - 60, HALF_HEIGHT);
    SelectObject(hdc, oldPen);
    DeleteObject(hPen);

    HFONT hFontSub = CrearFuente(18, FW_NORMAL, FALSE, "Segoe UI");
    HGDIOBJ oldFont = SelectObject(hdc, hFontSub);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, COLOR_GRAY_DIM);
    RECT rcSub = { 0, SCREEN_HEIGHT - 50, SCREEN_WIDTH, SCREEN_HEIGHT - 20 };
    DrawTextA(hdc, "Seleccione su tipo de atencion", -1, &rcSub,
              DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hdc, oldFont);
    DeleteObject(hFontSub);
}

/* ═══════════════════════════════════════════════════
   BOTON CON IMAGEN BITMAP
   Dibuja la imagen escalada + texto a la derecha
   ═══════════════════════════════════════════════════ */
void UI_DrawOvalButton(HDC hdc, int index, int hover, int press)
{
    int x  = BUTTON_X;
    int y  = BUTTON_START_Y + index * (BUTTON_HEIGHT + BUTTON_GAP);
    int w  = BUTTON_WIDTH;
    int h  = BUTTON_HEIGHT;
    int dy = press ? 5 : 0;

    /* Seleccionar bitmap segun el boton */
    HBITMAP hBmp = NULL;
    switch (index) {
        case 0: hBmp = g_bmpVerde; break;
        case 1: hBmp = g_bmpAzul;  break;
        case 2: hBmp = g_bmpRojo;  break;
    }

    if (hBmp) {
        /* Dibujar imagen del boton escalada al tamano del boton */
        HDC hdcMem = CreateCompatibleDC(hdc);
        HGDIOBJ oldBmp = SelectObject(hdcMem, hBmp);

        /* Efecto de presion: desplazar hacia abajo */
        int drawY = y + dy;

        /* Sombra sutil detras del boton */
        if (!press) {
            HBRUSH hSh = CreateSolidBrush(RGB(0, 0, 0));
            RECT rcSh = { x + 4, drawY + 6, x + w + 4, drawY + h + 6 };
            /* Usar region redondeada para la sombra */
            HRGN hRgnSh = CreateRoundRectRgn(x + 4, drawY + 6, x + w + 4, drawY + h + 6, 30, 30);
            SelectClipRgn(hdc, hRgnSh);
            FillRect(hdc, &rcSh, hSh);
            SelectClipRgn(hdc, NULL);
            DeleteObject(hRgnSh);
            DeleteObject(hSh);
        }

        /* Dibujar imagen con transparencia */
        SetStretchBltMode(hdc, HALFTONE);
        TransparentBlt(hdc, x, drawY, w, h,
                       hdcMem, 0, 0, 600, 600, RGB(255, 255, 255));

        SelectObject(hdcMem, oldBmp);
        DeleteDC(hdcMem);
    } else {
        /* Fallback: boton programatico si no hay bitmap */
        HRGN hRgn = CreateRoundRectRgn(x, y + dy, x + w, y + h + dy, h, h);
        SelectClipRgn(hdc, hRgn);
        HBRUSH hBr = CreateSolidBrush(COLOR_GOLD);
        RECT rcB = { x, y + dy, x + w, y + h + dy };
        FillRect(hdc, &rcB, hBr);
        SelectClipRgn(hdc, NULL);
        DeleteObject(hRgn);
        DeleteObject(hBr);
    }

    /* ─── TEXTO A LA DERECHA ─── */
    int textX = x + w + 40;
    int textW = SCREEN_WIDTH - textX - 50;
    HGDIOBJ oldFont;

    /* NOMBRE DEL SERVICIO (grande, blanco) */
    HFONT hFontName = CrearFuente(46, FW_BOLD, FALSE, "Segoe UI");
    oldFont = SelectObject(hdc, hFontName);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, COLOR_WHITE);
    RECT rcName = { textX, y + dy + 15, textX + textW, y + h / 2 + dy + 15 };
    DrawTextA(hdc, BUTTON_NAMES[index], -1, &rcName,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hdc, oldFont);
    DeleteObject(hFontName);

    /* DESCRIPCION (mas pequena, gris) */
    HFONT hFontDesc = CrearFuente(22, FW_NORMAL, FALSE, "Segoe UI");
    SelectObject(hdc, hFontDesc);
    SetTextColor(hdc, COLOR_GRAY_LIGHT);
    RECT rcDesc = { textX, y + h / 2 + dy - 5, textX + textW, y + h + dy - 10 };
    DrawTextA(hdc, BUTTON_DESCS[index], -1, &rcDesc,
              DT_LEFT | DT_TOP | DT_WORDBREAK);
    SelectObject(hdc, oldFont);
    DeleteObject(hFontDesc);
}

/* ═══════════════════════════════════════════════════
   HIT TEST
   ═══════════════════════════════════════════════════ */
int UI_HitTestButton(int x, int y)
{
    for (int i = 0; i < BUTTON_COUNT; i++) {
        int bx = BUTTON_X;
        int by = BUTTON_START_Y + i * (BUTTON_HEIGHT + BUTTON_GAP);
        if (x >= bx - 20 && x <= SCREEN_WIDTH - 20 &&
            y >= by - 20 && y <= by + BUTTON_HEIGHT + 20)
            return i;
    }
    return -1;
}

/* ═══════════════════════════════════════════════════
   TICKET PREVIEW
   ═══════════════════════════════════════════════════ */
void UI_DrawTicketPreview(HDC hdc, const char* numero, const char* criterio)
{
    RECT rcTop = { 0, 0, SCREEN_WIDTH, HALF_HEIGHT };
    HBRUSH hBrush = CreateSolidBrush(RGB(8, 16, 32));
    FillRect(hdc, &rcTop, hBrush);
    DeleteObject(hBrush);

    int cardW = 700, cardH = 500;
    int cardX = (SCREEN_WIDTH - cardW) / 2;
    int cardY = 120;

    HBRUSH hSh = CreateSolidBrush(RGB(2, 5, 12));
    RECT rcSh = { cardX + 8, cardY + 8, cardX + cardW + 8, cardY + cardH + 8 };
    FillRect(hdc, &rcSh, hSh);
    DeleteObject(hSh);

    HBRUSH hCard = CreateSolidBrush(CARD_BG);
    RECT rcCard = { cardX, cardY, cardX + cardW, cardY + cardH };
    FillRect(hdc, &rcCard, hCard);
    DeleteObject(hCard);

    HPEN hBrd = CreatePen(PS_SOLID, 2, COLOR_GOLD);
    HBRUSH hNb = (HBRUSH)GetStockObject(NULL_BRUSH);
    HGDIOBJ op = SelectObject(hdc, hBrd);
    HGDIOBJ ob = SelectObject(hdc, hNb);
    RoundRect(hdc, cardX, cardY, cardX + cardW, cardY + cardH, 20, 20);
    SelectObject(hdc, op);
    SelectObject(hdc, ob);
    DeleteObject(hBrd);

    HGDIOBJ oldFont;

    HFONT hf1 = CrearFuente(24, FW_NORMAL, FALSE, "Segoe UI");
    oldFont = SelectObject(hdc, hf1);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, COLOR_GRAY_LIGHT);
    RECT r1 = { cardX, cardY + 25, cardX + cardW, cardY + 65 };
    DrawTextA(hdc, "-- TICKET DE ATENCION --", -1, &r1, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hdc, oldFont);
    DeleteObject(hf1);

    HPEN hLn = CreatePen(PS_SOLID, 1, COLOR_GOLD);
    SelectObject(hdc, hLn);
    MoveToEx(hdc, cardX + 60, cardY + 80, NULL);
    LineTo(hdc, cardX + cardW - 60, cardY + 80);
    SelectObject(hdc, op);
    DeleteObject(hLn);

    HFONT hf2 = CrearFuente(120, FW_BOLD, FALSE, "Segoe UI");
    SelectObject(hdc, hf2);
    SetTextColor(hdc, COLOR_GOLD);
    RECT r2 = { cardX, cardY + 90, cardX + cardW, cardY + 260 };
    DrawTextA(hdc, numero, -1, &r2, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hdc, oldFont);
    DeleteObject(hf2);

    HFONT hf3 = CrearFuente(36, FW_BOLD, FALSE, "Segoe UI");
    SelectObject(hdc, hf3);
    SetTextColor(hdc, COLOR_WHITE);
    RECT r3 = { cardX, cardY + 260, cardX + cardW, cardY + 320 };
    DrawTextA(hdc, criterio, -1, &r3, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hdc, oldFont);
    DeleteObject(hf3);

    SYSTEMTIME st;
    GetLocalTime(&st);
    char fecha[64], hora[64], fh[160];
    GetDateFormatA(LOCALE_SYSTEM_DEFAULT, DATE_LONGDATE, &st, NULL, fecha, sizeof(fecha));
    GetTimeFormatA(LOCALE_SYSTEM_DEFAULT, TIME_NOSECONDS, &st, NULL, hora, sizeof(hora));
    snprintf(fh, sizeof(fh), "%s  -  %s", fecha, hora);

    HFONT hf4 = CrearFuente(22, FW_NORMAL, FALSE, "Segoe UI");
    SelectObject(hdc, hf4);
    SetTextColor(hdc, COLOR_GRAY_LIGHT);
    RECT r4 = { cardX, cardY + 330, cardX + cardW, cardY + 370 };
    DrawTextA(hdc, fh, -1, &r4, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hdc, oldFont);
    DeleteObject(hf4);

    HPEN hLn2 = CreatePen(PS_SOLID, 1, COLOR_GOLD);
    SelectObject(hdc, hLn2);
    MoveToEx(hdc, cardX + 60, cardY + cardH - 90, NULL);
    LineTo(hdc, cardX + cardW - 60, cardY + cardH - 90);
    SelectObject(hdc, op);
    DeleteObject(hLn2);

    HFONT hf5 = CrearFuente(20, FW_NORMAL, TRUE, "Segoe UI");
    SelectObject(hdc, hf5);
    SetTextColor(hdc, COLOR_GOLD);
    RECT r5 = { cardX, cardY + cardH - 80, cardX + cardW, cardY + cardH - 30 };
    DrawTextA(hdc, "Consulte su turno en la pantalla de llamados", -1, &r5,
              DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hdc, oldFont);
    DeleteObject(hf5);
}

/* ═══════════════════════════════════════════════════
   PANEL DE LLAMADOS
   ═══════════════════════════════════════════════════ */
void UI_DrawLlamadosPanel(HDC hdc, Llamado* llamados, int count)
{
    RECT rcTop = { 0, 0, SCREEN_WIDTH, HALF_HEIGHT };
    HBRUSH hBrush = CreateSolidBrush(RGB(8, 16, 32));
    FillRect(hdc, &rcTop, hBrush);
    DeleteObject(hBrush);

    HGDIOBJ oldFont;

    HFONT hf1 = CrearFuente(48, FW_BOLD, FALSE, "Segoe UI");
    oldFont = SelectObject(hdc, hf1);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, COLOR_GOLD);
    RECT r1 = { 0, 30, SCREEN_WIDTH, 110 };
    DrawTextA(hdc, "LLAMADOS ACTIVOS", -1, &r1, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hdc, oldFont);
    DeleteObject(hf1);

    HPEN hLn = CreatePen(PS_SOLID, 2, COLOR_GOLD);
    HGDIOBJ oldPen = SelectObject(hdc, hLn);
    MoveToEx(hdc, 200, 120, NULL);
    LineTo(hdc, SCREEN_WIDTH - 200, 120);
    SelectObject(hdc, oldPen);
    DeleteObject(hLn);

    if (count == 0) {
        UI_DrawWaitingMessage(hdc);
        return;
    }

    HFONT hf2 = CrearFuente(24, FW_NORMAL, FALSE, "Segoe UI");
    SelectObject(hdc, hf2);
    SetTextColor(hdc, COLOR_GRAY_LIGHT);
    RECT r2 = { 0, 130, SCREEN_WIDTH, 170 };
    DrawTextA(hdc, "Dirijase al modulo indicado", -1, &r2,
              DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hdc, oldFont);
    DeleteObject(hf2);

    int maxItems = count < 4 ? count : 4;
    int startY = 190;
    int slotH = (HALF_HEIGHT - startY - 60) / maxItems;
    if (slotH < 160) slotH = 160;

    int cm = 40;

    for (int i = 0; i < maxItems; i++) {
        int cy = startY + i * slotH;

        HBRUSH hCb = CreateSolidBrush(CARD_BG);
        RECT rcC = { cm, cy + 8, SCREEN_WIDTH - cm, cy + slotH - 8 };
        FillRect(hdc, &rcC, hCb);
        DeleteObject(hCb);

        HPEN hCb2 = CreatePen(PS_SOLID, 1, CARD_BORDER);
        HBRUSH hNb = (HBRUSH)GetStockObject(NULL_BRUSH);
        HGDIOBJ op2 = SelectObject(hdc, hCb2);
        HGDIOBJ ob2 = SelectObject(hdc, hNb);
        RoundRect(hdc, cm, cy + 8, SCREEN_WIDTH - cm, cy + slotH - 8, 15, 15);
        SelectObject(hdc, op2);
        SelectObject(hdc, ob2);
        DeleteObject(hCb2);

        HFONT hfN = CrearFuente(72, FW_BOLD, FALSE, "Segoe UI");
        SelectObject(hdc, hfN);
        SetTextColor(hdc, COLOR_GOLD);
        RECT rcN = { cm + 30, cy + 8, cm + 380, cy + slotH - 8 };
        DrawTextA(hdc, llamados[i].numero, -1, &rcN,
                  DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        SelectObject(hdc, oldFont);
        DeleteObject(hfN);

        HPEN hSep = CreatePen(PS_SOLID, 2, COLOR_GOLD);
        SelectObject(hdc, hSep);
        MoveToEx(hdc, cm + 400, cy + 30, NULL);
        LineTo(hdc, cm + 400, cy + slotH - 30);
        SelectObject(hdc, op2);
        DeleteObject(hSep);

        HFONT hfM = CrearFuente(52, FW_BOLD, FALSE, "Segoe UI");
        SelectObject(hdc, hfM);
        SetTextColor(hdc, COLOR_WHITE);
        RECT rcM = { cm + 430, cy + 8, SCREEN_WIDTH - cm - 30, cy + slotH - 8 };
        DrawTextA(hdc, llamados[i].modulo, -1, &rcM,
                  DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        SelectObject(hdc, oldFont);
        DeleteObject(hfM);
    }
}

/* ═══════════════════════════════════════════════════
   MENSAJE SIN LLAMADOS
   ═══════════════════════════════════════════════════ */
void UI_DrawWaitingMessage(HDC hdc)
{
    HFONT hf = CrearFuente(48, FW_NORMAL, FALSE, "Segoe UI");
    HGDIOBJ oldFont = SelectObject(hdc, hf);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, COLOR_GRAY_DIM);
    RECT rc = { 0, HALF_HEIGHT / 2 - 60, SCREEN_WIDTH, HALF_HEIGHT / 2 + 60 };
    DrawTextA(hdc, "Sin llamados activos", -1, &rc,
              DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hdc, oldFont);
    DeleteObject(hf);
}
