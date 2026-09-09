#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_task_wdt.h"
#include "esp_log.h"

static const char *TAG = "BUFFER_CIRCULAR";

// DEFINIÇÃO DE BUFFER CIRCULAR
#define BUFFERLENGTH 1000
#define LIMIAR_ADC 3500
#define ADC_PORT ADC_CHANNEL_6 // GPIO34

typedef struct
{
  int head;
  int tail;
  int size;
  int buffer[BUFFERLENGTH];
} BufferCircular;

static BufferCircular circular;

void Buffer_init(BufferCircular *bc)
{
  bc->head = 0;
  bc->tail = 0;
  bc->size = 0;
}

int Buffer_getSize(BufferCircular *bc)
{
  return bc->size;
}

bool Buffer_push(BufferCircular *bc, int value)
{
  if (bc->size != BUFFERLENGTH)
  {

    if (value >= LIMIAR_ADC)
    {
      ESP_LOGW(TAG, "Valor lido acima do limiar: %d", value);
      return false;
    }
    bc->buffer[bc->head] = value;
    bc->size++;
    bc->head++;
    if (bc->head == BUFFERLENGTH)
    {
      bc->head = 0;
    }
    return true;
  }
  return false;
}

int Buffer_pop(BufferCircular *bc)
{
  if (bc->size > 0)
  {
    int popVal = bc->buffer[bc->tail];
    bc->size--;
    bc->tail++;
    if (bc->tail == BUFFERLENGTH)
    {
      bc->tail = 0;
    }
    return popVal;
  }

  return -1; // Retorna -1 caso nao possua valor
}

// FIM CONFIGURAÇÃO BUFFER CIRCULAR

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

    bool add = Buffer_push(&circular, sinal_adc * sinal_adc); // Multiplicação para armazenar Potencia do Sinal no Buffer.
    if (!add)
    {
      ESP_LOGE(TAG, "Não foi possível adicionar o valor lido.");
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

#define MEDIA_MOVEL_TAMANHO 64

void app_main()
{
  esp_task_wdt_deinit(); // Desligando Watchdog timer

  Buffer_init(&circular);
  init_leitor_adc();

  static int listaMedia[MEDIA_MOVEL_TAMANHO] = {0};
  int indiceMedia = 0;
  int qtdAmostras = 0;
  long somaMedia = 0;

  while (true)
  {
    int pop = Buffer_pop(&circular);
    if (pop >= 0)
    {
      int potencia = pop*pop; // Converte Tensao para Potencia: P = (V^2)/R com R=1 e V=valor do ADC

      // Atualiza a média móvel
      // da janela e adiciona a contribuição da nova amostra
      somaMedia -= listaMedia[indiceMedia];
      listaMedia[indiceMedia] = potencia;
      somaMedia += potencia;

      indiceMedia = (indiceMedia + 1) % MEDIA_MOVEL_TAMANHO;
      if(qtdAmostras < MEDIA_MOVEL_TAMANHO){
        qtdAmostras++;
      }

      int media_movel = (int)(somaMedia / qtdAmostras);
      ESP_LOGI(TAG, "Média móvel (Potencia): %d", media_movel);
    }
    else
    {
      ESP_LOGW(TAG, "Buffer Vazio.");
    }
  
    vTaskDelay(1);
  }
}