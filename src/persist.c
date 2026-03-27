#include "persist.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>

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

/* Comparador para qsort — orden lexicográfico = cronológico para nombres timestamp */
static int cmp_filenames(const void *a, const void *b)
{
    return strcmp((const char *)a, (const char *)b);
}

int persist_list_pending(const char *dir_path,
                         PendingFileName *out_files,
                         int max_files)
{
    DIR *dir = opendir(dir_path);
    if (dir == NULL)
    {
        fprintf(stderr, "[persist] No se pudo abrir directorio '%s': %s\n",
                dir_path, strerror(errno));
        return -1;
    }

    /* Modo count-only: out_files==NULL y max_files==0 cuenta todos sin almacenar */
    int count = 0;
    int count_only = (out_files == NULL || max_files == 0);
    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;

        size_t len = strlen(entry->d_name);
        if (len < 6 || strcmp(entry->d_name + len - 5, ".json") != 0)
            continue;

        if (!count_only && count < max_files)
        {
            strncpy(out_files[count], entry->d_name, sizeof(PendingFileName) - 1);
            out_files[count][sizeof(PendingFileName) - 1] = '\0';
        }
        count++;
        if (!count_only && count >= max_files)
            break;
    }

    closedir(dir);
    if (!count_only)
        qsort(out_files, count, sizeof(PendingFileName), cmp_filenames);
    return count;
}

char *persist_read_file(const char *dir_path, const char *filename)
{
    char path[256];
    int n = snprintf(path, sizeof(path), "%s/%s", dir_path, filename);
    if (n < 0 || (size_t)n >= sizeof(path))
        return NULL;

    FILE *f = fopen(path, "r");
    if (f == NULL)
    {
        fprintf(stderr, "[persist] No se pudo leer '%s': %s\n",
                path, strerror(errno));
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    char *buf = malloc((size_t)size + 1);
    if (buf == NULL)
    {
        fclose(f);
        return NULL;
    }

    fread(buf, 1, (size_t)size, f);
    buf[size] = '\0';
    fclose(f);
    return buf;
}

int persist_delete(const char *dir_path, const char *filename)
{
    char path[256];
    int n = snprintf(path, sizeof(path), "%s/%s", dir_path, filename);
    if (n < 0 || (size_t)n >= sizeof(path))
        return -1;

    if (remove(path) != 0)
    {
        fprintf(stderr, "[persist] No se pudo eliminar '%s': %s\n",
                path, strerror(errno));
        return -1;
    }
    return 0;
}
