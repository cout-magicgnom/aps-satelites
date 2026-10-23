/*
  ordenador.c
 
    Ferramenta em C que implementa BubbleSort, MergeSort e QuickSort para ler e 
    ordenar arquivos .txt de uma pasta (cada um contendo uma linha "ID <numero>"),
    pela opcao de gerar n IDs aleatorios, e salva o resultado em um arquivo de log 
    tambem le metadados de arquivos .png (largura, altura e data/hora de modificacao) 
    e ordena com base em argumentos fornecidos pela CLI.
 
  Compilar:  gcc -O2 -Wall -Wextra -o ordenador ordenador.c
 
  Exemplos:
    ./ordenador -d ./dados                          (le a pasta, roda os 3 algoritmos)
    ./ordenador -d ./dados -a quick -o saida.txt    (le a pasta, roda apenas o sort escolhido e salva o log em 'saida.txt')
    ./ordenador -r 20000 -a todos                   (20000 IDs aleatorios e roda todos os algoritmos)
    ./ordenador -f imagem.png -a quick -o saida.txt (le metadados do arquivo, ordena e salva o log em 'saida.txt')
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#ifdef _WIN32
    #include <io.h>
#else
    #include <dirent.h>
#endif

#define MAX_NOME 260

/* ======================================================
    TIPOS
   ====================================================== */

// Um "registro" = um arquivo .txt e o ID lido de dentro dele
typedef struct {
    int  id;
    long long chave;        // valor usado na comparacao dos sorts (id, largura, altura ou data)
    unsigned  largura;      // so preenchido para .png
    unsigned  altura;       // so preenchido para .png
    long long data;         // AAAAMMDDhhmmss (0 = png sem chunk tIME)
    char nome[MAX_NOME];
} Registro;

typedef void (*FuncOrdenacao)(Registro v[], int n);

// criterio de ordenacao escolhido na CLI com -k
typedef enum { CRIT_ID, CRIT_LARGURA, CRIT_ALTURA, CRIT_DATA } Criterio;

/* ======================================================
    UTILITARIOS
   ====================================================== */

static void troca(Registro *a, Registro *b) {
    Registro t = *a;
    *a = *b;
    *b = t;
}

static int esta_ordenado(const Registro v[], int n) {
    for (int i = 1; i < n; i++)
        if (v[i - 1].chave > v[i].chave) return 0;
    return 1;
}

static int termina_com_txt(const char *nome) {
    size_t len = strlen(nome);
    if (len < 4) return 0;
    const char *ext = nome + len - 4;
    return tolower((unsigned char)ext[0]) == '.' &&
           tolower((unsigned char)ext[1]) == 't' &&
           tolower((unsigned char)ext[2]) == 'x' &&
           tolower((unsigned char)ext[3]) == 't';
}

// copia o criterio escolhido para o campo 'chave' de cada registro 
static void aplica_criterio(Registro v[], int n, Criterio c) {
    for (int i = 0; i < n; i++) {
        switch (c) {
            case CRIT_LARGURA: v[i].chave = v[i].largura; break;
            case CRIT_ALTURA:  v[i].chave = v[i].altura;  break;
            case CRIT_DATA:    v[i].chave = v[i].data;    break;
            default:           v[i].chave = v[i].id;      break;
        }
    }
}

// transforma AAAAMMDDhhmmss em "AAAA-MM-DD hh:mm:ss" */
static void formata_data(long long d, char *buf, size_t tam) {
    if (d == 0) {
        snprintf(buf, tam, "(sem tIME)");
        return;
    }
    snprintf(buf, tam, "%04lld-%02lld-%02lld %02lld:%02lld:%02lld",
             d / 10000000000LL, d / 100000000LL % 100, d / 1000000LL % 100,
             d / 10000LL % 100, d / 100LL % 100, d % 100);
}

/* ======================================================
    BUBBLESORT
   ====================================================== */
static void bubbleSort(Registro v[], int n) {
    for (int i = 0; i < n - 1; i++) {
        int trocou = 0;
        for (int j = 0; j < n - i - 1; j++) {
            if (v[j].chave > v[j + 1].chave) {
                troca(&v[j], &v[j + 1]);
                trocou = 1;
            }
        }
        if (!trocou) break; /* ja esta ordenado */
    }
}

/* ======================================================
    MERGESORT
    (buffer auxiliar alocado uma unica vez, nao a cada merge)
   ====================================================== */
static void merge(Registro v[], Registro aux[], int l, int m, int r) {
    int i = l, j = m + 1, k = l;

    while (i <= m && j <= r) {
        if (v[i].chave <= v[j].chave) aux[k++] = v[i++];
        else                          aux[k++] = v[j++];
    }
    while (i <= m) aux[k++] = v[i++];
    while (j <= r) aux[k++] = v[j++];

    memcpy(&v[l], &aux[l], (size_t)(r - l + 1) * sizeof(Registro));
}

static void mergeSortRec(Registro v[], Registro aux[], int l, int r) {
    if (l >= r) return;
    int m = l + (r - l) / 2;
    mergeSortRec(v, aux, l, m);
    mergeSortRec(v, aux, m + 1, r);
    merge(v, aux, l, m, r);
}

static void mergeSort(Registro v[], int n) {
    if (n < 2) return;
    Registro *aux = malloc((size_t)n * sizeof(Registro));
    if (!aux) {
        fprintf(stderr, "[ ERR ] Sem memoria para o MergeSort.\n");
        exit(EXIT_FAILURE);
    }
    mergeSortRec(v, aux, 0, n - 1);
    free(aux);
}

/* ======================================================
    QUICKSORT
    Pivo = mediana de 3 (evita O(n^2) em vetores ja ordenados)
    Recursao so na parte menor (pilha limitada a O(log n))
   ====================================================== */
static int particiona(Registro v[], int low, int high) {
    int mid = low + (high - low) / 2;

    /* mediana de 3 vai para v[high] */
    if (v[mid].chave  < v[low].chave) troca(&v[mid],  &v[low]);
    if (v[high].chave < v[low].chave) troca(&v[high], &v[low]);
    if (v[mid].chave  < v[high].chave) troca(&v[mid], &v[high]);

    long long pivo = v[high].chave;
    int i = low - 1;

    for (int j = low; j < high; j++) {
        if (v[j].chave < pivo) {
            i++;
            troca(&v[i], &v[j]);
        }
    }
    troca(&v[i + 1], &v[high]);
    return i + 1;
}

static void quickSortRec(Registro v[], int low, int high) {
    while (low < high) {
        int p = particiona(v, low, high);
        if (p - low < high - p) {
            quickSortRec(v, low, p - 1);
            low = p + 1;
        } else {
            quickSortRec(v, p + 1, high);
            high = p - 1;
        }
    }
}

static void quickSort(Registro v[], int n) {
    quickSortRec(v, 0, n - 1);
}

/* ======================================================
    CRONOMETRO
    Copia o vetor original, ordena a copia e mede o tempo.
   ====================================================== */
static double mede_tempo(FuncOrdenacao f, const Registro original[],
                         Registro trabalho[], int n) {
    memcpy(trabalho, original, (size_t)n * sizeof(Registro));

    clock_t ini = clock();
    f(trabalho, n);
    clock_t fim = clock();

    return (double)(fim - ini) / CLOCKS_PER_SEC;
}

/* ======================================================
    LEITURA DE ARQUIVOS
   ====================================================== */

// Procura qualquer linha contendo "ID" e le o numero que vem depois.
//   Aceita "ID 42", "ID: 42", "ID=42"... Retorna 1 se achou. 

static int le_id_arquivo(const char *caminho, int *id) {
    FILE *f = fopen(caminho, "r");
    if (!f) {
        perror("[ ERR ] Nao foi possivel abrir o arquivo");
        return 0;
    }

    char linha[256];
    int achou = 0;

    while (fgets(linha, sizeof(linha), f)) {
        char *p = strstr(linha, "ID");
        if (!p) continue;

        p += 2;
        while (*p && !isdigit((unsigned char)*p)) p++;
        if (*p) {
            *id = (int)strtol(p, NULL, 10);
            achou = 1;
        }
        break;
    }

    fclose(f);
    return achou;
}

static void adiciona_registro(Registro **v, int *n, int *cap,
                              int id, const char *nome) {
    if (*n == *cap) {
        *cap = (*cap == 0) ? 64 : *cap * 2;
        Registro *novo = realloc(*v, (size_t)*cap * sizeof(Registro));
        if (!novo) {
            fprintf(stderr, "[ ERR ] Sem memoria.\n");
            exit(EXIT_FAILURE);
        }
        *v = novo;
    }
    (*v)[*n].id = id;
    (*v)[*n].chave = id;
    (*v)[*n].largura = 0;
    (*v)[*n].altura = 0;
    (*v)[*n].data = 0;
    snprintf((*v)[*n].nome, MAX_NOME, "%s", nome);
    (*n)++;
}

// Le todos os .txt da pasta. Retorna o numero de registros (ou -1 em erro). 
static int carrega_pasta(const char *pasta, Registro **saida) {
    Registro *v = NULL;
    int n = 0, cap = 0;
    char caminho[MAX_NOME * 2];

// faz a verificacao de arquivos de forma diferente no Windows e no Linux
#ifdef _WIN32
    struct _finddata_t find;
    snprintf(caminho, sizeof(caminho), "%s\\*.txt", pasta);
    intptr_t h = _findfirst(caminho, &find);
    if (h == -1) {
        fprintf(stderr, "[ ERR ] Nenhum .txt encontrado em '%s'.\n", pasta);
        return -1;
    }
    do { //utiliza o do-while pra garantir que o primeiro arquivo encontrado seja processado
        if (find.attrib & _A_SUBDIR) continue;
        snprintf(caminho, sizeof(caminho), "%s\\%s", pasta, find.name);
        int id;
        if (le_id_arquivo(caminho, &id))
            adiciona_registro(&v, &n, &cap, id, find.name);
        else
            fprintf(stderr, "[ AVISO ] '%s' sem campo ID, ignorado.\n", find.name);
    } while (_findnext(h, &find) == 0);
    _findclose(h);
#else // caso for SO linux
    DIR *dir = opendir(pasta);
    if (!dir) {
        perror("[ ERR ] Nao foi possivel abrir a pasta");
        return -1;
    }
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (!termina_com_txt(entry->d_name)) continue;
        snprintf(caminho, sizeof(caminho), "%s/%s", pasta, entry->d_name);
        int id;
        if (le_id_arquivo(caminho, &id))
            adiciona_registro(&v, &n, &cap, id, entry->d_name);
        else
            fprintf(stderr, "[ AVISO ] '%s' sem campo ID, ignorado.\n", entry->d_name);
    }
    closedir(dir);
#endif

    *saida = v;
    return n;
}

/* ======================================================
    LEITURA DE METADADOS PNG (argumento -f <arquivo.png>)
   ====================================================== */

// lê 4 bytes big-endian
static unsigned be32(const unsigned char *p) {
    return ((unsigned)p[0] << 24) | ((unsigned)p[1] << 16) | ((unsigned)p[2] << 8) | (unsigned)p[3];
}

// Preenche largura, altura e data do registro. Retorna 1 se leu o IHDR.
static int le_png(const char *caminho, Registro *r) {
    FILE *f = fopen(caminho, "rb");
    if (!f) { perror("[ ERR ] Nao foi possivel abrir o arquivo"); return 0; }

    // verifica a assinatura se eh um arquivo PNG (8 bytes)
    unsigned char sig[8];
    // le fread(destino, tamanho, quantidade, arquivo)
    if (fread(sig, 1, 8, f) != 8 || memcmp(sig, "\x89PNG\r\n\x1a\n", 8) != 0) { //memory compare != 0 (conteudo dos bytes diferentes)
        fprintf(stderr, "[ERRO] Arquivo '%s' nao e um arquivo .png\n", caminho);
        fclose(f);
        return 0;
    }

    r->largura = 0;
    r->altura  = 0;
    r->data    = 0;
    int tem_ihdr = 0;

    // percorre os chunks: [tamanho 4][tipo 4][dados][crc 4]
    unsigned char cab[8]; // cabecalho do chunk: tamanho + tipo
    unsigned char d[16];  // buffer para dados do chunk (max 16 bytes) IHDR 13 bytes, tIME 7 bytes
    while (fread(cab, 1, 8, f) == 8) { // le o cabecalho do chunk
        unsigned len = be32(cab);
        char tipo[5] = {0};
        memcpy(tipo, cab + 4, 4);

        if (!strcmp(tipo, "IHDR")) { // "rotulacao" do chunk IHDR
            if (len < 13 || fread(d, 1, 13, f) != 13) break; // leitura dos 13 bytes do chunk IHDR
            r->largura = be32(d);
            r->altura  = be32(d + 4);
            tem_ihdr = 1;
            fseek(f, (long)(len - 13 + 4), SEEK_CUR);          // pula dados + CRC
        } else if (!strcmp(tipo, "tIME")) { // "rotulacao" do chunk tIME
            if (len < 7 || fread(d, 1, 7, f) != 7) break; // leitura dos 7 bytes do chunk tIME
            // data/hora (UTC) no padrao ISO 8601, guardada como AAAAMMDDhhmmss
            r->data = (long long)((d[0] << 8) | d[1]) * 10000000000LL
                    + d[2] * 100000000LL + d[3] * 1000000LL
                    + d[4] * 10000LL     + d[5] * 100LL     + d[6];
            fseek(f, (long)(len - 7 + 4), SEEK_CUR); // mesma logica de bloco de pular dados
        } else if (!strcmp(tipo, "IEND")) {
            break;
        } else {
            fseek(f, (long)len + 4, SEEK_CUR);    // pula dados + CRC
        }
    }

    fclose(f);
    if (!tem_ihdr)
        fprintf(stderr, "[ERRO] '%s' sem chunk IHDR valido.\n", caminho);
    return tem_ihdr;
}

/* ======================================================
    gerador de IDs random com argumento -r <quantidade>
   ====================================================== */
static Registro *valores_aleatorios(int n) {
    Registro *v = malloc((size_t)n * sizeof(Registro));
    if (!v) {
        fprintf(stderr, "[ ERR ] Sem memoria.\n");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < n; i++) {
        // combina dois rand() porque RAND_MAX pode ser so 32767 
        v[i].id = (int)((((unsigned)rand() << 15) | (unsigned)rand()) % 1000000u);
        v[i].chave = v[i].id;
        v[i].largura = 0;
        v[i].altura = 0;
        v[i].data = 0;
        snprintf(v[i].nome, MAX_NOME, "aleatorio_%d.txt", i);
    }
    return v;
}

/* ======================================================
    salva o log ordenado em arquivo de saida
   ====================================================== */
static int salva_ordenado(const char *caminho, const Registro v[], int n, int modo_png) {
    FILE *f = fopen(caminho, "w");
    if (!f) {
        perror("[ ERR ] Nao foi possivel criar o arquivo de saida");
        return 0;
    }
    for (int i = 0; i < n; i++) {
        if (modo_png) {
            char data[32];
            formata_data(v[i].data, data, sizeof(data));
            fprintf(f, "PNG %ux%u %s %s\n", v[i].largura, v[i].altura, data, v[i].nome);
        } else {
            fprintf(f, "ID %d %s\n", v[i].id, v[i].nome);
        }
    }
    fclose(f);
    return 1;
}

/* ======================================================
    esqueleto da CLI
   ====================================================== */
static void uso(const char *prog) {
    printf("Uso: %s (-d <pasta> | -r <quantidade> | -f <arquivo.png> ...) [-a <algoritmo>] [-k <chave>] [-o <saida.txt>]\n\n", prog);
    printf("  -d <pasta>      le os .txt da pasta (cada um com uma linha \"ID <numero>\")\n");
    printf("  -r <n>          gera n IDs aleatorios em vez de ler arquivos\n");
    printf("  -f <arquivo>    le metadados de um .png (pode ser repetido para varios arquivos)\n");
    printf("  -a <algoritmo>  bubble | merge | quick | todos (padrao: todos)\n");
    printf("  -k <chave>      criterio de ordenacao: id (para -d/-r) | largura | altura | data (para -f)\n");
    printf("                  padrao: id com -d/-r, largura com -f\n");
    printf("  -o <arquivo>    salva a lista ordenada neste arquivo\n");
}

typedef struct {
    const char   *chave;
    const char   *titulo;
    FuncOrdenacao func;
} Algoritmo;

static const Algoritmo ALGORITMOS[] = {
    { "bubble", "BubbleSort", bubbleSort },
    { "merge",  "MergeSort",  mergeSort  },
    { "quick",  "QuickSort",  quickSort  },
};
#define NUM_ALGORITMOS ((int)(sizeof(ALGORITMOS) / sizeof(ALGORITMOS[0])))

int main(int argc, char *argv[]) {
    const char *pasta = NULL, *saida = NULL, *algo = "todos", *crit = NULL;
    int aleatorios = 0;
    const char **pngs = malloc((size_t)(argc > 0 ? argc : 1) * sizeof(*pngs));
    int num_pngs = 0;
    if (!pngs) {
        fprintf(stderr, "[ ERR ] Sem memoria.\n");
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0 && i + 1 < argc)      pasta = argv[++i];
        else if (strcmp(argv[i], "-r") == 0 && i + 1 < argc) aleatorios = atoi(argv[++i]);
        else if (strcmp(argv[i], "-f") == 0 && i + 1 < argc) pngs[num_pngs++] = argv[++i];
        else if (strcmp(argv[i], "-a") == 0 && i + 1 < argc) algo = argv[++i];
        else if (strcmp(argv[i], "-k") == 0 && i + 1 < argc) crit = argv[++i];
        else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) saida = argv[++i];
        else {
            printf("Opcao invalida: %s\n\n", argv[i]);
            uso(argv[0]);
            free(pngs);
            return 1;
        }
    }

    // exatamente uma fonte de dados: pasta, aleatorios ou png
    int fontes = (pasta != NULL) + (aleatorios > 0) + (num_pngs > 0);
    if (fontes != 1) {
        uso(argv[0]);
        free(pngs);
        return 1;
    }
    int modo_png = (num_pngs > 0);

// valida o criterio de ordenacao (-k)
    Criterio criterio = CRIT_ID;
    if (modo_png) {
        if (!crit || strcmp(crit, "largura") == 0) criterio = CRIT_LARGURA;
        else if (strcmp(crit, "altura") == 0)      criterio = CRIT_ALTURA;
        else if (strcmp(crit, "data") == 0)        criterio = CRIT_DATA;
        else {
            printf("Chave invalida para -f: %s\n\n", crit);
            uso(argv[0]);
            free(pngs);
            return 1;
        }
    } else if (crit && strcmp(crit, "id") != 0) {
        printf("Chave invalida para -d/-r: %s\n\n", crit);
        uso(argv[0]);
        free(pngs);
        return 1;
    }

// valida o algoritmo escolhido
    int todos = (strcmp(algo, "todos") == 0);
    if (!todos) {
        int ok = 0;
        for (int i = 0; i < NUM_ALGORITMOS; i++)
            if (strcmp(algo, ALGORITMOS[i].chave) == 0) ok = 1;
        if (!ok) {
            printf("Algoritmo desconhecido: %s\n\n", algo);
            uso(argv[0]);
            free(pngs);
            return 1;
        }
    }

// carrega os dados
    Registro *dados = NULL;
    int n;

    if (pasta) {
        n = carrega_pasta(pasta, &dados);
        if (n < 0) { free(pngs); return 1; }
        if (n == 0) {
            fprintf(stderr, "[ ERR ] Nenhum arquivo valido encontrado.\n");
            free(dados);
            free(pngs);
            return 1;
        }
        printf("%d arquivo(s) lido(s) de '%s'\n\n", n, pasta);
    } else if (modo_png) {
        int cap = 0;
        n = 0;
        for (int i = 0; i < num_pngs; i++) {
            Registro r;
            if (le_png(pngs[i], &r)) {
                adiciona_registro(&dados, &n, &cap, 0, pngs[i]);
                dados[n - 1].largura = r.largura;
                dados[n - 1].altura  = r.altura;
                dados[n - 1].data    = r.data;
            } else {
                fprintf(stderr, "[ AVISO ] '%s' ignorado.\n", pngs[i]);
            }
        }
        if (n == 0) {
            fprintf(stderr, "[ ERR ] Nenhum arquivo valido encontrado.\n");
            free(dados);
            free(pngs);
            return 1;
        }
        printf("%d arquivo(s) .png lido(s)\n\n", n);
    } else {
        srand((unsigned)time(NULL));
        n = aleatorios;
        dados = valores_aleatorios(n);
        printf("%d IDs aleatorios gerados\n\n", n);
    }
    free(pngs);

    aplica_criterio(dados, n, criterio);

    Registro *trabalho = malloc((size_t)n * sizeof(Registro));
    if (!trabalho) {
        fprintf(stderr, "[ ERR ] Sem memoria.\n");
        free(dados);
        return 1;
    }

// roda e cronometra
    printf("%-12s %12s  %s\n", "Algoritmo", "Tempo (s)", "Ordenado?");
    printf("--------------------------------------\n");

    for (int i = 0; i < NUM_ALGORITMOS; i++) {
        if (!todos && strcmp(algo, ALGORITMOS[i].chave) != 0) continue;

        double t = mede_tempo(ALGORITMOS[i].func, dados, trabalho, n);
        printf("%-12s %12.6f  %s\n", ALGORITMOS[i].titulo, t,
               esta_ordenado(trabalho, n) ? "sim" : "NAO (bug!)");
    }

// mostra os metadados na ordem final (poucos arquivos, vale a pena ver)
    if (modo_png) {
        printf("\n%-10s %-20s %s\n", "Dimensao", "Data/hora (UTC)", "Arquivo");
        for (int i = 0; i < n; i++) {
            char data[32], dim[32];
            formata_data(trabalho[i].data, data, sizeof(data));
            snprintf(dim, sizeof(dim), "%ux%u", trabalho[i].largura, trabalho[i].altura);
            printf("%-10s %-20s %s\n", dim, data, trabalho[i].nome);
        }
    }

// salva o resultado da ultima execucao (todas produzem o mesmo resultado)
    if (saida) {
        if (salva_ordenado(saida, trabalho, n, modo_png))
            printf("\nResultado salvo em '%s'\n", saida);
    }

    free(trabalho);
    free(dados);
    return 0;
}