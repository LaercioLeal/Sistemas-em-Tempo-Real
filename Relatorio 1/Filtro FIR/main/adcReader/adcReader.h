#ifndef ADC_READER
#define ADC_READER

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "esp_adc/adc_oneshot.h"

#define ADC_PORT ADC_CHANNEL_6       // GPIO34
#define TIMER_RESOLUTION_HZ 1000000U // 1 MHz -> 1 tick = 1 us
#define SAMPLE_PERIOD_US 10000U      // 10 ms -> Fs = 100 Hz

bool timer_on_alarm_cb(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx);

void init_leitor_adc(TaskHandle_t sampling_task);

esp_err_t adc_ler(int *adc_raw);

#endif