#include "i2sMic.h"

static i2s_chan_handle_t rx_chan;

void i2s_mic_init(void)
{
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num = 6;
    chan_cfg.dma_frame_num = 256;
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

/* Preenche mic_buffer[] (linear), decimando o fluxo I2S por media.
 * Retorna o numero de amostras escritas (I2S_BUFFER_SIZE) ou 0 em caso de erro. */
size_t mic_fill_buffer(int16_t *dst, size_t len)
{
    static int32_t raw[READ_CHUNK];
    size_t out_idx = 0;
    int32_t acc = 0;
    int cnt = 0;

    while (out_idx < len)
    {
        size_t bytes_read = 0;
        esp_err_t err = i2s_channel_read(rx_chan, raw, sizeof(raw),
                                         &bytes_read, I2S_READ_TIMEOUT_MS);
        if (err != ESP_OK)
        {
            return 0;
        }

        size_t n = bytes_read / sizeof(int32_t);
        for (size_t i = 0; i < n && out_idx < len; i++)
        {
            /* Dado de 24 bits alinhado a esquerda no slot de 32 bits.
             * >> 16 -> valor de 16 bits com sinal. */
            acc += (raw[i] >> 16);

            if (++cnt == DECIMATION)
            {
                dst[out_idx++] = (int16_t)(acc / DECIMATION);
                acc = 0;
                cnt = 0;
            }
        }
    }
    return out_idx;
}

void fill_i2s_mic()
{
    if (mic_fill_buffer(mic_buffer, I2S_BUFFER_SIZE) != I2S_BUFFER_SIZE)
    {
        return;
    }
}

// Main de como utilizar o leitor do mic
// void app_main(void)
// {
//     i2s_mic_init();

//     ESP_LOGI(TAG, "I2S iniciado: %d Hz, decimacao %d -> %d Hz",
//              I2S_SAMPLE_RATE, DECIMATION, FIR_FS);

//     while (1)
//     {
//         /* Enche o buffer padrao de 256 posicoes (~2,56 s a 100 Hz) */
//         if (mic_fill_buffer(mic_buffer, I2S_BUFFER_SIZE) != I2S_BUFFER_SIZE)
//         {
//             vTaskDelay(pdMS_TO_TICKS(100));
//             continue;
//         }

//         size_t n = fir_process_buffer(mic_buffer, fir_out, I2S_BUFFER_SIZE);
//         ESP_LOGI(TAG, "FIR: %u amostras filtradas", (unsigned)n);

//         for (size_t i = 0; i < n; i++)
//         {
//             /* bruto alinhado (atraso de 2 amostras), filtrado */
//             printf("%d,%.2f\n", mic_buffer[i + 2], fir_out[i]);
//         }
//     }
// }