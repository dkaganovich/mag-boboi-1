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

	sem_t *sem = sem_open("/sem", O_CREAT, S_IRUSR | S_IWUSR, 1);
	if (sem == SEM_FAILED) {
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
    
    int sval;
    while (!stopping) {
    	if (sem_getvalue(sem, &sval) == -1) {
			perror("sem_getvalue error");
			return 1;
		}
		if (sval <= 0 && strlen(msg) > 0) {
			printf("> %s\n", msg);
			msg[0] = 0;
			fsync(fd);
			if (sem_post(sem) == -1) {
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

    if (sem_close(sem) == -1) {
		perror("sem_close failure");
		return 1;
	}
    if (sem_unlink("/sem") == -1) {
    	perror("sem_unlink failure");
    	return 1;
    }

    return 0;
}