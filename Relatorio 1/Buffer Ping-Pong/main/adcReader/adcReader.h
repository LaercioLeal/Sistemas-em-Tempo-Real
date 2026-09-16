#ifndef ADC_READER
#define ADC_READER

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "esp_adc/adc_oneshot.h"
#include "bffPingPong.h"

#define ADC_PORT ADC_CHANNEL_6 // GPIO34
static const char *TAG = "ADC_LOG";

static bool IRAM_ATTR on_timer(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx);

static void adc_task(void *args);

static void init_leitor_adc();
#endif