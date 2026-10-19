
#include "calcEnergia.h"

float calcula_energia(int *data, int num_samples)
{
    float soma_quadrados = 0.0f;

    for (int i = 0; i < num_samples; i++)
    {
        float sample_norm = (float)data[i] / 32768.0f;
        soma_quadrados += sample_norm * sample_norm;
    }
    
    /* [ALTERAÇÃO] Retorna E = (1/N)·Σy² (README §7).
     * Antes: sqrtf(energia_media), que é RMS e não a energia média pedida. */
    return soma_quadrados / num_samples;
}