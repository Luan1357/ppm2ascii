/*
 * ppm2ascii.c — Conversor de imagem PPM binário (P6) para ASCII Art
 *
 * Compilar: gcc -Wall -Wextra -o ppm2ascii ppm2ascii.c
 * Uso:      ./ppm2ascii   (digitar o nome do arquivo quando solicitado)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* Rampa de caracteres: do mais escuro (espaço) ao mais claro ($) — 57 chars */
#define RAMP       " .':;|!(l1it[]{}?/\\<>+r=xcvunUJCLQ0OZmwqpdbkhao*#MW&8%B@$"
#define RAMP_SIZE  ((int)(sizeof(RAMP) - 1))

/* ------------------------------------------------------------------ */
/* Estruturas                                                           */
/* ------------------------------------------------------------------ */

typedef struct {
    int width;
    int height;
    int maxval;
} CabecalhoPPM;

typedef struct {
    unsigned char r, g, b;
} Pixel;

/* ------------------------------------------------------------------ */
/* Funções auxiliares internas                                          */
/* ------------------------------------------------------------------ */

/* Avança o ponteiro de arquivo saltando espaços em branco e comentários
   (linhas iniciadas com '#') do cabeçalho PPM. */
static void pular_espacos_comentarios(FILE *fp)
{
    int c;
    while ((c = fgetc(fp)) != EOF) {
        if (c == '#') {
            /* Ignora o resto da linha de comentário */
            while ((c = fgetc(fp)) != EOF && c != '\n')
                ;
        } else if (!isspace((unsigned char)c)) {
            ungetc(c, fp);
            break;
        }
    }
}

/* ------------------------------------------------------------------ */
/* ler_cabecalho                                                        */
/* ------------------------------------------------------------------ */

/*
 * Lê e valida o cabeçalho de um arquivo PPM P6.
 * Retorna 1 em sucesso, 0 em falha (mensagem de erro já impressa).
 *
 * Formato esperado:
 *   P6\n
 *   [# comentário\n]...
 *   <largura> <altura>\n
 *   <maxval>\n
 *   <dados binários>
 */
int ler_cabecalho(FILE *fp, CabecalhoPPM *cabecalho)
{
    char magic[3];
    int c;

    magic[0] = magic[1] = magic[2] = '\0';

    /* --- Magic number -------------------------------------------- */
    if (fscanf(fp, "%2s", magic) != 1) {
        fprintf(stderr, "Erro: nao foi possivel ler o magic number.\n");
        return 0;
    }
    if (strcmp(magic, "P6") != 0) {
        fprintf(stderr,
                "Erro: formato nao suportado. Esperado P6 (PPM binario), "
                "encontrado '%s'.\n", magic);
        return 0;
    }

    /* --- Largura -------------------------------------------------- */
    pular_espacos_comentarios(fp);
    if (fscanf(fp, "%d", &cabecalho->width) != 1 || cabecalho->width <= 0) {
        fprintf(stderr, "Erro: largura invalida no cabecalho.\n");
        return 0;
    }

    /* --- Altura --------------------------------------------------- */
    pular_espacos_comentarios(fp);
    if (fscanf(fp, "%d", &cabecalho->height) != 1 || cabecalho->height <= 0) {
        fprintf(stderr, "Erro: altura invalida no cabecalho.\n");
        return 0;
    }

    /* --- Maxval --------------------------------------------------- */
    pular_espacos_comentarios(fp);
    if (fscanf(fp, "%d", &cabecalho->maxval) != 1 ||
        cabecalho->maxval < 1 || cabecalho->maxval > 65535) {
        fprintf(stderr, "Erro: maxval invalido no cabecalho.\n");
        return 0;
    }

    /*
     * A especificação NetPBM exige exatamente UM caractere de espaço em branco
     * separando o cabeçalho dos dados binários.
     */
    c = fgetc(fp);
    if (c == EOF || !isspace((unsigned char)c)) {
        fprintf(stderr,
                "Erro: separador invalido apos maxval "
                "(esperado 1 whitespace antes dos dados binarios).\n");
        return 0;
    }

    return 1;
}

/* ------------------------------------------------------------------ */
/* ler_pixels                                                           */
/* ------------------------------------------------------------------ */

/*
 * Aloca e preenche um vetor de Pixels lendo os dados binários do arquivo.
 * Suporta maxval <= 255 (8 bits/canal) e maxval > 255 (16 bits/canal, big-endian).
 * Retorna o ponteiro alocado (caller deve liberar), ou NULL em falha.
 */
Pixel *ler_pixels(FILE *fp, const CabecalhoPPM *cabecalho)
{
    int total;
    size_t lidos;
    unsigned char *buf;
    Pixel *pixels;
    int i;

    /* Verifica overflow na multiplicação antes de alocar */
    if (cabecalho->width > 32767 || cabecalho->height > 32767) {
        fprintf(stderr, "Erro: dimensoes muito grandes (max 32767x32767).\n");
        return NULL;
    }
    total = cabecalho->width * cabecalho->height;

    pixels = malloc((size_t)total * sizeof(Pixel));
    if (!pixels) {
        fprintf(stderr, "Erro: falha ao alocar memoria para %d pixels.\n", total);
        return NULL;
    }

    if (cabecalho->maxval <= 255) {
        /* ---- 8 bits por canal: 3 bytes por pixel ---- */
        buf = malloc((size_t)total * 3);
        if (!buf) {
            fprintf(stderr, "Erro: falha ao alocar buffer de leitura.\n");
            free(pixels);
            return NULL;
        }

        lidos = fread(buf, 3, (size_t)total, fp);
        if ((int)lidos != total) {
            fprintf(stderr,
                    "Erro: dados insuficientes (%lu de %d pixels lidos).\n",
                    (unsigned long)lidos, total);
            free(buf);
            free(pixels);
            return NULL;
        }

        for (i = 0; i < total; i++) {
            pixels[i].r = buf[i * 3];
            pixels[i].g = buf[i * 3 + 1];
            pixels[i].b = buf[i * 3 + 2];
        }
        free(buf);

    } else {
        /* ---- 16 bits por canal: 6 bytes por pixel, big-endian ---- */
        int r16, g16, b16;

        buf = malloc((size_t)total * 6);
        if (!buf) {
            fprintf(stderr, "Erro: falha ao alocar buffer de leitura.\n");
            free(pixels);
            return NULL;
        }

        lidos = fread(buf, 6, (size_t)total, fp);
        if ((int)lidos != total) {
            fprintf(stderr,
                    "Erro: dados insuficientes (%lu de %d pixels lidos).\n",
                    (unsigned long)lidos, total);
            free(buf);
            free(pixels);
            return NULL;
        }

        for (i = 0; i < total; i++) {
            r16 = ((int)buf[i * 6]     << 8) | buf[i * 6 + 1];
            g16 = ((int)buf[i * 6 + 2] << 8) | buf[i * 6 + 3];
            b16 = ((int)buf[i * 6 + 4] << 8) | buf[i * 6 + 5];
            /* Normaliza para 8 bits */
            pixels[i].r = (unsigned char)(r16 * 255 / cabecalho->maxval);
            pixels[i].g = (unsigned char)(g16 * 255 / cabecalho->maxval);
            pixels[i].b = (unsigned char)(b16 * 255 / cabecalho->maxval);
        }
        free(buf);
    }

    return pixels;
}

/* ------------------------------------------------------------------ */
/* converter_para_ascii                                                 */
/* ------------------------------------------------------------------ */

/*
 * Converte o vetor de pixels em uma string de ASCII art.
 * Cada pixel vira um caractere; cada linha termina com '\n'.
 *
 * Fórmula de luminância: L = 0.299*R + 0.587*G + 0.114*B
 * Índice na rampa:       idx = L * (RAMP_SIZE - 1) / 255
 *
 * Retorna a string alocada (caller deve liberar), ou NULL em falha.
 */
char *converter_para_ascii(const Pixel *pixels, const CabecalhoPPM *cabecalho)
{
    int w = cabecalho->width;
    int h = cabecalho->height;
    /* h linhas de (w chars + '\n'), mais o '\0' final */
    size_t art_size = (size_t)h * ((size_t)w + 1) + 1;
    char *art;
    int x, y, idx;
    double lum;
    const Pixel *p;

    art = malloc(art_size);
    if (!art) {
        fprintf(stderr, "Erro: falha ao alocar memoria para ASCII art.\n");
        return NULL;
    }

    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            p = &pixels[y * w + x];
            lum = 0.299 * p->r + 0.587 * p->g + 0.114 * p->b;
            idx = (int)(lum * (RAMP_SIZE - 1) / 255.0);
            if (idx < 0)           idx = 0;
            if (idx >= RAMP_SIZE)  idx = RAMP_SIZE - 1;
            art[y * (w + 1) + x] = RAMP[idx];
        }
        art[y * (w + 1) + w] = '\n';
    }
    art[(size_t)h * ((size_t)w + 1)] = '\0';

    return art;
}

/* ------------------------------------------------------------------ */
/* salvar_arquivo                                                       */
/* ------------------------------------------------------------------ */

/*
 * Salva a ASCII art em um arquivo .txt com o mesmo nome base da imagem.
 * Exemplo: "foto.ppm" → "foto.txt", "19.ppm" → "19.txt".
 * Retorna 1 em sucesso, 0 em falha.
 */
int salvar_arquivo(const char *nome_imagem, const char *art)
{
    char nome_saida[512];
    const char *ponto;
    size_t base_len, len;
    FILE *fp;

    ponto = strrchr(nome_imagem, '.');

    if (ponto != NULL) {
        base_len = (size_t)(ponto - nome_imagem);
        if (base_len >= sizeof(nome_saida) - 4) {
            fprintf(stderr, "Erro: nome de arquivo muito longo.\n");
            return 0;
        }
        memcpy(nome_saida, nome_imagem, base_len);
        nome_saida[base_len] = '\0';
    } else {
        len = strlen(nome_imagem);
        if (len >= sizeof(nome_saida) - 4) {
            fprintf(stderr, "Erro: nome de arquivo muito longo.\n");
            return 0;
        }
        memcpy(nome_saida, nome_imagem, len + 1);
    }
    strcat(nome_saida, ".txt");

    fp = fopen(nome_saida, "w");
    if (!fp) {
        fprintf(stderr, "Erro: nao foi possivel criar '%s'.\n", nome_saida);
        return 0;
    }

    if (fputs(art, fp) == EOF) {
        fprintf(stderr, "Erro: falha ao escrever em '%s'.\n", nome_saida);
        fclose(fp);
        return 0;
    }

    fclose(fp);
    printf("ASCII art salvo em: %s\n", nome_saida);
    return 1;
}

/* ------------------------------------------------------------------ */
/* main                                                                 */
/* ------------------------------------------------------------------ */

int main(void)
{
    char nome_arquivo[512];
    FILE *fp;
    CabecalhoPPM cabecalho;
    Pixel *pixels;
    char *art;
    int ok;

    printf("=== Conversor PPM Binario (P6) para ASCII Art ===\n");
    printf("Nome do arquivo de imagem: ");
    fflush(stdout);

    if (scanf("%511s", nome_arquivo) != 1) {
        fprintf(stderr, "Erro: falha ao ler o nome do arquivo.\n");
        return EXIT_FAILURE;
    }

    /* --- Abre o arquivo em modo binário --- */
    fp = fopen(nome_arquivo, "rb");
    if (!fp) {
        fprintf(stderr, "Erro: nao foi possivel abrir '%s'.\n", nome_arquivo);
        return EXIT_FAILURE;
    }

    /* --- Lê e valida o cabeçalho --- */
    if (!ler_cabecalho(fp, &cabecalho)) {
        fclose(fp);
        return EXIT_FAILURE;
    }
    printf("Imagem: %d x %d pixels, maxval=%d\n",
           cabecalho.width, cabecalho.height, cabecalho.maxval);

    /* --- Lê os pixels binários --- */
    pixels = ler_pixels(fp, &cabecalho);
    fclose(fp);
    if (!pixels)
        return EXIT_FAILURE;

    /* --- Converte para ASCII art --- */
    art = converter_para_ascii(pixels, &cabecalho);
    free(pixels);
    if (!art)
        return EXIT_FAILURE;

    /* --- Exibe no terminal --- */
    printf("\n--- ASCII Art ---\n");
    fputs(art, stdout);
    printf("--- Fim ---\n\n");

    /* --- Salva em arquivo .txt --- */
    ok = salvar_arquivo(nome_arquivo, art);
    free(art);

    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
