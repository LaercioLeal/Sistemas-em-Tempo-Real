#include "bffPingPong.h"

static int primary_buffer[BUFFERLENGTH];
static int secondary_buffer[BUFFERLENGTH];

static int * write_buffer;
static int * read_buffer;
static int write_counter;
static bool primary_signal; // Indica se esta utilizando o primeiro ou o segundo buffer.
static bool can_read; // Indica se um novo buffer esta pronto para leitura

void Buffer_init()
{
    write_buffer = primary_buffer;
    read_buffer = secondary_buffer;
    write_counter = 0;
    primary_signal = true;
    can_read = false;
}

bool Buffer_push(int value)
{
    if (write_counter >= BUFFERLENGTH || write_buffer == nullptr)
    {

        if (primary_signal)
        { // Verifica se esta usando o buffer primario para escrita
            write_buffer = secondary_buffer;
            read_buffer = primary_buffer;
        }
        else
        {
            write_buffer = primary_buffer;
            read_buffer = secondary_buffer;
        }
        write_counter = 0;

        primary_signal = !primary_signal;
        can_read = true;
    }

    if (write_counter < BUFFERLENGTH)
    {
        write_buffer[write_counter] = value;
        write_counter++;
        return true;
    }
    return false;
}

int *Buffer_read()
{
    if (can_read)
    {
        return read_buffer;
    }

    return nullptr;
}