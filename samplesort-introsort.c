/*
USAGE: mpiexec --oversubscribe -n <p> samplesort-introsort <n>
*/

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <mpi.h> /* Include MPI's header file */
#include <math.h>

// Swap function
void Swap(int *a, int *b)
{
    int temp = *a;
    *a = *b;
    *b = temp;
}

// Insertion function
void InsertionSort(int arr[], int *begin, int *end)
{
    for (int *i = begin + 1; i <= end; i++)
    {
        int key = *i;
        int *j = i - 1;

        while (j >= begin && *j > key)
        {
            *(j + 1) = *j;
            j--;
            +
        }

        *(j + 1) = key;
    }
}

// Partition point
int *Partition(int arr[], int *low, int *high)
{
    int pivot = *high;
    int *i = low - 1;

    for (int *j = low; j <= high - 1; j++)
    {
        if (*j <= pivot)
        {
            i++;
            Swap(i, j);
        }
    }

    Swap(i + 1, high);
    return i + 1;
}

// Medium pivot
int *MedianOfThree(int *a, int *b, int *c)
{
    if (*a < *b && *b < *c)
        return b;

    if (*a < *c && *c <= *b)
        return c;

    if (*b <= *a && *a < *c)
        return a;

    if (*b < *c && *c <= *a)
        return c;

    if (*c <= *a && *a < *b)
        return a;

    if (*c <= *b && *b <= *a)
        return b;

    return a;
}

// Function to perform heapsort
void Heapsort(int arr[], int *begin, int *end)
{
    int size = end - begin + 1;

    for (int *i = begin + size / 2 - 1; i >= begin; i--)
    {
        // Build max heap
        int *current = i;
        int *largest = i;
        while (1)
        {
            int leftChildIdx = 2 * (current - begin) + 1;
            int rightChildIdx = leftChildIdx + 1;

            if (leftChildIdx < size && arr[leftChildIdx] > arr[largest - begin])
                largest = begin + leftChildIdx;

            if (rightChildIdx < size && arr[rightChildIdx] > arr[largest - begin])
                largest = begin + rightChildIdx;

            if (largest == current)
                break;

            int temp = *current;
            *current = *largest;
            *largest = temp;

            current = largest;
        }
    }

    for (int *i = end; i > begin; i--)
    {
        int temp = *begin;
        *begin = *i;
        *i = temp;

        size--;
        int *current = begin;
        int *largest = begin;
        while (1)
        {
            int leftChildIdx = 2 * (current - begin) + 1;
            int rightChildIdx = leftChildIdx + 1;

            if (leftChildIdx < size && arr[leftChildIdx] > arr[largest - begin])
                largest = begin + leftChildIdx;

            if (rightChildIdx < size && arr[rightChildIdx] > arr[largest - begin])
                largest = begin + rightChildIdx;

            if (largest == current)
                break;

            int temp = *current;
            *current = *largest;
            *largest = temp;

            current = largest;
        }
    }
}

void IntrosortUtil(int arr[], int *begin, int *end, int depthLimit)
{
    // Count the number of elements
    int size = end - begin;

    // If partition size is low then do insertion sort
    if (size < 16)
    {
        InsertionSort(arr, begin, end);
        return;
    }

    // If the depth is zero use heapsort
    if (depthLimit == 0)
    {
        Heapsort(arr, begin, end);
        return;
    }

    // Else use a median-of-three concept to
    // find a good pivot
    int *pivot = MedianOfThree(begin, begin + size / 2, end);
    Swap(pivot, end);

    int *partitionPoint = Partition(arr, begin, end);
    IntrosortUtil(arr, begin, partitionPoint - 1, depthLimit - 1);
    IntrosortUtil(arr, partitionPoint + 1, end, depthLimit - 1);
}

void Introsort(int arr[], int *begin, int *end)
{
    int depthLimit = 2 * log(end - begin + 1);
    IntrosortUtil(arr, begin, end, depthLimit);
}

int *SampleSort(int n, int *elmnts, int *nsorted, MPI_Comm comm)
{
    int i, j, nlocal, npes, myrank;
    int *sorted_elmnts, *splitters, *allpicks;
    int *scounts, *sdispls, *rcounts, *rdispls;

    /* Get communicator-related information */
    MPI_Comm_size(comm, &npes);
    MPI_Comm_rank(comm, &myrank);
    nlocal = n / npes;

    /* Allocate memory for the arrays that will store the splitters */
    splitters = (int *)malloc(npes * sizeof(int));
    allpicks = (int *)malloc(npes * (npes - 1) * sizeof(int));

    /* Sort local array using introsort */
    Introsort(elmnts, elmnts, elmnts + nlocal - 1);

    /* Select local npes-1 equally spaced elements */
    for (i = 1; i < npes; i++)
        splitters[i - 1] = elmnts[i * nlocal / npes];

    /* Gather the samples in the processors */
    MPI_Allgather(splitters, npes - 1, MPI_INT, allpicks, npes - 1,
                  MPI_INT, comm);

    /* Sort these samples using introsort */
    Introsort(allpicks, allpicks, allpicks + npes * (npes - 1) - 1);

    /* Select splitters */
    for (i = 1; i < npes; i++)
        splitters[i - 1] = allpicks[i * npes];
    splitters[npes - 1] = INT_MAX;

    /* Compute the number of elements that belong to each bucket */
    scounts = (int *)malloc(npes * sizeof(int));
    for (i = 0; i < npes; i++)
        scounts[i] = 0;

    for (j = i = 0; i < nlocal; i++)
    {
        if (elmnts[i] < splitters[j])
            scounts[j]++;
        else
            scounts[++j]++;
    }

    /* Determine the starting location of each bucket's elements in the elmnts array */
    sdispls = (int *)malloc(npes * sizeof(int));
    sdispls[0] = 0;

    for (i = 1; i < npes; i++)
        sdispls[i] = sdispls[i - 1] + scounts[i - 1];

    /* Perform an all-to-all to inform the corresponding processes of the number of elements */
    /* they are going to receive. This information is stored in rcounts array */

    rcounts = (int *)malloc(npes * sizeof(int));
    MPI_Alltoall(scounts, 1, MPI_INT, rcounts, 1, MPI_INT, comm);

    /* Based on rcounts determine where in the local array the data from each processor */
    /* will be stored. This array will store the received elements as well as the final */
    /* sorted sequence */

    rdispls = (int *)malloc(npes * sizeof(int));
    rdispls[0] = 0;
    for (i = 1; i < npes; i++)
        rdispls[i] = rdispls[i - 1] + rcounts[i - 1];
    *nsorted = rdispls[npes - 1] + rcounts[i - 1];
    sorted_elmnts = (int *)malloc((*nsorted) * sizeof(int));

    /* Each process sends and receives the corresponding elements, using the MPI_Alltoallv */
    /* operation. The arrays scounts and sdispls are used to specify the number of elements */
    /* to be sent and where these elements are stored, respectively. The arrays rcounts */
    /* and rdispls are used to specify the number of elements to be received, and where these */
    /* elements will be stored, respectively. */
    MPI_Alltoallv(elmnts, scounts, sdispls, MPI_INT, sorted_elmnts, rcounts, rdispls, MPI_INT, comm);

    /* Perform the final local sort using introsort */
    Introsort(sorted_elmnts, sorted_elmnts, sorted_elmnts + (*nsorted) - 1);

    free(splitters);
    free(allpicks);
    free(scounts);
    free(sdispls);

    free(rcounts);
    free(rdispls);
    return sorted_elmnts;
}

int main(int argc, char *argv[])
{
    int n;
    int npes;
    int myrank;
    int nlocal;
    int *elmnts;  /* array that stores the local elements */
    int *vsorted; /* array that stores the final sorted elements */
    int nsorted;  /* number de elements in vsorted */
    int i;
    // MPI_Status status;
    double stime, etime;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &npes);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);

    if (argc != 2)
    {
        if (myrank == 0)
        {
            printf("Uso: mpiexec -n <p> %s <n>\n", argv[0]);
        }
        MPI_Finalize();
        return 1;
    }

    n = atoi(argv[1]);
    nlocal = n / npes; /* Compute the number of elements to be stored locally. */

    /* Allocate memory for the various arrays */
    elmnts = (int *)malloc(nlocal * sizeof(int));

    /* Fill-in the elmnts array with random elements */
    srandom(myrank);

    for (i = 0; i < nlocal; i++)
    {
        elmnts[i] = random() % (10 * n + 1);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    stime = MPI_Wtime();

    vsorted = SampleSort(n, elmnts, &nsorted, MPI_COMM_WORLD);

    etime = MPI_Wtime();

    MPI_Barrier(MPI_COMM_WORLD);

    if (myrank == 0)
    {
        printf("SAMPLESORT WITH INTROSORT\n");
        printf("Sorting time: %e sec\n", etime - stime);
    }

    free(elmnts);
    free(vsorted);

    MPI_Finalize();

    return 0;
}
