#ifndef CONFIG_H
#define CONFIG_H

/* ─── Resolución de pantalla ─── */
#define SCREEN_WIDTH  1080
#define SCREEN_HEIGHT 1920
#define HALF_HEIGHT   960

/* ─── Botones ─── */
#define BUTTON_COUNT   3
#define BUTTON_HEIGHT  200
#define BUTTON_WIDTH   220
#define BUTTON_X       80
#define BUTTON_START_Y 1020
#define BUTTON_GAP     30

/* ─── Temporizadores ─── */
#define PREVIEW_DURATION_MS 2000
#define LLAMADOS_REFRESH_MS 5000

/* ─── Colores principales ─── */
#define COLOR_BG_DARK      RGB(10, 22, 40)
#define COLOR_BG_LIGHT     RGB(26, 42, 74)
#define COLOR_GOLD         RGB(212, 168, 67)
#define COLOR_GOLD_LIGHT   RGB(235, 200, 110)
#define COLOR_GOLD_DARK    RGB(160, 120, 30)
#define COLOR_WHITE        RGB(255, 255, 255)
#define COLOR_GRAY_LIGHT   RGB(140, 160, 190)
#define COLOR_GRAY_DIM     RGB(80, 100, 130)

/* ─── Colores para botones 3D ─── */
#define BTN_TOP           RGB(235, 200, 110)
#define BTN_BOTTOM        RGB(170, 125, 35)
#define BTN_SHADOW        RGB(5, 10, 20)
#define BTN_BORDER        RGB(180, 140, 45)
#define BTN_HOVER_TOP     RGB(245, 220, 150)
#define BTN_HOVER_BOTTOM  RGB(195, 150, 55)
#define BTN_PRESS_TOP     RGB(160, 120, 35)
#define BTN_PRESS_BOTTOM  RGB(130, 95, 25)
#define BTN_TEXT_INSIDE   RGB(20, 15, 5)

/* ─── Colores para tarjetas de llamados ─── */
#define CARD_BG           RGB(18, 32, 58)
#define CARD_BORDER       RGB(40, 65, 110)
#define CARD_GOLD_BG      RGB(30, 25, 15)

/* ─── IDs de recursos (bitmaps de botones) ─── */
#define IDB_VERDE  101
#define IDB_AZUL   102
#define IDB_ROJO   103
#define IDB_NEGRO  104

#endif
