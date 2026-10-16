#include "filtroFIR.h"
#include "i2sMic.h"
#include "esp_attr.h"

static const DRAM_ATTR int16_t coeff[N_FILTRO] = {0.0039718464, -0.0119763651, -0.0411194981, 0.1146870637, 
    0.4344369532, 0.4344369532, 0.1146870637, -0.0411194981, -0.0119763651, 0.0039718464};

/* Linha de atraso circular "espelhada" (2*N): evita modulo no laco de MAC.
 * hist[pos + k] = x[n-k] para k = 0..N-1. */
static DRAM_ATTR int16_t hist[2 * N_FILTRO];
static DRAM_ATTR int pos;


void fir_init(void)
{
    for (int i = 0; i < 2 * N_FILTRO; i++)
    {
        hist[i] = 0;
    }
    pos = 0;
}
 
int16_t IRAM_ATTR fir_process_sample(int16_t x)
{
    pos = (pos == 0) ? (N_FILTRO - 1) : (pos - 1);
    hist[pos] = x;
    hist[pos + N_FILTRO] = x;
 
    int32_t acc = 0;
    const int16_t *h = &hist[pos];
    for (int k = 0; k < N_FILTRO; k++)
    {
        acc += (int32_t)coeff[k] * (int32_t)h[k];
    }
 
    acc += (1 << (FIR_Q - 1)); /* arredondamento */
    acc >>= FIR_Q;
 
    if (acc > INT16_MAX) acc = INT16_MAX;
    if (acc < INT16_MIN) acc = INT16_MIN;
    return (int16_t)acc;
}