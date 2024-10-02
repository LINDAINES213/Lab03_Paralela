/* File:     vector_add.c
 *
 * Purpose:  Implement vector addition
 *
 * Compile:  gcc -g -Wall -o vector_add vector_add2.c
 * Run:      ./vector_add2
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void Read_n(int* n_p);
void Allocate_vectors(double** x_pp, double** y_pp, double** z_pp, int n);
void Generate_random_vector(double a[], int n);
void Print_vector(double b[], int n, char title[]);
void Vector_sum(double x[], double y[], double z[], int n);

/*---------------------------------------------------------------------*/
int main(void) {
   int n = 100000;
   double *x, *y, *z;

   clock_t start, end;
   double cpu_time_used;

   srand(time(NULL));

   start = clock();

   Allocate_vectors(&x, &y, &z, n);

   Generate_random_vector(x, n);
   Generate_random_vector(y, n);
   
   /*Read_vector(x, n, "x");
   Read_vector(y, n, "y");*/
   
   Vector_sum(x, y, z, n);

   end = clock();

   cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC * 1000;

   Print_vector(x, n, "\nVector x:");
   Print_vector(y, n, "\nVector y:");
   Print_vector(z, n, "\nThe sum is:");

   // Imprimir el tiempo de ejecución
   printf("\nTook %f ms to run\n", cpu_time_used);

   free(x);
   free(y);
   free(z);

   return 0;
}  /* main */

/*---------------------------------------------------------------------
 * Function:  Allocate_vectors
 * Purpose:   Allocate storage for the vectors
 * In arg:    n:  the order of the vectors
 * Out args:  x_pp, y_pp, z_pp:  pointers to storage for the vectors
 *
 * Errors:    If one of the mallocs fails, the program terminates
 */
void Allocate_vectors(
      double**  x_pp  /* out */, 
      double**  y_pp  /* out */, 
      double**  z_pp  /* out */, 
      int       n     /* in  */) {
   *x_pp = malloc(n*sizeof(double));
   *y_pp = malloc(n*sizeof(double));
   *z_pp = malloc(n*sizeof(double));
   if (*x_pp == NULL || *y_pp == NULL || *z_pp == NULL) {
      fprintf(stderr, "Can't allocate vectors\n");
      exit(-1);
   }
}  /* Allocate_vectors */

/*---------------------------------------------------------------------
 * Function:  Generate_random_vector
 * Purpose:   Generar un vector con valores aleatorios
 * In args:   n:  tamaño del vector
 * Out arg:   a:  el vector a ser llenado con números aleatorios
 */
void Generate_random_vector(
      double  a[]   /* out */, 
      int     n     /* in  */) {
   for (int i = 0; i < n; i++)
      a[i] = ((double) rand() / RAND_MAX) * 100.0; // Valores aleatorios entre 0 y 100
}  /* Generate_random_vector */

/*---------------------------------------------------------------------
 * Function:  Print_vector
 * Purpose:   Print the contents of a vector
 * In args:   b:  the vector to be printed
 *            n:  the order of the vector
 *            title:  title for print out
 */
void Print_vector(
      double  b[]     /* in */, 
      int     n       /* in */, 
      char    title[] /* in */) {
   int i;
   printf("%s\n", title);
   printf("First 10 elements:\n");
   for (i = 0; i < 10; i++)
      printf("%f ", b[i]);
   printf("\nLast 10 elements:\n");
   for (i = n-10; i < n; i++)
      printf("%f ", b[i]);
   printf("\n");
}  /* Print_vector */

/*---------------------------------------------------------------------
 * Function:  Vector_sum
 * Purpose:   Add two vectors
 * In args:   x:  the first vector to be added
 *            y:  the second vector to be added
 *            n:  the order of the vectors
 * Out arg:   z:  the sum vector
 */
void Vector_sum(
      double  x[]  /* in  */, 
      double  y[]  /* in  */, 
      double  z[]  /* out */, 
      int     n    /* in  */) {
   int i;

   for (i = 0; i < n; i++)
      z[i] = x[i] + y[i];
}  /* Vector_sum */
