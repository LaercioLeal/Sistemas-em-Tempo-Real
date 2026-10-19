idf_component_register(SRCS "main.c" "projetoCfg/projetoCfg.h" "execCiclico/execCiclico.c"
                    "bffPingPong/bffPingPong.c" "filtroFIR/filtroFIR.c"
                    "i2sMic/i2sMic.c" "fft/fft.c" "notaMusical/NotaMusical.c"
                    "calcEnergia/calcEnergia.c" "detectEvent/detectEvent.c"
                    INCLUDE_DIRS "." "projetoCfg" "execCiclico" "bffPingPong"
                    "filtroFIR" "i2sMic" "fft" "notaMusical" "calcEnergia" "detectEvent"
                    REQUIRES esp_driver_gpio esp_driver_gptimer esp_driver_i2s esp_timer)
