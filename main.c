/*
Para compilar de dentro do VS Code:

Windows/Linux: CTRL+SHIFT+B
macOS: COMMAND+SHIFT+B

Para executar:
Windows/Linux: CTRL+F5

Pelo terminal:
Windows: mingw32-make
Linux/macOS: make

Para executar:
./bagunceitor [arquivo] [aplicar] [reverter]

Modos disponíveis para [aplicar] e [reverter]:
    nenhum
    cor
    espiral
    ambos

Exemplos:
    ./bagunceitor imagem.png cor cor
    ./bagunceitor imagem.png espiral espiral
    ./bagunceitor imagem.png ambos ambos
    ./bagunceitor imagem.png ambos cor
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG
#include <stb_image.h>
#include <stb_image_write.h>

// Tamanho dos blocos da espiral, por ex. 16, 32, 64...
#define TAM_BLOCO 16

// Um pixel Pixel (24 bits)
typedef struct {
    unsigned char r, g, b;
} Pixel;

// Uma imagem Pixel
typedef struct {
    int width, height;  // largura, altura
    int channels;       // qtd de canais (geralmente 3, RGB)
    Pixel* pixels;
} Img;

// As 2 imagens
Img in = {0}, out = {0};

// =========================================
// MODOS DE PROCESSAMENTO
// =========================================

#define PROCESSO_COR      1
#define PROCESSO_ESPIRAL  2

// Protótipos
void load(const char* name, Img* pic);
int parseModo(const char* texto);
void imprimirUso(const char* nomePrograma);

// Cor
void aplicarTransformacoesRGB(Pixel* p);
void reverterTransformacoesRGB(Pixel* p);

// Espiral em blocos
int aplicarEspiral(Img* img);
int reverterEspiral(Img* img);

// Auxiliares da espiral
void gerarOrdemEspiralBlocos(int gridW, int gridH, size_t* ordem);
size_t calcularDeslocamentoEspiral(size_t total);
void copiarBloco(
    Pixel* destino, const Pixel* origem,
    int imgW,
    int blocoOrigX, int blocoOrigY,
    int blocoDestX, int blocoDestY,
    int blocoTam
);

// =========================================
// TRANSFORMACOES RGB
// =========================================

// 1. XOR no canal vermelho
void aplicarXorVermelho(Pixel* p) {
    p->r ^= 0x5A;
}

void reverterXorVermelho(Pixel* p) {
    p->r ^= 0x5A;
}

// 2. Soma modular no canal verde
void somarVerdeModular(Pixel* p) {
    p->g = (unsigned char)(p->g + 37);
}

void reverterSomaVerdeModular(Pixel* p) {
    p->g = (unsigned char)(p->g - 37);
}

// 3. Troca dos canais vermelho e azul
void trocarCanaisVermelhoAzul(Pixel* p) {
    unsigned char temp = p->r;
    p->r = p->b;
    p->b = temp;
}

// =========================================
// SEQUENCIA DAS TRANSFORMACOES RGB
// =========================================

void aplicarTransformacoesRGB(Pixel* p) {
    aplicarXorVermelho(p);
    somarVerdeModular(p);
    trocarCanaisVermelhoAzul(p);
}

void reverterTransformacoesRGB(Pixel* p) {
    trocarCanaisVermelhoAzul(p);
    reverterSomaVerdeModular(p);
    reverterXorVermelho(p);
}

// =========================================
// ESPIRAL REVERSIVEL EM BLOCOS
// =========================================

// Gera a ordem dos blocos em espiral.
// Cada elemento do vetor "ordem" guarda o indice do bloco:
// indice = by * gridW + bx
void gerarOrdemEspiralBlocos(int gridW, int gridH, size_t* ordem) {
    int topo = 0;
    int baixo = gridH - 1;
    int esquerda = 0;
    int direita = gridW - 1;
    size_t k = 0;

    while (esquerda <= direita && topo <= baixo) {
        // Linha superior
        for (int bx = esquerda; bx <= direita; bx++) {
            ordem[k++] = (size_t)topo * gridW + bx;
        }
        topo++;

        // Coluna direita
        for (int by = topo; by <= baixo; by++) {
            ordem[k++] = (size_t)by * gridW + direita;
        }
        direita--;

        // Linha inferior
        if (topo <= baixo) {
            for (int bx = direita; bx >= esquerda; bx--) {
                ordem[k++] = (size_t)baixo * gridW + bx;
            }
            baixo--;
        }

        // Coluna esquerda
        if (esquerda <= direita) {
            for (int by = baixo; by >= topo; by--) {
                ordem[k++] = (size_t)by * gridW + esquerda;
            }
            esquerda++;
        }
    }
}

// Calcula um deslocamento perceptivel ao longo da espiral.
// Quanto maior, mais forte o embaralhamento.
size_t calcularDeslocamentoEspiral(size_t total) {
    if (total <= 1) return 0;

    // Um deslocamento perceptivel:
    // aproximadamente 1/3 da quantidade de blocos.
    size_t deslocamento = total / 3;

    if (deslocamento == 0) deslocamento = 1;
    if (deslocamento >= total) deslocamento = total - 1;

    return deslocamento;
}

// Copia um bloco inteiro da imagem origem para a imagem destino.
void copiarBloco(
    Pixel* destino, const Pixel* origem,
    int imgW,
    int blocoOrigX, int blocoOrigY,
    int blocoDestX, int blocoDestY,
    int blocoTam
) {
    int origX0 = blocoOrigX * blocoTam;
    int origY0 = blocoOrigY * blocoTam;
    int destX0 = blocoDestX * blocoTam;
    int destY0 = blocoDestY * blocoTam;

    for (int dy = 0; dy < blocoTam; dy++) {
        for (int dx = 0; dx < blocoTam; dx++) {
            size_t idxOrig = (size_t)(origY0 + dy) * imgW + (origX0 + dx);
            size_t idxDest = (size_t)(destY0 + dy) * imgW + (destX0 + dx);

            destino[idxDest] = origem[idxOrig];
        }
    }
}

int aplicarEspiral(Img* img) {
    int blocoTam = TAM_BLOCO;

    int gridW = img->width / blocoTam;
    int gridH = img->height / blocoTam;

    // Se a imagem for pequena demais para formar 2 blocos, nao ha o que fazer.
    if (gridW * gridH <= 1) {
        return 1;
    }

    size_t totalPixels = (size_t)img->width * img->height;
    size_t bytes = totalPixels * sizeof(Pixel);

    size_t totalBlocos = (size_t)gridW * gridH;

    Pixel* origem = malloc(bytes);
    size_t* ordem = malloc(totalBlocos * sizeof(size_t));

    if (origem == NULL || ordem == NULL) {
        free(origem);
        free(ordem);
        return 0;
    }

    memcpy(origem, img->pixels, bytes);
    gerarOrdemEspiralBlocos(gridW, gridH, ordem);

    size_t deslocamento = calcularDeslocamentoEspiral(totalBlocos);

    // Move cada bloco ao longo da espiral
    for (size_t i = 0; i < totalBlocos; i++) {
        size_t idxOrigBloco = ordem[i];
        size_t idxDestBloco = ordem[(i + deslocamento) % totalBlocos];

        int origBx = (int)(idxOrigBloco % gridW);
        int origBy = (int)(idxOrigBloco / gridW);

        int destBx = (int)(idxDestBloco % gridW);
        int destBy = (int)(idxDestBloco / gridW);

        copiarBloco(
            img->pixels, origem,
            img->width,
            origBx, origBy,
            destBx, destBy,
            blocoTam
        );
    }

    // As bordas que nao completam blocos ficam inalteradas.
    free(origem);
    free(ordem);
    return 1;
}

int reverterEspiral(Img* img) {
    int blocoTam = TAM_BLOCO;

    int gridW = img->width / blocoTam;
    int gridH = img->height / blocoTam;

    if (gridW * gridH <= 1) {
        return 1;
    }

    size_t totalPixels = (size_t)img->width * img->height;
    size_t bytes = totalPixels * sizeof(Pixel);

    size_t totalBlocos = (size_t)gridW * gridH;

    Pixel* origem = malloc(bytes);
    size_t* ordem = malloc(totalBlocos * sizeof(size_t));

    if (origem == NULL || ordem == NULL) {
        free(origem);
        free(ordem);
        return 0;
    }

    memcpy(origem, img->pixels, bytes);
    gerarOrdemEspiralBlocos(gridW, gridH, ordem);

    size_t deslocamento = calcularDeslocamentoEspiral(totalBlocos);

    // Reverte o deslocamento circular
    for (size_t i = 0; i < totalBlocos; i++) {
        size_t idxDestOriginal = ordem[i];
        size_t idxOrigAtual = ordem[(i + deslocamento) % totalBlocos];

        int destBx = (int)(idxDestOriginal % gridW);
        int destBy = (int)(idxDestOriginal / gridW);

        int origBx = (int)(idxOrigAtual % gridW);
        int origBy = (int)(idxOrigAtual / gridW);

        copiarBloco(
            img->pixels, origem,
            img->width,
            origBx, origBy,
            destBx, destBy,
            blocoTam
        );
    }

    free(origem);
    free(ordem);
    return 1;
}

// =========================================
// MAIN
// =========================================

int main(int argc, char* argv[]) {
    if (argc < 4) {
        imprimirUso(argv[0]);
        return 1;
    }

    if (sizeof(Pixel) != 3) {
        printf("Erro: tamanho inesperado de Pixel.\n");
        return 1;
    }

    int modoAplicar = parseModo(argv[2]);
    int modoReverter = parseModo(argv[3]);

    if (modoAplicar < 0 || modoReverter < 0) {
        printf("Erro: modo invalido.\n\n");
        imprimirUso(argv[0]);
        return 1;
    }

    // Carrega a imagem original
    load(argv[1], &in);

    printf("Origem: %s %d x %d\n", argv[1], in.width, in.height);
    printf("Aplicar : %s\n", argv[2]);
    printf("Reverter: %s\n", argv[3]);
    printf("Tamanho do bloco da espiral: %d x %d\n", TAM_BLOCO, TAM_BLOCO);

    size_t largura = (size_t)in.width;
    size_t altura = (size_t)in.height;

    if (largura > SIZE_MAX / altura ||
        largura * altura > SIZE_MAX / sizeof(Pixel)) {
        printf("Erro: imagem muito grande.\n");
        stbi_image_free(in.pixels);
        return 1;
    }

    size_t tam = largura * altura;
    size_t bytes = tam * sizeof(Pixel);

    // Cria a imagem de saida
    out = in;
    out.pixels = malloc(bytes);

    if (out.pixels == NULL) {
        printf("Erro ao alocar memoria.\n");
        stbi_image_free(in.pixels);
        return 1;
    }

    // Copia os pixels originais para a saida
    memcpy(out.pixels, in.pixels, bytes);

    printf("Processando...\n");

    // =====================================
    // APLICA OS PROCESSOS ESCOLHIDOS
    // Ordem de aplicacao:
    // 1. Cor
    // 2. Espiral
    // =====================================

    if (modoAplicar & PROCESSO_COR) {
        for (size_t i = 0; i < tam; i++) {
            aplicarTransformacoesRGB(&out.pixels[i]);
        }
        printf("Filtro de cores aplicado.\n");
    }

    if (modoAplicar & PROCESSO_ESPIRAL) {
        if (!aplicarEspiral(&out)) {
            printf("Erro ao aplicar espiral.\n");
            free(out.pixels);
            stbi_image_free(in.pixels);
            return 1;
        }
        printf("Espiral aplicada.\n");
    }

    // Salva a imagem baguncada
    if (!stbi_write_png(
        "saida_baguncada.png",
        out.width, out.height,
        3, out.pixels, 0
    )) {
        printf("Erro ao salvar imagem baguncada.\n");
        free(out.pixels);
        stbi_image_free(in.pixels);
        return 1;
    }

    printf("Imagem baguncada salva!\n");

    // =====================================
    // REVERTE OS PROCESSOS ESCOLHIDOS
    // Ordem de reversao:
    // 1. Espiral
    // 2. Cor
    // =====================================

    if (modoReverter & PROCESSO_ESPIRAL) {
        if (!reverterEspiral(&out)) {
            printf("Erro ao reverter espiral.\n");
            free(out.pixels);
            stbi_image_free(in.pixels);
            return 1;
        }
        printf("Espiral revertida.\n");
    }

    if (modoReverter & PROCESSO_COR) {
        for (size_t i = 0; i < tam; i++) {
            reverterTransformacoesRGB(&out.pixels[i]);
        }
        printf("Filtro de cores revertido.\n");
    }

    // =====================================
    // COMPARA OS BYTES
    // =====================================

    int restauracaoExata =
        memcmp(in.pixels, out.pixels, bytes) == 0;

    if (restauracaoExata) {
        printf("SUCESSO: todos os pixels foram restaurados exatamente!\n");
    } else {
        printf("ERRO: existem diferencas nos pixels!\n");
        printf("Isso pode ser esperado se os modos de reversao\n");
        printf("nao corresponderem exatamente aos modos aplicados.\n");
    }

    // Salva a imagem restaurada / resultado final
    if (!stbi_write_png(
        "saida_restaurada.png",
        out.width, out.height,
        3, out.pixels, 0
    )) {
        printf("Erro ao salvar imagem restaurada.\n");
        free(out.pixels);
        stbi_image_free(in.pixels);
        return 1;
    }

    printf("Imagem restaurada salva!\n");

    // Libera a memoria
    stbi_image_free(in.pixels);
    free(out.pixels);

    return restauracaoExata ? 0 : 1;
}

// =========================================
// CARREGAMENTO DA IMAGEM
// =========================================

void load(const char* name, Img* pic) {
    int canaisOriginais;

    pic->pixels = (Pixel*)stbi_load(
        name,
        &pic->width,
        &pic->height,
        &canaisOriginais,
        3
    );

    if (!pic->pixels) {
        printf("Erro de leitura: %s\n", stbi_failure_reason());
        exit(1);
    }

    pic->channels = 3;

    printf("Load: %d x %d x %d\n",
           pic->width,
           pic->height,
           pic->channels);

    // Exibe ate 8x8 pixels em hexadecimal
    for (int i = 0; i < 8 && i < pic->height; i++) {
        for (int j = 0; j < 8 && j < pic->width; j++) {
            Pixel p = pic->pixels[(size_t)i * pic->width + j];
            printf("[%02X %02X %02X] ", p.r, p.g, p.b);
        }
        printf("\n");
    }
}

// =========================================
// FUNCOES AUXILIARES
// =========================================

int parseModo(const char* texto) {
    if (strcmp(texto, "nenhum") == 0) {
        return 0;
    }
    if (strcmp(texto, "cor") == 0) {
        return PROCESSO_COR;
    }
    if (strcmp(texto, "espiral") == 0) {
        return PROCESSO_ESPIRAL;
    }
    if (strcmp(texto, "ambos") == 0) {
        return PROCESSO_COR | PROCESSO_ESPIRAL;
    }

    return -1;
}

void imprimirUso(const char* nomePrograma) {
    printf("Uso:\n");
    printf("  %s [arquivo] [aplicar] [reverter]\n\n", nomePrograma);

    printf("Modos disponiveis:\n");
    printf("  nenhum\n");
    printf("  cor\n");
    printf("  espiral\n");
    printf("  ambos\n\n");

    printf("Exemplos:\n");
    printf("  %s imagem.png cor cor\n", nomePrograma);
    printf("  %s imagem.png espiral espiral\n", nomePrograma);
    printf("  %s imagem.png ambos ambos\n", nomePrograma);
    printf("  %s imagem.png ambos cor\n", nomePrograma);
}