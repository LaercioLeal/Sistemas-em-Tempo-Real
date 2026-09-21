#include "adcReader.h"
// LEITOR ADC
static adc_oneshot_unit_handle_t adc_unit = NULL;

/* Contador apenas para depuracao/diagnostico. */
static volatile uint32_t isr_count = 0;

/* Handle do timer, mantido globalmente para permitir futura parada/desalocacao. */
static gptimer_handle_t sample_timer = NULL;

/* --------------------------------------------------------------------------
 * Timer
 *
 * A ISR NAO le o ADC e NAO manipula o buffer.
 * Ela apenas notifica a tarefa que uma nova amostra deve ser adquirida.
 * -------------------------------------------------------------------------- */
bool IRAM_ATTR timer_on_alarm_cb(gptimer_handle_t timer,
                                 const gptimer_alarm_event_data_t *edata,
                                 void *user_ctx)
{
    (void)timer;
    (void)edata;

    BaseType_t higher_priority_task_woken = pdFALSE;
    TaskHandle_t sampling_task = (TaskHandle_t)user_ctx;

    isr_count++;

    vTaskNotifyGiveFromISR(sampling_task, &higher_priority_task_woken);

    /* O GPTimer usa o retorno para saber se deve ocorrer yield ao sair da ISR. */
    return (higher_priority_task_woken == pdTRUE);
}

void init_leitor_adc(TaskHandle_t sampling_task)
{
    // Configurando leitor ADC

    // Definindo qual unidade de ADC sera utilizada
    adc_oneshot_unit_init_cfg_t adc_unit_cfg = {
        .unit_id = ADC_UNIT_1, // ADC 1 é melhor pois o ADC 2 apresenta falha se utilizado com wifi
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&adc_unit_cfg, &adc_unit));

    adc_oneshot_chan_cfg_t adc_canal = {
        .atten = ADC_ATTEN_DB_12,    // Definindo faixa de tensão
        .bitwidth = ADC_BITWIDTH_12, // Definindo quantidade de bits.
    };

    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_unit, ADC_PORT, &adc_canal));

    // Configurando Timer
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = TIMER_RESOLUTION_HZ,
    };

    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &sample_timer));

    gptimer_event_callbacks_t callbacks = {
        .on_alarm = timer_on_alarm_cb,
    };

    ESP_ERROR_CHECK(
        gptimer_register_event_callbacks(sample_timer, &callbacks, sampling_task));

    gptimer_alarm_config_t alarm_config = {
        .reload_count = 0,
        .alarm_count = SAMPLE_PERIOD_US,
        .flags.auto_reload_on_alarm = true,
    };

    ESP_ERROR_CHECK(
        gptimer_set_alarm_action(sample_timer, &alarm_config));

    ESP_ERROR_CHECK(gptimer_enable(sample_timer));

    ESP_ERROR_CHECK(gptimer_start(sample_timer));
}

esp_err_t adc_ler(int *adc_raw){
    return adc_oneshot_read(adc_unit, ADC_CHANNEL_6, adc_raw);
}
