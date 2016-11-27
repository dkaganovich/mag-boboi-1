#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>
#include <semaphore.h>


int main() 
{
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

    char *msg = mmap(NULL, (MSG_LEN + 1) * sizeof(char), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (msg == MAP_FAILED) {
        perror("mmap failure");
        return 1;
    }

    fprintf(stderr, "%s\n", "Client is running");
    
    char* buf = (char*)malloc((MSG_LEN + 1) * sizeof(char));
    while (fgets(buf, MSG_LEN + 1, stdin) != NULL) {// buffered?!
    	buf[strcspn(buf, "\n")] = 0;
    	if (strlen(buf) > 0) {
    		if (sem_wait(sem) == -1) {
				perror("sem_wait error");
				return 1;
			}
			printf("> %s\n", buf);
			strcpy(msg, buf);
			fsync(fd);
    	}
    }

	if (munmap(msg, (MSG_LEN + 1) * sizeof(char)) == -1) {
        perror("munmap failure");
        return 1;
    }

    if (sem_close(sem) == -1) {
		perror("sem_close failure");
		return 1;
	}

    return 0;
}