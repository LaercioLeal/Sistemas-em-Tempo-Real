#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gptimer.h"
#include "esp_err.h"
#include "esp_log.h"
#include "adcReader.h"

/* --------------------------------------------------------------------------
 * Filtro FIR
 * --------------------------------------------------------------------------
 * Fs = 100 Hz  -> Ts = 10 ms
 * h[k] = {-3, 12, 17, 12, -3} / 35
 * Comprimento: 5 taps
 * Ordem: 4
 * -------------------------------------------------------------------------- */

#define N_FILTRO 5
#define COEFF_SUM 35.0f

static const int coeff[N_FILTRO] = {-3, 12, 17, 12, -3};

static const char *TAGFIR = "FILTRO_FIR";

/* --------------------------------------------------------------------------
 * Buffer circular
 *
 * Nesta versao, o buffer e manipulado somente pela tarefa principal.
 * A ISR nao acessa o buffer, eliminando a disputa entre ISR e tarefa.
 * -------------------------------------------------------------------------- */

typedef struct
{
    int buffer[N_FILTRO];
    uint32_t head;
    uint32_t tail;
} circular_buffer;

static circular_buffer cbuf;

static void cb_init(circular_buffer *buf)
{
    buf->head = 0;
    buf->tail = 0;

    for (int i = 0; i < N_FILTRO; i++)
    {
        buf->buffer[i] = 0;
    }
}

static uint32_t cb_get_filled(const circular_buffer *buf)
{
    return buf->head - buf->tail;
}

static uint32_t cb_get_avail(const circular_buffer *buf)
{
    return N_FILTRO - cb_get_filled(buf);
}

static bool cb_push(circular_buffer *buf, int data)
{
    if (cb_get_avail(buf) == 0)
    {
        return false;
    }

    buf->buffer[buf->head % N_FILTRO] = data;
    buf->head++;

    return true;
}

static bool cb_pop(circular_buffer *buf, int *data)
{
    if (cb_get_filled(buf) == 0)
    {
        return false;
    }

    if (data != NULL)
    {
        *data = buf->buffer[buf->tail % N_FILTRO];
    }

    buf->tail++;
    return true;
}

/* Calcula o FIR usando as N_FILTRO amostras a partir de tail.
 * Esta funcao deve ser chamada somente quando o buffer estiver cheio.
 */
static float fir_calculate(const circular_buffer *buf)
{
    int32_t acc = 0;

    for (int i = 0; i < N_FILTRO; i++)
    {
        int sample = buf->buffer[(buf->tail + (uint32_t)i) % N_FILTRO];
        acc += (int32_t)coeff[i] * (int32_t)sample;
    }

    return (float)acc / COEFF_SUM;
}

/* --------------------------------------------------------------------------
 * Aplicacao principal
 * -------------------------------------------------------------------------- */

void app_main(void)
{
    /* 1. Inicializa primeiro todos os recursos usados na aquisicao. */
    cb_init(&cbuf);

    /* A propria app_main sera a tarefa de amostragem/processamento. */
    TaskHandle_t sampling_task = xTaskGetCurrentTaskHandle();

    /* 2. Somente agora o timer e iniciado. */
    init_leitor_adc(sampling_task);

    int adc_raw;

    static int listaMedia[N_FILTRO] = {0};
    int indiceMedia = 0;
    int qtdAmostras = 0;
    long somaMedia = 0;

    int qtdMensagem = 0;

    while (true)
    {
        /*
         * Bloqueia a tarefa ate o timer sinalizar um periodo de amostragem.
         * O valor retornado informa quantas notificacoes ficaram pendentes.
         */
        uint32_t notifications = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        /*
         * Em operacao normal, notifications == 1.
         * Se for > 1, a tarefa nao conseguiu atender todos os instantes de
         * amostragem em tempo real. Fazemos uma unica leitura atual do ADC,
         * pois leituras repetidas agora nao recuperariam amostras passadas.
         */
        if (notifications == 0)
        {
            continue;
        }

        ESP_ERROR_CHECK(adc_ler(&adc_raw));

        /*
         * O fluxo normal sempre deixa uma vaga no buffer depois de calcular
         * cada saida. Portanto, falha aqui indica que o processamento deixou
         * de consumir o buffer como esperado.
         */
        if (!cb_push(&cbuf, adc_raw))
        {
            int discarded;
            (void)cb_pop(&cbuf, &discarded);
            (void)cb_push(&cbuf, adc_raw);
        }

        /*
         * Quando existem 5 amostras, calcula:
         *
         * y[n] = (-3*x[n-4] + 12*x[n-3] + 17*x[n-2]
         *         +12*x[n-1] - 3*x[n]) / 35
         *
         * Depois remove apenas a amostra mais antiga, formando uma janela
         * deslizante de 5 pontos.
         */
        if (cb_get_filled(&cbuf) >= N_FILTRO)
        {
            float output = fir_calculate(&cbuf);

            int potencia = output * output; // Converte para Potencia

            // Atualiza a média móvel
            // da janela e adiciona a contribuição da nova amostra
            somaMedia -= listaMedia[indiceMedia];
            listaMedia[indiceMedia] = potencia;
            somaMedia += potencia;
            indiceMedia = (indiceMedia + 1) % N_FILTRO;
            if (qtdAmostras < N_FILTRO)
            {
                qtdAmostras++;
            }

            int media_movel = (int)(somaMedia / qtdAmostras);

            if (qtdMensagem >= 1000)
            {
                ESP_LOGI(TAGFIR, "Media Movel: %d", media_movel);
                qtdMensagem = 0;
            }
            (void)cb_pop(&cbuf, NULL);
            qtdMensagem++;
        }
    }
}
