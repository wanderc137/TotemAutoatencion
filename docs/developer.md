# Guía del desarrollador — Tótem de Autoatención

Documentación técnica del código fuente. Describe la arquitectura, cada módulo, las funciones públicas, el flujo de mensajes de Win32, la secuencia ESC/POS y las vías para extender el programa.

- **Lenguaje:** C99 (compatible con el dialecto que acepta MinGW-w64 por defecto)
- **API:** Win32 clásica (`windows.h`), sin frameworks
- **Compilador:** `x86_64-w64-mingw32-gcc`
- **Fuente de verdad:** el código en `src/`. Este documento se verificó contra los archivos, no contra notas de diseño previas.

---

## Índice

1. [Filosofía y restricciones de diseño](#1-filosofía-y-restricciones-de-diseño)
2. [Mapa de módulos](#2-mapa-de-módulos)
3. [main.c — el bucle y la máquina de estados](#3-mainc--el-bucle-y-la-máquina-de-estados)
4. [ui.c — dibujo GDI](#4-uic--dibujo-gdi)
5. [ticket.c — numeración y formato](#5-ticketc--numeración-y-formato)
6. [printer.c — ESC/POS sobre puerto serie](#6-printerc--escpos-sobre-puerto-serie)
7. [llamados.c — parser del archivo](#7-llamadosc--parser-del-archivo)
8. [Referencia de la API](#8-referencia-de-la-api)
9. [Flujo completo de un ticket](#9-flujo-completo-de-un-ticket)
10. [Diseño de la interfaz](#10-diseño-de-la-interfaz)
11. [Gestión de recursos y memoria](#11-gestión-de-recursos-y-memoria)
12. [Compilación y depuración](#12-compilación-y-depuración)
13. [Guías de extensión](#13-guías-de-extensión)
14. [Deuda técnica conocida](#14-deuda-técnica-conocida)
15. [Convenciones y estilo](#15-convenciones-y-estilo)

---

## 1. Filosofía y restricciones de diseño

El programa se diseñó con tres restricciones deliberadas:

1. **Cero dependencias externas.** Solo la API de Windows y la biblioteca C estándar. Nada de Qt, .NET, Electron ni frameworks de UI. El binario resultante es autónomo y arranca en un equipo recién instalado.
2. **Un solo hilo.** Toda la lógica corre en el hilo de la interfaz: el bucle de mensajes, el dibujo, la lectura del archivo, el incremento de contadores y la escritura al puerto. No hay `CreateThread`, ni colas, ni sincronización.
3. **Sin estado oculto.** El estado persistente son dos archivos en disco de formato trivial: un binario de 24 bytes y un texto plano. No hay registro, base de datos ni servicio.

La consecuencia es un programa pequeño (≈ 1.500 líneas útiles) y fácil de auditar, a cambio de simplicidad operativa: la impresión bloquea, no hay reintentos y la detección de hardware es heurística.

### Por qué C y no C#

Un tótem arranca con Windows y debe funcionar sin runtime instalado. C con Win32 produce un `.exe` que solo depende de DLLs del sistema (`gdi32`, `user32`, `kernel32`, `msimg32`, `winspool`). C# habría requerido .NET, cuya versión y parches son un problema de mantenimiento en equipos de producción.

### Por qué ESC/POS crudo y no la cola de impresión

La API `StartDocPrinter`/`WritePrinter` exige una impresora instalada con driver. En un equipo de tótem, el driver suele ser un genérico que mete el trabajo en cola, muestra diálogos o aplica márgenes. Escribir bytes ESC/POS al puerto COM evita todo eso: el ticket sale en el momento, con el formato exacto. El precio es que hay que hablar el idioma de la impresora y detectar el puerto a mano.

---

## 2. Mapa de módulos

```
src/
├── config.h      Constantes de pantalla, colores, botones, tiempos e IDs de recurso
├── main.c        WinMain, WndProc, máquina de estados, timers, orquestación
├── ui.h/.c       Dibujo GDI: fondo, botones ovalados, panel de llamados, vista previa
├── ticket.h/.c   Contadores diarios en disco y formato del texto del ticket
├── printer.h/.c  Detección del puerto y envío ESC/POS
└── llamados.h/.c Lectura de llamados.txt a un arreglo dinámico
res/
├── buttons.rc    Declaración de los cuatro bitmaps
└── btn_*.bmp     Recursos gráficos
```

### Dependencias entre módulos

```
main.c ──► ui.c ──► config.h
   │        └────► (GDI, msimg32)
   ├────► ticket.c ──► (winbase: fecha/hora)
   ├────► printer.c ─► (winspool, kernel32: COM)
   └────► llamados.c ► (libc: fopen/malloc)
```

No hay dependencias cruzadas entre `ui`, `ticket`, `printer` y `llamados`: cada uno expone funciones y `main.c` los compone. Eso permite reemplazar cualquiera sin tocar los demás.

### Cabeceras

| Archivo | Contenido |
|---|---|
| `config.h` | `#define` de geometría, colores `RGB()`, IDs `IDB_*`, tiempos |
| `ui.h` | Prototipos de dibujo, `extern HBITMAP` de los bitmaps |
| `ticket.h` | `Ticket_CheckDailyReset`, `Ticket_Generate`, `Ticket_FormatOutput` |
| `printer.h` | `Printer_PrintTicket`, `Printer_IsAvailable` |
| `llamados.h` | `typedef struct Llamado`, `Llamados_Load`, `Llamados_Free` |

---

## 3. main.c — el bucle y la máquina de estados

### Estado global

```c
typedef enum { TICKET_PREVIEW, LLAMADOS_PANEL } AppState;

static AppState g_state = LLAMADOS_PANEL;
static char g_lastNumero[32];
static char g_lastCriterio[64];
static Llamado* g_llamados;
static int g_llamadosCount;
static int g_hoverBtn = -1;
```

El programa no tiene un "modelo" separado: el estado de la vista es el estado global. Es intencional y suficiente para dos pantallas.

### Constantes de tiempo

```c
#define ID_TIMER_PREVIEW  1
#define ID_TIMER_LLAMADOS 2
#define PREVIEW_DURATION_MS 2000
#define LLAMADOS_REFRESH_MS 5000
```

Solo hay un timer activo a la vez: el de vista previa mientras se muestra un ticket, o el de llamados en reposo. Al cambiar de estado se mata uno y se arma el otro; nunca conviven.

### WinMain

1. Registra la clase `"TotemAutoatencionClass"` con `CS_HREDRAW | CS_VREDRAW`, cursor de flecha y `hbrBackground = NULL_BRUSH` (el fondo lo pinta `WM_ERASEBKGND`).
2. Crea la ventana con `CreateWindowExA(0, clase, "Tótem de Autoatención", WS_POPUP | WS_VISIBLE, 0, 0, 1080, 1920, ...)`.
3. **Busca un monitor adecuado.** Recorre con `EnumDisplayDevicesA`, filtra los adjuntos y activos, y consulta el modo actual con `EnumDisplaySettingsExA`. Acepta el monitor si `(ancho >= 1080 && alto >= 1920) || (alto >= 1080 && ancho >= 1920)` — es decir, sirve tanto un panel vertical como uno horizontal que Windows reporte rotado.
4. Si lo encuentra, calcula el origen para centrar la ventana de 1080 × 1920 y la coloca con `SetWindowPos(..., HWND_TOPMOST, ..., SWP_SHOWWINDOW)`.
5. Si no, cae a `MonitorFromWindow(MONITOR_DEFAULTTOPRIMARY)` y centra con límites en 0; como último recurso, `ShowWindow(SW_SHOWMAXIMIZED)`.
6. Entra al bucle `GetMessageA` / `TranslateMessage` / `DispatchMessageA`.

### WndProc, mensaje por mensaje

| Mensaje | Comportamiento |
|---|---|
| `WM_CREATE` | Carga los bitmaps con `LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_*))`; llama `Ticket_CheckDailyReset()` y `CargarLlamados()`; arma el timer de 5 s |
| `WM_ERASEBKGND` | Devuelve 1: evita el borrado del sistema y el parpadeo |
| `WM_PAINT` | `BeginPaint` → `UI_DrawBackground` → según estado `UI_DrawLlamadosPanel` o `UI_DrawTicketPreview` → tres `UI_DrawOvalButton` → `EndPaint` |
| `WM_MOUSEMOVE` | `UI_HitTestButton`; si cambia el índice, repinta e intercambia el cursor entre `IDC_HAND` y `IDC_ARROW` |
| `WM_LBUTTONDOWN` | Hit test; si hay botón: marca hover, repinta, `Sleep(100)` como feedback, `AlPresionarBoton`, limpia hover |
| `WM_SETCURSOR` | `IDC_HAND` si el puntero está sobre un botón |
| `WM_TIMER` (1) | Mata el timer de vista previa, vuelve a `LLAMADOS_PANEL`, arma el de 5 s, invalida |
| `WM_TIMER` (2) | `CargarLlamados()` e invalida |
| `WM_KEYDOWN` | `VK_ESCAPE` o `VK_F4` → `DestroyWindow` |
| `WM_DESTROY` | `DeleteObject` de los tres bitmaps, `Llamados_Free`, mata ambos timers, `PostQuitMessage(0)` |

> `WM_LBUTTONDOWN` usa `Sleep(100)` en el hilo de UI para que el usuario perciba el "hundido" del botón antes de que la pantalla cambie. Es un atajo aceptable en este contexto, pero congela el repintado durante 100 ms.

### `AlPresionarBoton`

```c
static void AlPresionarBoton(HWND hwnd, int index)
{
    char numero[32];
    char ticketText[512];
    // valida index en [0, 3)
    Ticket_Generate(PREFIJOS[index], numero, sizeof(numero));
    strncpy(g_lastNumero, numero, sizeof(g_lastNumero) - 1);
    g_lastNumero[sizeof(g_lastNumero) - 1] = '\0';
    strncpy(g_lastCriterio, CRITERIOS[index], sizeof(g_lastCriterio) - 1);
    g_lastCriterio[sizeof(g_lastCriterio) - 1] = '\0';
    Ticket_FormatOutput(ticketText, 512, numero, g_lastCriterio);

    if (!Printer_PrintTicket(ticketText)) {
        MessageBoxA(hwnd,
            "No se pudo imprimir el ticket.\n"
            "Verifique que la impresora esté conectada\n"
            "y configurada como predeterminada.",
            "Aviso de impresora", MB_ICONWARNING | MB_OK);
    }

    g_state = TICKET_PREVIEW;
    KillTimer(hwnd, ID_TIMER_LLAMADOS);
    SetTimer(hwnd, ID_TIMER_PREVIEW, PREVIEW_DURATION_MS, NULL);
    InvalidateRect(hwnd, NULL, TRUE);
    UpdateWindow(hwnd);
}
```

El orden importa: el contador se incrementa **antes** de imprimir. Si la impresora falla, el número ya está consumido y mostrado en pantalla; es preferible un hueco en la secuencia a dos clientes con el mismo número.

### Carga de llamados

```c
static void CargarLlamados(HWND hwnd)
{
    if (g_llamados) { Llamados_Free(g_llamados, g_llamadosCount); g_llamados = NULL; }
    g_llamadosCount = 0;
    Llamados_Load(LLAMADOS_FILE, &g_llamados, &g_llamadosCount);
}
```

Se libera y se vuelve a leer cada 5 segundos. Con archivos de pocas líneas el costo es despreciable y evita toda la complejidad de detectar cambios con `ReadDirectoryChangesW`.

---

## 4. ui.c — dibujo GDI

Todo el dibujo es inmediato: no hay doble buffer, no hay `CreateCompatibleBitmap`. Se pinta directamente sobre el DC de la ventana. En una pantalla de 1080 × 1920 el parpadeo se evita con `WM_ERASEBKGND → 1` y porque el fondo se repinta completo en cada frame.

### `UI_DrawBackground`

1. `FillRect` con `COLOR_BG_DARK` (`RGB(10, 22, 40)`) sobre todo el cliente.
2. `GradientFill` vertical de `#0A1628` (arriba) a `#1A2A4A` (abajo), con `TRIVERTEX` armados a mano.
3. Línea dorada de 2 px en `y = 960`, de `x = 60` a `1020`.
4. Texto "Seleccione su tipo de atencion" en gris, 18 px, Segoe UI, centrado en la franja `y = 1870…1900`.

### `UI_DrawOvalButton`

```c
void UI_DrawOvalButton(HDC hdc, int index, int hover, int press);
```

Geometría: `x = 80`, `y = 1020 + index * 230`, ancho 220, alto 200. El parámetro `press` desplaza 5 px hacia abajo y omite la sombra.

El botón es un bitmap de 600 × 600 estirado a 220 × 200 con `StretchBlt` en modo `HALFTONE` y transparencia de blanco con `TransparentBlt`. Como no se respeta la relación de aspecto, el círculo original queda ovalado: es el efecto buscado.

A la derecha (`x = 340`) se dibuja el nombre en blanco, 46 px negrita, y la descripción en `COLOR_GRAY_LIGHT`, 22 px, con `DT_WORDBREAK`.

> **El parámetro `hover` no se usa.** Está en la firma por compatibilidad con la intención original de un efecto hover, pero el dibujo no cambia. El cursor sí cambia a mano, lo que da una pista visual mínima.

Si el bitmap no cargó (`NULL`), hay un fallback: rectángulo redondeado relleno de dorado. Así la interfaz sigue siendo usable aunque falten recursos.

### `UI_HitTestButton`

```c
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
```

El área táctil es deliberadamente mayor que el botón: cubre desde `x = 60` hasta `x = 1060` (toda la franja, incluido el texto) y 20 px por arriba y por abajo. En una pantalla táctil de tótem, los dedos no apuntan con precisión.

### `UI_DrawTicketPreview`

Vista de 2 segundos. Borra la mitad superior con `RGB(8, 16, 32)`, dibuja una tarjeta de 700 × 500 centrada en `x = 190`, `y = 120`, con sombra desplazada 8 px, borde dorado redondeado y:

| Elemento | Fuente | Color |
|---|---|---|
| `-- TICKET DE ATENCION --` | 24 px | blanco |
| Número (`A-041`) | 120 px negrita | dorado |
| Criterio | 36 px negrita | blanco |
| Fecha y hora | 22 px | gris claro |
| Pie "Consulte su turno..." | 20 px itálica | dorado |

La fecha se obtiene con `GetLocalTime` + `GetDateFormatA(LOCALE_SYSTEM_DEFAULT, DATE_LONGDATE)` y `GetTimeFormatA(..., TIME_NOSECONDS)`.

### `UI_DrawLlamadosPanel`

Pinta la mitad superior, título "LLAMADOS ACTIVOS" (48 px, dorado), línea dorada, y hasta cuatro tarjetas. El alto de cada tarjeta se calcula como `(960 - 190 - 60) / n`, con piso de 160 px, de modo que con 1 llamado la tarjeta es enorme y con 4 se compacta.

Cada tarjeta: fondo `CARD_BG`, borde `CARD_BORDER` redondeado, número a 72 px en dorado, separador vertical y módulo a 52 px en blanco.

### `UI_DrawWaitingMessage`

Cuando no hay llamados, "Sin llamados activos" en 48 px gris apagado, centrado en `y = 420…540`.

### Fuentes

```c
static HFONT CrearFuente(int altura, int peso, int italic, const char* font)
{
    return CreateFontA(altura, 0, 0, 0, peso, italic, FALSE, FALSE,
                       DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                       CLEARTYPE_QUALITY, DEFAULT_PITCH, font);
}
```

Se crea una `HFONT` por cada texto y **no se guarda**: el patrón habitual es crearla, seleccionarla, dibujar, restaurar y `DeleteObject`. Es correcto pero repetitivo; una caché de fuentes sería la primera optimización si se agregan más textos.

---

## 5. ticket.c — numeración y formato

### Estructura en disco

```c
typedef struct {
    char fecha[9];   // "yyyyMMdd" + '\0'
    int ultimoA;
    int ultimoB;
    int ultimoC;
} Contadores;
```

En x64, `sizeof(Contadores) == 24` por el padding tras `fecha[9]`. El archivo `contadores.dat` contiene exactamente ese registro, escrito con `fwrite`.

### `Ticket_CheckDailyReset`

Se llama una vez al arrancar (`WM_CREATE`). Obtiene la fecha con `GetDateFormatA(..., "yyyyMMdd", ...)`; si el archivo no existe o la fecha guardada difiere, pone los tres contadores a cero y reescribe. Si no puede leer, no falla: trata el archivo como vacío.

### `Ticket_Generate`

```c
void Ticket_Generate(const char* prefix, char* out, int maxlen);
```

1. Lee el registro (o arranca en cero si no existe).
2. Si la fecha cambió respecto de hoy, resetea en memoria.
3. Elige el contador según el prefijo (`A`, `B`, `C`); si el prefijo no es válido, retorna sin escribir.
4. Incrementa y formatea con `snprintf(out, maxlen, "%s-%03d", prefix, contador)`.
5. Reescribe el archivo completo.

No hay bloqueo de archivo: dos instancias del programa sobre la misma carpeta se pisarían los contadores. El diseño asume un solo tótem.

### `Ticket_FormatOutput`

```c
void Ticket_FormatOutput(char* out, int maxlen, const char* numero, const char* criterio);
```

Arma el texto de doce líneas con `\r\n` (necesario para impresoras térmicas) y fecha/hora local. El buffer que usa `main.c` es de 512 bytes y el texto ronda los 250, así que hay margen.

### Formato exacto

```
================================
    TOTEM DE AUTOATENCION
================================

  Numero: %s
  Criterio: %s

  Fecha: %s
  Hora: %s

  Espere su turno.
================================
```

Las líneas con `===` son las que `printer.c` detecta como separadores para aplicar doble ancho.

---

## 6. printer.c — ESC/POS sobre puerto serie

Es el módulo más delicado: habla con hardware que no se puede interrogar de forma estándar.

### `FindPrinterPort`

```c
static HANDLE FindPrinterPort(void);
```

Estrategia en dos fases:

**Fase 1 — barrido de COM1 a COM20.**

```c
for (int i = 1; i <= 20; i++) {
    char portName[16];
    snprintf(portName, sizeof(portName), "\\\\.\\COM%d", i);
    HANDLE h = CreateFileA(portName, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                           OPEN_EXISTING, 0, NULL);
    if (h == INVALID_HANDLE_VALUE) continue;
    DCB dcb = {0};
    dcb.DCBlength = sizeof(dcb);
    GetCommState(h, &dcb);
    dcb.BaudRate = CBR_9600;
    dcb.ByteSize = 8;
    dcb.Parity   = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    dcb.fDtrControl = DTR_CONTROL_ENABLE;
    dcb.fRtsControl = RTS_CONTROL_ENABLE;
    if (SetCommState(h, &dcb)) { /* timeouts, return h */ }
    CloseHandle(h);
}
```

No pregunta qué dispositivo es. El primer puerto que acepte 9600 8N1 gana. En un equipo con otros dispositivos serie (lector de código de barras, balanza), esto puede elegir el equivocado.

**Fase 2 — impresora predeterminada.**

```c
GetDefaultPrinterA(printerName, &size);        // dos llamadas: tamaño y datos
OpenPrinterA(printerName, &hPrinter, NULL);
GetPrinterA(hPrinter, 2, buffer, needed, &needed);
PRINTER_INFO_2A* pi = (PRINTER_INFO_2A*)buffer;
snprintf(fullPort, ..., "\\\\.\\%s", pi->pPortName);   // p. ej. "USB001"
```

Si el puerto existe, se abre con la misma configuración. Es el camino que funciona cuando el driver no creó un COM.

### `SendRaw`

```c
static int SendRaw(HANDLE hPort, const unsigned char* data, DWORD len);
```

`WriteFile` y devuelve los bytes escritos o -1. No hay reintento ni verificación de flujo.

### `Printer_PrintTicket`

```c
int Printer_PrintTicket(const char* text);
```

1. `FindPrinterPort()`; si falla, retorna 0.
2. Inicializa: `ESC @` (1B 40), `Sleep(50)`.
3. `ESC 3 18` (1B 33 18): interlineado de 24 puntos.
4. `ESC t 0F` (1B 74 0F): página de códigos CP1252.
5. Recorre el texto línea por línea separando por `'\n'` (el `\r` queda dentro de la línea; las impresoras lo ignoran).
6. Clasifica cada línea:

| Condición | Estilo aplicado |
|---|---|
| Contiene `===` o `---` | `ESC a 01` centrado + `ESC E 01` negrita + `GS ! 11` doble |
| Contiene `TOTEM` o `TICKET` | igual que separador |
| Empieza por `A`/`B`/`C` con `-` y longitud 3–6 | igual + `GS ! 22` triple |
| Resto | `ESC a 00` + `ESC E 00` + `GS ! 00` |

7. Envía el texto más `LF`; tras una línea especial resetea tamaño y alineación.
8. Línea vacía → `ESC d 01` (avanza una línea).
9. Al final: `ESC d 03` (avanza tres), `Sleep(50)`, `ESC i` (1B 69, corte), `Sleep(100)`, `CloseHandle`.

> **La rama de triple tamaño está inactiva con el formato actual.** El número viaja dentro de `"  Numero: A-041"`, que no empieza por A/B/C. El código es correcto pero nunca se ejecuta. Para activarla, `Ticket_FormatOutput` debería emitir el número en una línea propia, sola.

### `Printer_IsAvailable`

Abre y cierra el puerto; devuelve 1 o 0. No está en uso desde la interfaz, pero sirve para diagnóstico.

### Detección ciega: por qué no hay `DLE EOT`

Las impresoras ESC/POS reales responden a `DLE EOT n` (1B 04 n) con un byte de estado: papel presente, tapa cerrada, error. El código no lo usa. Motivos:

- Muchos adaptadores USB-COM no dejan leer del puerto o descartan las respuestas.
- Implementar timeout de lectura y reintentos agrega complejidad que este prototipo no necesitaba.
- Sin hardware para probar, una consulta de estado mal implementada es peor que ninguna.

Si se retoma, el camino es: escribir `1B 04 01`, leer un byte con `ReadFile` y timeout de 200 ms, y mapear bits según el manual de la impresora.

---

## 7. llamados.c — parser del archivo

```c
typedef struct {
    char numero[20];
    char modulo[50];
} Llamado;
```

### `Llamados_Load`

```c
int Llamados_Load(const char* filename, Llamado** out, int* count);
```

- Capacidad inicial 10; duplica con `realloc` cuando se llena.
- Lee con `fgets` en un buffer de 256 bytes.
- Recorta `\n` y `\r` del final.
- Ignora líneas vacías y líneas sin `|`.
- Copia con `strncpy` y termina manualmente.
- Si `realloc` falla, libera lo acumulado y devuelve 0.

No valida longitudes más allá del truncamiento: una línea de 300 caracteres se corta en `fgets`, y el resto se interpreta como otra línea (probablemente sin `|`, y se descarta). Es aceptable para un archivo que edita el operador.

### `Llamados_Free`

```c
void Llamados_Free(Llamado* llamados, int count);
```

`free(llamados)`. El parámetro `count` existe por simetría con la carga y no se usa.

### Formato tolerado

```
A-41|Modulo 4
A-42 | Modulo 4      ← espacios conservados, se muestran
linea sin separador  ← ignorada
                     ← ignorada
```

---

## 8. Referencia de la API

### ticket.h

| Función | Firma | Devuelve |
|---|---|---|
| `Ticket_CheckDailyReset` | `void Ticket_CheckDailyReset(void)` | — |
| `Ticket_Generate` | `void Ticket_Generate(const char* prefix, char* out, int maxlen)` | — (escribe el número en `out`; si el prefijo no es A/B/C no escribe) |
| `Ticket_FormatOutput` | `void Ticket_FormatOutput(char* out, int maxlen, const char* numero, const char* criterio)` | — (escribe el texto en `out`) |

### printer.h

| Función | Firma | Devuelve |
|---|---|---|
| `Printer_PrintTicket` | `int Printer_PrintTicket(const char* text)` | 1 si el puerto abrió (no confirma impresión) |
| `Printer_IsAvailable` | `int Printer_IsAvailable(void)` | 1 si hay puerto disponible |

### llamados.h

| Función | Firma | Devuelve |
|---|---|---|
| `Llamados_Load` | `int Llamados_Load(const char* filename, Llamado** out, int* count)` | 1 si leyó, 0 si no hay archivo o falló |
| `Llamados_Free` | `void Llamados_Free(Llamado* llamados, int count)` | — |

### ui.h

| Función | Firma | Uso |
|---|---|---|
| `UI_DrawBackground` | `void UI_DrawBackground(HDC hdc)` | Fondo completo |
| `UI_DrawOvalButton` | `void UI_DrawOvalButton(HDC hdc, int index, int hover, int press)` | Un botón con su texto |
| `UI_HitTestButton` | `int UI_HitTestButton(int x, int y)` | Índice o -1 |
| `UI_DrawTicketPreview` | `void UI_DrawTicketPreview(HDC hdc, const char* numero, const char* criterio)` | Tarjeta de 2 s |
| `UI_DrawLlamadosPanel` | `void UI_DrawLlamadosPanel(HDC hdc, Llamado* llamados, int count)` | Panel superior |
| `UI_DrawWaitingMessage` | `void UI_DrawWaitingMessage(HDC hdc)` | Mensaje sin llamados |

---

## 9. Flujo completo de un ticket

```
WM_LBUTTONDOWN
   └─ UI_HitTestButton(x, y) ──► -1? fin
   └─ AlPresionarBoton(hwnd, idx)
        ├─ Ticket_Generate("A", buf, 32)
        │    ├─ leer contadores.dat
        │    ├─ reset si cambió la fecha
        │    ├─ ultimoA++ → snprintf("A-%03d")
        │    └─ reescribir contadores.dat
        ├─ Ticket_FormatOutput(texto, 512, "A-041", "Atencion General")
        ├─ Printer_PrintTicket(texto)
        │    ├─ FindPrinterPort()  (COM1..COM20 → predeterminada)
        │    ├─ ESC @ / ESC 3 18 / ESC t 0F
        │    ├─ por línea: estilo + texto + LF
        │    └─ ESC d 03 / ESC i / CloseHandle
        ├─ [si falla] MessageBoxA(...)
        ├─ g_state = TICKET_PREVIEW
        ├─ KillTimer(ID_TIMER_LLAMADOS)
        ├─ SetTimer(ID_TIMER_PREVIEW, 2000)
        └─ InvalidateRect + UpdateWindow

2 segundos después:
WM_TIMER(1) ──► g_state = LLAMADOS_PANEL
             ├─ KillTimer(preview)
             ├─ SetTimer(llamados, 5000)
             └─ InvalidateRect
```

---

## 10. Diseño de la interfaz

### Paleta

| Rol | Color | Hex |
|---|---|---|
| Fondo superior | `RGB(10, 22, 40)` | `#0A1628` |
| Fondo inferior | `RGB(26, 42, 74)` | `#1A2A4A` |
| Oro principal | `RGB(212, 168, 67)` | `#D4A843` |
| Oro claro | `RGB(235, 200, 110)` | `#EBC86E` |
| Oro oscuro | `RGB(160, 120, 30)` | `#A0781E` |
| Texto principal | blanco | `#FFFFFF` |
| Texto secundario | `RGB(140, 160, 190)` | `#8CA0BE` |
| Texto apagado | `RGB(80, 100, 130)` | `#506482` |
| Tarjeta | `RGB(18, 32, 58)` | `#12203A` |
| Borde de tarjeta | `RGB(40, 65, 110)` | `#28416E` |

La paleta busca contraste alto sobre pantallas de bajo brillo y un aire "bancario" sin recurrir a azul corporativo genérico.

### Composición vertical

```
0 ─────────────────────────
        Panel de llamados (960 px)
960 ─── línea dorada ────────
        Botones (1020 → 1700)
1870 ── instrucción ─────────
1920 ────────────────────────
```

La mitad superior es información (llamados), la inferior es acción (botones). El cliente mira arriba y toca abajo; el operador ve lo mismo.

### Los bitmaps

Los botones se dibujan desde BMP de 600 × 600 con fondo blanco, que `TransparentBlt` vuelve transparente. El archivo `btn_negro.bmp` (2,5 MB) está declarado en el `.rc` como recurso 104 pero el código nunca lo carga: es el fondo del botón en estado presionado, previsto y no usado. Eliminarlo ahorraría 2,5 MB del ejecutable.

---

## 11. Gestión de recursos y memoria

| Recurso | Dónde se crea | Dónde se libera |
|---|---|---|
| `HBITMAP` verde/azul/rojo | `WM_CREATE` (globales en `ui.c`) | `WM_DESTROY` |
| `HFONT` | En cada función de dibujo | Inmediatamente tras dibujar |
| `HBRUSH` / `HPEN` | En cada función | Tras `FillRect`/`DrawText` |
| `HRGN` | `CreateRoundRectRgn` | `DeleteObject` en la misma función |
| `Llamado*` | `Llamados_Load` | `Llamados_Free` antes de recargar y en `WM_DESTROY` |
| `HANDLE` de puerto | `FindPrinterPort` | `CloseHandle` al final de imprimir |
| Buffer de `GetPrinter` | `malloc` | `free` en todos los caminos |

Puntos de atención si se modifica:

- Cada `CreateFontA` debe tener su `DeleteObject`. El patrón actual lo cumple.
- `SelectObject` siempre debe restaurar el objeto anterior antes de borrar el nuevo.
- Los bitmaps globales se cargan una vez; no recargarlos en cada `WM_PAINT`.

---

## 12. Compilación y depuración

### Makefile

```make
CC      = x86_64-w64-mingw32-gcc
WINDRES = x86_64-w64-mingw32-windres
CFLAGS  = -Os -s -Wall -mwindows
LDFLAGS = -lgdi32 -lwinspool -lcomctl32 -lmsimg32 -limm32 -lmsimg32
```

- `-Os`: optimiza por tamaño (importante con 6,9 MB de recursos).
- `-s`: quita símbolos.
- `-mwindows`: subsistema GUI, sin consola.
- `-limm32` y `-lcomctl32` no se usan; `-lmsimg32` está duplicado. Son inocuos pero se pueden limpiar.

### Compilar

```bash
# Linux
make            # genera obj/*.o y totem.exe
make clean      # borra obj/ y totem.exe
```

En Windows, con MinGW-w64 en el `PATH`, se puede usar el mismo Makefile con `mingw32-make`, o compilar a mano (ver `datasheet.md`).

### Verificar que los recursos entraron

```bash
x86_64-w64-mingw32-objdump -h totem.exe | grep -i rsrc
```

Debe aparecer una sección `.rsrc` con tamaño no trivial. Si no está, `windres` no se ejecutó o el `.o` no se enlazó.

### Depurar

El programa no tiene logging. Para investigar en campo:

1. Compila sin `-mwindows` y con `-g` para ver la consola y los `printf` que agregues.
2. Usa `OutputDebugStringA` y un visor como DebugView.
3. Instrumenta `Printer_PrintTicket` con la cantidad de bytes escritos por `SendRaw`.
4. Comprueba el puerto con PuTTY o `mode COM3: BAUD=9600 PARITY=n DATA=8 STOP=1` y un editor serie.

### Pruebas manuales sugeridas

| Caso | Qué verificar |
|---|---|
| Sin `contadores.dat` | Arranca en cero y crea el archivo |
| `contadores.dat` de ayer | Reinicia a cero |
| `contadores.dat` corrupto | No debe romper el arranque |
| `llamados.txt` ausente | Panel con "Sin llamados activos" |
| `llamados.txt` con 10 líneas | Muestra solo 4 |
| Línea sin `|` | Se ignora sin error |
| Impresora desconectada | Aviso modal, contador igual avanza |
| Toque en el borde del botón | Debe responder (margen de 20 px) |
| `Esc` / `F4` | Cierra la ventana |

---

## 13. Guías de extensión

### Agregar un cuarto servicio

1. En `config.h`: `#define BUTTON_COUNT 4`.
2. En `main.c`: agregar `"D"` a `PREFIJOS[]` y el nombre a `CRITERIOS[]`.
3. En `ui.c`: agregar el nombre a `BUTTON_NAMES[]` y la descripción a `BUTTON_DESCS[]`.
4. Ajustar `BUTTON_START_Y` y `BUTTON_GAP` para que cuatro botones quepan entre 1020 y 1700. Con alto 200, el paso actual de 230 deja espacio hasta el cuarto (1020 + 3 × 230 = 1710) pero roza el texto inferior.
5. En `ticket.c`: agregar `ultimoD` a `Contadores` (el `sizeof` cambia y el archivo viejo deja de ser válido: hay que migrarlo o borrarlo).
6. En `ui.c`, `UI_DrawOvalButton`: mapear el índice 3 a un bitmap (o al fallback dorado).

### Cambiar la paleta

Todo está en `config.h`. Para un tema claro hay que invertir también los `FillRect` de `ui.c`, que usan colores literales (`RGB(8, 16, 32)`) en las mitades superiores. Es deuda técnica: esos literales deberían ser constantes con nombre.

### Activar el triple tamaño del número

En `Ticket_FormatOutput`, emitir el número en su propia línea:

```
  Numero:
A-041
```

Al empezar por `A`, la línea entra en la rama `isTicketNum` y se imprime a triple ancho y alto. Requiere ajustar el ancho de la impresora (72 mm útiles) para que no se corte.

### Agregar registro de auditoría

Un archivo `tickets.log` en modo append, una línea por ticket: fecha, hora, servicio, número. Punto de inserción: `AlPresionarBoton`, después de `Ticket_Generate`. Cuidado con el bloqueo del archivo si hay dos instancias.

### Mitigar la detección ciega

1. Implementar `Printer_QueryStatus(HANDLE)` con `DLE EOT 1`, `ReadFile` y timeout.
2. Llamarla después de abrir el puerto; si no responde en 300 ms, cerrar y pasar al siguiente COM.
3. En `Printer_PrintTicket`, devolver 0 si el estado reporta papel agotado o tapa abierta.

### Empaquetar como instalador

Un `.bat` o un instalador Inno Setup que copie `totem.exe`, `llamados.txt` vacío y un acceso directo en `shell:startup`. No hay claves de registro ni servicios que instalar.

### Reemplazar el archivo por un socket o base de datos

`llamados.c` es el único punto de lectura. Cambiar `Llamados_Load` por una consulta a SQLite o a un socket TCP no afecta al resto del programa, siempre que respete la firma y llene el arreglo. Para varios tótems, la fuente de llamados debería ser compartida y los contadores centralizados.

---

## 14. Deuda técnica conocida

| Deuda | Impacto | Arreglo sugerido |
|---|---|---|
| `hover` sin usar en `UI_DrawOvalButton` | Sin feedback al pasar el dedo | Aplicar un brillo o borde extra |
| `Printer_PrintTicket` siempre devuelve 1 si abrió | No detecta papel ni errores | Consultar `DLE EOT` |
| Rama de triple tamaño muerta | Número no resaltado en el ticket | Línea propia para el número |
| Contadores sin bloqueo | Corrupción con dos instancias | `LockFileEx` o instancia única con mutex |
| Sin techo de numeración | `A-1000` rompe el ancho | Reset a 1 al llegar a 999 o 4 dígitos |
| Colores literales en `ui.c` | Difícil cambiar de tema | Mover a `config.h` |
| `btn_negro.bmp` sin uso | +2,5 MB en el ejecutable | Quitarlo del `.rc` |
| Sin doble buffer | Posible parpadeo en equipos lentos | Dibujar en bitmap y volcar |
| `Sleep(100)` en el hilo de UI | Congela 100 ms por toque | Timer de feedback |
| Un solo monitor soportado | No hay selección por configuración | Leer un `.ini` con el índice |

---

## 15. Convenciones y estilo

- **Nombres:** `PascalCase` para funciones públicas (`Ticket_Generate`), `camelCase` para estáticas y variables locales, `g_` para globales, `MAYÚSCULAS` para macros.
- **Prefijo por módulo:** `Ticket_`, `Printer_`, `Llamados_`, `UI_`. Evita colisiones y hace evidente el origen.
- **Anchos:** API ANSI (`A`) en todas las llamadas, coherente con `CreateWindowExA` y `MessageBoxA`.
- **Textos:** sin tildes ni `ñ`, para que la impresora configurada en CP1252 no muestre caracteres extraños.
- **Buffers:** tamaños literales pequeños (`char numero[32]`), sin asignación dinámica salvo en `llamados.c`.
- **Errores:** se absorben. Un archivo que no se puede leer no detiene el programa; la interfaz sigue funcionando.

Si agregas código, mantén esas reglas: son la razón por la que el proyecto se lee de un tirón.
