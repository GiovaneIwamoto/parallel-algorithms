#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mpi.h>

void imprimeVetor(int *vet, int tam) {
    int i;
    for (i = 0; i < tam; ++i)
        printf("%d ", vet[i]);
    printf("\n\n");
}

void troca(int *vet, int i, int j) {
    int temp = vet[i];
    vet[i] = vet[j];
    vet[j] = temp;
}

int particiona(int *vet, int inicio, int fim) {
    int i = inicio, pivo = vet[inicio], temp;
    for (int j = inicio + 1; j < fim; j++) {
        if (vet[j] <= pivo) {
            i++;
            troca(vet, i, j);
        }
    }
    troca(vet, inicio, i);
    return i;
}

void quicksort(int *vet, int inicio, int fim) {
    if (inicio < fim) {
        int pivo = particiona(vet, inicio, fim);
        quicksort(vet, inicio, pivo);
        quicksort(vet, pivo + 1, fim);
    }
}

void quicksortMPI(int *vet, int inicio, int fim, int rank, int np, int rank_index) {
    int dest = rank + (1 << rank_index);

    if (dest >= np) {
        quicksort(vet, inicio, fim);
    } else if (inicio < fim) {
        int pivo = particiona(vet, inicio, fim);
        if (pivo - inicio > fim - pivo - 1) {
            MPI_Send(&vet[pivo + 1], fim - pivo - 1, MPI_INT, dest, 1, MPI_COMM_WORLD);
            quicksortMPI(vet, inicio, pivo, rank, np, rank_index + 1);
            MPI_Recv(&vet[pivo + 1], fim - pivo - 1, MPI_INT, dest, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        } else {
            MPI_Send(&vet[inicio], pivo - inicio, MPI_INT, dest, 1, MPI_COMM_WORLD);
            quicksortMPI(vet, pivo + 1, fim, rank, np, rank_index + 1);
            MPI_Recv(&vet[inicio], pivo - inicio, MPI_INT, dest, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
    }
}

int main(int argc, char **argv) {
    int np, rank;
    int *vet, tam;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &np);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Status msg;

    double ti, ti_2, tf, tf_2;

    // Rank 0 -> Mestre
    if (rank == 0) {
        // Gere um vetor aleatório
        printf("Tamanho do vetor: ");
        scanf("%d", &tam);
        vet = (int *)malloc(sizeof(int) * tam);
        srand(time(NULL));
        for (int i = 0; i < tam; i++) {
            vet[i] = rand() % 1000; // Valores aleatórios entre 0 e 999
        }

        // Inicializa tempo de execução
        ti = MPI_Wtime();
        quicksortMPI(vet, 0, tam, rank, np, 0);

        // Finaliza tempo de execução
        tf = MPI_Wtime();

        printf("\n-------------- Vetor Ordenado --------------\n");
        imprimeVetor(vet, tam);
        printf("Tamanho: %d\n", tam);
        printf("Tempo de Execução: %.4f seconds\n", tf - ti);

        int i, end = -1;
        for (i = 0; i < np; i++)
            MPI_Send(&end, 1, MPI_INT, i, 2, MPI_COMM_WORLD);

        free(vet);
    } else {
        int tamanho, origem, index_count = 0;
        while (1 << index_count <= rank)
            index_count++;

        MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &msg);

        if (msg.MPI_TAG == 2) {
            MPI_Finalize();
            return 0;
        }

        MPI_Get_count(&msg, MPI_INT, &tamanho);

        origem = msg.MPI_SOURCE;
        vet = (int *)malloc(sizeof(int) * tamanho);
        MPI_Recv(vet, tamanho, MPI_INT, origem, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        quicksortMPI(vet, 0, tamanho, rank, np, index_count);
        MPI_Send(vet, tamanho, MPI_INT, origem, 1, MPI_COMM_WORLD);
        free(vet);
    }

    MPI_Finalize();
    return 0;
}