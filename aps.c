/*
 * ordenador.c
 *
 * Ferramenta em C que implementa BubbleSort, MergeSort e QuickSort,
 * le arquivos .txt de uma pasta (cada um contendo uma linha "ID <numero>"),
 * ordena os arquivos pelo ID e compara o tempo de execucao de cada algoritmo.
 * Tambem pode gerar IDs aleatorios para benchmark.
 *
 * Compilar:  gcc -O2 -Wall -Wextra -o ordenador ordenador.c
 *
 * Exemplos:
 *   ./ordenador -d ./dados                 (le a pasta, roda os 3 algoritmos)
 *   ./ordenador -d ./dados -a quick -o saida.txt
 *   ./ordenador -r 20000 -a todos          (20000 IDs aleatorios)
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
 *  TIPOS
 * ====================================================== */

/* Um "registro" = um arquivo .txt e o ID lido de dentro dele */
typedef struct {
    int  id;
    char nome[MAX_NOME];
} Registro;

typedef void (*FuncOrdenacao)(Registro v[], int n);

/* ======================================================
 *  UTILITARIOS
 * ====================================================== */

static void troca(Registro *a, Registro *b) {
    Registro t = *a;
    *a = *b;
    *b = t;
}

static int esta_ordenado(const Registro v[], int n) {
    for (int i = 1; i < n; i++)
        if (v[i - 1].id > v[i].id) return 0;
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

/* ======================================================
 *  BUBBLESORT
 * ====================================================== */
static void bubbleSort(Registro v[], int n) {
    for (int i = 0; i < n - 1; i++) {
        int trocou = 0;
        for (int j = 0; j < n - i - 1; j++) {
            if (v[j].id > v[j + 1].id) {
                troca(&v[j], &v[j + 1]);
                trocou = 1;
            }
        }
        if (!trocou) break; /* ja esta ordenado */
    }
}

/* ======================================================
 *  MERGESORT
 *  (buffer auxiliar alocado uma unica vez, nao a cada merge)
 * ====================================================== */
static void merge(Registro v[], Registro aux[], int l, int m, int r) {
    int i = l, j = m + 1, k = l;

    while (i <= m && j <= r) {
        if (v[i].id <= v[j].id) aux[k++] = v[i++];
        else                    aux[k++] = v[j++];
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
 *  QUICKSORT
 *  Pivo = mediana de 3 (evita O(n^2) em vetores ja ordenados)
 *  Recursao so na parte menor (pilha limitada a O(log n))
 * ====================================================== */
static int particiona(Registro v[], int low, int high) {
    int mid = low + (high - low) / 2;

    /* mediana de 3 vai para v[high] */
    if (v[mid].id  < v[low].id) troca(&v[mid],  &v[low]);
    if (v[high].id < v[low].id) troca(&v[high], &v[low]);
    if (v[mid].id  < v[high].id) troca(&v[mid], &v[high]);

    int pivo = v[high].id;
    int i = low - 1;

    for (int j = low; j < high; j++) {
        if (v[j].id < pivo) {
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
 *  CRONOMETRO
 *  Copia o vetor original, ordena a copia e mede o tempo.
 * ====================================================== */
static double mede_tempo(FuncOrdenacao f, const Registro original[],
                         Registro trabalho[], int n) {
    memcpy(trabalho, original, (size_t)n * sizeof(Registro));

    clock_t ini = clock();
    f(trabalho, n);
    clock_t fim = clock();

    return (double)(fim - ini) / CLOCKS_PER_SEC;
}

/* ======================================================
 *  LEITURA DE ARQUIVOS
 * ====================================================== */

/* Procura uma linha contendo "ID" e le o numero que vem depois.
 * Aceita "ID 42", "ID: 42", "ID=42"... Retorna 1 se achou. */
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
    snprintf((*v)[*n].nome, MAX_NOME, "%s", nome);
    (*n)++;
}

/* Le todos os .txt da pasta. Retorna o numero de registros (ou -1 em erro). */
static int carrega_pasta(const char *pasta, Registro **saida) {
    Registro *v = NULL;
    int n = 0, cap = 0;
    char caminho[MAX_NOME * 2];

#ifdef _WIN32
    struct _finddata_t find;
    snprintf(caminho, sizeof(caminho), "%s\\*.txt", pasta);
    intptr_t h = _findfirst(caminho, &find);
    if (h == -1) {
        fprintf(stderr, "[ ERR ] Nenhum .txt encontrado em '%s'.\n", pasta);
        return -1;
    }
    do {
        if (find.attrib & _A_SUBDIR) continue;
        snprintf(caminho, sizeof(caminho), "%s\\%s", pasta, find.name);
        int id;
        if (le_id_arquivo(caminho, &id))
            adiciona_registro(&v, &n, &cap, id, find.name);
        else
            fprintf(stderr, "[ AVISO ] '%s' sem campo ID, ignorado.\n", find.name);
    } while (_findnext(h, &find) == 0);
    _findclose(h);
#else
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
 *  IDs ALEATORIOS
 * ====================================================== */
static Registro *valores_aleatorios(int n) {
    Registro *v = malloc((size_t)n * sizeof(Registro));
    if (!v) {
        fprintf(stderr, "[ ERR ] Sem memoria.\n");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < n; i++) {
        /* combina dois rand() porque RAND_MAX pode ser so 32767 */
        v[i].id = (int)((((unsigned)rand() << 15) | (unsigned)rand()) % 1000000u);
        snprintf(v[i].nome, MAX_NOME, "aleatorio_%d.txt", i);
    }
    return v;
}

/* ======================================================
 *  SAIDA
 * ====================================================== */
static int salva_ordenado(const char *caminho, const Registro v[], int n) {
    FILE *f = fopen(caminho, "w");
    if (!f) {
        perror("[ ERR ] Nao foi possivel criar o arquivo de saida");
        return 0;
    }
    for (int i = 0; i < n; i++)
        fprintf(f, "ID %d %s\n", v[i].id, v[i].nome);
    fclose(f);
    return 1;
}

/* ======================================================
 *  CLI
 * ====================================================== */
static void uso(const char *prog) {
    printf("Uso: %s (-d <pasta> | -r <quantidade>) [-a <algoritmo>] [-o <saida.txt>]\n\n", prog);
    printf("  -d <pasta>      le os .txt da pasta (cada um com uma linha \"ID <numero>\")\n");
    printf("  -r <n>          gera n IDs aleatorios em vez de ler arquivos\n");
    printf("  -a <algoritmo>  bubble | merge | quick | todos (padrao: todos)\n");
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
    const char *pasta = NULL, *saida = NULL, *algo = "todos";
    int aleatorios = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0 && i + 1 < argc)      pasta = argv[++i];
        else if (strcmp(argv[i], "-r") == 0 && i + 1 < argc) aleatorios = atoi(argv[++i]);
        else if (strcmp(argv[i], "-a") == 0 && i + 1 < argc) algo = argv[++i];
        else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) saida = argv[++i];
        else {
            printf("Opcao invalida: %s\n\n", argv[i]);
            uso(argv[0]);
            return 1;
        }
    }

    if ((!pasta && aleatorios <= 0) || (pasta && aleatorios > 0)) {
        uso(argv[0]);
        return 1;
    }

    /* valida o algoritmo escolhido */
    int todos = (strcmp(algo, "todos") == 0);
    if (!todos) {
        int ok = 0;
        for (int i = 0; i < NUM_ALGORITMOS; i++)
            if (strcmp(algo, ALGORITMOS[i].chave) == 0) ok = 1;
        if (!ok) {
            printf("Algoritmo desconhecido: %s\n\n", algo);
            uso(argv[0]);
            return 1;
        }
    }

    /* carrega os dados */
    Registro *dados = NULL;
    int n;

    if (pasta) {
        n = carrega_pasta(pasta, &dados);
        if (n < 0) return 1;
        if (n == 0) {
            fprintf(stderr, "[ ERR ] Nenhum arquivo valido encontrado.\n");
            free(dados);
            return 1;
        }
        printf("%d arquivo(s) lido(s) de '%s'\n\n", n, pasta);
    } else {
        srand((unsigned)time(NULL));
        n = aleatorios;
        dados = valores_aleatorios(n);
        printf("%d IDs aleatorios gerados\n\n", n);
    }

    Registro *trabalho = malloc((size_t)n * sizeof(Registro));
    if (!trabalho) {
        fprintf(stderr, "[ ERR ] Sem memoria.\n");
        free(dados);
        return 1;
    }

    /* roda e cronometra */
    printf("%-12s %12s  %s\n", "Algoritmo", "Tempo (s)", "Ordenado?");
    printf("--------------------------------------\n");

    for (int i = 0; i < NUM_ALGORITMOS; i++) {
        if (!todos && strcmp(algo, ALGORITMOS[i].chave) != 0) continue;

        double t = mede_tempo(ALGORITMOS[i].func, dados, trabalho, n);
        printf("%-12s %12.6f  %s\n", ALGORITMOS[i].titulo, t,
               esta_ordenado(trabalho, n) ? "sim" : "NAO (bug!)");
    }

    /* salva o resultado da ultima execucao (todas produzem o mesmo resultado) */
    if (saida) {
        if (salva_ordenado(saida, trabalho, n))
            printf("\nResultado salvo em '%s'\n", saida);
    }

    free(trabalho);
    free(dados);
    return 0;
}