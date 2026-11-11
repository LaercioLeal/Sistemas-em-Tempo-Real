#include <stdio.h>
#include "esp_task_wdt.h"
#include "i2sMic.h"
#include "execCiclico.h"
#include "monotonico.h"
#include "bffPingPong.h"

#define TIPO_ESCALONAMENTO 1 // 0 = Executivo cíclico; 1 = Monotonico

extern volatile bool flag_exec_ciclico;

void app_main(void)
{
    esp_task_wdt_deinit();
    Buffer_init();
    fir_init();
    i2s_mic_init();

    // Utilizando Executivo Cíclico
    if(TIPO_ESCALONAMENTO == 0){
        uint32_t quadro = 0;

        exec_ciclico_executar_quadro(quadro); 

        quadro = 1;

        config_exec_ciclico_timer();

        uint32_t ciclos = 0;

        while (1)
        {
            if (flag_exec_ciclico)
            {
                set_exec_ciclico_flag(false);

                exec_ciclico_executar_quadro(quadro);

                // [ALTERAÇÃO] opcional: imprime os C_max uma vez, após 100 ciclos de 64 ms.
                // Fora do quadro medido; serve apenas para preencher a tabela do relatório.
                if (quadro == 1 && ++ciclos % 250 == 0)
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
    // Utilizando Taxa Monotonica
    else if(TIPO_ESCALONAMENTO == 1){
        monotonico_init();
        monotonico_despachar();
    }
}