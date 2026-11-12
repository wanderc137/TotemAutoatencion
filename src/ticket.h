#ifndef TICKET_H
#define TICKET_H

void Ticket_Generate(const char* prefix, char* out, int maxlen);
void Ticket_FormatOutput(char* out, int maxlen, const char* numero, const char* criterio);
void Ticket_CheckDailyReset(void);

#endif
