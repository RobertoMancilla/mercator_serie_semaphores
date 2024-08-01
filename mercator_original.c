/*
    Para compilar incluir la librería m (matemáticas)

    Ejemplo:
        gcc -o mercator mercator.c -lm
*/

// librerias necesarias
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/shm.h>

#define NPROCS 4
#define SERIES_MEMBER_COUNT 200000

double *sums;
double x = 1.0;

int *proc_count; // contador de procesos completos (completados)
int *start_all;  // flag para saber si los procesos pueden empezar
double *res;     // resultado final de la serie

// calacular termino mercator (n y x)
double get_member(int n, double x)
{
    int i;
    double numerator = 1; // para calcular el termino de la serie de mercator

    // multiplicar "x" por si mismo "n" veces (x^n)
    for (i = 0; i < n; i++)
        numerator = numerator * x;

    // si "n" es impar x^n/n, si no -x^n/n
    if (n % 2 == 0)
        return (-numerator / n);
    else
        return numerator / n;
}

// funcion de los procesos esclavos
void proc(int proc_num)
{
    int i;

    // esperar a que la bandera sea 1
    while (!(*start_all))
        ;

    // inicializar el resultado parcial del proceso a 0
    sums[proc_num] = 0;

    // calcular los terminos de la serie para cada proceso
    for (i = proc_num; i < SERIES_MEMBER_COUNT; i += NPROCS)
        sums[proc_num] += get_member(i + 1, x);

    // incremenetamos contador de procesos finalizados
    (*proc_count)++;

    // terminar proceso
    exit(0);
}

void master_proc()
{
    int i;

    sleep(1);       // esperar para que peromirir que se inicialicen los procesos esclavos
    *start_all = 1; // pueden comenzar los procesos

    // busy wait until all threads are don with computation of partial sums
    while (*proc_count != NPROCS)
    {
    }

    *res = 0;

    // calcular el resultado final sumando resultados parciales (de cada proceso)
    for (i = 0; i < NPROCS; i++)
        *res += sums[i];

    // terminamos el proceso master
    exit(0);
}

int main()
{
    // int *threadIdPtr;

    long long start_ts;
    long long stop_ts;
    long long elapsed_time;
    long lElapsedTime;

    struct timeval ts;

    int i;
    int p;
    int shmid;

    void *shmstart;

    // obetener el segmento de memoria compartida
    shmid = shmget(0x1234, NPROCS * sizeof(double) + 2 * sizeof(int), 0666 | IPC_CREAT);
    shmstart = shmat(shmid, NULL, 0);

    // asignar punteros a los segmentos de memoria compartida
    sums = shmstart;
    proc_count = shmstart + NPROCS * sizeof(double);
    start_all = shmstart + NPROCS * sizeof(double) + sizeof(int);
    res = shmstart + NPROCS * sizeof(double) + 2 * sizeof(int);

    // inicializar los valores (en memoria compartida)
    *proc_count = 0;
    *start_all = 0;

    // cosas para medir el tiempo
    gettimeofday(&ts, NULL);
    start_ts = ts.tv_sec; // Tiempo inicial

    // crear procesos
    for (i = 0; i < NPROCS; i++)
    {
        p = fork();

        if (p == 0)
            proc(i);
    }

    // proceso maestro
    p = fork();
    if (p == 0)
        master_proc();

    // esperar a que terminen los procesos hijos
    for (int i = 0; i < NPROCS + 1; i++)
        wait(NULL);

    printf("El recuento de ln(1 + x) miembros de la serie de Mercator es %d\n", SERIES_MEMBER_COUNT);
    printf("El valor del argumento x es %f\n", (double)x);

    // Tiempo final
    gettimeofday(&ts, NULL);
    stop_ts = ts.tv_sec;
    elapsed_time = stop_ts - start_ts;

    printf("Tiempo = %lld segundos\n", elapsed_time);
    printf("El resultado es %10.8f\n", *res);
    printf("Llamando a la función ln(1 + %f) = %10.8f\n", x, log(1 + x));

    shmdt(shmstart);
    shmctl(shmid, IPC_RMID, NULL);

    return 0;
}