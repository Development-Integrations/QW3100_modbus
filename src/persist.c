#include "persist.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

int persist_write(const char *dir_path, long ts, const char *json)
{
    /* Crear directorio si no existe */
    if (mkdir(dir_path, 0755) == -1 && errno != EEXIST)
    {
        fprintf(stderr, "[persist] No se pudo crear directorio '%s': %s\n",
                dir_path, strerror(errno));
        return -1;
    }

    /* Construir ruta completa: <dir_path>/<ts>.json */
    char path[256];
    int n = snprintf(path, sizeof(path), "%s/%ld.json", dir_path, ts);
    if (n < 0 || (size_t)n >= sizeof(path))
    {
        fprintf(stderr, "[persist] Ruta demasiado larga para '%s'\n", dir_path);
        return -1;
    }

    FILE *f = fopen(path, "w");
    if (f == NULL)
    {
        fprintf(stderr, "[persist] No se pudo abrir '%s' para escritura: %s\n",
                path, strerror(errno));
        return -1;
    }

    if (fputs(json, f) == EOF)
    {
        fprintf(stderr, "[persist] Error al escribir en '%s': %s\n",
                path, strerror(errno));
        fclose(f);
        return -1;
    }

    fclose(f);
    return 0;
}
