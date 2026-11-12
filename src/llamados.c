#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "llamados.h"

int Llamados_Load(const char* filename, Llamado** out, int* count)
{
    FILE* f = fopen(filename, "r");
    if (!f) {
        *out   = NULL;
        *count = 0;
        return 0;
    }

    int capacity = 10;
    int cnt      = 0;
    Llamado* llamados = (Llamado*)malloc(sizeof(Llamado) * capacity);
    if (!llamados) {
        fclose(f);
        *out   = NULL;
        *count = 0;
        return 0;
    }

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        size_t len = strlen(line);
        while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
            line[--len] = '\0';

        if (len == 0)
            continue;

        char* sep = strchr(line, '|');
        if (!sep)
            continue;

        *sep = '\0';

        if (cnt >= capacity) {
            capacity *= 2;
            Llamado* tmp = (Llamado*)realloc(llamados, sizeof(Llamado) * capacity);
            if (!tmp) {
                free(llamados);
                fclose(f);
                *out   = NULL;
                *count = 0;
                return 0;
            }
            llamados = tmp;
        }

        strncpy(llamados[cnt].numero, line, sizeof(llamados[cnt].numero) - 1);
        llamados[cnt].numero[sizeof(llamados[cnt].numero) - 1] = '\0';

        strncpy(llamados[cnt].modulo, sep + 1, sizeof(llamados[cnt].modulo) - 1);
        llamados[cnt].modulo[sizeof(llamados[cnt].modulo) - 1] = '\0';

        cnt++;
    }

    fclose(f);
    *out   = llamados;
    *count = cnt;
    return 1;
}

void Llamados_Free(Llamado* llamados, int count)
{
    (void)count;
    free(llamados);
}
