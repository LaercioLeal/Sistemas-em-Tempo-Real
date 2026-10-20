#ifndef EXEC_CICLICO
#define EXEC_CICLICO

#include <stdint.h>
#include <stdbool.h>

#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_task_wdt.h"
#include "projetoCfg.h"

/* [ALTERAÇÃO] Quadro (ciclo menor) F = 32 ms.
 * Regras: F divide H = 64 ms; F >= ΣC do quadro mais pesado (≈16,5 ms);
 *         2F - mdc(F,T) = 32 ms <= D = 64 ms.
 * Antes: 64 ms x 8 quadros = 512 ms de ciclo (período efetivo das tarefas = 512 ms). */
// #define CICLO_BASE_US 32000u
#define CICLO_BASE_US 4000u

/* [ALTERAÇÃO] Nº de quadros = H / F = 64 ms / 32 ms = 2. 
16 quadros*/
#define TOTAL_QUADROS (BLOCK_PERIOD_US / CICLO_BASE_US)

_Static_assert((BLOCK_PERIOD_US % CICLO_BASE_US) == 0,
               "CICLO_BASE_US deve dividir o periodo do bloco (64 ms)");

/* [ALTERAÇÃO] Fases das tarefas (todas com período de TOTAL_QUADROS quadros = 64 ms) */
#define Q_AQUISICAO     0   /* Quadros 0 e 8 T1 I2S + T2 FIR/push */
#define Q_FFT           2   // Quadros 1 e 9 (Q*2 % 16 = 2)
#define Q_CALC_EVENT    8   // Quadros 4 e 12
#define Q_IMPRESSAO     10  // Quadros 5 e 13

void set_exec_ciclico_flag(bool val);
void exec_ciclico_executar_quadro(uint32_t quadro);
gptimer_handle_t config_exec_ciclico_timer(void);
void exec_ciclico_imprime_tempos(void);   /* [ALTERAÇÃO] tabela de C_max para o relatório */

#endif