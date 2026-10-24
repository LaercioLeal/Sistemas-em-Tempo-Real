#include "detectEvent.h"
#include <stdio.h>
#include <stdint.h>
#include "esp_log.h"

uint32_t block_count = 0;

bool detecta_evento(float energy)
{
    static int64_t ultimo_bloco_evento = -1000;

    if (energy >= LIMIAR_ENERGIA)
    {
        if ((block_count - ultimo_bloco_evento) > REFRACTORY_BLOCKS)
        {
            ultimo_bloco_evento = block_count;
            return true;
        }
    }
    return false;
}

void imprime_evento(bool flag_evento,
                    float energy,
                    float freq_dominante,
                    NotaInfo nota)
{
    float t_s = block_count * ((float)BLOCK_SIZE / AUDIO_FS_HZ);
    
    printf("t = %.3f s\n"
            "Energia = %.2e\n"
            "Evento Detectado = %s\n"
            "Frequência dominante = %.1f Hz\n"
            "Nota = %s\n"
            "Erro = %+.1f cents\n"
            "Status = %s\n"
            "---------------------------------------------\n",
            t_s, energy, flag_evento ? "sim" : "não",
            freq_dominante, nota.nome, nota.cents, nota.status);

}