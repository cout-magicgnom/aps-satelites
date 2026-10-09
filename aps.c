#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/*
Algoritmos:
1. BubbleSort
2. MergeSort
3. QuickSort

objetivo: ferramenta em C que implementa 3 algoritmos de ordenacao, le arquivos .txt e compara o tempo de 
execucao de cada algoritmo para cada conjunto de arquivo (exemplo utilizado, lista encadeada). 


TODO:
    x implementar os algoritmos
    x implementar time.h em cada chamada de algoritmo (fazer uma funcao separada)
    - fazer os algoritmos ordenarem os arquivos 
        -> abrir arquivo -> ler ID -> ordenar
        -> apenas ordenar a pasta 
    - criar valores_aleatorios() para geracao de IDs aleatorios
    - criar CLI

ERROS:
    - 102 compilação função swap()

*/

// Função auxiliar para trocar dois elementos de posição
void swap(int* a, int* b) {
    int t = *a;
    *a = *b;
    *b = t;
}

#ifdef WIN32
    #include "io.h"
#else
    #include <sys/stat.h>
    #include <dirent.h>
    #include <unistd.h>
#endif

#include <stdio.h>
#include <string.h>

void list_dir (void ) {

#ifdef WIN32
    int done;
    struct _finddata_t find;

    //-------------------------------------------
    // Find ONLY DIRECTORY
    //-------------------------------------------
    done = _findfirst("*.*", &find);
    do {

//        if (strcmp(find.name, "."))
        if (find.attrib & _A_SUBDIR) {
            printf ("%s\n", find.name);
        }

    } while ( !_findnext(done, &find) );
    _findclose(done);

#endif // WIN32

#ifdef __linux__
    DIR *dir;
    struct dirent *entry;
    struct stat s;

    //-------------------------------------------
    // Find ONLY DIRECTORY
    //-------------------------------------------
    dir = opendir("./");
    for (;;) {
        entry = readdir(dir);
        if (!entry) break;

//        if (strcmp(entry->d_name, "."))
        if ( stat(entry->d_name, &s) == 0 && S_ISDIR(s.st_mode) ) {
            printf ("%s\n", entry->d_name);
        }
    }
    closedir(dir);

#endif // __linux__

}// list_dir ()







// Função de partição usada pelo Quick Sort (usa o último elemento como pivô)
int partition(int arr[], int low, int high) {
    int pivot = arr[high]; // Pivô
    int i = (low - 1);     // Índice do menor elemento

    for (int j = low; j <= high - 1; j++) {
        // Se o elemento atual for menor que o pivô
        if (arr[j] < pivot) {
            i++; // Incrementa o índice do menor elemento
            swap(&arr[i], &arr[j]);
        }
    }
    swap(&arr[i + 1], &arr[high]);
    return (i + 1);
}

void le_arquivotxt(const char *arquivo, int vetor[], int tammax){
    FILE *file = fopen("arquivo", "r");
    //verificaca se arquivo existe
    if(arquivo == NULL){
        perror("[ ERR ] Arquivo nao existente ou corrompido. error:00x1 ");
    }
    char buffer[3];

    // fgets lê até (tamanho - 1) caracteres, ou seja, no máximo 2 caracteres aqui
    if (fgets(buffer, sizeof(buffer), file) != NULL) {
        printf("Lido do arquivo: \"%s\"\n", buffer);

        if (buffer[0] == 'I' && buffer[1] == 'D') {
            printf("\nsucesso\n");
        } else {
        printf("O arquivo esta vazio ou ocorreu um erro na leitura.\n");
        }
    }

    // Sempre feche o arquivo ao terminar
    fclose(file);
}


/*      
======================================
            MERGESORT
======================================
*/
void merge(int arr[], int l, int m, int r) {
    int n1 = m - l + 1;
    int n2 = r - m;

    int *L = (int *)malloc(n1 * sizeof(int));
    int *R = (int *)malloc(n2 * sizeof(int));

    for (int i = 0; i < n1; i++)
        L[i] = arr[l + i];
    for (int j = 0; j < n2; j++)
        R[j] = arr[m + 1 + j];

    int i = 0, j = 0, k = l;
    while (i < n1 && j < n2) {
        if (L[i] <= R[j]) {
            arr[k++] = L[i++];
        } else {
            arr[k++] = R[j++];
        }
    }

    while (i < n1) arr[k++] = L[i++];
    while (j < n2) arr[k++] = R[j++];

    free(L);
    free(R);
}

void mergeSort(int arr[], int l, int r) {
    if (l < r) {
        int m = l + (r - l) / 2;
        mergeSort(arr, l, m);
        mergeSort(arr, m + 1, r);
        merge(arr, l, m, r);
    }
}
/*      
======================================
            QUICKSORT   
======================================

int partition(int arr[], int low, int high) {
    int pivot = arr[high]; // Pivô como último elemento
    int i = (low - 1);

    for (int j = low; j <= high - 1; j++) {
        if (arr[j] < pivot) {
            i++;
            swap(&arr[i], &arr[j]);
        }
    }
    swap(&arr[i + 1], &arr[high]);
    return (i + 1);
}
*/
void quickSort(int arr[], int low, int high) {
    if (low < high) {
        int pi = partition(arr, low, high);
        quickSort(arr, low, pi - 1);
        quickSort(arr, pi + 1, high);
    }
}
/*      
======================================
            BUBBLESORT
======================================
*/
void bubbleSort(int arr[], int n) {
    for (int i = 0; i < n - 1; i++) {
        int swapped = 0;
        for (int j = 0; j < n - i - 1; j++) {
            if (arr[j] > arr[j + 1]) {
                swap(&arr[j], &arr[j + 1]);
                swapped = 1;
            }
        }
        // Se nenhum elemento foi trocado, o vetor já está ordenado
        if (!swapped) break;
    }
}

int main(int argc, char *argv[]) {
    // Check if enough arguments are provided
    if (argc < 3) {
        printf("Usage: %s <option> <filename>\n", argv[0]);
        printf("Options: -c (count chars), -w (count words)\n");
        return 1;
    }

    char *option = argv[1];
    char *filename = argv[2];
    FILE *file = fopen(filename, "r");

    if (file == NULL) {
        printf("Error: Could not open file %s\n", filename);
        return 1;
    }

    if (strcmp(option, "-c") == 0) {
        
        printf("Counting characters in %s...\n", filename);
    } else if (strcmp(option, "-w") == 0) {
        // Count words logic would go here
        printf("Counting words in %s...\n", filename);
    } else {
        printf("Unknown option: %s\n", option);
    }

    fclose(file);
    return 0;
}