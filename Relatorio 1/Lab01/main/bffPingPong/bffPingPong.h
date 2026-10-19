#ifndef BUFFER_PING_PONG
#define BUFFER_PING_PONG

#include <stdbool.h>          /* [ALTERAÇÃO] 'bool' sem stdbool.h não compila isoladamente */
#include "projetoCfg.h"

#define BUFFERLENGTH BLOCK_SIZE

void Buffer_init(void);
bool Buffer_push(int value);   /* false = amostra perdida (outro buffer ainda em uso) */
int *Buffer_read(void);        /* devolve bloco pronto UMA vez; NULL se não há novo */
void Buffer_release(void);     /* [ALTERAÇÃO] consumidor terminou de processar o bloco */

#endif