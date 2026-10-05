#include <stdio.h>
#include "esp_task_wdt.h"
#include "execCiclico.h"
#include "bffPingPong.h"

// Flag do executivo Ciclico
extern volatile bool flag_exec_ciclico;

void app_main(void)
{
    esp_task_wdt_deinit(); // Desligando Watchdog timer
    Buffer_init();

    uint32_t quadro = 0;

    exec_ciclico_executar_quadro(quadro);

    quadro = 1;

    // Inicia timer do executivo ciclico
    config_exec_ciclico_timer();
    
    while (1)
    {
        if (flag_exec_ciclico)
        {
            set_exec_ciclico_flag(false);

            exec_ciclico_executar_quadro(quadro);

            quadro++;

            if (quadro >= TOTAL_QUADROS)
            {
                quadro = 0;
            }
        }
    }
}
