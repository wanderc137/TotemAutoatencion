# Bitácora del proyecto — Tótem de Autoatención

Registro de decisiones, problemas y descartes durante el desarrollo. Está escrito para que quien retome el proyecto **no repita los mismos errores** y entienda por qué el código es como es.

> Estado general: prototipo funcional a nivel de código, **sin validación en hardware físico**. Esta bitácora distingue explícitamente lo verificado de lo supuesto.

---

## 1. Contexto y método

El objetivo era construir un tótem de autoatención para una oficina: pantalla vertical táctil, tres tipos de servicio, ticket impreso y panel de llamados. Las restricciones de partida:

- Windows 10/11 en el equipo del local.
- Impresora térmica de 80 mm sin ganas de pelear con drivers.
- Sin internet, sin servidor, sin base de datos.
- Un solo desarrollador y tiempo acotado.

El método fue: definir el flujo mínimo (tocar → numerar → imprimir → llamar), escribir primero la impresión y la numeración —lo más riesgoso—, y dejar la interfaz para el final. La interfaz se dibuja entera con GDI porque no hay controles estándar que se vean bien a 1080 × 1920.

---

## 2. Decisiones técnicas

### 2.1 C con Win32 en lugar de C# o Electron

**Decisión:** aplicación nativa en C, sin framework.

**Por qué:** un tótem debe arrancar en un equipo recién instalado, sin runtime de .NET ni Node. El binario depende solo de DLLs del sistema. Además, el acceso al puerto serie y a la API de impresión es directo, sin capas que escondan los bytes.

**Costo:** toda la interfaz es manual: no hay layout, ni estilos, ni controles. Cada botón, tarjeta y texto es una llamada GDI.

### 2.2 ESC/POS crudo en lugar de la cola de impresión

**Decisión:** escribir bytes al puerto COM con `WriteFile`.

**Por qué:** `StartDocPrinter` exige una impresora instalada con driver. En la práctica, eso significa un driver genérico, una cola, posibles diálogos y márgenes que no se controlan. El tótem necesita que el ticket salga **ya**, con un formato fijo.

**Costo:** hay que conocer los comandos, detectar el puerto a mano y aceptar que no hay confirmación de impresión.

**Alternativa considerada:** imprimir a PDF y que el operador imprima. Se descartó porque rompe la experiencia de autoservicio.

### 2.3 Contadores en archivo binario de 24 bytes

**Decisión:** `contadores.dat` con una struct `{ char fecha[9]; int ultimoA, ultimoB, ultimoC; }`.

**Por qué:** el estado es mínimo. Un archivo binario se lee y escribe en una operación, sin parser, sin dependencias y sin riesgo de corrupción por texto mal formado. La fecha en formato `yyyyMMdd` permita comparar con `strcmp`.

**Costo:** el archivo no es legible por humanos, y un cambio en la struct invalida los archivos viejos. Se acepta porque el reinicio diario hace que el dato dure un día.

### 2.4 Llamados en archivo de texto

**Decisión:** `llamados.txt` con líneas `NUMERO|MODULO`.

**Por qué:** el operador puede llamar clientes con el Bloc de notas, sin una pantalla de administración. El programa relee cada 5 segundos y no necesita integración alguna.

**Costo:** no hay validación, no hay orden ni prioridad, y cualquiera con acceso a la carpeta puede editarlo.

**Descartado:** un socket TCP o una base de datos. Para un solo tótem, agregan infraestructura sin beneficio. Si algún día hay varios puestos, el archivo deja de servir y habrá que centralizar.

### 2.5 Un solo hilo y timers de Win32

**Decisión:** todo en el hilo de UI, con `SetTimer`.

**Por qué:** el volumen de trabajo es mínimo: leer un archivo de pocas líneas cada 5 segundos y escribir un ticket. Un hilo adicional agregaría sincronización para un problema que no existe a esta escala.

**Costo:** la impresión bloquea la interfaz. Si la impresora tarda o el puerto queda colgado, la pantalla se congela. Está documentado como limitación.

### 2.6 Ventana `WS_POPUP` topmost en lugar de kiosco real

**Decisión:** ventana sin bordes, siempre encima, centrada en el monitor vertical. Sin hooks de teclado.

**Por qué:** un kiosco real (bloquear `Alt+Tab`, `Ctrl+Alt+Supr`, el shell) requiere privilegios, políticas de grupo y un modo de mantenimiento. Para un prototipo, `topmost` es suficiente y no compromete la seguridad del equipo.

**Costo:** el público puede cerrar el programa con `Esc` o `F4` si tiene teclado. Se mitiga no dejando teclado accesible.

### 2.7 Bitmaps incrustados en el ejecutable

**Decisión:** `.rc` con cuatro BMP, cargados con `LoadBitmap`.

**Por qué:** un solo archivo para copiar. No hay rutas de imágenes que se rompan ni carpetas que se pierdan.

**Costo:** el ejecutable pesa 6,9 MB, casi todo imágenes. Además, los BMP se guardan sin compresión. Es el precio de la simplicidad de despliegue.

### 2.8 Textos sin tildes

**Decisión:** "Atencion", "Creditos", "Preferencial", "Dirijase".

**Por qué:** la impresora se configura en CP1252 con `ESC t 0F`. Si el sistema está en otra página de códigos, las tildes se convierten en caracteres extraños. Eliminarlas evita la clase entera de errores.

**Costo:** se ve menos profesional. Es un compromiso consciente de un prototipo que prioriza que el ticket salga legible.

---

## 3. Problemas encontrados y cómo se resolvieron

### 3.1 El ejecutable pesaba demasiado

**Síntoma:** el primer enlace superaba los 9 MB.

**Causa:** los BMP de botones estaban a 600 × 600 sin compresión y uno de ellos (el fondo negro) pesaba 2,5 MB.

**Solución:** se mantuvieron los bitmaps (la calidad del ovalado lo justifica), se compiló con `-Os -s` y se aceptó el tamaño. `btn_negro.bmp` sigue declarado pero sin uso: eliminarlo ahorraría 2,5 MB y quedó anotado como deuda.

**Lección:** en un proyecto con recursos gráficos, el tamaño del binario lo define el `.rc`, no el código.

### 3.2 La ventana no siempre quedaba en el monitor vertical

**Síntoma:** en equipos con dos monitores, la ventana aparecía en el principal aunque el tótem estuviera en el secundario.

**Solución:** `WinMain` recorre los adaptadores con `EnumDisplayDevicesA`, filtra los activos y adjuntos al escritorio, y consulta el modo con `EnumDisplaySettingsExA`. Acepta el monitor si en algún sentido mide al menos 1080 × 1920, y centra la ventana con `SetWindowPos(..., HWND_TOPMOST, ...)`. Si no encuentra ninguno, cae al monitor primario y, como último recurso, maximiza.

**Limitación:** no hay configuración para elegir monitor; se elige el primero que cumpla el tamaño. En un equipo con dos pantallas verticales, puede elegir la equivocada.

### 3.3 El número del ticket no salía en tamaño triple

**Síntoma:** el código tenía una rama para imprimir el número a triple ancho y alto, pero nunca se ejecutaba.

**Causa:** el formato del ticket emite `"  Numero: A-041"`. La detección de `printer.c` busca líneas que **empiecen** por `A`, `B` o `C` y tengan un guion. La línea empieza con espacios y con la palabra "Numero", así que nunca coincide.

**Estado:** no se corrigió. La rama es correcta pero está muerta. Para activarla hay que cambiar `Ticket_FormatOutput` para que el número vaya en una línea propia.

**Lección:** una rama de código que nunca se ejecuta es peor que no tenerla: da falsa sensación de cobertura. Si se escribe, hay que probarla.

### 3.4 El hit test era más grande que el botón

**Observación:** el área táctil abarca de `x = 60` a `x = 1060` y 20 px arriba y abajo del botón, no solo los 220 px del óvalo.

**Por qué se dejó así:** en un tótem, el usuario toca con el dedo, de pie y con prisa. Un objetivo grande reduce errores. El texto a la derecha del óvalo también es tocable, lo que hace que el botón completo (óvalo + descripción) funcione como una sola zona.

**Efecto secundario:** las franjas táctiles de los tres botones se tocan entre sí con el margen de 20 px. Con el paso actual de 230 px y alto 200, los márgenes se superponen 10 px en los bordes. El primer botón que coincide gana, así que el borde superior del botón 2 responde como botón 1. No se corrigió porque el efecto es imperceptible con el dedo.

### 3.5 Lectura del archivo de llamados con `\r\n`

**Síntoma:** al editar `llamados.txt` en Windows, el módulo aparecía con un `\r` al final.

**Solución:** `Llamados_Load` recorta tanto `\n` como `\r` del final de cada línea antes de parsear. Es un detalle pequeño pero necesario si el archivo se edita con el Bloc de notas.

### 3.6 Padding de la struct de contadores

**Observación:** `sizeof(Contadores)` es 24, no 21. El compilador inserta 3 bytes de padding después de `char fecha[9]` para alinear los `int`.

**Consecuencia:** el archivo tiene 24 bytes. Si se agregara un campo `char` al final, el tamaño podría seguir siendo 24 por padding final, y una lectura vieja seguiría funcionando por casualidad. No se debe confiar en eso: cualquier cambio de struct exige migración.

### 3.7 La documentación no coincidía con el código

**Síntoma:** la documentación previa describía funciones que no existen (`Printer_GetDefault`, `StartDocPrinter`/`WritePrinter`) y un módulo `display.c` que nunca se creó. También mencionaba imágenes `.png` cuando los recursos son `.bmp`, y un comando `ESC G` en lugar de `ESC E`.

**Causa:** se documentó una intención de diseño y no el código final; luego el código cambió y la documentación no.

**Solución:** se reescribieron `README.md`, `docs/datasheet.md` y `docs/developer.md` leyendo el código fuente línea por línea, con verificación de tamaños, firmas y constantes. La regla que queda: **la fuente de verdad es el código**; el plan y los informes de auditoría son contexto, no evidencia.

**Lección:** documentar al final, pero verificar cada afirmación contra el archivo. Un dato inventado en un datasheet es peor que un datasheet corto.

### 3.8 Artefactos al recortar el README

**Síntoma:** al ajustar el README al rango de caracteres pedido, quedaron bloques de código vacíos, un fence huérfano y una tabla de casos de prueba desnumerada.

**Solución:** se contaron los fences (deben ser pares), se revisó cada tabla y se renumeraron los casos. Se incorporó una verificación final con `grep` de fences, enlaces prohibidos y datos sensibles.

**Lección:** recortar texto por script es rápido pero rompe estructuras; hay que validar después del recorte, no antes.

### 3.9 Datos sensibles en la documentación anterior

**Síntoma:** el README viejo incluía IPs privadas reales, una ruta de usuario de Windows y enlaces a documentos internos.

**Solución:** se eliminaron y se reemplazaron por marcadores genéricos (por ejemplo, `C:\Totem\` para la carpeta de instalación y "impresora predeterminada" para el dispositivo). Se verificó con búsquedas de rangos de red privados, rutas de usuario y los documentos internos.

**Lección:** la documentación de un repositorio público se escribe pensando en el público desde la primera línea. Es más fácil no incluir el dato que acordarse de quitarlo.

### 3.10 No había hardware para probar

**Situación:** no se dispuso de pantalla táctil ni de impresora MS800 física durante el desarrollo.

**Consecuencia:** todo el código de impresión y de interfaz está validado por lectura y compilación, no por ejecución contra el dispositivo. El README y el datasheet lo dicen explícitamente.

**Mitigación:** se diseñó una cascada de detección (COM1–COM20 → impresora predeterminada) y un fallback visual si los bitmaps no cargan. Son defensas para el día de la instalación, no sustitutos de la prueba.

---

## 4. Enfoques descartados

| Enfoque | Por qué se descartó |
|---|---|
| C# / WinForms / WPF | Requiere .NET instalado; el despliegue en tótem se complica |
| Electron | 150 MB de runtime para dos pantallas; consumo de RAM desproporcionado |
| Qt | Licencia y DLLs extra; GDI alcanza |
| Python + Tkinter | Requiere intérprete; el acceso a ESC/POS y a la API de impresión es indirecto |
| `StartDocPrinter` + driver | Cola, diálogos y dependencia de un driver instalado |
| PDF + impresión manual | Rompe el autoservicio |
| Base de datos (SQLite) | 24 bytes de estado no justifican un motor |
| Socket TCP para llamados | Infraestructura sin beneficio para un solo tótem |
| Contadores en el registro de Windows | Más difícil de respaldar y de inspeccionar |
| Doble buffer GDI | Complejidad; el parpadeo no se observó en pruebas de escritorio |
| Hook de teclado para kiosco | Riesgo de seguridad y falsa sensación de bloqueo |
| Consulta `DLE EOT` | Sin hardware para validar la respuesta, una implementación a ciegas es peor que ninguna |
| Instalador MSI | Un `.bat` que copia archivos es suficiente para el prototipo |

---

## 5. Lecciones aplicables a proyectos futuros

1. **La impresión es lo primero que hay que validar.** Es la parte con más variables externas (puerto, driver, modelo, comandos). Dejarla para el final es garantía de sorpresas.
2. **Escribir la documentación contra el código, no contra el plan.** Los documentos de intención envejecen mal.
3. **Las ramas de código muertas se detectan probando.** Si una condición nunca se cumple con el formato actual, no es cobertura: es ruido.
4. **Un archivo de texto como interfaz de administración es una decisión válida** si el operador es técnico y el volumen es bajo. No todo necesita una UI de administración.
5. **Los recursos gráficos dominan el tamaño del binario.** Si el tamaño importa, optimizar BMP antes que el código.
6. **Los textos sin tildes son un parche, no una solución.** La solución correcta es enviar UTF-8 y configurar la impresora, pero requiere hardware para verificar.
7. **La detección de hardware por prueba y error es frágil.** Un identificador (VID/PID) o una consulta de estado es más robusto que abrir puertos a ciegas.
8. **El estado mínimo simplifica todo.** Dos archivos y cero dependencias hacen que el sistema sea fácil de instalar, respaldar y explicar.
9. **Contar caracteres reales, no estimados.** En documentación con tablas, el conteo real puede ser 1,5× la estimación.
10. **Documentar lo que no se probó.** Un "no validado en hardware" explícito ahorra una tarde de diagnóstico a quien instale.

---

## 6. Estado y próximos pasos

### Lo que está hecho

- Emisión de tickets A/B/C con contadores diarios persistentes.
- Impresión ESC/POS directa con detección de puerto en cascada.
- Panel de llamados con refresco cada 5 segundos.
- Interfaz GDI completa a 1080 × 1920 con pantalla de reposo y vista previa.
- Compilación cruzada con MinGW-w64 y recursos incrustados.
- Documentación de usuario, de desarrollador y referencias.

### Lo que falta para producción

1. **Probar con hardware real:** pantalla táctil, impresora MS800, equipo de gama baja.
2. **Consulta de estado (`DLE EOT`)** para detectar papel y errores.
3. **Bloqueo de instancia única** para evitar dos procesos sobre los mismos contadores.
4. **Techo de numeración** o formato de cuatro dígitos.
5. **Feedback visual de hover** y eliminación del `Sleep(100)`.
6. **Limpieza del `.rc`** (quitar `btn_negro.bmp`) y de las librerías no usadas en el `Makefile`.
7. **Instalador y autoarranque** documentados y probados en un equipo limpio.
8. **Registro de tickets** para auditoría, si la operación lo requiere.

---

*Bitácora del Tótem de Autoatención. Se actualiza cuando cambia una decisión, no cuando cambia una línea.*
