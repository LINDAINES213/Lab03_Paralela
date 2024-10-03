#!/bin/bash

# Número de mediciones
N=10

# Programas a medir
SEC_PROG="./vector_add2"  # Programa secuencial
PAR_PROG="./mpi_vector_add2"  # Programa paralelo (supongamos MPI)

# Cantidad de elementos (deberías ajustar este valor para obtener tiempos de ~5 segundos)
ELEMENTS=100000000

# Archivos temporales para almacenar tiempos
SEC_TIMES_FILE="sec_times.txt"
PAR_TIMES_FILE="par_times.txt"

# Inicializar los archivos de tiempo
> $SEC_TIMES_FILE
> $PAR_TIMES_FILE

# Medir el tiempo del programa secuencial
echo "Midiendo tiempos para el programa secuencial..."
for i in $(seq 1 $N); do
    echo "Ejecución secuencial $i..."
    TIME=$( $SEC_PROG $ELEMENTS | grep 'Took' | awk '{print $2}' )
    echo $TIME >> $SEC_TIMES_FILE
done

# Medir el tiempo del programa paralelo
echo "Midiendo tiempos para el programa paralelo..."
for i in $(seq 1 $N); do
    echo "Ejecución paralela $i..."
    TIME=$( mpirun -np 4 $PAR_PROG $ELEMENTS | grep 'Took' | awk '{print $2}' )
    echo $TIME >> $PAR_TIMES_FILE
done

# Calcular el promedio del tiempo secuencial
SEC_AVG=$(awk '{ sum += $1; n++ } END { if (n > 0) print sum / n; }' $SEC_TIMES_FILE)
echo "Promedio de tiempo secuencial: $SEC_AVG ms"

# Calcular el promedio del tiempo paralelo
PAR_AVG=$(awk '{ sum += $1; n++ } END { if (n > 0) print sum / n; }' $PAR_TIMES_FILE)
echo "Promedio de tiempo paralelo: $PAR_AVG ms"

# Calcular el speedup
if [ "$PAR_AVG" != "0" ]; then
    SPEEDUP=$(echo "$SEC_AVG / $PAR_AVG" | bc -l)
    echo "Speedup logrado: $SPEEDUP"
else
    echo "Error: El tiempo paralelo es cero, no se puede calcular el speedup"
fi

# Limpiar archivos temporales
rm $SEC_TIMES_FILE $PAR_TIMES_FILE