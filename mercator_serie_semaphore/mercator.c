#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <sys/time.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/wait.h>

#define NPROCS 4
#define SERIES_MEMBER_COUNT 200000

double *sums; //shared memory foro partial sums
double x = 1.0; // valor de x para la serie

int *proc_count; // contador de procesos completos (completados)
int *start_all; // flag para saber si los procesos pueden empezar
double *res;     // resultado final de la serie

sem_t *sem_master;  //semaforo para el proceso maestro
sem_t *sem_slave;   //semaforo para los procesos slaves

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

void proc(int proc_num) {
    int i;
    printf("[] Proceso %d esperando la señal para comenzar...\n", proc_num);
    sem_wait(sem_slave);    //wait for sem_post to start, for each process (llega del main el post  )
    printf("[] Proceso %d recibió la señal para comenzar el trabajo.\n", proc_num);

    sums[proc_num] = 0; // nuestro array de las sumas parciales de 
    for (i = proc_num; i < SERIES_MEMBER_COUNT; i += NPROCS)
        sums[proc_num] += get_member(i + 1, x);

    sem_post(sem_master); //mandar señal al semaforo del proceso maestro 
    printf("[] Proceso %d terminó su trabajo.\n", proc_num);
    exit(0);
}

void master_proc() {
    int i;
    printf("[] Esperando a que los procesos esclavos completen su trabajo...\n");
    for (i = 0; i < NPROCS; i++) {
        sem_wait(sem_master); // wait for completion signal from each slave p
        printf("[] Señal recibida del proceso esclavo %d\n", i);
    }

    *res = 0;
    // calcular el resultado final sumando resultados parciales (de cada proceso)
    for (i = 0; i < NPROCS; i++)
        *res += sums[i];
    
    // mandar señal al semaforo de que cada proceso slave termina
    for (i = 0; i < NPROCS; i++)
        sem_post(sem_slave); // aqui tenemos que mandar la señal post al semaforo slave para llvar nuestro contador de procesos
    printf("[] Todos los procesos esclavos han completado su trabajo.\n");

    exit(0);
}

int main() {
    long long start_ts, stop_ts, elapsed_time;
    struct timeval ts;
    int i;
    int p;
    int shmid;
    void *shmstart;

    // obetener el segmento de memoria compartida
    shmid = shmget(0x1234, NPROCS * sizeof(double) + 2, 0666 | IPC_CREAT);
    shmstart = shmat(shmid, NULL, 0);

    // asignar punteros a los segmentos de memoria compartida
    sums = shmstart;
    // proc_count = shmstart + NPROCS * sizeof(double);
    // start_all = shmstart + NPROCS * sizeof(double) + sizeof(int);
    res = shmstart + NPROCS * sizeof(double) + 2;

    // inicializar los valores (en memoria compartida)
    // *proc_count = 0;
    // *start_all = 0;

    // cosas para medir el tiempo
    gettimeofday(&ts, NULL);
    start_ts = ts.tv_sec;

    //initialize semaphores
    sem_master = sem_open("/master_semaphore", O_CREAT | O_EXCL, 0644, 0);
    sem_slave = sem_open("/slave_semaphore", O_CREAT | O_EXCL, 0644, 0);

    // create slave processes
    for (i = 0; i < NPROCS; i++) {
        p = fork();
        if (p == 0)
            proc(i);
    }

    // create master processes
    p = fork();
    if (p == 0)
        master_proc();

    // Signal slave processes to start
    for (int i = 0; i < NPROCS; i++)
        sem_post(sem_slave);

    // Wait for all child processes to terminate
    for (int i = 0; i < NPROCS + 1; i++)
        wait(NULL);

    // fin del timer
    gettimeofday(&ts, NULL);
    stop_ts = ts.tv_sec;
    elapsed_time = stop_ts - start_ts;

    printf("\n=====================================================================\n");
    printf("El recuento de ln(1 + x) miembros de la serie de Mercator es %d\n", SERIES_MEMBER_COUNT);
    printf("El valor del argumento x es %f\n", (double)x);
    printf("Tiempo = %lld segundos\n", elapsed_time);
    printf("El resultado es %10.8f\n", *res);
    printf("Llamando a la función ln(1 + %f) = %10.8f\n", x, log(1 + x));

    // Cleanup shared memory and semaphores
    shmdt(shmstart);
    shmctl(shmid, IPC_RMID, NULL);

    sem_close(sem_master);
    sem_close(sem_slave);
    sem_unlink("/master_semaphore");
    sem_unlink("/slave_semaphore");

    return 0;
}
