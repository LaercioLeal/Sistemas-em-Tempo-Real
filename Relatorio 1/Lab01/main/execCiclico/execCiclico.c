#include "execCiclico.h"
#include "i2sMic.h"
#include "filtroFIR.h"
#include "bffPingPong.h"
#include "fft.h"
#include "mediaMovel.h"
#include "notaMusical.h"
#include "calcEnergia.h"
#include "detectEvent.h"

// Flag de interrupção do timer
volatile bool flag_exec_ciclico = false;
volatile bool flag_i2s_ready = false;
volatile bool flag_fir_ready = false;
volatile bool flag_fft_ready = false;
volatile bool flag_media_ready = false;
volatile bool flag_energia_ready = false;
volatile bool flag_evento_pendente = false;

static int bff_ic2_mic[BLOCK_SIZE] ;

static int *buffer_atual = NULL;

static float freq_dominante = 0.0f;
static float freq_suavizada = 0.0f;

static float energia = 0.0f;
static float energia_evento = 0.0f;

static NotaInfo nota_atual;

void set_exec_ciclico_flag(bool val)
{
    flag_exec_ciclico = val;
}

// INTERRUPÇÃO DO TIMER
static bool IRAM_ATTR timer_callback(
    gptimer_handle_t timer,
    const gptimer_alarm_event_data_t *edata,
    void *user_ctx)
{
    flag_exec_ciclico = true;

    return false;
}

// CONFIGURAÇÃO DO TIMER
gptimer_handle_t config_exec_ciclico_timer(void)
{
    gptimer_handle_t timer = NULL;

    // 1 tick = 1 us
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000};

    ESP_ERROR_CHECK(
        gptimer_new_timer(
            &timer_config,
            &timer));

    // Callback que será chamada pela interrupção
    gptimer_event_callbacks_t callbacks = {
        .on_alarm = timer_callback};

    ESP_ERROR_CHECK(
        gptimer_register_event_callbacks(
            timer,
            &callbacks,
            NULL));

    // 150000 us = 150 ms
    gptimer_alarm_config_t alarm_config = {
        .reload_count = 0,
        .alarm_count = CICLO_BASE_US,
        .flags.auto_reload_on_alarm = true};

    ESP_ERROR_CHECK(
        gptimer_set_alarm_action(
            timer,
            &alarm_config));

    ESP_ERROR_CHECK(gptimer_enable(timer));

    ESP_ERROR_CHECK(gptimer_start(timer));

    return timer;
}

static void tarefa_i2s_mic()
{
    flag_i2s_ready = false;

    mic_read_filter_push(bff_ic2_mic);

    flag_i2s_ready = true;
}

static void tarefa_fir()
{
    if (!flag_i2s_ready)
    {
        return;
    }

    flag_fir_ready = false;

    for (int i = 0; i < BLOCK_SIZE; i++)
    {
        int16_t y = fir_process_sample(bff_ic2_mic[i]);

        Buffer_push(y);
    }
    flag_fir_ready = true;
}

static void tarefa_fft()
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

static void tarefa_media_movel()
{
    if (!flag_fft_ready || buffer_atual == NULL)
    {
        return;
    }
    flag_media_ready = false;

    freq_suavizada = calc_media_movel(buffer_atual);

    flag_media_ready = true;
}

static void tarefa_identifica_nota()
{
    if (!flag_media_ready)
    {
        return;
    }
    nota_atual = identificar_nota_musical(freq_suavizada);
}

void tarefa_calcula_energia(void)
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
        energia_evento = energia;
    }

    buffer_atual = NULL;

    flag_fft_ready = false;
    flag_energia_ready = false;

    block_count++;
}

static void tarefa_impressao(void)
{
    imprime_evento(
        flag_evento_pendente,
        energia_evento,
        freq_dominante,
        freq_suavizada,
        nota_atual);

    flag_evento_pendente = false;
}

void exec_ciclico_executar_quadro(uint32_t quadro)
{

    if(buffer_atual == NULL){
        buffer_atual = Buffer_read();
    }

    // T1 executa nos quadros:
    if (quadro == 0)
    {
        tarefa_i2s_mic();
    }

    // T2 executa nos quadros:
    if (quadro == 1)
    {
        tarefa_fir();
    }

    // T3 executa nos quadros:
    if (quadro == 3)
    {
        tarefa_fft();
        tarefa_calcula_energia();
    }

    if (quadro == 5)
    {
        tarefa_detecta_evento();
        tarefa_media_movel();
    }

    if (quadro == 7)
    {
        tarefa_identifica_nota();
    }

    if (quadro == 8)
    {
        tarefa_impressao();
    }
}