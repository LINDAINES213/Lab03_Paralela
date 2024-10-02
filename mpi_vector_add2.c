/* File:     mpi_vector_add2.c
 *
 *
 * Compile:  mpicc -g -Wall -o mpi_vector_add2 mpi_vector_add2.c
 * Run:      mpiexec ./mpi_vector_add2 <number_of_elements>
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>

void Check_for_error(int local_ok, char fname[], char message[],
      MPI_Comm comm);
void Allocate_vectors(double** local_x_pp, double** local_y_pp,
      double** local_z_pp, int local_n, MPI_Comm comm);
void Initialize_vector(double local_a[], int local_n, int n, int my_rank, int vector_id);
void Print_vector(double local_b[], int local_n, int n, char title[],
      int my_rank, MPI_Comm comm);
void Parallel_vector_sum(double local_x[], double local_y[],
      double local_z[], int local_n);
void Read_n(int* n_p, int* local_n_p, int my_rank, int comm_sz, MPI_Comm comm, int argc, char *argv[]);

/*-------------------------------------------------------------------*/
int main(int argc, char *argv[]) {
   int n; 
   int local_n;
   int comm_sz, my_rank;
   double *local_x, *local_y, *local_z;
   MPI_Comm comm;
   double tstart, tend;

   MPI_Init(&argc, &argv);
   comm = MPI_COMM_WORLD;
   MPI_Comm_size(comm, &comm_sz);
   MPI_Comm_rank(comm, &my_rank);

   // Leer el tamaño del vector desde los argumentos de línea de comandos
   Read_n(&n, &local_n, my_rank, comm_sz, comm, argc, argv);

   tstart = MPI_Wtime();
   Allocate_vectors(&local_x, &local_y, &local_z, local_n, comm);

   // Inicializa los vectores con valores aleatorios diferentes
   Initialize_vector(local_x, local_n, n, my_rank, 0);
   Initialize_vector(local_y, local_n, n, my_rank, 1);

   Parallel_vector_sum(local_x, local_y, local_z, local_n);
   tend = MPI_Wtime();

   // Imprimir primeros y últimos 10 elementos
   Print_vector(local_x, local_n, n, "\nVector x", my_rank, comm);
   Print_vector(local_y, local_n, n, "\nVector y", my_rank, comm);
   Print_vector(local_z, local_n, n, "\nThe sum is", my_rank, comm);

   double cpu_time_used = ((double) (tend - tstart)) * 1000;

   if(my_rank == 0)
       printf("\nTook %f ms to run\n", cpu_time_used);

   free(local_x);
   free(local_y);
   free(local_z);

   MPI_Finalize();

   return 0;
}  /* main */


/*-------------------------------------------------------------------
 * Function:  Check_for_error
 * Purpose:   Check whether any process has found an error.  If so,
 *            print message and terminate all processes.  Otherwise,
 *            continue execution.
 * In args:   local_ok:  1 if calling process has found an error, 0
 *               otherwise
 *            fname:     name of function calling Check_for_error
 *            message:   message to print if there's an error
 *            comm:      communicator containing processes calling
 *                       Check_for_error:  should be MPI_COMM_WORLD.
 *
 * Note:
 *    The communicator containing the processes calling Check_for_error
 *    should be MPI_COMM_WORLD.
 */
void Check_for_error(
      int       local_ok   /* in */,
      char      fname[]    /* in */,
      char      message[]  /* in */,
      MPI_Comm  comm       /* in */) {
   int ok;

   MPI_Allreduce(&local_ok, &ok, 1, MPI_INT, MPI_MIN, comm);
   if (ok == 0) {
      int my_rank;
      MPI_Comm_rank(comm, &my_rank);
      if (my_rank == 0) {
         fprintf(stderr, "Proc %d > In %s, %s\n", my_rank, fname,
               message);
         fflush(stderr);
      }
      MPI_Finalize();
      exit(-1);
   }
}  /* Check_for_error */


/*-------------------------------------------------------------------
 * Function:  Read_n
 * Purpose:   Get the order of the vectors from command line arguments
 *            on proc 0 and broadcast to other processes.
 * In args:   my_rank:    process rank in communicator
 *            comm_sz:    number of processes in communicator
 *            comm:       communicator containing all the processes
 *                        calling Read_n
 * Out args:  n_p:        global value of n
 *            local_n_p:  local value of n = n/comm_sz
 *
 * Errors:    n should be positive and evenly divisible by comm_sz
 */
void Read_n(
      int*      n_p        /* out */,
      int*      local_n_p  /* out */,
      int       my_rank    /* in  */,
      int       comm_sz    /* in  */,
      MPI_Comm  comm       /* in  */,
      int       argc,
      char*     argv[]) {
   int local_ok = 1;
   char *fname = "Read_n";

   if (my_rank == 0) {
      if (argc < 2) {
         fprintf(stderr, "Usage: %s <number_of_elements>\n", argv[0]);
         MPI_Abort(comm, 1);
      }
      *n_p = atoi(argv[1]);
      printf("Proc 0 read n = %d\n", *n_p);
   }

   // Comunicar el tamaño a todos los procesos
   MPI_Bcast(n_p, 1, MPI_INT, 0, comm);

   if (*n_p <= 0 || *n_p % comm_sz != 0) local_ok = 0;
   Check_for_error(local_ok, fname,
         "n should be > 0 and evenly divisible by comm_sz", comm);
   *local_n_p = *n_p / comm_sz;
}  /* Read_n */


/*-------------------------------------------------------------------
 * Function:  Allocate_vectors
 * Purpose:   Allocate storage for x, y, and z
 * In args:   local_n:  the size of the local vectors
 *            comm:     the communicator containing the calling processes
 * Out args:  local_x_pp, local_y_pp, local_z_pp:  pointers to memory
 *               blocks to be allocated for local vectors
 *
 * Errors:    One or more of the calls to malloc fails
 */
void Allocate_vectors(
      double**   local_x_pp  /* out */,
      double**   local_y_pp  /* out */,
      double**   local_z_pp  /* out */,
      int        local_n     /* in  */,
      MPI_Comm   comm        /* in  */) {
   int local_ok = 1;
   char* fname = "Allocate_vectors";

   *local_x_pp = malloc(local_n*sizeof(double));
   *local_y_pp = malloc(local_n*sizeof(double));
   *local_z_pp = malloc(local_n*sizeof(double));

   if (*local_x_pp == NULL || *local_y_pp == NULL ||
       *local_z_pp == NULL) local_ok = 0;
   Check_for_error(local_ok, fname, "Can't allocate local vector(s)",
         comm);
}  /* Allocate_vectors */


/*-------------------------------------------------------------------
 * Function:   Initialize_vector
 * Purpose:    Inicializa un vector con valores aleatorios
 */
void Initialize_vector(
      double local_a[]   /* out */,
      int    local_n     /* in  */,
      int    n           /* in  */,
      int    my_rank     /* in  */,
      int    vector_id   /* in  */) {

   srand(time(NULL) + my_rank + vector_id);  // Seed único por proceso
   for (int i = 0; i < local_n; i++) {
      local_a[i] = rand() % 100; // Genera valores aleatorios entre 0 y 99
   }
}


/*-------------------------------------------------------------------
 * Function:  Print_vector
 * Purpose:   Print a vector that has a block distribution to stdout
 * In args:   local_b:  local storage for vector to be printed
 *            local_n:  order of local vectors
 *            n:        order of global vector (local_n*comm_sz)
 *            title:    title to precede print out
 *            comm:     communicator containing processes calling
 *                      Print_vector
 *
 * Error:     if process 0 can't allocate temporary storage for
 *            the full vector, the program terminates.
 *
 * Note:
 *    Assumes order of vector is evenly divisible by the number of
 *    processes
 */
void Print_vector(
      double    local_b[]  /* in */,
      int       local_n    /* in */,
      int       n          /* in */,
      char      title[]    /* in */,
      int       my_rank    /* in */,
      MPI_Comm  comm       /* in */) {

   double* b = NULL;
   int i;
   int local_ok = 1;
   char* fname = "Print_vector";

   if (my_rank == 0) {
      b = malloc(n*sizeof(double));
      MPI_Gather(local_b, local_n, MPI_DOUBLE, b, local_n, MPI_DOUBLE, 0, comm);
      printf("%s:\n", title);

      // Imprimir primeros 10 elementos
      printf("First 10 elements: ");
      for (i = 0; i < 10 && i < n; i++)
         printf("%f ", b[i]);
      printf("\n");

      // Imprimir últimos 10 elementos
      printf("Last 10 elements: ");
      for (i = n-10; i < n; i++)
         printf("%f ", b[i]);
      printf("\n");

      free(b);
   } else {
      MPI_Gather(local_b, local_n, MPI_DOUBLE, b, local_n, MPI_DOUBLE, 0, comm);
   }
}  /* Print_vector */


/*-------------------------------------------------------------------
 * Function:  Parallel_vector_sum
 * Purpose:   Add a vector that's been distributed among the processes
 * In args:   local_x:  local storage of one of the vectors being added
 *            local_y:  local storage for the second vector being added
 *            local_n:  the number of components in local_x, local_y,
 *                      and local_z
 * Out arg:   local_z:  local storage for the sum of the two vectors
 */
void Parallel_vector_sum(
      double  local_x[]  /* in  */,
      double  local_y[]  /* in  */,
      double  local_z[]  /* out */,
      int     local_n    /* in  */) {
   int local_i;

   for (local_i = 0; local_i < local_n; local_i++)
      local_z[local_i] = local_x[local_i] + local_y[local_i];
}  /* Parallel_vector_sum */
