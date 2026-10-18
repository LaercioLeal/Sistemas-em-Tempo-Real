#include "mediaMovel.h"

static int32_t listaMedia[MEDIA_MOVEL_TAMANHO] = {0};
static int indiceMedia = 0;
static int qtdAmostras = 0;
static int64_t somaMedia = 0; // 64 bits evita overflow em janelas grandes

// Último bloco processado. Como o ping pong alterna entre dois buffers
// fixos, o ponteiro retornado por Buffer_read() muda a cada novo bloco.
static const int *ultimoBlocoLido = NULL;

void media_movel_init(void)
{
    for (int i = 0; i < MEDIA_MOVEL_TAMANHO; i++)
    {
        listaMedia[i] = 0;
    }
    indiceMedia = 0;
    qtdAmostras = 0;
    somaMedia = 0;
    ultimoBlocoLido = NULL;
}

// Insere uma amostra na janela deslizante
static void media_movel_add(int amostra)
{
    int32_t potencia = (int32_t)amostra * amostra; // P = (V^2)/R com R=1

    somaMedia -= listaMedia[indiceMedia];
    listaMedia[indiceMedia] = potencia;
    somaMedia += potencia;

    indiceMedia = (indiceMedia + 1) % MEDIA_MOVEL_TAMANHO;
    if (qtdAmostras < MEDIA_MOVEL_TAMANHO)
    {
        qtdAmostras++;
    }
}

float calc_media_movel(int * bloco)
{
    // Só processa se há bloco pronto e ele ainda não foi consumido
    if (bloco != NULL && bloco != ultimoBlocoLido)
    {
        for (int i = 0; i < BUFFERLENGTH; i++)
        {
            media_movel_add(bloco[i]);
        }
        ultimoBlocoLido = bloco;
    }

    return (qtdAmostras > 0) ? ((float)somaMedia / (float)qtdAmostras) : 0.0f;
}