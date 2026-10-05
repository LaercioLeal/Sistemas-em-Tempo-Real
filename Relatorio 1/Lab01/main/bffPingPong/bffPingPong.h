#ifndef BUFFER_PING_PONG

#define BUFFER_PING_PONG

#define BUFFERLENGTH 1024

void Buffer_init();

bool Buffer_push(int value);

int * Buffer_read();

#endif