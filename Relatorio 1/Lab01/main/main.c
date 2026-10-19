#include <stdio.h>
#include "esp_task_wdt.h"
#include "i2sMic.h"
#include "execCiclico.h"
#include "bffPingPong.h"

extern volatile bool flag_exec_ciclico;

void app_main(void)
{
    esp_task_wdt_deinit();
    Buffer_init();
    fir_init();
    i2s_mic_init();

    uint32_t quadro = 0;

    exec_ciclico_executar_quadro(quadro);   /* quadro 0: 1º bloco (bloqueia ~64 ms) */

    quadro = 1;                             /* o 1º tick do timer processa esse bloco */

    config_exec_ciclico_timer();

    uint32_t ciclos = 0;                    /* [ALTERAÇÃO] opcional, só para o relatório */

    while (1)
    {
        if (flag_exec_ciclico)
        {
            set_exec_ciclico_flag(false);

            exec_ciclico_executar_quadro(quadro);

            /* [ALTERAÇÃO] opcional: imprime os C_max uma vez, após 100 ciclos de 64 ms.
             * Fora do quadro medido; serve apenas para preencher a tabela do relatório. */
            if (quadro == Q_PROCESSAMENTO && ++ciclos == 100)
            {
                exec_ciclico_imprime_tempos();
            }

            quadro++;

            if (quadro >= TOTAL_QUADROS)
            {
                quadro = 0;
            }
        }
    }
}