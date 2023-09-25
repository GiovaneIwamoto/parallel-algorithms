/*
USAGE: mpiexec --oversubscribe -n <p> samplesort <n>
*/

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <mpi.h> /* Include MPI's header file */

/* The IncOrder function that is called by qsort is defined as follows */
int IncOrder(const void *e1, const void *e2)
{
    return (*((int *)e1) - *((int *)e2));
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

    /* Sort local array */
    qsort(elmnts, nlocal, sizeof(int), IncOrder);

    /* Select local npes-1 equally spaced elements */
    for (i = 1; i < npes; i++)
        splitters[i - 1] = elmnts[i * nlocal / npes];

    /* Gather the samples in the processors */
    MPI_Allgather(splitters, npes - 1, MPI_INT, allpicks, npes - 1,
                  MPI_INT, comm);

    /* sort these samples */
    qsort(allpicks, npes * (npes - 1), sizeof(int), IncOrder);

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

    /* Perform the final local sort */
    qsort(sorted_elmnts, *nsorted, sizeof(int), IncOrder);
    free(splitters);
    free(allpicks);
    free(scounts);
    free(sdispls);

    free(rcounts);
    free(rdispls);
    return sorted_elmnts;
}

/* main program  */

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
    MPI_Status status;
    double stime, etime;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &npes);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);

    n = atoi(argv[1]);
    nlocal = n / npes; /* Compute the number of elements to be stored locally. */

    /* Allocate memory for the various arrays */
    elmnts = (int *)malloc(nlocal * sizeof(int));

    /* Fill-in the elmnts array with random elements */

    srandom(myrank);

    for (i = 0; i < nlocal; i++)
        elmnts[i] = random() % (10 * n + 1);

    MPI_Barrier(MPI_COMM_WORLD);

    stime = MPI_Wtime();

    vsorted = SampleSort(n, elmnts, &nsorted, MPI_COMM_WORLD);

    etime = MPI_Wtime();

    MPI_Barrier(MPI_COMM_WORLD);
    /*
      for (i=0; i<nlocal; i++)
      printf("%d %d ", myrank, elmnts[i]);
      printf("\n"); */

    // printf("time = %f\n", etime - stime);

    if (myrank == 0)
    {
        printf("Sorting time: %e sec\n", etime - stime);
    }

    free(elmnts);
    free(vsorted);

    MPI_Finalize();

    return 0;
}