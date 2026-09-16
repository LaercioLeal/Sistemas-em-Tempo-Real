#ifndef BUFFER_PING_PONG

#define BUFFER_PING_PONG

#define BUFFERLENGTH 1024

static int primary_buffer[BUFFERLENGTH];
static int secondary_buffer[BUFFERLENGTH];

int * write_buffer;
int * read_buffer;
int write_counter;
bool primary_signal; // Indica se esta utilizando o primeiro ou o segundo buffer.
bool can_read; // Indica se um novo buffer esta pronto para leitura

void Buffer_init();

bool Buffer_push(int value);

int * Buffer_read();

#endif