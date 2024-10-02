/* File:     mpi_vector_add.c
 *
 * Purpose:  Implement parallel vector addition using a block
 *           distribution of the vectors.  This version also
 *           illustrates the use of MPI_Scatter and MPI_Gather.
 *
 * Compile:  mpicc -g -Wall -o mpi_vector_add2 mpi_vector_add2.c
 * Run:      mpiexec ./vector_add2
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
void Calculate_dot_product(double local_x[], double local_y[], double *local_dot_product, int local_n);
void Scalar_multiply(double local_a[], double scalar, double local_result[], int local_n);
void Read_n(int* n_p, int* local_n_p, double* scalar_p, int my_rank, int comm_sz, MPI_Comm comm, int argc, char *argv[]);

/*-------------------------------------------------------------------*/
int main(int argc, char *argv[]) {
   int n; 
   int local_n;
   int comm_sz, my_rank;
   double *local_x, *local_y, *local_z;
   MPI_Comm comm;
   double tstart, tend;
   double scalar; // Variable para almacenar el escalar

   MPI_Init(&argc, &argv);
   comm = MPI_COMM_WORLD;
   MPI_Comm_size(comm, &comm_sz);
   MPI_Comm_rank(comm, &my_rank);

   // Leer el tamaño del vector y el escalar desde los argumentos de línea de comandos
   Read_n(&n, &local_n, &scalar, my_rank, comm_sz, comm, argc, argv);

   tstart = MPI_Wtime();
   Allocate_vectors(&local_x, &local_y, &local_z, local_n, comm);

   // Se pasa vector_id como 0 para local_x y 1 para local_y
   Initialize_vector(local_x, local_n, n, my_rank, 0);
   Initialize_vector(local_y, local_n, n, my_rank, 1);

   // Sumar vectores
   Parallel_vector_sum(local_x, local_y, local_z, local_n);

   // Calcular producto punto
   double local_dot_product = 0.0;
   Calculate_dot_product(local_x, local_y, &local_dot_product, local_n);
   double global_dot_product;
   MPI_Reduce(&local_dot_product, &global_dot_product, 1, MPI_DOUBLE, MPI_SUM, 0, comm);

   // Multiplicación de escalar
   double *scaled_x = malloc(local_n * sizeof(double));
   double *scaled_y = malloc(local_n * sizeof(double));
   Scalar_multiply(local_x, scalar, scaled_x, local_n);
   Scalar_multiply(local_y, scalar, scaled_y, local_n);

   tend = MPI_Wtime();

   // Imprimir resultados
   Print_vector(local_x, local_n, n, "\nVector x", my_rank, comm);
   Print_vector(local_y, local_n, n, "\nVector y", my_rank, comm);
   Print_vector(local_z, local_n, n, "\nThe sum is", my_rank, comm);
   Print_vector(scaled_x, local_n, n, "\nScaled Vector x", my_rank, comm);
   Print_vector(scaled_y, local_n, n, "\nScaled Vector y", my_rank, comm);

   if(my_rank == 0)
       printf("\nGlobal dot product = %f\n", global_dot_product);

   double cpu_time_used = ((double) (tend - tstart)) * 1000;
   if(my_rank == 0)
       printf("\nTook %f ms to run\n", cpu_time_used);

   free(local_x);
   free(local_y);
   free(local_z);
   free(scaled_x);
   free(scaled_y);

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
 * Purpose:   Get the order of the vectors and the scalar from command line arguments
 *            on proc 0 and broadcast to other processes.
 * In args:   my_rank:    process rank in communicator
 *            comm_sz:    number of processes in communicator
 *            comm:       communicator containing all the processes
 *                        calling Read_n
 * Out args:  n_p:        global value of n
 *            local_n_p:  local value of n = n/comm_sz
 *            scalar_p:   value of the scalar
 *
 * Errors:    n should be positive and evenly divisible by comm_sz
 */
void Read_n(
      int*      n_p        /* out */,
      int*      local_n_p  /* out */,
      double*   scalar_p    /* out */,
      int       my_rank    /* in  */,
      int       comm_sz    /* in  */,
      MPI_Comm  comm       /* in  */,
      int       argc,
      char*     argv[]) {
   int local_ok = 1;
   char *fname = "Read_n";

   if (my_rank == 0) {
      if (argc < 3) { // Cambiado a 3 para incluir el escalar
         fprintf(stderr, "Usage: %s <number_of_elements> <scalar>\n", argv[0]);
         MPI_Abort(comm, 1);
      }
      *n_p = atoi(argv[1]);
      *scalar_p = atof(argv[2]); // Leer el escalar
      printf("Proc 0 read n = %d and scalar = %f\n", *n_p, *scalar_p);
   }

   // Comunicar el tamaño y el escalar a todos los procesos
   MPI_Bcast(n_p, 1, MPI_INT, 0, comm);
   MPI_Bcast(scalar_p, 1, MPI_DOUBLE, 0, comm);

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
 * Function:  Initialize_vector
 * Purpose:   Initialize a vector with random values
 */
void Initialize_vector(
      double local_a[]   /* out */,
      int    local_n     /* in  */,
      int    n           /* in  */,
      int    my_rank     /* in  */,
      int    vector_id   /* in  */) {
   int i;

   // Se usa vector_id para crear diferentes semillas para diferentes vectores
   srand(time(NULL) + vector_id + my_rank);

   for (i = 0; i < local_n; i++) {
      local_a[i] = (double)(rand() % 100);  // Cambiar el rango según sea necesario
   }
}  /* Initialize_vector */

/*-------------------------------------------------------------------
 * Function:  Print_vector
 * Purpose:   Print the elements of the vector
 * In args:   local_b:   local portion of vector
 *            local_n:   local size of the vector
 *            n:         global size of the vector
 *            title:     title to print before the vector
 *            my_rank:   rank of calling process
 *            comm:      communicator containing all processes calling
 *                        Print_vector
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
 * Purpose:   Compute the sum of two vectors
 * In args:   local_x: local portion of x
 *            local_y: local portion of y
 *            local_z: local portion of z
 *            local_n: size of local vectors
 */
void Parallel_vector_sum(
      double local_x[]   /* in */,
      double local_y[]   /* in */,
      double local_z[]   /* out */,
      int    local_n     /* in */) {
   int i;
   for (i = 0; i < local_n; i++)
      local_z[i] = local_x[i] + local_y[i];
}  /* Parallel_vector_sum */

/*-------------------------------------------------------------------
 * Function:  Calculate_dot_product
 * Purpose:   Compute the dot product of two vectors
 * In args:   local_x: local portion of x
 *            local_y: local portion of y
 *            local_dot_product: pointer to store local dot product
 *            local_n: size of local vectors
 */
void Calculate_dot_product(
      double local_x[]   /* in */,
      double local_y[]   /* in */,
      double* local_dot_product /* out */,
      int    local_n     /* in */) {
   int i;
   *local_dot_product = 0.0;
   for (i = 0; i < local_n; i++)
      *local_dot_product += local_x[i] * local_y[i];
}  /* Calculate_dot_product */

/*-------------------------------------------------------------------
 * Function:  Scalar_multiply
 * Purpose:   Multiply a vector by a scalar
 * In args:   local_a: local vector
 *            scalar: scalar value
 *            local_result: pointer to store result
 *            local_n: size of local vector
 */
void Scalar_multiply(
      double local_a[]   /* in */,
      double scalar      /* in */,
      double local_result[] /* out */,
      int local_n        /* in */) {
   int i;
   for (i = 0; i < local_n; i++)
      local_result[i] = scalar * local_a[i];
}  /* Scalar_multiply */
