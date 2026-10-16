#ifndef FILTRO_FIR
#define FILTRO_FIR

#include <stdint.h>
#include <stddef.h>

/* --------------------------------------------------------------------------
 * Filtro FIR
 * --------------------------------------------------------------------------
 * Fs = 16000 Hz
 * Fc = 4000 Hz
 * Comprimento: 10 taps
 * -------------------------------------------------------------------------- */
#define FIR_FS_HZ 16000
#define FIR_FC_HZ 4000
#define FIR_Q 15
#define N_FILTRO 31

/* Zera a linha de atraso. Chamar UMA vez na inicializacao.
 * NAO chamar na troca de buffer: o estado do FIR e continuo (README, sec. 4). */
void fir_init(void);
 
/* Filtra UMA amostra (Q15, inteiro). Segura para uso em ISR (sem FPU, sem malloc). */
int16_t fir_process_sample(int16_t x);

#endif