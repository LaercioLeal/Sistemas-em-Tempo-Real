#ifndef PROJETO_CFG_H
#define PROJETO_CFG_H

#include <stdint.h>

#define AUDIO_FS_HZ      16000u
#define BLOCK_SIZE       1024u
#define BLOCK_PERIOD_US  ((1000000u * BLOCK_SIZE) / AUDIO_FS_HZ) /* 64000 us */

/* [ALTERAÇÃO] A energia agora é E = (1/N)·Σy² (README §7), não mais RMS.
 * 0.09 equivale ao limiar anterior (0.3 RMS → 0.3² = 0.09). Recalibre com palma/batida. */
#define LIMIAR_ENERGIA 0.09f
#define REFRACTORY_BLOCKS 10

/* [ALTERAÇÃO] Faixa de busca do pico (README §8) */
#define FFT_FMIN_HZ 80u
#define FFT_FMAX_HZ 1200u

#define PRINT_PERIOD_BLOCKS 16u

typedef int16_t amostra_t;

#endif