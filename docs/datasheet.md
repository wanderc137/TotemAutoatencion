# Manual del operador — Tótem de Autoatención

**Versión documentada:** 1.0 (prototipo)
**Plataforma:** Windows 10 / 11 (x64)
**Pantalla objetivo:** vertical, 1080 × 1920 px
**Impresora:** térmica de 80 mm compatible ESC/POS (probada en diseño con Masung MS800 por puerto COM virtual)

> Este manual está escrito para la persona que **opera** el tótem en el local: la que abre el sistema por la mañana, entrega tickets, llama clientes y apaga el equipo al cerrar. No hace falta saber programar. Si necesitas compilar o modificar el programa, consulta `developer.md`.

---

## Índice

1. [Qué es este equipo](#1-qué-es-este-equipo)
2. [Cómo funciona en 60 segundos](#2-cómo-funciona-en-60-segundos)
3. [Partes del equipo](#3-partes-del-equipo)
4. [Puesta en marcha](#4-puesta-en-marcha)
5. [Operación diaria](#5-operación-diaria)
6. [Emitir un ticket, paso a paso](#6-emitir-un-ticket-paso-a-paso)
7. [El panel de llamados](#7-el-panel-de-llamados)
8. [Llamar a un cliente](#8-llamar-a-un-cliente)
9. [Mantenimiento](#9-mantenimiento)
10. [Especificaciones técnicas](#10-especificaciones-técnicas)
11. [Dependencias y compilación](#11-dependencias-y-compilación)
12. [Diagnóstico de problemas](#12-diagnóstico-de-problemas)
13. [Seguridad y precauciones](#13-seguridad-y-precauciones)
14. [Limitaciones conocidas](#14-limitaciones-conocidas)
15. [Glosario](#15-glosario)

---

## 1. Qué es este equipo

El Tótem de Autoatención es una aplicación de escritorio para Windows que convierte una pantalla táctil vertical en un **puesto de emisión de turnos**. El cliente llega, toca el servicio que necesita (Atención General, Créditos o Preferencial), el sistema le entrega un ticket impreso con un número correlativo —por ejemplo `A-041`— y el operador llama a ese número desde el panel de llamados de la misma pantalla.

El equipo **no necesita internet**, no usa base de datos y no requiere un servidor. Todo vive en una carpeta: el ejecutable, tres archivos de imagen incrustados en el binario, un archivo de texto con los llamados y un archivo binario con los contadores del día.

Es un proyecto de **prototipo**: el código está completo y compila, pero no se probó contra hardware físico (ni pantalla táctil real ni impresora real). Esta honestidad es parte del manual: si algo falla en la instalación, la sección de diagnóstico te dice qué mirar primero.

### Para quién es este manual

| Perfil | Qué encontrará aquí |
|---|---|
| Operador del local | Cómo abrir, emitir tickets, llamar clientes y cerrar |
| Responsable de instalación | Puesta en marcha, conexión de pantalla e impresora, verificación |
| Soporte técnico | Diagnóstico, códigos de síntoma y límites del sistema |
| Desarrollador | solo el resumen; el detalle está en `developer.md` |

---

## 2. Cómo funciona en 60 segundos

1. El programa arranca y se coloca a pantalla completa sobre el monitor vertical, siempre encima del resto de ventanas.
2. Muestra la **pantalla de reposo**: el título, una línea dorada y tres botones ovalados con su descripción.
3. Cada 5 segundos relee `llamados.txt`. Si hay llamados, los muestra en la mitad superior; si no, muestra "Sin llamados activos".
4. El cliente toca un botón. El programa incrementa el contador del servicio, arma el texto del ticket, lo envía a la impresora por el puerto serie y muestra el número en pantalla durante 2 segundos.
5. Pasados esos 2 segundos, la pantalla vuelve sola al panel de llamados y sigue refrescándose.

No hay menús, no hay ratón, no hay configuración en pantalla. Todo lo que cambia se toca o se edita en archivos.

---

## 3. Partes del equipo

| Parte | Función | Notas |
|---|---|---|
| Mini PC o PC integrado | Ejecuta `totem.exe` | Windows 10/11 x64; cualquier equipo con 2 GB de RAM alcanza |
| Monitor táctil vertical | Interfaz de cliente | 1080 × 1920 px recomendado; también acepta 1920 × 1080 si el sistema lo declara |
| Impresora térmica 80 mm | Entrega el ticket | Compatible ESC/POS; en el diseño: Masung MS800 |
| Cable USB de impresora | Datos y alimentación | Crea un puerto COM virtual en el sistema |
| Cable HDMI / DisplayPort | Video | El táctil suele ir por USB aparte |
| Teclado (opcional, solo instalación) | Salir del programa con `Esc` | No se entrega al público |
| Carpeta `C:\Totem\` | Programa y datos | Ruta de ejemplo; puede ser cualquier carpeta local |

### Sobre la impresora

La impresora no se instala con driver de Windows. El programa abre el **puerto COM virtual** que crea el driver USB de la impresora y le escribe bytes ESC/POS crudos. Eso evita el diálogo de impresión, la cola de documentos y los controladores genéricos: el ticket sale directo.

El programa busca la impresora en `COM1`…`COM20` a 9600 baudios, 8 bits, sin paridad, 1 bit de parada. Si no encuentra ningún puerto, intenta el puerto de la **impresora predeterminada** del sistema. Si tampoco, avisa con un cuadro de diálogo y no imprime.

---

## 4. Puesta en marcha

### Antes de encender

- [ ] Monitor táctil conectado y configurado en **vertical** (rotación 90° o 270° en el panel de video).
- [ ] Resolución del monitor en 1080 × 1920, o al menos que Windows reporte el modo.
- [ ] Impresora encendida, con rollo de 80 mm cargado y papel asomando.
- [ ] Driver USB de la impresora instalado (crea el COM virtual; consulta el manual del fabricante).
- [ ] Carpeta `C:\Totem\` copiada con `totem.exe`, `llamados.txt` y permisos de escritura.
- [ ] Probado un ticket de ejemplo (ver abajo).

### Primera ejecución

1. Copia la carpeta del programa a `C:\Totem\`.
2. Verifica que `llamados.txt` exista junto al ejecutable. Si no existe, el programa arranca igual y muestra "Sin llamados activos", pero conviene crearlo.
3. Haz doble clic en `totem.exe`. La ventana debe ocupar la pantalla vertical y quedar por encima de la barra de tareas.
4. Toca un botón. Debe salir un ticket y verse el número en pantalla durante 2 segundos.
5. Si el ticket no sale, revisa la sección [Diagnóstico](#12-diagnóstico-de-problemas).

### Arranque automático (opcional)

1. Pulsa `Win + R`, escribe `shell:startup` y acepta.
2. Crea un acceso directo a `totem.exe` dentro de esa carpeta.
3. Reinicia y confirma que el tótem aparece solo.

Para que un operador no cierre el programa por accidente, se puede configurar una cuenta de Windows sin privilegios y evitar que use `Alt + F4`; aun así, el programa acepta `Esc` y `F4` como salida deliberada (ver limitaciones).

---

## 5. Operación diaria

### Apertura

1. Enciende la impresora y espera a que termine de inicializarse (luz fija).
2. Enciende el PC. Si hay autoarranque, el tótem aparece solo.
3. Comprueba que la pantalla muestre "Sin llamados activos" o los llamados del día anterior si el archivo no se limpió.
4. Vacía `llamados.txt` si quedaron líneas viejas (ver [Llamar a un cliente](#8-llamar-a-un-cliente)).
5. Emite un ticket de prueba y verifica que el número sea el correcto y el papel salga cortado.

### Durante el día

- El programa no requiere atención. Solo se toca para que el cliente saque su ticket.
- Cada llamado se agrega al archivo; el panel se actualiza solo en 5 segundos.
- Los contadores **no se reinician** al mediodía: duran hasta el cambio de fecha del sistema.
- Si la impresora se queda sin papel, el programa **no lo detecta**; el ticket se pierde y aparece un aviso genérico. Recarga el rollo y repite.

### Cierre

1. Anota o respalda `contadores.dat` si necesitas el conteo del día.
2. Vacía `llamados.txt` para que el día siguiente empiece limpio.
3. Cierra el programa con `Esc` (el operador, no el público).
4. Apaga la impresora y el PC.

> No borres `contadores.dat` a mano salvo que quieras reiniciar la numeración. El programa lo reinicia solo al detectar un cambio de fecha.

---

## 6. Emitir un ticket, paso a paso

1. **El cliente toca el botón.** El área táctil es generosa: abarca toda la franja horizontal del botón más 20 px por arriba y por abajo. No hace falta precisión.
2. **El programa genera el número.** Toma el contador del servicio elegido, lo incrementa y lo formatea con tres dígitos: `A-001`, `B-014`, `C-007`.
3. **Arma el texto** con membrete, número, criterio, fecha, hora y la frase "Espere su turno".
4. **Envía los bytes a la impresora** por el puerto COM. La impresora imprime y corta.
5. **Muestra el número** en una tarjeta grande durante 2 segundos, para que el cliente lo memorice.
6. **Vuelve al panel** de llamados automáticamente.

### Qué contiene el ticket

```
================================
    TOTEM DE AUTOATENCION
================================

  Numero: A-041
  Criterio: Atencion General

  Fecha: miércoles, 12 de noviembre de 2026
  Hora: 10:32

  Espere su turno.
================================
```

El texto se genera con el formato regional del sistema: si Windows está en español, la fecha sale en español. El programa no usa tildes en los textos internos para evitar problemas de codificación en impresoras configuradas en CP1252.

### Si el ticket no sale

Aparece un cuadro de aviso: "No se pudo imprimir el ticket. Verifique que la impresora esté conectada y configurada como predeterminada." El contador **ya se incrementó** y el número mostrado en pantalla es válido aunque no haya papel. Anota el número, recarga la impresora y, si hace falta, entrega el ticket a mano.

---

## 7. El panel de llamados

La mitad superior de la pantalla es el panel de llamados. Muestra hasta **cuatro** llamados simultáneos, ordenados como aparecen en el archivo.

| Elemento | Descripción |
|---|---|
| Título | "LLAMADOS ACTIVOS" en dorado |
| Subtítulo | "Dirijase al modulo indicado" |
| Tarjetas | Número en grande + módulo destino |
| Sin llamados | Mensaje "Sin llamados activos" en gris |

El panel se refresca cada **5 segundos**. No hay botón de actualizar: si editas el archivo y no ves el cambio, espera unos segundos.

Si hay más de cuatro llamados en el archivo, solo se muestran los cuatro primeros. El resto queda en el archivo pero no se ve: conviene mantener el archivo corto.

---

## 8. Llamar a un cliente

Los llamados se editan en **`llamados.txt`**, un archivo de texto plano junto al ejecutable. Cada línea tiene el formato:

```
NUMERO|MODULO
```

Ejemplo:

```
A-41|Modulo 4
B-12|Modulo 1
C-3|Modulo 5
```

Reglas:

- El separador es la barra vertical `|`. Sin ella, la línea se ignora.
- Las líneas vacías se ignoran.
- Los espacios al inicio o al final se conservan; no los agregues.
- El número y el módulo son texto libre: `A-41`, `A-041` o `A-999` funcionan igual.
- El panel muestra **como máximo 4 líneas**. Para limpiar la pantalla, vacía el archivo.

### Cómo editarlo sin salir del tótem

1. Abre `llamados.txt` con el Bloc de notas.
2. Agrega la línea del llamado al final.
3. Guarda. En 5 segundos el panel se actualiza.

No hace falta reiniciar el programa. Si el archivo está abierto en otro programa, el tótem igual puede leerlo mientras el otro no lo bloquee.

> El archivo debe estar en la **misma carpeta** que `totem.exe`. El programa usa rutas relativas; si lo ejecutas desde otro directorio, buscará el archivo allí.

---

## 9. Mantenimiento

| Tarea | Frecuencia | Cómo |
|---|---|---|
| Recargar rollo | Según consumo | Abre la tapa, coloca el rollo con el papel saliendo por arriba, cierra |
| Limpiar cabezal | Semanal | Apaga la impresora; usa paño sin pelusa con alcohol isopropílico |
| Limpiar pantalla | Diario | Paño de microfibra ligeramente humedecido; nunca spray directo |
| Respaldar contadores | Diario o semanal | Copia `contadores.dat` a otra carpeta o pendrive |
| Limpiar llamados | Diario | Vacía `llamados.txt` al cerrar |
| Verificar fecha/hora | Semanal | El reinicio diario depende del reloj de Windows |

### Sobre el rollo de papel

Usa rollo térmico de 80 mm de ancho y 80 g/m². Papel de menor calidad se borra con el tiempo o con el calor. Guarda los tickets lejos de fuentes de calor y luz solar directa.

### Sobre los contadores

`contadores.dat` es un archivo binario pequeño (24 bytes) que guarda la fecha y el último número de cada servicio. No lo edites con el Bloc de notas: se corrompe. Si necesitas reiniciar la numeración, bórralo con el programa cerrado; al arrancar, los contadores vuelven a cero.

---

## 10. Especificaciones técnicas

| Parámetro | Valor |
|---|---|
| Sistema operativo | Windows 10 / 11, 64 bits |
| Lenguaje | C (Win32 API, sin frameworks) |
| Interfaz | GDI puro, sin controles estándar |
| Resolución de diseño | 1080 × 1920 px |
| Modo de ventana | `WS_POPUP`, siempre encima, sin bordes |
| Estados | Panel de llamados (reposo) y vista previa de ticket (2 s) |
| Servicios | 3: Atención General (A), Créditos (B), Preferencial (C) |
| Numeración | Correlativa por servicio, 3 dígitos, reinicio diario |
| Persistencia | `contadores.dat` (binario) y `llamados.txt` (texto) |
| Impresión | ESC/POS crudo sobre puerto serie virtual |
| Velocidad serie | 9600 baudios, 8N1, DTR y RTS activos |
| Detección de puerto | `COM1`…`COM20`, luego impresora predeterminada |
| Corte de papel | `ESC i` (1B 69) envía corte total; si el modelo no corta, probar `GS V` |
| Consumo de disco | < 1 MB de datos; ~6,9 MB el ejecutable |
| Dependencias externas | Ninguna en tiempo de ejecución |

### Recursos del ejecutable

| Recurso | Tamaño | Uso |
|---|---|---|
| `totem.exe` | ~6,9 MB | Programa completo con imágenes incrustadas |
| `btn_verde.bmp` | 1,4 MB | Botón Atención General |
| `btn_azul.bmp` | 1,4 MB | Botón Créditos |
| `btn_rojo.bmp` | 1,4 MB | Botón Preferencial |
| `btn_negro.bmp` | 2,5 MB | Fondo de botón (declarado, no usado) |

El tamaño del ejecutable se explica casi por completo por los bitmaps: se incrustan a 600 × 600 px y el linker no los comprime.

---

## 11. Dependencias y compilación

Este proyecto **no usa Python ni `requirements.txt`**: es C nativo compilado con MinGW-w64. Las "dependencias" son el compilador y las librerías de Windows que se enlazan.

| Dependencia | Versión sugerida | Para qué |
|---|---|---|
| MinGW-w64 (GCC) | 13 o superior | Compilar `src/*.c` |
| `windres` | incluido en MinGW-w64 | Compilar `res/buttons.rc` |
| Librería `gdi32` | del sistema | Dibujo de la interfaz |
| Librería `winspool` | del sistema | Consulta de la impresora predeterminada |
| Librería `msimg32` | del sistema | Degradados (`GradientFill`) |

### Compilar en Linux

```bash
sudo apt install mingw-w64
make
```

Genera `totem.exe` en la raíz del proyecto.

### Compilar en Windows

Con MinGW-w64 instalado y en el `PATH`:

```bat
gcc -Os -s -Wall -mwindows -o totem.exe src\*.c obj\buttons.o -lgdi32 -lwinspool -lcomctl32 -lmsimg32 -limm32
windres -i res\buttons.rc -o obj\buttons.o --input-format=rc --output-format=coff
```

El orden importa: `windres` primero, luego `gcc`. En Linux, `make` hace ambas cosas.

### Verificar la compilación

El ejecutable debe pesar alrededor de 6,9 MB. Si pesa mucho menos, los recursos no se enlazaron: revisa que `obj/buttons.o` exista y esté en la línea de enlace.

---

## 12. Diagnóstico de problemas

### La pantalla no aparece o se ve mal

| Síntoma | Causa probable | Solución |
|---|---|---|
| Ventana en horizontal | Monitor no rotado | Configura rotación en el panel de video |
| Ventana no cubre la pantalla | Resolución distinta de 1080 × 1920 | Ajusta la resolución o usa el modo soportado |
| Se ve la barra de tareas | Otra ventana robótica encima | Cierra aplicaciones; el tótem es *topmost*, no kiosco total |
| Todo negro al iniciar | Falta `msimg32.dll` o recurso | Reinstala el runtime de Windows; recompila |
| Texto cortado | Escalado de Windows al 125 % | Pon el escalado al 100 % en ese monitor |

### La impresora no imprime

| Síntoma | Causa probable | Solución |
|---|---|---|
| Aviso "No se pudo imprimir" | Puerto COM no detectado | Revisa el driver USB y el número de COM en Administrador de dispositivos |
| Sale en blanco | Cabezal sucio o papel al revés | Limpia el cabezal; revisa la orientación del rollo |
| Caracteres raros | Página de códigos distinta | El programa usa CP1252; configura la impresora igual |
| Corta antes de imprimir | Modelo con comando de corte distinto | Prueba la impresora con su utilidad oficial |
| Imprime solo la primera línea | Timeout serie demasiado corto | Aumenta `ReadTotalTimeoutConstant` en `printer.c` |
| No corta el papel | Modelo sin cutter | Corte manual; el comando se envía igual |

### El programa no arranca o se cierra

| Síntoma | Causa probable | Solución |
|---|---|---|
| No pasa de la pantalla de inicio | Ejecutable bloqueado por antivirus | Agrega excepción; el binario no está firmado |
| Se cierra al tocar un botón | `contadores.dat` corrupto | Bórralo con el programa cerrado |
| Se cierra solo | `llamados.txt` con caracteres raros | Guarda el archivo en UTF-8 sin BOM o ANSI |
| No responde al tacto | Táctil no calibrado | Calibra desde el panel de Windows |
| Se cierra con `Esc` | Comportamiento normal | Es la salida deliberada del operador |

### Diagnóstico rápido desde PowerShell

```powershell
# ¿Qué puertos COM existen?
Get-WmiObject Win32_SerialPort | Select-Object DeviceID, Description

# ¿Cuál es la impresora predeterminada?
Get-WmiObject Win32_Printer | Where-Object Default -eq $true | Select-Object Name, PortName

# ¿El proceso está corriendo?
Get-Process totem -ErrorAction SilentlyContinue
```

Si `PortName` de la impresora es `USB001` y no hay COM, el driver no creó puerto serie: instala el driver del fabricante, no el genérico.

---

## 13. Seguridad y precauciones

- **No expongas el equipo a internet.** No lo necesita y no tiene defensas de red.
- **Usa una cuenta de Windows sin privilegios** para el operador. El programa no requiere administrador.
- **El programa no es un kiosco cerrado.** Acepta `Esc` y `F4` para salir y no bloquea `Ctrl + Alt + Supr`. Si el público tiene acceso al teclado, un usuario puede cerrarlo.
- **Los archivos de datos no están protegidos.** Cualquiera con acceso a la carpeta puede editar `llamados.txt` o borrar `contadores.dat`.
- **No hay registro de auditoría.** Si dos operadores usan el equipo, no se puede saber quién emitió qué ticket.
- **Los tickets no son comprobantes fiscales.** Son turnos internos, sin valor tributario.
- **Riesgo eléctrico:** usa una regleta con protección para PC, monitor e impresora. No conectes la impresora a una toma conmutada por el mismo interruptor del monitor si eso provoca cortes en medio de una impresión.
- **Papel térmico:** no es tóxico, pero no lo ingieras ni lo uses en contacto con alimentos.

---

## 14. Limitaciones conocidas

Este es un prototipo y conviene saber exactamente qué **no** hace:

1. **No se probó con hardware real.** El código compila y la lógica está revisada, pero no hay evidencia de funcionamiento contra una pantalla táctil ni una Masung MS800 física. La primera instalación debe hacerse con acompañamiento técnico.
2. **La detección de impresora es ciega.** Abre cada puerto COM y prueba configurarlo; no pregunta "¿quién eres?" ni lee estado. Un dispositivo que acepte la configuración pero no sea la impresora puede confundir al programa.
3. **No hay confirmación de impresión.** El programa asume que si el puerto abrió, el ticket salió. No consulta el estado de la impresora ni el sensor de papel.
4. **No hay control de duplicados ni techo de numeración.** Después de `A-999` el formato pasa a `A-1000`, que rompe el ancho de tres dígitos en el ticket.
5. **Un solo tótem.** No hay sincronización entre equipos: dos tótems con la misma carpeta no comparten contadores.
6. **La interfaz se bloquea mientras imprime.** La escritura al puerto es síncrona; una impresora lenta congela la pantalla hasta terminar.
7. **Los textos no llevan tildes.** Es deliberado, para evitar problemas de codificación; se ve "Atencion", no "Atención".
8. **No hay actualización automática.** Cada cambio de versión implica reemplazar el ejecutable a mano.

Si tu operación necesita alguno de estos puntos, el sistema actual no alcanza: hay que extenderlo (ver `developer.md`).

---

## 15. Glosario

| Término | Significado |
|---|---|
| **Tótem** | Pantalla vertical de autoservicio donde el cliente interactúa |
| **Ticket** | Papel con el número de turno del cliente |
| **Llamado** | Aviso en pantalla que indica número y módulo |
| **Criterio** | Tipo de atención: general, créditos o preferencial |
| **ESC/POS** | Lenguaje de comandos de impresoras térmicas, creado por Epson |
| **Puerto COM virtual** | Puerto serie simulado por el driver USB de la impresora |
| **Contador** | Número correlativo del último ticket emitido por servicio |
| **CP1252** | Página de códigos de Windows para Europa occidental |
| **Topmost** | Ventana que se dibuja por encima de las demás |
| **GDI** | Interfaz gráfica de Windows usada para dibujar la pantalla |

---

*Manual del operador — Tótem de Autoatención. Para detalle técnico, consulta `docs/developer.md`; para la bibliografía, `docs/REFERENCES.md`.*
