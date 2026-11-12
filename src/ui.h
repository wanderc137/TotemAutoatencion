#ifndef UI_H
#define UI_H

#include <windows.h>
#include "llamados.h"

/* Bitmaps de botones (cargados desde recursos) */
extern HBITMAP g_bmpVerde;
extern HBITMAP g_bmpAzul;
extern HBITMAP g_bmpRojo;

void UI_DrawBackground(HDC hdc);
void UI_DrawOvalButton(HDC hdc, int index, int hover, int press);
int  UI_HitTestButton(int x, int y);
void UI_DrawTicketPreview(HDC hdc, const char* numero, const char* criterio);
void UI_DrawLlamadosPanel(HDC hdc, Llamado* llamados, int count);
void UI_DrawWaitingMessage(HDC hdc);

#endif
