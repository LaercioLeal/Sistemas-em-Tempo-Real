#include "fft.h"
#include <math.h>

#include "projetoCfg.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void bit_reversal(float *real, float *imag, int n)
{
    int j = 0;
    for (int i = 0; i < n - 1; i++)
    {
        if (i < j)
        {
            float temp_r = real[i];
            float temp_i = imag[i];
            real[i] = real[j];
            imag[i] = imag[j];
            real[j] = temp_r;
            imag[j] = temp_i;
        }
        int k = n / 2;
        while (k <= j)
        {
            j -= k;
            k /= 2;
        }
        j += k;
    }
}
// Algoritmo FFT in-place (Substitui o dsps_fft2r_fc32)
void calcular_fft_radix2(float *real, float *imag, int n)
{
    // 1. Reorganiza os dados de entrada
    bit_reversal(real, imag, n);

    // 2. Executa os estágios da borboleta (butterflies)
    for (int len = 2; len <= n; len <<= 1)
    {
        float angle = -2.0f * M_PI / len;
        float wlen_r = cosf(angle);
        float wlen_i = sinf(angle);

        for (int i = 0; i < n; i += len)
        {
            int half_len = len / 2;
            float w_r = 1.0f;
            float w_i = 0.0f;

            for (int j = 0; j < half_len; j++)
            {
                int u_idx = i + j;
                int v_idx = i + j + half_len;

                // Multiplicação complexa: v * w
                float t_r = real[v_idx] * w_r - imag[v_idx] * w_i;
                float t_i = real[v_idx] * w_i + imag[v_idx] * w_r;

                // Combinação da borboleta
                real[v_idx] = real[u_idx] - t_r;
                imag[v_idx] = imag[u_idx] - t_i;
                real[u_idx] += t_r;
                imag[u_idx] += t_i;

                // Atualiza o fator de rotação (w = w * wlen)
                float next_w_r = w_r * wlen_r - w_i * wlen_i;
                w_i = w_r * wlen_i + w_i * wlen_r;
                w_r = next_w_r;
            }
        }
    }
}

void calcular_fft_e_frequencia(int *data, float *freq_dominante)
{
    static float fft_real[BLOCK_SIZE];
    static float fft_imag[BLOCK_SIZE];

    for (int i = 0; i < BLOCK_SIZE; i++)
    {
        float window = 0.5f * (1.0f - cosf(2.0f * M_PI * i / (BLOCK_SIZE - 1)));
        fft_real[i] = (float)data[i] * window / 32768.0f;
        fft_imag[i] = 0.0f;
    }

    calcular_fft_radix2(fft_real, fft_imag, BLOCK_SIZE);

    float max_magnitude = 0.0f;
    int max_index = 0;

    /* [ALTERAÇÃO] Faixa 80–1200 Hz (README §8). Antes: 60–2000 Hz.
     * bin_min = ceil(80/15,625)  = 6  (93,75 Hz)  -> também ignora DC
     * bin_max = floor(1200/15,625) = 76 (1187,5 Hz), inclusivo */
    const int bin_min = (int)ceilf((float)FFT_FMIN_HZ * BLOCK_SIZE / AUDIO_FS_HZ);
    const int bin_max = (int)floorf((float)FFT_FMAX_HZ * BLOCK_SIZE / AUDIO_FS_HZ);

    for (int i = bin_min; i <= bin_max; i++)   /* [ALTERAÇÃO] <= para incluir bin_max */
    {
        float magnitude = fft_real[i] * fft_real[i] + fft_imag[i] * fft_imag[i];
        if (magnitude > max_magnitude)
        {
            max_magnitude = magnitude;
            max_index = i;
        }
    }

    /* [ALTERAÇÃO] f[k] = k·Fs/N do maior pico (README §8; exemplo 437,5 Hz = bin 28).
     * Removida a interpolação parabólica: não consta no README e, feita sobre
     * potência (|X|²), enviesa a estimativa (da ordem de 1,5 Hz, ~12 cents em
     * 220 Hz, conforme a posição do tom entre bins). Se o grupo quiser mantê-la,
     * interpole com sqrtf(|X|²) (ou em log), nunca com a potência. */
    *freq_dominante = (float)max_index * AUDIO_FS_HZ / BLOCK_SIZE;
}