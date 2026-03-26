#ifndef PERSIST_H
#define PERSIST_H

/*
 * Capa de persistencia local.
 *
 * persist_write() guarda la cadena JSON en un archivo dentro de `dir_path`.
 * El nombre del archivo es el timestamp Unix en segundos: <ts>.json
 *
 * Crea el directorio si no existe (un solo nivel; no crea padres).
 *
 * Retorna  0 en éxito, -1 en error (mensaje en stderr).
 */
int persist_write(const char *dir_path, long ts, const char *json);

#endif /* PERSIST_H */
