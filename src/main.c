#define _WIN32_WINNT 0x0600
#include <windows.h>
#include <stdio.h>
#include "config.h"
#include "ticket.h"
#include "printer.h"
#include "llamados.h"
#include "ui.h"

/* ─── Estados de la aplicación ─── */
typedef enum {
    TICKET_PREVIEW,
    LLAMADOS_PANEL
} AppState;

/* ─── Variables globales ─── */
static AppState  g_state          = LLAMADOS_PANEL;
static char      g_lastNumero[32] = "";
static char      g_lastCriterio[64] = "";
static Llamado*  g_llamados       = NULL;
static int       g_llamadosCount  = 0;
static int       g_hoverBtn       = -1;
static HINSTANCE g_hInst          = NULL;

#define ID_TIMER_PREVIEW   1
#define ID_TIMER_LLAMADOS  2

static const char* LLAMADOS_FILE = "llamados.txt";

/* ─── Datos de los servicios ─── */
static const char* PREFIJOS[]  = { "A", "B", "C" };
static const char* CRITERIOS[] = {
    "Atencion General",
    "Creditos",
    "Preferencial"
};

/* ═══════════════════════════════════════════════════
   Cargar / Liberar llamados
   ═══════════════════════════════════════════════════ */
static void CargarLlamados(void)
{
    if (g_llamados) {
        Llamados_Free(g_llamados, g_llamadosCount);
        g_llamados      = NULL;
        g_llamadosCount = 0;
    }
    Llamados_Load(LLAMADOS_FILE, &g_llamados, &g_llamadosCount);
}

/* ═══════════════════════════════════════════════════
   Acción al presionar un botón
   ═══════════════════════════════════════════════════ */
static void AlPresionarBoton(HWND hwnd, int index)
{
    if (index < 0 || index >= BUTTON_COUNT)
        return;

    /* Generar número de ticket */
    char numero[32];
    Ticket_Generate(PREFIJOS[index], numero, sizeof(numero));

    /* Guardar para mostrar en preview */
    strncpy(g_lastNumero, numero, sizeof(g_lastNumero) - 1);
    g_lastNumero[sizeof(g_lastNumero) - 1] = '\0';
    strncpy(g_lastCriterio, CRITERIOS[index], sizeof(g_lastCriterio) - 1);
    g_lastCriterio[sizeof(g_lastCriterio) - 1] = '\0';

    /* Formatear e imprimir ticket */
    char ticketText[512];
    Ticket_FormatOutput(ticketText, sizeof(ticketText), numero, CRITERIOS[index]);

    if (!Printer_PrintTicket(ticketText)) {
        /* Si falla la impresión, mostrar aviso temporal */
        MessageBoxA(hwnd,
            "No se pudo imprimir el ticket.\n"
            "Verifique que la impresora esté conectada\n"
            "y configurada como predeterminada.",
            "Aviso de impresora", MB_ICONWARNING | MB_OK);
    }

    /* Cambiar a vista previa del ticket */
    g_state = TICKET_PREVIEW;
    KillTimer(hwnd, ID_TIMER_LLAMADOS);
    SetTimer(hwnd, ID_TIMER_PREVIEW, PREVIEW_DURATION_MS, NULL);
    InvalidateRect(hwnd, NULL, TRUE);
    UpdateWindow(hwnd);
}

/* ═══════════════════════════════════════════════════
   WINDOW PROCEDURE
   ═══════════════════════════════════════════════════ */
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {

    case WM_CREATE:
        /* Cargar bitmaps de botones desde recursos */
        g_bmpVerde = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_VERDE));
        g_bmpAzul  = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_AZUL));
        g_bmpRojo  = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_ROJO));
        Ticket_CheckDailyReset();
        CargarLlamados();
        SetTimer(hwnd, ID_TIMER_LLAMADOS, LLAMADOS_REFRESH_MS, NULL);
        break;

    case WM_ERASEBKGND:
        return 1; /* Evitar flicker */

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        UI_DrawBackground(hdc);

        if (g_state == TICKET_PREVIEW)
            UI_DrawTicketPreview(hdc, g_lastNumero, g_lastCriterio);
        else
            UI_DrawLlamadosPanel(hdc, g_llamados, g_llamadosCount);

        /* Dibujar botones (sin hover/press visual por ahora) */
        for (int i = 0; i < BUTTON_COUNT; i++)
            UI_DrawOvalButton(hdc, i, (g_hoverBtn == i), 0);

        EndPaint(hwnd, &ps);
        break;
    }

    case WM_MOUSEMOVE: {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);
        int newHover = UI_HitTestButton(x, y);
        if (newHover != g_hoverBtn) {
            g_hoverBtn = newHover;
            InvalidateRect(hwnd, NULL, TRUE);
            UpdateWindow(hwnd);
            /* Cambiar cursor a mano si está sobre un botón */
            SetCursor(LoadCursor(NULL, newHover >= 0 ? IDC_HAND : IDC_ARROW));
        }
        break;
    }

    case WM_LBUTTONDOWN: {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);
        int btn = UI_HitTestButton(x, y);
        if (btn >= 0) {
            /* Efecto de presión: redibujar con press=1 */
            g_hoverBtn = btn;
            InvalidateRect(hwnd, NULL, TRUE);
            UpdateWindow(hwnd);
            Sleep(100); /* Breve efecto visual de press */

            AlPresionarBoton(hwnd, btn);
            g_hoverBtn = -1;
        }
        break;
    }

    case WM_SETCURSOR:
        if (g_hoverBtn >= 0) {
            SetCursor(LoadCursor(NULL, IDC_HAND));
            return TRUE;
        }
        break;

    case WM_TIMER:
        if (wParam == ID_TIMER_PREVIEW) {
            KillTimer(hwnd, ID_TIMER_PREVIEW);
            g_state = LLAMADOS_PANEL;
            SetTimer(hwnd, ID_TIMER_LLAMADOS, LLAMADOS_REFRESH_MS, NULL);
            InvalidateRect(hwnd, NULL, TRUE);
        } else if (wParam == ID_TIMER_LLAMADOS) {
            CargarLlamados();
            InvalidateRect(hwnd, NULL, TRUE);
        }
        break;

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE || wParam == VK_F4) {
            /* ESC o F4 para salir */
            DestroyWindow(hwnd);
        }
        break;

    case WM_DESTROY:
        /* Liberar bitmaps */
        if (g_bmpVerde) DeleteObject(g_bmpVerde);
        if (g_bmpAzul)  DeleteObject(g_bmpAzul);
        if (g_bmpRojo)  DeleteObject(g_bmpRojo);
        if (g_llamados)
            Llamados_Free(g_llamados, g_llamadosCount);
        KillTimer(hwnd, ID_TIMER_PREVIEW);
        KillTimer(hwnd, ID_TIMER_LLAMADOS);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProcA(hwnd, msg, wParam, lParam);
    }
    return 0;
}

/* ═══════════════════════════════════════════════════
   WINMAIN — Punto de entrada
   ═══════════════════════════════════════════════════ */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;
    g_hInst = hInstance;

    /* Registrar clase de ventana */
    WNDCLASSA wc;
    ZeroMemory(&wc, sizeof(wc));
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    wc.lpszClassName = "TotemAutoatencionClass";

    if (!RegisterClassA(&wc)) {
        MessageBoxA(NULL, "Error al registrar la ventana.", "Error", MB_ICONERROR);
        return 1;
    }

    /* Crear ventana fullscreen sin bordes */
    HWND hwnd = CreateWindowExA(
        0, "TotemAutoatencionClass",
        "Tótem de Autoatención",
        WS_POPUP | WS_VISIBLE,
        0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
        NULL, NULL, hInstance, NULL);

    if (!hwnd) {
        MessageBoxA(NULL, "Error al crear la ventana.", "Error", MB_ICONERROR);
        return 1;
    }

    /* Buscar monitor con resolución adecuada */
    BOOL found = FALSE;
    DISPLAY_DEVICEA dd;
    dd.cb = sizeof(dd);
    for (DWORD i = 0; EnumDisplayDevicesA(NULL, i, &dd, 0); i++) {
        if (!(dd.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP))
            continue;
        if (!(dd.StateFlags & DISPLAY_DEVICE_ACTIVE))
            continue;

        DEVMODEA dm;
        ZeroMemory(&dm, sizeof(dm));
        dm.dmSize = sizeof(dm);
        if (!EnumDisplaySettingsExA(dd.DeviceName, ENUM_CURRENT_SETTINGS, &dm, 0))
            continue;

        LONG w = dm.dmPelsWidth;
        LONG h = dm.dmPelsHeight;

        /* Buscar monitor vertical 1080x1920 o similar */
        if ((w >= SCREEN_WIDTH && h >= SCREEN_HEIGHT) ||
            (h >= SCREEN_WIDTH && w >= SCREEN_HEIGHT)) {
            int x = dm.dmPosition.x + (w - SCREEN_WIDTH) / 2;
            int y = dm.dmPosition.y + (h - SCREEN_HEIGHT) / 2;
            SetWindowPos(hwnd, HWND_TOPMOST, x, y, SCREEN_WIDTH, SCREEN_HEIGHT,
                         SWP_SHOWWINDOW);
            found = TRUE;
            break;
        }
    }

    if (!found) {
        /* Fallback: usar monitor primario */
        HMONITOR hMon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY);
        MONITORINFO mi;
        mi.cbSize = sizeof(mi);
        if (GetMonitorInfoA(hMon, &mi)) {
            LONG mw = mi.rcMonitor.right  - mi.rcMonitor.left;
            LONG mh = mi.rcMonitor.bottom - mi.rcMonitor.top;
            int x = mi.rcMonitor.left + (mw - SCREEN_WIDTH) / 2;
            int y = mi.rcMonitor.top  + (mh - SCREEN_HEIGHT) / 2;
            if (x < 0) x = 0;
            if (y < 0) y = 0;
            SetWindowPos(hwnd, HWND_TOPMOST, x, y, SCREEN_WIDTH, SCREEN_HEIGHT,
                         SWP_SHOWWINDOW);
        } else {
            ShowWindow(hwnd, SW_SHOWMAXIMIZED);
        }
    }

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    /* Bucle de mensajes */
    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    return (int)msg.wParam;
}
