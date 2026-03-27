#include "circuit_breaker.h"
#include "logger.h"

#include <time.h>

void cb_init(CircuitBreaker *cb)
{
    cb->state           = CB_CLOSED;
    cb->fail_count      = 0;
    cb->next_retry      = 0;
    cb->current_timeout = 0;
}

int cb_allow_send(CircuitBreaker *cb)
{
    if (cb->state == CB_CLOSED)
        return 1;

    if (cb->state == CB_OPEN)
    {
        long remaining = (long)(cb->next_retry - time(NULL));
        if (remaining > 0)
        {
            LOG_WARN("[cb] OPEN — envío pausado (%ld s restantes)", remaining);
            return 0;
        }
        cb->state = CB_HALF_OPEN;
        LOG_INFO("[cb] HALF_OPEN — probando conexión");
    }

    /* CB_HALF_OPEN: permite un intento */
    return 1;
}

void cb_on_success(CircuitBreaker *cb)
{
    if (cb->state != CB_CLOSED)
        LOG_INFO("[cb] CLOSED — circuito restablecido tras éxito");

    cb->state           = CB_CLOSED;
    cb->fail_count      = 0;
    cb->current_timeout = 0;
}

/* Abre el circuito con backoff exponencial */
static void open_circuit(CircuitBreaker *cb,
                          uint32_t open_timeout_sec,
                          uint32_t backoff_max_sec)
{
    if (cb->current_timeout == 0)
        cb->current_timeout = open_timeout_sec;
    else
    {
        cb->current_timeout *= 2;
        if (cb->current_timeout > backoff_max_sec)
            cb->current_timeout = backoff_max_sec;
    }

    cb->state      = CB_OPEN;
    cb->next_retry = time(NULL) + (time_t)cb->current_timeout;

    LOG_WARN("[cb] OPEN — reintentará en %u s", cb->current_timeout);
}

void cb_on_transient_fail(CircuitBreaker *cb,
                           uint8_t  fail_threshold,
                           uint32_t open_timeout_sec,
                           uint32_t backoff_max_sec)
{
    cb->fail_count++;
    LOG_WARN("[cb] fallo transitorio %d/%d", cb->fail_count, (int)fail_threshold);

    /* Abre si alcanza el umbral o si ya estaba en HALF_OPEN (prueba fallida) */
    if (cb->fail_count >= (int)fail_threshold || cb->state == CB_HALF_OPEN)
        open_circuit(cb, open_timeout_sec, backoff_max_sec);
}

void cb_on_persistent_fail(CircuitBreaker *cb,
                            uint32_t open_timeout_sec,
                            uint32_t backoff_max_sec)
{
    LOG_ERROR("[cb] fallo persistente — abriendo circuito inmediatamente");
    cb->fail_count = 0;
    open_circuit(cb, open_timeout_sec, backoff_max_sec);
}
