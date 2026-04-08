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
 * out_files : array de PendingFileName a llenar (puede ser NULL).
 * max_files : tamaño máximo del array (puede ser 0).
 * Si out_files==NULL o max_files==0: modo count-only — cuenta todos los archivos
 * sin almacenarlos (útil para observabilidad sin reservar buffer).
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

/*
 * Mueve pending_dir/filename a sent_dir/filename usando rename().
 * Crea sent_dir si no existe (un solo nivel).
 * rename() es atómico si ambos directorios están en el mismo filesystem.
 * Retorna 0 en éxito, -1 en error.
 */
int persist_move_to_sent(const char *pending_dir,
                         const char *sent_dir,
                         const char *filename);

/*
 * Elimina los archivos más antiguos de sent_dir si la cantidad total
 * supera max_files. Usa orden FIFO (lexicográfico = cronológico por nombre ts).
 * Solo elimina el exceso: si hay 1005 y max=1000, elimina los 5 más antiguos.
 * Retorna número de archivos eliminados, -1 en error.
 */
int persist_rotate_sent(const char *sent_dir, int max_files);

#endif /* PERSIST_H */
