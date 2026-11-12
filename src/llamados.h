#ifndef LLAMADOS_H
#define LLAMADOS_H

typedef struct {
    char numero[20];
    char modulo[50];
} Llamado;

int Llamados_Load(const char* filename, Llamado** out, int* count);
void Llamados_Free(Llamado* llamados, int count);

#endif
