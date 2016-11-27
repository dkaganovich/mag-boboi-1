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

// #define NDEBUG


int calc(double a, double b, double step, double *result);


int main(int argc, char** argv) 
{
	assert(argc == 3);

	int p = atoi(argv[1]);
	int n = atoi(argv[2]);

	assert(p > 0);// num of processes available
	assert(n > 0);// interval partition

	const int p_cnt = (n > p ? p : n);

	const double a = 0, b = 1;// interval

	double step = (b - a) / n;
	double chunk = (b - a) / p_cnt;

	fprintf(stderr, "Func.: Sin(x)\nInt.: [%f, %f]\n", a, b);
	fprintf(stderr, "Step: %f\n", step);
	fprintf(stderr, "Chunk: %f\n", chunk);

	// create a binary semaphore
	sem_t *sem = sem_open("/sem", O_CREAT, S_IRUSR | S_IWUSR, 1);
	if (sem == SEM_FAILED) {
		perror("sem_open failure");
		return 1;
	}

	// // allocate shared memory for result var
	// int fd = shm_open("/share", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
 //    if (fd == -1) {
 //        perror("shm_open failure");
 //        return 1;
 //    }
 //    if (ftruncate(fd, sizeof(double)) == -1) {
 //        perror("ftruncate failure");
 //        return 1;
 //    }

 //    // map result var to the process address space (mapping is inhereted by child processes)
 //    double *result = mmap(NULL, sizeof(double), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
 //    if (result == MAP_FAILED) {
 //        perror("mmap failure");
 //        return 1;
 //    }
 //    *result = 0;

    // map result var to the process address space (mapping is inhereted by child processes)
    double *result = mmap(NULL, sizeof(double), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (result == MAP_FAILED) {
        perror("mmap failure");
        return 1;
    }
    *result = 0;

    // run child processes
    for (int i = 0; i < p_cnt; ++i) {
    	pid_t pid = fork();
    	if (pid == -1) {
    		perror("fork failure");
    		return 1;
    	}
    	if (!pid) 
    	{
    		double part;
    		calc(a + i * chunk, a + (i + 1) * chunk > b ? b : a + (i + 1) * chunk, step, &part);

    		// fprintf(stderr, "p%d: [%f %f] -> %f\n", i, a + i * chunk, a + (i + 1) * chunk > b ? b : a + (i + 1) * chunk, part);

    		sem = sem_open("/sem", O_CREAT, S_IRUSR|S_IWUSR, 1);
    		if (sem_wait(sem) == -1) {
    			perror("sem_wait error");
    			return 1;
    		}

    		*result += part;

    		if (sem_post(sem) == -1) {
    			perror("sem_post failure");
    			return 1;
    		}
    		if (sem_close(sem) == -1) {
    			perror("sem_close failure");
    			return 1;
    		}

    		return 0;
    	}
    }

    int retcode = 0;
    int status;
    for (int i = 0; i < p_cnt; ++i) {
        if (waitpid(-1, &status, 0) == -1) {
            perror("waitpid failure");
            retcode = 1;
            break;
        }
        if (WIFEXITED(status)) {// normal exit
            if (WEXITSTATUS(status) != 0) {
                retcode = 1;
            }
        }
    }

    fprintf(stderr, "Result: %f\n", *result);

    if (munmap(result, sizeof(double)) == -1) {
        perror("munmap failure");
        return 1;
    }

    // if (shm_unlink("/share") == -1) {
    //     perror("shm_unlink failure");
    //     return 1;
    // }

    if (sem_close(sem) == -1) {
		perror("sem_close failure");
		return 1;
	}
    if (sem_unlink("/sem") == -1) {
    	perror("sem_unlink failure");
    	return 1;
    }

	return retcode;
}

int calc(double a, double b, double step, double *result) {
	*result = 0;
	while (a < b) {
		*result += step * (sin(a) + sin(a + step)) / 2;
		a += step;
	}
	return 0;
}
