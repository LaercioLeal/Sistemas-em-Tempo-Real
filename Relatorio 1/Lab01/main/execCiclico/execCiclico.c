#include "execCiclico.h"

// Flag de interrupção do timer
volatile bool flag_exec_ciclico = false;

void set_exec_ciclico_flag(bool val){
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

void exec_ciclico_executar_quadro(uint32_t quadro)
{
    
    // T1 executa nos quadros:
    // 0, 2, 4,
    if ((quadro % 2) == 0)
    {
        printf("T1");
    }

    // T2 executa nos quadros:
    // 0, 3
    if ((quadro % 3) == 0)
    {        
        printf("T2");
    }

    // T3 executa nos quadros:
    // 0
    if ((quadro % 6) == 0)
    {
        printf("T3");
    }
}