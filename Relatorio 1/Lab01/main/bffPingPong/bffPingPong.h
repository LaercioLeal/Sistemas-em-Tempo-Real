#ifndef BUFFER_PING_PONG

#define BUFFER_PING_PONG

#include "projetoCfg.h"

#define BUFFERLENGTH BLOCK_SIZE

void Buffer_init();

bool Buffer_push(int value);

int * Buffer_read();

#endif