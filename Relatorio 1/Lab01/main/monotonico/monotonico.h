#ifndef MONOTONICO
#define MONOTONICO

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "driver/gpio.h"
#include "driver/gptimer.h"

#define RM_TICK_US          1000u       /* período do tick: 1 ms              */
#define RM_TIMER_RES_HZ     1000000u    /* resolução do timer: 1 MHz (1 us)   */

typedef void (*rm_funcao_t)(void);
 
typedef struct {
    const char          *nome;            /* nome da tarefa                    */
    rm_funcao_t          funcao;          /* função a ser executada            */
    uint32_t             periodo_ms;      /* período (e deadline) em ms        */
 
    volatile uint32_t    tempo_restante;  /* ticks até a próxima liberação     */
    volatile uint32_t    proxima;         /* flag: 1 = é a próxima a executar  */
    volatile uint32_t    pendentes;       /* liberações ainda não executadas   */
    volatile uint32_t    executando;      /* 1 enquanto a função está rodando  */
 
    volatile uint32_t    execucoes;       /* estatística: execuções completas  */
    volatile uint32_t    perdas;          /* estatística: deadlines perdidos   */
} rm_tarefa_t;

/*
 * tempo_restante inicia em 1 para que todas as tarefas sejam liberadas no
 * primeiro tick (instante crítico, pior caso para o RM).
 */
#define RM_TAREFA(n, f, p) \
    { .nome = (n), .funcao = (f), .periodo_ms = (p), .tempo_restante = 1u }

void tarefa_i2s_mic(void);
void tarefa_fir(void);
void tarefa_fft(void);
void tarefa_calcula_energia(void);
void tarefa_detecta_evento(void);
void tarefa_identifica_nota(void);
void tarefa_impressao(void);

#define RM_NUM_TAREFAS  (sizeof(tabela) / sizeof(tabela[0]))
 
void monotonico_init(void);
void monotonico_despachar(void);

#endif