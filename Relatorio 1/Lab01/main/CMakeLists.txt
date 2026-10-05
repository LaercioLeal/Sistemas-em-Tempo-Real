idf_component_register(SRCS "main.c" "execCiclico/execCiclico.c" "bffPingPong/bffPingPong.c"
                    INCLUDE_DIRS "." "execCiclico" "bffPingPong"
                    REQUIRES esp_driver_gpio esp_driver_gptimer)
