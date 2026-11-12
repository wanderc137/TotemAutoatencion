#ifndef PRINTER_H
#define PRINTER_H

/*
 * printer.h — Impresion ESC/POS directa a impresora termica
 *
 * Envio directo de comandos ESC/POS al puerto COM virtual USB
 * de la impresora (Masung MS800 o compatible). No requiere driver.
 */

int Printer_PrintTicket(const char* text);
int Printer_IsAvailable(void);

#endif
