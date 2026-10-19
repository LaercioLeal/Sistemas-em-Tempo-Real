#include "bffPingPong.h"

static int primary_buffer[BUFFERLENGTH];
static int secondary_buffer[BUFFERLENGTH];

static int *write_buffer;
static int *read_buffer;
static volatile int write_counter;
static volatile bool can_read;   /* bloco completo aguardando o consumidor */
static volatile bool read_busy;  /* [ALTERAÇÃO] consumidor está processando read_buffer */

void Buffer_init(void)
{
    write_buffer = primary_buffer;
    read_buffer = secondary_buffer;
    write_counter = 0;
    can_read = false;
    read_busy = false;
}

/* [ALTERAÇÃO] Troca só se o outro buffer já foi consumido E liberado.
 * Atende README §6: nunca sobrescrever um buffer antes do fim do processamento. */
static bool Buffer_try_swap(void)
{
    if (can_read || read_busy)
    {
        return false;
    }
    int *tmp = write_buffer;
    write_buffer = read_buffer;
    read_buffer = tmp;
    write_counter = 0;
    can_read = true;
    return true;
}

bool Buffer_push(int value)
{
    /* troca que ficou pendente por buffer ocupado */
    if (write_counter >= BUFFERLENGTH && !Buffer_try_swap())
    {
        return false; /* overrun: amostra descartada */
    }

    write_buffer[write_counter] = value;
    write_counter++;

    /* [ALTERAÇÃO] Troca IMEDIATA ao completar o bloco (README §4).
     * Antes: a troca só ocorria no primeiro push do bloco seguinte. */
    if (write_counter >= BUFFERLENGTH)
    {
        Buffer_try_swap();
    }
    return true;
}

int *Buffer_read(void)
{
    if (can_read)
    {
        can_read = false;   /* [ALTERAÇÃO] consome: o mesmo bloco não é lido duas vezes */
        read_busy = true;
        return read_buffer;
    }
    return NULL;
}

void Buffer_release(void)
{
    read_busy = false;
}