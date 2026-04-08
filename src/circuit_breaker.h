#ifndef CIRCUIT_BREAKER_H
#define CIRCUIT_BREAKER_H

#include <time.h>
#include <stdint.h>

typedef enum
{
    CB_CLOSED = 0,  /* normal — envía datos */
    CB_OPEN,        /* pausado — demasiados fallos consecutivos */
    CB_HALF_OPEN    /* probando si el servidor volvió (1 intento) */
} CbState;

typedef struct
{
    CbState  state;
    int      fail_count;        /* fallos transitorios consecutivos */
    time_t   next_retry;        /* timestamp mínimo para volver a intentar */
    uint32_t current_timeout;   /* timeout actual en segundos (crece con backoff) */
} CircuitBreaker;

/* Inicializa el circuit breaker en estado CLOSED */
void cb_init(CircuitBreaker *cb);

/*
 * Devuelve 1 si se permite enviar ahora, 0 si el circuito está abierto.
 * Transiciona automáticamente de OPEN a HALF_OPEN cuando expira next_retry.
 */
int cb_allow_send(CircuitBreaker *cb);

/* Llamar tras cada envío exitoso (HTTP 200) — resetea el circuito a CLOSED */
void cb_on_success(CircuitBreaker *cb);

/*
 * Evalúa si el timeout de un CB en OPEN ha expirado y lo transiciona a
 * HALF_OPEN. Usar para "tickear" el CB primario cuando se está enviando
 * por el CB de failover, sin logs ruidosos de "envío pausado".
 */
void cb_maybe_recover(CircuitBreaker *cb);

/*
 * Llamar tras error transitorio (timeout, 5xx, sin conexión).
 * Incrementa fail_count. Abre el circuito si alcanza fail_threshold.
 * fail_threshold   : número de fallos consecutivos para abrir el circuito
 * open_timeout_sec : tiempo base de espera al abrir (segundos)
 * backoff_max_sec  : límite máximo del backoff exponencial (segundos)
 */
void cb_on_transient_fail(CircuitBreaker *cb,
                           uint8_t  fail_threshold,
                           uint32_t open_timeout_sec,
                           uint32_t backoff_max_sec);

/*
 * Llamar tras error persistente (400, 401, 403).
 * Abre el circuito inmediatamente — no tiene sentido reintentar sin
 * corregir la configuración (credenciales, endpoint).
 */
void cb_on_persistent_fail(CircuitBreaker *cb,
                            uint32_t open_timeout_sec,
                            uint32_t backoff_max_sec);

#endif /* CIRCUIT_BREAKER_H */
