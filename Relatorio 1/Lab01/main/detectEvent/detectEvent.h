#ifndef DETECTOR_EVENTO
#define DETECTOR_EVENTO

#include <stdbool.h>
#include <stdint.h>

#include "notaMusical.h"
#include "projetoCfg.h"

extern uint32_t block_count;

bool detecta_evento(float energy);

void imprime_evento(bool flag_evento,
                    float energy,
                    float freq_dominante,
                    NotaInfo nota,
                    uint32_t tempo_proc_us);

#endif