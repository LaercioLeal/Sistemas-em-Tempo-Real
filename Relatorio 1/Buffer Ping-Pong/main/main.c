#include <stdio.h>
#include "esp_task_wdt.h"
#include "esp_log.h"
#include "bffPingPong.h"

static const char *TAG = "BUFFER_PINGPONG";

int * buffer = nullptr;

float calc_media_buffer(){
    buffer = Buffer_read();

    float media = 0;
    if(buffer != nullptr){
        for(int i = 0; i < BUFFERLENGTH; i++){
            media += buffer[i]*buffer[i];
        }
        media = media/BUFFERLENGTH;
    }
    else{
        ESP_LOGW(TAG, "Buffer indisponivel para leitura.");
    }

    return media;
}

void app_main()
{
  esp_task_wdt_deinit(); // Desligando Watchdog timer

  Buffer_init();
  init_leitor_adc();

  float media = 0;
  while (true)
  {
    media = calc_media_buffer();
    ESP_LOGI(TAG, "Média (Potencia): %.2f", media);
  
    vTaskDelay(1);
  }
}