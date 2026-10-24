#include "execCiclico.h"
#include "esp_timer.h" /* [ALTERAÇÃO] medição de tempos (README §5, §10, §11, §12) */
#include <stdio.h>
#include "i2sMic.h"
#include "filtroFIR.h"
#include "bffPingPong.h"
#include "fft.h"
#include "notaMusical.h"
#include "calcEnergia.h"
#include "detectEvent.h"

/* [ALTERAÇÃO] Contagem de ticks. Antes: só um flag booleano, que perdia os ticks ocorridos
 * durante tarefas de vários quadros (FFT/Print) e desalinhava 'quadro' do tempo real.
 * ticks_total: só a ISR escreve. ticks_consumidos: só o laço principal escreve. */
static volatile uint32_t ticks_total = 0;
static volatile uint32_t ticks_consumidos = 0;

volatile bool flag_exec_ciclico = false;
volatile bool flag_i2s_ready = false;
volatile bool flag_fir_ready = false;
volatile bool flag_fft_ready = false;
volatile bool flag_nota_ready = false; /* [ALTERAÇÃO] substitui flag_media_ready */
volatile bool flag_energia_ready = false;
volatile bool flag_evento_pendente = false;

static int bff_ic2_mic[BLOCK_SIZE];
static int *buffer_atual = NULL;
static float freq_dominante = 0.0f;
static float energia = 0.0f;
/* [ALTERAÇÃO] Removidos freq_suavizada e energia_evento (a saída usa os valores do bloco). */
static NotaInfo nota_atual;

static uint32_t tempo_proc_us = 0;      /* [ALTERAÇÃO] "Tempo de processamento" (README §11) */
static uint32_t quadros_estourados = 0; /* [ALTERAÇÃO] contagem de prazos perdidos */
static uint32_t amostras_perdidas = 0;  /* [ALTERAÇÃO] overruns do ping-pong */

/* ---------- [ALTERAÇÃO] Medição do tempo de execução de cada tarefa ---------- */
typedef enum
{
    T_I2S,
    T_FIR,
    T_FFT,
    T_ENERGIA,
    T_EVENTO,
    T_NOTA,
    T_PRINT,
    T_NUM
} tarefa_id_t;
static const char *nome_tarefa[T_NUM] = {"I2S", "FIR+push", "FFT", "Energia", "Evento", "Nota", "Print"};
static uint32_t c_max_us[T_NUM];
static uint32_t c_quadro_max_us[TOTAL_QUADROS];
#define MEDE_TAREFA(id, chamada)                               \
    do                                                         \
    {                                                          \
        int64_t _t0 = esp_timer_get_time();                    \
        chamada;                                               \
        uint32_t _dt = (uint32_t)(esp_timer_get_time() - _t0); \
        if (_dt > c_max_us[id])                                \
            c_max_us[id] = _dt;                                \
    } while (0)

void exec_ciclico_imprime_tempos(void)
{
    printf("\n-----------------Tempos de Execução-------------------\n");
    for (int i = 0; i < T_NUM; i++)
    {
        printf("C_max %-9s = %lu us\n", nome_tarefa[i], (unsigned long)c_max_us[i]);
    }
    printf("--------------------------------------------\n");
}

void set_exec_ciclico_flag(bool val)
{
    if (val)
    {
        flag_exec_ciclico = true;
        return;
    }

    /* [ALTERAÇÃO] val == false: o main acabou de consumir UM tick.
     * Se ainda houver ticks pendentes (tarefa que ocupou vários quadros), mantém o flag ligado
     * para o main executar os quadros seguintes imediatamente e recuperar o alinhamento. */
    ticks_consumidos++;
    flag_exec_ciclico = false;
    if (ticks_total != ticks_consumidos)
    {
        flag_exec_ciclico = true;
    }
}

static bool IRAM_ATTR timer_callback(
    gptimer_handle_t timer,
    const gptimer_alarm_event_data_t *edata,
    void *user_ctx)
{
    ticks_total++; /* [ALTERAÇÃO] conta o tick (antes só ligava o flag) */
    flag_exec_ciclico = true;
    return false;
}

gptimer_handle_t config_exec_ciclico_timer(void)
{
    gptimer_handle_t timer = NULL;

    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000}; /* 1 tick = 1 us */

    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &timer));

    gptimer_event_callbacks_t callbacks = {.on_alarm = timer_callback};
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(timer, &callbacks, NULL));

    gptimer_alarm_config_t alarm_config = {
        .reload_count = 0,
        .alarm_count = CICLO_BASE_US,
        .flags.auto_reload_on_alarm = true};

    ESP_ERROR_CHECK(gptimer_set_alarm_action(timer, &alarm_config));
    ESP_ERROR_CHECK(gptimer_enable(timer));
    ESP_ERROR_CHECK(gptimer_start(timer));

    return timer;
}

static void tarefa_i2s_mic(void)
{
    flag_i2s_ready = false;

    /* [ALTERAÇÃO] Só libera o FIR se o bloco chegou completo (antes o retorno era ignorado) */
    if (mic_read_filter_push(bff_ic2_mic) == I2S_BUFFER_SIZE)
    {
        flag_i2s_ready = true;
    }
}

static void tarefa_fir(void)
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

static void tarefa_fft(void)
{
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
static void tarefa_calcula_energia(void)
{
    if (!flag_fir_ready || buffer_atual == NULL)
    {
        return;
    }

    energia = calcula_energia(buffer_atual, BLOCK_SIZE);

    flag_energia_ready = true;
}

static void tarefa_detecta_evento(void)
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

static void tarefa_identifica_nota(void)
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

static void tarefa_impressao(void)
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

void exec_ciclico_executar_quadro(uint32_t quadro)
{
    static bool primeira_chamada = true;
    int64_t t_ini = esp_timer_get_time();

    const int q_atual = quadro % TOTAL_QUADROS;

    if (buffer_atual == NULL)
    {
        buffer_atual = Buffer_read();
    }

    if (q_atual == Q_AQUISICAO)
    {
        MEDE_TAREFA(T_I2S, tarefa_i2s_mic());
        MEDE_TAREFA(T_FIR, tarefa_fir());
    }

    if (q_atual == Q_FFT)
    {
        int64_t t0 = esp_timer_get_time();
        MEDE_TAREFA(T_FFT, tarefa_fft());
        tempo_proc_us = (uint32_t)(esp_timer_get_time() - t0);
    }

    if (q_atual == Q_CALC_EVENT)
    {
        int64_t t0 = esp_timer_get_time();
        MEDE_TAREFA(T_ENERGIA, tarefa_calcula_energia());
        MEDE_TAREFA(T_EVENTO, tarefa_detecta_evento());
        MEDE_TAREFA(T_NOTA, tarefa_identifica_nota());
        tempo_proc_us += (uint32_t)(esp_timer_get_time() - t0);
    }

    if (q_atual == Q_IMPRESSAO)
    {
        MEDE_TAREFA(T_PRINT, tarefa_impressao());
    }

    uint32_t dt = (uint32_t)(esp_timer_get_time() - t_ini);

    if (primeira_chamada)
    {
        for (int i = 0; i < T_NUM; i++)
        {
            c_max_us[i] = 0;
        }
    }
    else if (dt > c_quadro_max_us[quadro % TOTAL_QUADROS])
    {
        c_quadro_max_us[quadro % TOTAL_QUADROS] = dt;
    }

    if (!primeira_chamada && dt > CICLO_BASE_US)
    {
        quadros_estourados++;
        printf("Quadros estourados (> orcamento) = %lu | amostras perdidas = %lu",
               (unsigned long)quadros_estourados, (unsigned long)amostras_perdidas);
    }
    primeira_chamada = false;
}