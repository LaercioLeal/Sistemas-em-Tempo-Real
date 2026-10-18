#ifndef PROJETO_CFG_H
#define PROJETO_CFG_H

#include <stdint.h>

/* Parametros unicos do projeto (README, secao 2). Nao repetir em outros arquivos. */
#define AUDIO_FS_HZ      16000u
#define BLOCK_SIZE       1024u                                   /* amostras por bloco / FFT */
#define BLOCK_PERIOD_US  ((1000000u * BLOCK_SIZE) / AUDIO_FS_HZ) /* 64000 us = 64 ms */

#define LIMIAR_ENERGIA 0.3f
#define REFRACTORY_BLOCKS 10

/* Amostra ja filtrada (int16). O FIR roda em ISR (sem FPU), por isso e inteira.
 * A conversao para float deve ser feita na tarefa da FFT (contexto normal). */
typedef int16_t amostra_t;

#endif