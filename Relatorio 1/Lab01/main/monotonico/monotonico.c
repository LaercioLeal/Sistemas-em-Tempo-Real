#include "monotonico.h"
#include <stdio.h>
#include "i2sMic.h"
#include "filtroFIR.h"
#include "bffPingPong.h"
#include "fft.h"
#include "notaMusical.h"
#include "calcEnergia.h"
#include "detectEvent.h"

static volatile uint32_t rm_tick_ms = 0;   /* contador global de ticks (ms)  */

// Constantes de Tarefa
volatile bool flag_i2s_ready = false;
volatile bool flag_fir_ready = false;
volatile bool flag_fft_ready = false;
volatile bool flag_nota_ready = false;
volatile bool flag_energia_ready = false;
volatile bool flag_evento_pendente = false;

static int bff_ic2_mic[BLOCK_SIZE];
static int *buffer_atual = NULL;
static float freq_dominante = 0.0f;
static float energia = 0.0f;
static NotaInfo nota_atual;

// TABELA DE TAREFAS
rm_tarefa_t tabela[] = {
    RM_TAREFA("i2s_mic", tarefa_i2s_mic, 64),
    RM_TAREFA("fir", tarefa_fir, 64),
    RM_TAREFA("fft", tarefa_fft, 64),
    RM_TAREFA("calc_energia", tarefa_calcula_energia, 64),
    RM_TAREFA("detecta_evento", tarefa_detecta_evento, 64),
    RM_TAREFA("identifica_nota", tarefa_identifica_nota, 64),
    RM_TAREFA("imprime_resultados", tarefa_impressao, 1024),
};

/* Ordena a tabela por período crescente (insertion sort, estável).
 * Deve ser chamada ANTES de iniciar o timer. */
static void rm_ordenar_tabela(void)
{
    for (size_t i = 1; i < RM_NUM_TAREFAS; i++) {
        rm_tarefa_t chave = tabela[i];
        size_t j = i;
        while (j > 0 && tabela[j - 1].periodo_ms > chave.periodo_ms) {
            tabela[j] = tabela[j - 1];
            j--;
        }
        tabela[j] = chave;
    }
}
 
/* Retorna o índice da tarefa de maior prioridade com liberação pendente,
 * ou -1 se não houver nenhuma. */
static inline int IRAM_ATTR rm_escolher_pendente(void)
{
    for (size_t i = 0; i < RM_NUM_TAREFAS; i++) {
        if (tabela[i].pendentes > 0) {
            return (int)i;
        }
    }
    return -1;
}

// ISR do escalonador - executa a cada 1 ms
static bool IRAM_ATTR rm_timer_isr(gptimer_handle_t timer,
                                   const gptimer_alarm_event_data_t *edata,
                                   void *user_ctx)
{
    (void)timer;
    (void)edata;
    (void)user_ctx;
 
    rm_tick_ms++;
 
    /* 1) Atualiza os contadores e libera as tarefas cujo tempo expirou */
    for (size_t i = 0; i < RM_NUM_TAREFAS; i++) {
        rm_tarefa_t *t = &tabela[i];
 
        if (--t->tempo_restante == 0) {
            t->tempo_restante = t->periodo_ms;
 
            /* Se a liberação anterior ainda não terminou, perdeu o deadline */
            if (t->pendentes > 0 || t->executando) {
                t->perdas++;
            }
            t->pendentes++;
        }
    }
 
    /* 2) Define qual é a próxima tarefa a executar (maior prioridade RM) */
    int escolhida = rm_escolher_pendente();
    for (size_t i = 0; i < RM_NUM_TAREFAS; i++) {
        tabela[i].proxima = ((int)i == escolhida) ? 1u : 0u;
    }
 
    return false;   /* não há task do FreeRTOS para acordar */
}

/* Consome atomicamente uma liberação pendente. false se não havia nenhuma. */
static bool rm_consumir_pendencia(rm_tarefa_t *t)
{
    uint32_t v = __atomic_load_n(&t->pendentes, __ATOMIC_RELAXED);
    while (v > 0) {
        if (__atomic_compare_exchange_n(&t->pendentes, &v, v - 1, false,
                                        __ATOMIC_SEQ_CST, __ATOMIC_RELAXED)) {
            return true;
        }
        /* v foi atualizado pelo CAS: tenta novamente */
    }
    return false;
}
 
/* Procura (em ordem de prioridade) a tarefa sinalizada e limpa a flag de
 * forma atômica. Retorna NULL se nenhuma estiver sinalizada ou se a ISR
 * reatribuiu a flag no meio da busca (o loop principal simplesmente repete). */
static rm_tarefa_t *rm_obter_proxima(void)
{
    for (size_t i = 0; i < RM_NUM_TAREFAS; i++) {
        if (tabela[i].proxima) {
            if (__atomic_exchange_n(&tabela[i].proxima, 0u, __ATOMIC_SEQ_CST)) {
                return &tabela[i];
            }
            return NULL;
        }
    }
    return NULL;
}


/* Após terminar uma tarefa (ou em idle), sinaliza a próxima pendente sem
 * esperar o próximo tick. Apenas ativa a flag, nunca a limpa. */
static void rm_sinalizar_proxima(void)
{
    int idx = rm_escolher_pendente();
    if (idx >= 0) {
        __atomic_store_n(&tabela[idx].proxima, 1u, __ATOMIC_SEQ_CST);
    }
}
 
void monotonico_despachar(void)
{
    for (;;) {
        rm_tarefa_t *t = rm_obter_proxima();
 
        if (t != NULL && rm_consumir_pendencia(t)) {
            t->executando = 1u;
            t->funcao();                 /* roda até o fim: sem preempção */
            t->executando = 0u;
            t->execucoes++;
        }
 
        /* Garante que, se ainda há tarefas pendentes, uma delas esteja
         * marcada como "próxima" antes de a varredura recomeçar. */
        rm_sinalizar_proxima();
    }
}

static void rm_timer_iniciar(void)
{
    gptimer_handle_t timer = NULL;
 
    gptimer_config_t cfg = {
        .clk_src       = GPTIMER_CLK_SRC_DEFAULT,
        .direction     = GPTIMER_COUNT_UP,
        .resolution_hz = RM_TIMER_RES_HZ,
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&cfg, &timer));
 
    gptimer_event_callbacks_t cbs = {
        .on_alarm = rm_timer_isr,
    };
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(timer, &cbs, NULL));
    ESP_ERROR_CHECK(gptimer_enable(timer));
 
    gptimer_alarm_config_t alarme = {
        .reload_count = 0,
        .alarm_count  = RM_TICK_US,                  /* 1000 us = 1 ms */
        .flags.auto_reload_on_alarm = true,
    };
    ESP_ERROR_CHECK(gptimer_set_alarm_action(timer, &alarme));
    ESP_ERROR_CHECK(gptimer_start(timer));
}
 
void monotonico_init(void)
{
    rm_ordenar_tabela();     /* RM: menor período = maior prioridade */
    rm_timer_iniciar();      /* a partir daqui a ISR roda a cada 1 ms */
}

// Tarefas
void tarefa_i2s_mic(void)
{
    flag_i2s_ready = false;

    /* [ALTERAÇÃO] Só libera o FIR se o bloco chegou completo (antes o retorno era ignorado) */
    if (mic_read_filter_push(bff_ic2_mic) == I2S_BUFFER_SIZE)
    {
        flag_i2s_ready = true;
    }
}

void tarefa_fir(void)
{
    if (!flag_i2s_ready)
    {
        return;
    }

    flag_fir_ready = false;

    for (int i = 0; i < I2S_BUFFER_SIZE; i++)
    {
        int16_t y = fir_process_sample(bff_ic2_mic[i]);

        if (!Buffer_push(y)) /* [ALTERAÇÃO] detecta overrun do ping-pong */
        {
            amostras_perdidas++;
        }
    }

    flag_i2s_ready = false; /* [ALTERAÇÃO] bloco bruto consumido */
    flag_fir_ready = true;
}

void tarefa_fft(void)
{
    
    if (buffer_atual == NULL)
    {
        buffer_atual = Buffer_read();
    }

    if (!flag_fir_ready || buffer_atual == NULL)
    {
        return;
    }

    flag_fft_ready = false;
    freq_dominante = 0.0f;

    calcular_fft_e_frequencia(buffer_atual, &freq_dominante);

    flag_fft_ready = true;
}

/* [ALTERAÇÃO] static (antes era pública sem protótipo) */
void tarefa_calcula_energia(void)
{
    if (!flag_fir_ready || buffer_atual == NULL)
    {
        return;
    }

    energia = calcula_energia(buffer_atual, BLOCK_SIZE);

    flag_energia_ready = true;
}

void tarefa_detecta_evento(void)
{
    if (!flag_energia_ready)
    {
        return;
    }

    if (detecta_evento(energia))
    {
        flag_evento_pendente = true;
    }

    flag_energia_ready = false;

    /* [ALTERAÇÃO] FFT e energia (únicos leitores do bloco) já rodaram: libera o buffer
     * para o ping-pong (README §6). Antes: buffer_atual = NULL sem avisar o ping-pong.
     * [ALTERAÇÃO] Removido "flag_fft_ready = false": isso impedia a tarefa seguinte
     * (nota/média) de executar. Agora ela é limpa em tarefa_identifica_nota(). */
    Buffer_release();
    buffer_atual = NULL;

    block_count++;
}

void tarefa_identifica_nota(void)
{
    /* [ALTERAÇÃO] Depende da FFT (flag_fft_ready), não mais da média móvel */
    if (!flag_fft_ready)
    {
        return;
    }

    /* [ALTERAÇÃO] Usa a frequência dominante da FFT (README §9).
     * Antes: freq_suavizada = calc_media_movel(...), que era potência média (V²). */
    nota_atual = identificar_nota_musical(freq_dominante);

    flag_fft_ready = false;
    flag_nota_ready = true;
}

void tarefa_impressao(void)
{
    if (!flag_nota_ready)
    {
        return;
    }

    /* [ALTERAÇÃO] Antes: imprimia a cada bloco (64 ms), ilegível e acima do prazo.
     * Agora: imprime se houve evento neste bloco OU a cada PRINT_PERIOD_BLOCKS blocos.
     * block_count já foi incrementado em tarefa_detecta_evento() neste mesmo quadro. */
    bool imprimir = flag_evento_pendente || ((block_count % PRINT_PERIOD_BLOCKS) == 0);

    if (imprimir)
    {
        imprime_evento(flag_evento_pendente, energia, freq_dominante, nota_atual);
    }

    /* [ALTERAÇÃO] Sempre limpa: a impressão roda no mesmo quadro da detecção,
     * então o evento não precisa ficar guardado para o próximo bloco. */
    flag_evento_pendente = false;
    flag_nota_ready = false;
}
