#ifndef PERSIST_H
#define PERSIST_H

/* Nombre de un archivo pendiente — solo el basename: "<ts>.json" */
typedef char PendingFileName[64];

/*
 * Guarda la cadena JSON en dir_path/<ts>.json.
 * Crea el directorio si no existe (un solo nivel).
 * Retorna 0 en éxito, -1 en error.
 */
int persist_write(const char *dir_path, long ts, const char *json);

/*
 * Lista los archivos .json en dir_path ordenados por nombre (FIFO cronológico).
 * out_files : array de PendingFileName a llenar.
 * max_files : tamaño máximo del array.
 * Retorna número de archivos encontrados, -1 si error al abrir el directorio.
 */
int persist_list_pending(const char *dir_path,
                         PendingFileName *out_files,
                         int max_files);

/*
 * Lee el contenido de dir_path/filename en un buffer heap.
 * El llamador debe liberar con free().
 * Retorna puntero al string, NULL en error.
 */
char *persist_read_file(const char *dir_path, const char *filename);

/*
 * Elimina dir_path/filename.
 * Retorna 0 en éxito, -1 en error.
 */
int persist_delete(const char *dir_path, const char *filename);

#endif /* PERSIST_H */
