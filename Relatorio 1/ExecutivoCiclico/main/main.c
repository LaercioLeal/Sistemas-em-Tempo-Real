#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_task_wdt.h"

#define LED_VERDE     GPIO_NUM_21
#define LED_VERMELHO  GPIO_NUM_22
#define LED_AMARELO   GPIO_NUM_23

// Ciclo básico do executivo
#define CICLO_BASE_US 150000  // 150 ms

// Quantidade de quadros no ciclo maior
#define TOTAL_QUADROS 6

static const char *TAG = "EXECUTIVO";

// Incrementado pela interrupção do timer
static volatile uint32_t timer_ticks = 0;

// INTERRUPÇÃO DO TIMER
static bool IRAM_ATTR timer_callback(
    gptimer_handle_t timer,
    const gptimer_alarm_event_data_t *edata,
    void *user_ctx)
{
    timer_ticks++;

    return false;
}

// Controla led verde a cada 300 ms.
static void tarefa_T1(void)
{
    static uint8_t estado = 0;

    if (estado == 0)
    {
        gpio_set_level(LED_VERDE, 1);
    }
    else
    {
        gpio_set_level(LED_VERDE, 0);
    }

    estado++;

    if (estado >= 3)
    {
        estado = 0;
    }
}

// Alterna o LED vermelho a cada 450 ms.
static void tarefa_T2(void)
{
    static bool estado = false;

    estado = !estado;

    gpio_set_level(LED_VERMELHO, estado);
}


// Alterna o LED amarelo a cada 900 ms.
static void tarefa_T3(void)
{
    static bool estado = false;

    estado = !estado;

    gpio_set_level(LED_AMARELO, estado);
}

// EXECUTIVO CÍCLICO
static void executar_quadro(uint32_t quadro)
{
    
    // T1 executa nos quadros:
    // 0, 2, 4,
    if ((quadro % 2) == 0)
    {
        tarefa_T1();
    }

    // T2 executa nos quadros:
    // 0, 3
    if ((quadro % 3) == 0)
    {
        tarefa_T2();
    }

    // T3 executa nos quadros:
    // 0
    if ((quadro % 6) == 0)
    {
        tarefa_T3();
    }
}

static void configurar_gpio(void)
{
    gpio_reset_pin(LED_VERDE);
    gpio_reset_pin(LED_VERMELHO);
    gpio_reset_pin(LED_AMARELO);

    gpio_set_direction(LED_VERDE, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_VERMELHO, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_AMARELO, GPIO_MODE_OUTPUT);

    gpio_set_level(LED_VERDE, 0);
    gpio_set_level(LED_VERMELHO, 0);
    gpio_set_level(LED_AMARELO, 0);
}

// CONFIGURAÇÃO DO TIMER
static gptimer_handle_t configurar_timer(void)
{
    gptimer_handle_t timer = NULL;

    // 1 tick = 1 us
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000
    };

    ESP_ERROR_CHECK(
        gptimer_new_timer(
            &timer_config,
            &timer
        )
    );

    // Callback que será chamada pela interrupção
    gptimer_event_callbacks_t callbacks = {
        .on_alarm = timer_callback
    };

    ESP_ERROR_CHECK(
        gptimer_register_event_callbacks(
            timer,
            &callbacks,
            NULL
        )
    );

    // 150000 us = 150 ms
    gptimer_alarm_config_t alarm_config = {
        .reload_count = 0,
        .alarm_count = CICLO_BASE_US,
        .flags.auto_reload_on_alarm = true
    };

    ESP_ERROR_CHECK(
        gptimer_set_alarm_action(
            timer,
            &alarm_config
        )
    );

    ESP_ERROR_CHECK(gptimer_enable(timer));

    ESP_ERROR_CHECK(gptimer_start(timer));

    return timer;
}

void app_main(void)
{
    esp_task_wdt_deinit(); // Desligando Watchdog timer

    configurar_gpio();

    ESP_LOGI(TAG, "Executivo ciclico iniciado");
    ESP_LOGI(TAG, "Ciclo base = 150 ms");
    ESP_LOGI(TAG, "Ciclo maior = 900 ms");

    uint32_t quadro = 0;

    executar_quadro(quadro);

    quadro = 1;

    // Inicia timer periódico de 150 ms
    configurar_timer();

    uint32_t ticks_processados = 0;

    while (1)
    {
        if (timer_ticks != ticks_processados)
        {
            ticks_processados++;

            executar_quadro(quadro);

            quadro++;

            if (quadro >= TOTAL_QUADROS)
            {
                quadro = 0;
            }
        }
    }
}