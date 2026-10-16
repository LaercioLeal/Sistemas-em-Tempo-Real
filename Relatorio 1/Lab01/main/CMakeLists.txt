idf_component_register(SRCS "main.c" "execCiclico/execCiclico.c" "bffPingPong/bffPingPong.c" "filtroFIR/filtroFIR.c" "i2sMic/i2sMic.c" "projetoCfg/projetoCfg.h"
                    INCLUDE_DIRS "." "execCiclico" "bffPingPong" "filtroFIR" "i2sMic"
                    REQUIRES esp_driver_gpio esp_driver_gptimer esp_driver_i2s)
