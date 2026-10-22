// png_min.c
// Compilar: gcc png_min.c -o png_min
#include <stdio.h>
#include <string.h>

// lê 4 bytes big-endian
static unsigned be32(const unsigned char *p) {
    return (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
}

int main(int argc, char **argv) {
    if (argc < 2) { printf("Uso: %s arquivo.png\n", argv[0]); return 1; }

    FILE *f = fopen(argv[1], "rb");
    if (!f) { perror("fopen\n"); return 1; }

    // tamanho do arquivo
    fseek(f, 0, SEEK_END);
    printf("Tamanho: %ld bytes\n", ftell(f));
    rewind(f);

    // verifica a assinatura se eh um arquivo PNG (8 bytes)
    unsigned char sig[8];
    fread(sig, 1, 8, f); // le fread(destino, tamanho, quantidade, arquivo)
    if (memcmp(sig, "\x89PNG\r\n\x1a\n", 8) != 0) { //memory compare != 0 (conteudo dos bytes diferentes)
        printf("[ERRO] Arquivo '%s' nao e um arquivo .png\n", argv[1]);
        return 1;
    }

    // percorre os chunks: [tamanho 4][tipo 4][dados][crc 4]
    unsigned char cab[8]; // cabecalho do chunk: tamanho + tipo
    unsigned char d[16];  // buffer para dados do chunk (max 16 bytes) IHDR 13 bytes, tIME 7 bytes
    while (fread(cab, 1, 8, f) == 8) { // le o cabecalho do chunk
        unsigned len = be32(cab);
        char tipo[5] = {0};
        memcpy(tipo, cab + 4, 4);

        if (!strcmp(tipo, "IHDR")) { // "rotulacao" do chunk IHDR
            fread(d, 1, 13, f);      // leitura dos 13 bytes do chunk IHDR
            printf("Largura: %u px\nAltura: %u px\n", be32(d), be32(d + 4));
            fseek(f, len - 13 + 4, SEEK_CUR);          // pula dados + CRC
        } else if (!strcmp(tipo, "tIME")) { // "rotulacao" do chunk tIME
            fread(d, 1, 7, f);       // leitura dos 7 bytes do chunk tIME
            printf("Data/hora: %04u-%02u-%02u %02u:%02u:%02u (UTC)\n", // padrao ISO 8601
                   (d[0] << 8) | d[1], d[2], d[3], d[4], d[5], d[6]);
            fseek(f, len - 7 + 4, SEEK_CUR); // mesma logica de bloco de pular dados
        } else if (!strcmp(tipo, "IEND")) {
            break;
        } else {
            fseek(f, len + 4, SEEK_CUR);    // pula dados + CRC
        }
    }

    fclose(f);
    return 0;
}