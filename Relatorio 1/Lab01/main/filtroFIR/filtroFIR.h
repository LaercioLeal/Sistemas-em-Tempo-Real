#ifndef FILTRO_FIR
#define FILTRO_FIR

#include <stdint.h>
#include "projetoCfg.h"   /* [ALTERAÇÃO] AUDIO_FS_HZ e BLOCK_SIZE são usados neste header */

/* --------------------------------------------------------------------------
 * Filtro FIR
 * --------------------------------------------------------------------------
 * Fs = 16000 Hz
 * Fc = 4000 Hz
 * Comprimento: 10 taps
 * -------------------------------------------------------------------------- */
#define FIR_FS_HZ AUDIO_FS_HZ
#define FIR_FC_HZ 4000
#define FIR_Q 15
#define N_FILTRO 10

/* Tamanho do bloco filtrado (= tamanho do bloco do projeto) */
#define FIR_OUT_SIZE BLOCK_SIZE
 
/* Zera a linha de atraso. Chamar UMA vez na inicializacao.
 * NAO chamar na troca de buffer: o estado do FIR e continuo (README, sec. 4). */
void fir_init(void);
 
/* Filtra UMA amostra (Q15, inteiro). Sem FPU, sem malloc. */
int16_t fir_process_sample(int16_t x);

#endif