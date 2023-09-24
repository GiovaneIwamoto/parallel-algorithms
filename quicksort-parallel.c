#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

const int RMAX = 100;

void Get_args(int argc, char *argv[], int *global_psize, int *local_psize, int my_rank, int p, MPI_Comm comm)
{
    if (argc != 2)
    {
        MPI_Finalize();
        exit(-1);
    }

    *global_psize = strtol(argv[1], NULL, 10);

    if (*global_psize <= 0 || *global_psize % p != 0)
    {
        MPI_Finalize();
        exit(-1);
    }

    *local_psize = *global_psize / p; // Divide
}

void Print_list(int *vet, int size)
{
    int i;
    for (i = 0; i < size; ++i)
        printf("%d ", vet[i]);
    printf("\n\n");
}

void Swap(int *vet, int i, int j)
{
    int temp = vet[i];
    vet[i] = vet[j];
    vet[j] = temp;
}

int Partition(int *vet, int start, int end)
{
    int i = start, pivot = vet[start];
    for (int j = start + 1; j < end; j++)
    {
        if (vet[j] <= pivot)
        {
            i++;
            Swap(vet, i, j);
        }
    }
    Swap(vet, start, i);
    return i;
}

void Quicksort(int *vet, int start, int end)
{
    if (start < end)
    {
        int pivot = Partition(vet, start, end);
        Quicksort(vet, start, pivot);
        Quicksort(vet, start + 1, end);
    }
}

void Quicksort_MPI(int *vet, int start, int end, int rank, int np, int rank_index)
{
    int dest = rank + (1 << rank_index);

    if (dest >= np)
    {
        Quicksort(vet, start, end);
    }
    else if (start < end)
    {
        int pivot = Partition(vet, start, end);

        if (pivot - start > end - pivot - 1)
        {
            MPI_Send(&vet[pivot + 1], end - pivot - 1, MPI_INT, dest, 1, MPI_COMM_WORLD);
            Quicksort_MPI(vet, start, pivot, rank, np, rank_index + 1);
            MPI_Recv(&vet[pivot + 1], end - pivot - 1, MPI_INT, dest, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }

        else
        {
            MPI_Send(&vet[start], pivot - start, MPI_INT, dest, 1, MPI_COMM_WORLD);
            Quicksort_MPI(vet, pivot + 1, end, rank, np, rank_index + 1);
            MPI_Recv(&vet[start], pivot - start, MPI_INT, dest, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
    }
}

int main(int argc, char **argv)
{
    int np, rank;
    int *vet, size;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &np);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    MPI_Status msg;

    double start_time, finish_time;
    int global_psize, local_psize;

    Get_args(argc, argv, &global_psize, &local_psize, rank, np, MPI_COMM_WORLD);
    size = global_psize;

    if (rank == 0)
    {
        vet = (int *)malloc(sizeof(int) * size);
        srand(time(NULL));

        for (int i = 0; i < size; i++)
        {
            vet[i] = rand() % RMAX;
        }

        /*
        Printing original list
        Print_list(vet, size);
        */

        // Start time
        start_time = MPI_Wtime();
        Quicksort_MPI(vet, 0, size, rank, np, 0);

        // Finish time
        finish_time = MPI_Wtime();

        /*
        Printing ordered list
        Print_list(vet, size);
        */

        printf("Size: %d\n", size);
        printf("Sorting time: %e sec\n", finish_time - start_time);

        int i, end = -1;
        for (i = 0; i < np; i++)
            MPI_Send(&end, 1, MPI_INT, i, 2, MPI_COMM_WORLD);

        free(vet);
    }
    else
    {
        int sz, source, index_count = 0;
        while (1 << index_count <= rank)
            index_count++;

        MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &msg);

        if (msg.MPI_TAG == 2)
        {
            MPI_Finalize();
            return 0;
        }

        MPI_Get_count(&msg, MPI_INT, &sz);

        source = msg.MPI_SOURCE;
        vet = (int *)malloc(sizeof(int) * sz);

        MPI_Recv(vet, sz, MPI_INT, source, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        Quicksort_MPI(vet, 0, sz, rank, np, index_count);
        MPI_Send(vet, sz, MPI_INT, source, 1, MPI_COMM_WORLD);
        free(vet);
    }

    MPI_Finalize();
    return 0;
}
