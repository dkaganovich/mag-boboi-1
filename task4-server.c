#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>
#include <semaphore.h>

static volatile int stopping = 0;

void sig_handler(int sig) {
	if (sig == SIGINT) {
		stopping = 1;
	}
}

int main() 
{
	if (signal(SIGINT, sig_handler) == SIG_ERR) {
		perror("signal failure");
		return 1;
	}

	const size_t MSG_LEN = 80;

	sem_t *sem_accept = sem_open("/sem_accept", O_CREAT, S_IRUSR | S_IWUSR, 1);
	if (sem_accept == SEM_FAILED) {
		perror("sem_open failure");
		return 1;
	}

    sem_t *sem_ready = sem_open("/sem_ready", O_CREAT, S_IRUSR | S_IWUSR, 0);
    if (sem_ready == SEM_FAILED) {
        perror("sem_open failure");
        return 1;
    }

	int fd = shm_open("/share", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        perror("shm_open failure");
        return 1;
    }
    if (ftruncate(fd, (MSG_LEN + 1) * sizeof(char)) == -1) {
        perror("ftruncate failure");
        return 1;
    }

    char *msg = mmap(NULL, (MSG_LEN + 1) * sizeof(char), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (msg == MAP_FAILED) {
        perror("mmap failure");
        return 1;
    }

    fprintf(stderr, "%s\n", "Server is up");
    
    int sem_val;
    while (!stopping) {
    	if (sem_getvalue(sem_accept, &sem_val) == -1) {
			perror("sem_getvalue error");
			return 1;
		}
		if (sem_val == 0) {
            if (sem_wait(sem_ready) == -1) {
                perror("sem_wait error");
                return 1;
            }
			printf("> %s\n", msg);
			if (sem_post(sem_accept) == -1) {
				perror("sem_post failure");
				return 1;
			}
		}
		sleep(3);
    }

    fprintf(stderr, "%s\n", "Server shut down...");

	if (munmap(msg, (MSG_LEN + 1) * sizeof(char)) == -1) {
        perror("munmap failure");
        return 1;
    }

    if (shm_unlink("/share") == -1) {
        perror("shm_unlink failure");
        return 1;
    }

    if (sem_close(sem_accept) == -1) {
        perror("sem_close failure");
        return 1;
    }
    if (sem_unlink("/sem_accept") == -1) {
        perror("sem_unlink failure");
        return 1;
    }

    if (sem_close(sem_ready) == -1) {
        perror("sem_close failure");
        return 1;
    }
    if (sem_unlink("/sem_ready") == -1) {
        perror("sem_unlink failure");
        return 1;
    }

    return 0;
}