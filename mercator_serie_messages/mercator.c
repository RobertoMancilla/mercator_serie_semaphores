#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <mqueue.h>

#define NPROCS 4
#define SERIES_MEMBER_COUNT 200000

double x = 1.0;

// create a message queue
mqd_t mq;

// calculate Mercator series term (n and x)
double get_member(int n, double x)
{
    int i;
    double numerator = 1;

    // multiply "x" by itself "n" times (x^n)
    for (i = 0; i < n; i++)
        numerator = numerator * x;

    // if "n" is odd x^n/n, if not -x^n/n
    if (n % 2 == 0)
        return (-numerator / n);
    else
        return numerator / n;
}

void proc(int proc_num)
{
    int i;
    double sum = 0;

    printf("Proceso slave %d comenzando suma parcial.\n", proc_num);

    // calculate the terms of the series for each process
    for (i = proc_num; i < SERIES_MEMBER_COUNT; i += NPROCS)
        sum += get_member(i + 1, x);

    printf("Proceso slave %d termino suma parcial. Enviando suma parcial al proceso maestro.\n", proc_num);

    // send the sum to the master process
    //  queue, mensaje(direccion de memoria de sum), tamaño del mensaje, nada
    mq_send(mq, (char *)&sum, sizeof(sum), 0);

    printf("--Proceso slave %d terminó.\n", proc_num);

    // terminate process
    exit(0);
}

void master_proc()
{
    int i;
    double res = 0; //declaramos nuestra variable para el resultado
    double sum; //sum de las sumas parciales que se reciben del mq.

    printf("..::Master proc esperando sumas parciales de los procesos esclavos::..\n");

    for (i = 0; i < NPROCS; i++) // vamos recibiendo por cada proceso
    {
        // tambien se hace la conversion explicita de cada suma parcial que seva recibiendo
        mq_receive(mq, (char *)&sum, sizeof(sum), NULL);
        // sumamos cada suma parcial en el resultado
        res += sum;

        printf("\tMaster proc recibió suma parcial del proceso esclavo %d.\n", i);
    }

    printf("\n[success] Proceso maestro calculó el resultado final.\n\n");

    printf("==================================================================\n");

    printf("El recuento de ln(1 + x) miembros de la serie de Mercator es %d\n", SERIES_MEMBER_COUNT);
    printf("El valor del argumento x es %f\n", (double)x);   // 0.6931446805662332
    printf("El resultado es %10.8f\n", res);

    // terminate the master process
    exit(0);
}

int main()
{
    long long start_ts;
    long long stop_ts;
    long long elapsed_time;

    struct timeval ts;

    int i;
    int p; // para la creacion de procesos

    struct mq_attr attr;

    attr.mq_flags = 0; // no hay banderas adicionales
    attr.mq_maxmsg = NPROCS; // numero maximo de mensajes en la mq 
    attr.mq_msgsize = sizeof(double); //tamaño maximo de cada mensaje en la mq
    attr.mq_curmsgs = 0; //Se establece el número actual de mensajes en lamq. 
        // se configura mq_curmsgs inicialmente en 0 para indicar que no hay mensajes en la cola en el momento de su creación.

    // tcosas del timer
    gettimeofday(&ts, NULL);
    start_ts = ts.tv_sec; // Start time

    // abrir la queue de mensajes
    mq = mq_open("/queue", O_CREAT | O_RDWR, 0666, &attr);

    //error al abrir la mq
    if (mq == (mqd_t)-1)
    {
        perror("mq_open");
        exit(1);
    }

    // create processes
    for (i = 0; i < NPROCS; i++)
    {
        p = fork();

        if (p == 0)
            proc(i);
    }

    // master process
    p = fork();
    if (p == 0)
        master_proc();

    // wait for child processes to finish
    for (int i = 0; i < NPROCS + 1; i++)
        wait(NULL);

    printf("Llamando a la función ln(1 + %f) = %10.8f\n", x, log(1 + x));

    gettimeofday(&ts, NULL);
    stop_ts = ts.tv_sec;
    elapsed_time = stop_ts - start_ts;

    printf("Tiempo = %lld segundos\n", elapsed_time);

    mq_close(mq);
    mq_unlink("/queue");

    return 0;
}

/*
-Si no hay espera ocupada, los procesos esclavos comienzan y terminan sus cálculos en
un orden aleatorio, y el proceso maestro recibe las sumas parciales a medida que c
ada proceso esclavo termina.

-Si hay espera ocupada, pel proceso maestro está esperando a que todos los procesos 
esclavos terminen antes de recibir las sumas parciales.
*/
