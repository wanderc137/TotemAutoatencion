# Referencias — Tótem de Autoatención

Bibliografía comentada del proyecto. No es una lista de enlaces: cada entrada explica **para qué se usó** y qué parte del código resuelve. Las referencias se agrupan por tema y se ordenan de lo general a lo específico.

> Las URLs se verificaron al momento de escribir esta documentación. Microsoft reorganiza su documentación con frecuencia; si un enlace cambia, el título de la página suele seguir siendo buscable.

---

## 1. Win32 y arquitectura de aplicaciones

### Microsoft Learn — Win32 API reference

- **URL:** <https://learn.microsoft.com/en-us/windows/win32/>
- **Qué es:** referencia oficial de la API de Windows: funciones, estructuras, mensajes y constantes.
- **Uso en el proyecto:** consulta permanente para `WndProc`, `CreateWindowExA`, `SetTimer`, `InvalidateRect`, `EnumDisplayDevicesA`, `SetWindowPos` y el modelo de mensajes.
- **Por qué:** es la fuente primaria. Los tutoriales de terceros suelen omitir parámetros o copiar firmas viejas.

### Microsoft Learn — Windows GDI

- **URL:** <https://learn.microsoft.com/en-us/windows/win32/gdi/windows-gdi>
- **Qué es:** guía de la interfaz de dispositivo gráfico: contextos, objetos, modos de dibujo.
- **Uso en el proyecto:** `ui.c` completo: `FillRect`, `CreateFontA`, `DrawTextA`, `CreateRoundRectRgn`, `SelectObject`, `DeleteObject`, `SetBkMode`, `SetTextColor`.
- **Por qué:** aclara el ciclo de vida de los objetos GDI y por qué hay que restaurar el objeto previo antes de borrar el nuevo.

### Microsoft Learn — `GradientFill`

- **URL:** <https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-gradientfill>
- **Qué es:** relleno de degradado con `TRIVERTEX` y `GRADIENT_TRIANGLE`.
- **Uso en el proyecto:** fondo de la pantalla, de `#0A1628` arriba a `#1A2A4A` abajo.
- **Nota:** los valores de `TRIVERTEX` van en 16 bits por canal, desplazados 8 bits (`r << 8`), no en 8 bits. Es el error más común al usarla.

### Microsoft Learn — `TransparentBlt`

- **URL:** <https://learn.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-transparentblt>
- **Qué es:** copia de bitmap tratando un color como transparente.
- **Uso en el proyecto:** dibujo de los botones ovalados desde BMP de 600 × 600 con fondo blanco.
- **Limitación:** no hace mezcla alfa; solo descarta el color exacto. Para bordes suaves haría falta `AlphaBlend`.

### Microsoft Learn — About Resource Files

- **URL:** <https://learn.microsoft.com/en-us/windows/win32/menurc/about-resource-files>
- **Qué es:** formato de los archivos `.rc` y cómo `windres`/`rc.exe` los compilan.
- **Uso en el proyecto:** `res/buttons.rc`, que declara los cuatro bitmaps con IDs 101–104.

### Charles Petzold — *Programming Windows*, 5.ª edición

- **Qué es:** el libro clásico de Win32 en C. Aunque es de 1998, el modelo de mensajes y GDI no cambió.
- **Uso en el proyecto:** estructura general de `WinMain` + `WndProc`, manejo de `WM_PAINT` y `WM_ERASEBKGND`.
- **Por qué:** explica el *porqué* del bucle de mensajes mejor que la referencia, que solo lista firmas.

### Jeffrey Richter — *Windows via C/C++*

- **Qué es:** referencia avanzada sobre procesos, hilos, memoria y sincronización.
- **Uso en el proyecto:** justificar el diseño de un solo hilo y entender qué implica `Sleep` en el hilo de UI.
- **Por qué:** útil si se decide pasar la impresión a un hilo aparte.

---

## 2. Puertos serie y comunicación

### Microsoft Learn — `DCB` structure

- **URL:** <https://learn.microsoft.com/en-us/windows/win32/api/winbase/ns-winbase-dcb>
- **Qué es:** estructura de configuración de un puerto de comunicaciones.
- **Uso en el proyecto:** `FindPrinterPort` configura `CBR_9600`, 8 bits, `NOPARITY`, `ONESTOPBIT`, `DTR_CONTROL_ENABLE`, `RTS_CONTROL_ENABLE`.
- **Por qué:** cada campo tiene implicancias eléctricas; DTR y RTS activos son necesarios en muchas impresoras que los usan como señal de "listo".

### Microsoft Learn — `COMMTIMEOUTS` structure

- **URL:** <https://learn.microsoft.com/en-us/windows/win32/api/winbase/ns-winbase-commtimeouts>
- **Qué es:** tiempos de espera de lectura y escritura en un puerto serie.
- **Uso en el proyecto:** `ReadIntervalTimeout = 100`, `ReadTotalTimeoutConstant = 100`.
- **Por qué:** sin timeouts, una lectura puede bloquear el hilo indefinidamente. En este programa no se lee, pero se configuran para que el puerto quede en un estado predecible.

### Microsoft Learn — `CreateFileA` (communications resources)

- **URL:** <https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilea>
- **Qué es:** apertura de recursos, incluidos `\\.\COM1`.
- **Uso en el proyecto:** apertura de cada puerto candidato.
- **Detalle:** el prefijo `\\.\` es obligatorio para puertos COM mayores que COM9; el código lo usa para todos, de COM1 a COM20.

### Microsoft Learn — Configuring a Communications Resource

- **URL:** <https://learn.microsoft.com/en-us/windows/win32/devio/configuring-a-communications-resource>
- **Qué es:** guía paso a paso de configuración serie en Win32.
- **Uso en el proyecto:** orden correcto de `GetCommState` → modificar `DCB` → `SetCommState` → `SetCommTimeouts`.

---

## 3. Impresión en Windows

### Microsoft Learn — `GetDefaultPrinterA`

- **URL:** <https://learn.microsoft.com/en-us/windows/win32/printdocs/getdefaultprinter>
- **Qué es:** obtiene el nombre de la impresora predeterminada del usuario.
- **Uso en el proyecto:** segundo camino de detección, cuando ningún COM respondió.
- **Detalle:** se llama dos veces: la primera con `NULL` para obtener el tamaño del buffer, la segunda para los datos.

### Microsoft Learn — `GetPrinter` y `PRINTER_INFO_2`

- **URL:** <https://learn.microsoft.com/en-us/windows/win32/printdocs/getprinter>
- **URL:** <https://learn.microsoft.com/en-us/windows/win32/printdocs/printer-info-2>
- **Qué es:** información detallada de una impresora instalada, incluido `pPortName`.
- **Uso en el proyecto:** obtener el puerto (`USB001`, `COM3`, etc.) de la impresora predeterminada y armar `\\.\<puerto>`.
- **Por qué:** es la forma documentada de traducir "impresora predeterminada" a un nombre de puerto abrible con `CreateFile`.

### Microsoft Learn — `OpenPrinter` / `ClosePrinter`

- **URL:** <https://learn.microsoft.com/en-us/windows/win32/printdocs/openprinter>
- **Qué es:** apertura de un objeto impresora para consultar su configuración.
- **Uso en el proyecto:** envolver la llamada a `GetPrinter` y liberar el handle en todos los caminos.

### Microsoft Learn — Printing and Print Spooler

- **URL:** <https://learn.microsoft.com/en-us/windows/win32/printdocs/printing-and-print-spooler>
- **Qué es:** visión general de la cola de impresión y la API de alto nivel (`StartDocPrinter`, `WritePrinter`).
- **Uso en el proyecto:** documentar **por qué no se usa**. La cola exige driver y agrega latencia; el tótem necesita bytes crudos al puerto.

---

## 4. ESC/POS

### Epson — ESC/POS Command Reference

- **URL:** <https://download4.epson.biz/sec_pubs/pos/reference_en/escpos/index.html>
- **Qué es:** referencia oficial de los comandos ESC/POS, con parámetros, valores por defecto y notas por modelo.
- **Uso en el proyecto:** todos los comandos de `printer.c`:

| Comando | Bytes | Efecto |
|---|---|---|
| `ESC @` | `1B 40` | Inicializar impresora |
| `ESC 3 n` | `1B 33 18` | Interlineado de 24 puntos |
| `ESC t n` | `1B 74 0F` | Página de códigos CP1252 |
| `ESC a n` | `1B 61 00/01` | Alineación izquierda/centro |
| `ESC E n` | `1B 45 00/01` | Negrita off/on |
| `GS ! n` | `1D 21 00/11/22` | Tamaño de carácter normal/doble/triple |
| `ESC d n` | `1B 64 01/03` | Avanzar n líneas |
| `ESC i` | `1B 69` | Corte total (modelos con cutter) |
| `GS V m` | `1D 56 ...` | Corte alternativo según modelo |

- **Por qué:** es la única fuente que documenta el comportamiento exacto de cada fabricante compatible. Los clones suelen respetar el subconjunto básico.

### Wikipedia — ESC/POS

- **URL:** <https://en.wikipedia.org/wiki/ESC/POS>
- **Qué es:** resumen histórico y lista de comandos frecuentes.
- **Uso en el proyecto:** contexto para la sección "Cómo se construyó" y para entender la familia de comandos.
- **Advertencia:** no es normativa; ante duda, prevalece la referencia de Epson.

### Epson — ESC/POS FAQ y notas de compatibilidad

- **URL:** <https://pos.epson.com/>
- **Qué es:** portal de soluciones POS de Epson.
- **Uso en el proyecto:** entender por qué el corte y el ancho de papel varían entre modelos de 58 y 80 mm.

### Manual del fabricante — Masung MS800

- **Qué es:** manual del proveedor de la impresora usada como referencia de diseño (80 mm, USB con puerto COM virtual, comandos ESC/POS, corte automático).
- **Uso en el proyecto:** configuración serie, ancho de papel y secuencia de corte.
- **Nota:** el manual se entrega con el equipo. No se reproduce aquí por derechos de autor. Si tu unidad es un clon, verifica que acepte `ESC i`; algunas solo cortan con `GS V`.

---

## 5. Compilación cruzada y herramientas

### MinGW-w64

- **URL:** <https://www.mingw-w64.org/>
- **Qué es:** toolchain GCC para Windows de 64 bits, con headers y librerías de la API Win32.
- **Uso en el proyecto:** compilador `x86_64-w64-mingw32-gcc` y `windres`.
- **Por qué:** permite compilar desde Linux y desde Windows con el mismo código, sin Visual Studio.

### MSYS2

- **URL:** <https://www.msys2.org/>
- **Qué es:** distribución de herramientas Unix para Windows, con paquetes de MinGW-w64.
- **Uso en el proyecto:** vía recomendada para instalar el toolchain en Windows (`pacman -S mingw-w64-x86_64-gcc`).
- **Por qué:** el instalador de MinGW-w64 a secas tiene variantes confusas; MSYS2 es reproducible.

### GNU Make

- **URL:** <https://www.gnu.org/software/make/manual/make.html>
- **Qué es:** manual de `make`.
- **Uso en el proyecto:** entender reglas de patrón (`obj/%.o: src/%.c`), variables y el target `clean`.

### GNU Binutils — `objdump`

- **URL:** <https://sourceware.org/binutils/docs/binutils/objdump.html>
- **Qué es:** inspección de secciones y símbolos de un ejecutable.
- **Uso en el proyecto:** verificar que la sección `.rsrc` existe y que los bitmaps se enlazaron.

---

## 6. Codificación de caracteres

### Wikipedia — Windows-1252

- **URL:** <https://en.wikipedia.org/wiki/Windows-1252>
- **Qué es:** tabla de la página de códigos 1252, usada por Windows en Europa occidental.
- **Uso en el proyecto:** `ESC t 0F` selecciona CP1252 en la impresora; los textos se escriben sin tildes para no depender de la configuración regional.
- **Por qué:** una impresora mal configurada en CP437 muestra `AtenciÃ³n`; evitar tildes elimina la clase de error.

### Microsoft Learn — Code Page Identifiers

- **URL:** <https://learn.microsoft.com/en-us/windows/win32/intl/code-page-identifiers>
- **Qué es:** lista de identificadores de páginas de códigos.
- **Uso en el proyecto:** confirmar que `0x0F` corresponde a 1252 y no a otra variante.

---

## 7. Diseño de interfaz para tótems

### Nielsen Norman Group — Touch targets

- **URL:** <https://www.nngroup.com/articles/touch-target-size/>
- **Qué es:** investigación sobre tamaño mínimo de objetivos táctiles.
- **Uso en el proyecto:** justificar botones de 220 × 200 px y un área de hit test aún mayor (toda la franja horizontal, con 20 px de margen).
- **Por qué:** en un tótem el usuario está de pie, con prisa y sin precisión; el tamaño generoso reduce errores.

### Material Design — Accessibility (touch target)

- **URL:** <https://m3.material.io/foundations/accessible-design/accessibility-basics>
- **Qué es:** guías de accesibilidad, incluido el mínimo de 48 × 48 dp.
- **Uso en el proyecto:** referencia comparativa; el tótem supera ampliamente el mínimo.

### W3C — WCAG 2.2, contraste

- **URL:** <https://www.w3.org/WAI/WCAG22/quickref/>
- **Qué es:** criterios de contraste y legibilidad.
- **Uso en el proyecto:** validar dorado sobre azul oscuro y blanco sobre `#12203A`.

---

## 8. Documentación y herramientas del repositorio

### Shields.io

- **URL:** <https://shields.io/>
- **Qué es:** generador de badges para README.
- **Uso en el proyecto:** badges de plataforma, lenguaje, build, impresión, estado y licencia.

### Keep a Changelog

- **URL:** <https://keepachangelog.com/es-ES/1.1.0/>
- **Qué es:** convención para registrar cambios.
- **Uso en el proyecto:** estructura sugerida para futuras versiones (no hay CHANGELOG todavía).

### Semantic Versioning

- **URL:** <https://semver.org/lang/es/>
- **Qué es:** convención de versiones mayor/menor/parche.
- **Uso en el proyecto:** la versión actual es `1.0.0` de prototipo; cualquier cambio de formato de `contadores.dat` obliga a subir la mayor.

### Git — documentación oficial

- **URL:** <https://git-scm.com/doc>
- **Qué es:** referencia de Git.
- **Uso en el proyecto:** flujo de un solo desarrollador, sin ramas de larga vida.

---

## 9. Lecturas descartadas

| Fuente | Por qué no se usó |
|---|---|
| Tutoriales de "kiosco Windows" con `SetWindowsHookEx` | Complejidad y riesgo de seguridad; el proyecto no bloquea el teclado |
| Ejemplos de ESC/POS en Python (`python-escpos`) | Otro lenguaje y otra capa; útil como referencia de comandos, no de implementación |
| `StartDocPrinter` + driver genérico | Requiere impresora instalada y agrega cola; contradice el objetivo de impresión directa |
| Frameworks de UI (Qt, Dear ImGui) | Agregan dependencias y tamaño; GDI alcanza para dos pantallas |
| Bases de datos embebidas (SQLite) | Los contadores son 24 bytes; un archivo binario es suficiente |
| Servicios web para llamados | El tótem no tiene red y no la necesita |

---

## Cómo citar este proyecto

Si usas el código o la documentación:

```
Tótem de Autoatención — emisión de turnos con impresión ESC/POS directa.
wanderc137, 2026. Licencia MIT.
```

Las referencias de este documento pertenecen a sus respectivos autores y se citan con fines educativos.
