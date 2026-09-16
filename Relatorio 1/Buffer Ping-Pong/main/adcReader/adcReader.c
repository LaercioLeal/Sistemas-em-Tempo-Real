#include "adcReader.h"
// LEITOR ADC
static adc_oneshot_unit_handle_t adc_unit;
static TaskHandle_t task_handle = NULL;
static gptimer_handle_t gptimer = NULL;

static bool IRAM_ATTR on_timer(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx)
{

  BaseType_t high_task_awoken = pdFALSE;
  vTaskNotifyGiveFromISR(task_handle, &high_task_awoken);
  return high_task_awoken == pdTRUE;
}

static void adc_task(void *args)
{
  int sinal_adc;

  while (true)
  {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // aguarda notificação da ISR
    adc_oneshot_read(adc_unit, ADC_CHANNEL_6, &sinal_adc);

    bool add = Buffer_push(sinal_adc);
    if (!add)
    {
      ESP_LOGE(TAG_ADC, "Não foi possível adicionar o valor lido.");
    }
  }
}

static void init_leitor_adc()
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
      .resolution_hz = 1 * 1000 * 1000, // 1 MHz
  };
  ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));

  gptimer_event_callbacks_t cbs = {
      .on_alarm = on_timer,
  };
  ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer, &cbs, NULL));
  ESP_ERROR_CHECK(gptimer_enable(gptimer));

  gptimer_alarm_config_t alarm_config = {
      .reload_count = 0,
      .alarm_count = 10 * 1000, // 10000 ticks = 10ms
      .flags.auto_reload_on_alarm = true,
  };

  xTaskCreate(adc_task, "adc_task", 2048, NULL, 5, &task_handle);

  ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer, &alarm_config));
  ESP_ERROR_CHECK(gptimer_start(gptimer));
}
