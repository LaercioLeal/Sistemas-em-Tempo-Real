#ifndef NOTA_MUSICAL
#define NOTA_MUSICAL

typedef struct
{
    char nome[16]; // Espaço extra para evitar o erro de -Werror=format-overflow=
    float cents;
    const char *status;
} NotaInfo;

NotaInfo identificar_nota_musical(float freq_suavizada);

#endif