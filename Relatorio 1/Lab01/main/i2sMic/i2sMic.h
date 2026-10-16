#ifndef I2S_MIC
#define I2S_MIC

#include "esp_log.h"
#include <stddef.h>
#include "projeto_cfg.h"
#include "driver/i2s_std.h"
#include "filtroFIR.h"

#define I2S_BUFFER_SIZE 1024

/* ---------------- Pinos (ajuste conforme sua ligacao) ---------------- */
#define I2S_BCLK_GPIO   GPIO_NUM_32   /* SCK / BCLK */
#define I2S_WS_GPIO     GPIO_NUM_25   /* WS / LRCL  */
#define I2S_DIN_GPIO    GPIO_NUM_33   /* SD / DOUT  */
/* Pino L/R do microfone em GND -> canal esquerdo */

/* ---------------- Amostragem ---------------- */
#define I2S_SAMPLE_RATE  AUDIO_FS_HZ                   /* Hz, taxa do I2S */
#define DECIMATION       (I2S_SAMPLE_RATE / FIR_FS)    /* 1 */
#define I2S_DMA_FRAMES   256                           /* amostras de 32 bits por leitura */
#define I2S_DMA_DESC_NUM 6

void i2s_mic_init(void);
size_t mic_fill_buffer(int16_t *dst, size_t len);

void fill_i2s_mic();

#endif