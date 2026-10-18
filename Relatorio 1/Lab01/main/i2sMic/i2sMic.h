#ifndef I2S_MIC
#define I2S_MIC

#include "esp_log.h"
#include <stddef.h>
#include "projetoCfg.h"
#include "driver/i2s_std.h"
#include "filtroFIR.h"

#define I2S_BUFFER_SIZE BLOCK_SIZE

/* ---------------- Pinos (ajuste conforme sua ligacao) ---------------- */
#define I2S_BCLK_GPIO   GPIO_NUM_32   /* SCK / BCLK */
#define I2S_WS_GPIO     GPIO_NUM_25   /* WS / LRCL  */
#define I2S_DIN_GPIO    GPIO_NUM_33   /* SD / DOUT  */
/* Pino L/R do microfone em GND -> canal esquerdo */

/* ---------------- Amostragem ---------------- */
#define I2S_SAMPLE_RATE  AUDIO_FS_HZ                   /* Hz, taxa do I2S */
#define DECIMATION       (I2S_SAMPLE_RATE / FIR_FS_HZ) /* 1 (sem decimacao) */
#define I2S_DMA_FRAMES   256                           /* amostras de 32 bits por descritor */
#define I2S_DMA_DESC_NUM 6
#define READ_CHUNK       I2S_DMA_FRAMES                /* amostras de 32 bits por leitura */
#define I2S_READ_TIMEOUT_MS 100                        /* 256 amostras = 16 ms; folga p/ erro */
 

void i2s_mic_init(void);
 
/* Le 'len' amostras do microfone; cada amostra (ja decimada) passa pelo FIR e a saida
 * filtrada e enviada ao buffer ping-pong (Buffer_push). Nao ha buffer intermediario de
 * amostras brutas alem do pedaco de DMA lido (READ_CHUNK).
 * Retorna o numero de amostras filtradas enviadas ao ping-pong (len se tudo OK). */
size_t mic_read_filter_push(int * buffer);

#endif