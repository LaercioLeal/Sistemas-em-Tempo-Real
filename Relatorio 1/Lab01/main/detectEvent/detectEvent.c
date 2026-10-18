#include "detectEvent.h"
#include "esp_log.h"

#include <stdio.h>
#include <stdint.h>

uint32_t block_count = 0;

static const char *TAG = "DETECTOR_EVENTOS";

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
                    float freq_suave,
                    NotaInfo nota)
{
    if (!flag_evento)
    {
        return;
    }

    float t_s = block_count * ((float)BLOCK_SIZE / AUDIO_FS_HZ); // tempo em segundos

    ESP_LOGI(TAG, "*** EVENTO DETECTADO ***\n");
    ESP_LOGI(TAG, "Bloco #%lu | t = %.3f s\n", block_count, t_s);
    ESP_LOGI(TAG, "Energia = %.3f\n", energy);
    ESP_LOGI(TAG, "Freq. dominante = %.1f Hz\n", freq_dominante);
    ESP_LOGI(TAG, "Freq. dominante Suavizada = %.1f Hz\n", freq_suave);
    ESP_LOGI(TAG, "Nota = %s\n", nota.nome);
    ESP_LOGI(TAG, "Erro = %+.1f cents\n", nota.cents);
    ESP_LOGI(TAG, "Status = %s\n", nota.status);
    ESP_LOGI(TAG, "------------------------\n");
}