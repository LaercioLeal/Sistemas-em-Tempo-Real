#include "i2sMic.h"

static i2s_chan_handle_t rx_chan;

void i2s_mic_init(void)
{
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num = I2S_DMA_DESC_NUM;
    chan_cfg.dma_frame_num = I2S_DMA_FRAMES;
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, NULL, &rx_chan));

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(I2S_SAMPLE_RATE),
        /* Mono, slot de 32 bits, formato Philips (INMP441) */
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT,
                                                        I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = I2S_BCLK_GPIO,
            .ws = I2S_WS_GPIO,
            .dout = I2S_GPIO_UNUSED,
            .din = I2S_DIN_GPIO,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };
    std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT; /* L/R do microfone em GND */

    ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_chan, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(rx_chan));
}

/* Fluxo (README, sec. 4), para cada amostra lida:
 *     amostra (24 bits -> 16 bits, decimada por media)
 *         -> fir_process_sample()      (estado do FIR continuo entre chamadas)
 *         -> Buffer_push()             (ping-pong so recebe amostra filtrada)
 *
 * Retorna o numero de amostras enviadas ao ping-pong. Em caso de erro de leitura
 * retorna o que ja foi enviado (< I2S_BUFFER_SIZE); o ping-pong continua consistente, pois cada
 * buffer e sempre preenchido com BUFFERLENGTH amostras filtradas. */
size_t mic_read_filter_push(int *buffer)
{
    static int32_t raw[READ_CHUNK];
    size_t pushed = 0;
    int32_t acc = 0;
    int cnt = 0;

    while (pushed < I2S_BUFFER_SIZE)
    {
        size_t bytes_read = 0;
        esp_err_t err = i2s_channel_read(rx_chan, raw, sizeof(raw),
                                         &bytes_read, I2S_READ_TIMEOUT_MS);
        if (err != ESP_OK)
        {
            return pushed;
        }

        size_t n = bytes_read / sizeof(int32_t);
        for (size_t i = 0; i < n && pushed < I2S_BUFFER_SIZE; i++)
        {
            /* Dado de 24 bits alinhado a esquerda no slot de 32 bits.
             * >> 16 -> valor de 16 bits com sinal. */
            acc += (raw[i] >> 16);

            if (++cnt == DECIMATION)
            {
                int16_t x = (int16_t)(acc / DECIMATION);
                acc = 0;
                cnt = 0;

                *(buffer + i) = x;
            
                pushed++;
            }
        }
    }
    return pushed;
}