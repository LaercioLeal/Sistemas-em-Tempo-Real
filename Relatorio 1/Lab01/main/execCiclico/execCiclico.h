#ifndef EXEC_CICLICO

#define EXEC_CICLICO

#include <stdint.h>
#include <stdbool.h>

#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_task_wdt.h"

// Ciclo básico do executivo
#define CICLO_BASE_US 150000  // 150 ms

// Quantidade de quadros no ciclo maior
#define TOTAL_QUADROS 6

void set_exec_ciclico_flag(bool val);
void exec_ciclico_executar_quadro(uint32_t quadro);
gptimer_handle_t config_exec_ciclico_timer(void);

#endif