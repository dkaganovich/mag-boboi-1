#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>
#include <semaphore.h>
#include <math.h>
#include <pthread.h>

// #define NDEBUG

typedef struct {
    int id;
    double a;
    double b;
    double step;   
} job;

double result = 0;
pthread_mutex_t lock;

void* calc(void *args);


int main(int argc, char** argv) 
{
    assert(argc == 3);

    int p = atoi(argv[1]);
    int n = atoi(argv[2]);

    assert(p > 0);// num of threads available
    assert(n > 0);// interval partition

    const int thread_cnt = (n > p ? p : n);

    const double a = 0, b = 1;// interval

    double step = (b - a) / n;
    double chunk = (b - a) / thread_cnt;

    pthread_t* thread_pool = (pthread_t*)malloc(thread_cnt * sizeof(pthread_t));

    if (pthread_mutex_init(&lock, NULL) != 0) {
        perror("pthread_mutex_init failure");
        return 1;
    }

    fprintf(stderr, "Func.: Sin(x)\nInt.: [%f, %f]\n", a, b);
    fprintf(stderr, "Step: %f\n", step);
    fprintf(stderr, "Chunk: %f\n", chunk);

    // run child processes
    for (int i = 0; i < thread_cnt; ++i) 
    {
        job* j = (job*)malloc(sizeof(job));
        j->id = i;
        j->a = a + i * chunk;
        j->b = (a + (i + 1) * chunk > b ? b : a + (i + 1) * chunk);
        j->step = step;

        if (pthread_create(thread_pool + i, NULL, &calc, (void*)j) != 0) {
            perror("pthread_create failure");
            return 1;
        }
    }

    for (int i = 0; i < thread_cnt; ++i) {
        pthread_join(thread_pool[i], NULL);
    }

    if (pthread_mutex_destroy(&lock) != 0) {
        perror("pthread_mutex_destroy failure");
        return 1;
    }

    free(thread_pool);

    fprintf(stderr, "Result: %f\n", result);

    return 0;
}

void* calc(void* args) 
{
    job* j = (job*) args;
    double a = j->a;
    double b = j->b;
    double step = j->step;
    
    double part = 0;
    while (a < b) {
        part += step * (sin(a) + sin(a + step)) / 2;
        a += step;
    }

    // fprintf(stderr, "thread%d: [%f, %f] -> %f\n", j->id, j->a, j->b, part);

    pthread_mutex_lock(&lock);
    result += part;
    pthread_mutex_unlock(&lock);

    free(args);

    return NULL;
}
