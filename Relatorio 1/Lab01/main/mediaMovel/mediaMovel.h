#ifndef MEDIA_MOVEL

#define MEDIA_MOVEL

#include <stdint.h>
#include <stdbool.h>
#include "bffPingPong.h"

// Tamanho da janela da média móvel (em amostras)
#ifndef MEDIA_MOVEL_TAMANHO
#define MEDIA_MOVEL_TAMANHO (BLOCK_SIZE/2)
#endif

/**
 * Zera a janela e os acumuladores da média móvel.
 */
void media_movel_init(void);

/**
 * Verifica se há um novo bloco pronto no buffer ping pong. Se houver,
 * processa todas as amostras do bloco (potência = V^2) e atualiza a média.
 *
 * @return Valor atual da média móvel (potência média).
 *         Retorna o último valor calculado se não houver bloco novo.
 */
float calc_media_movel(int * bloco);

#endif