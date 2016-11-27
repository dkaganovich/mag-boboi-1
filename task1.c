#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() 
{
	int n; pid_t pid; char str[100];

	pid = fork();
	switch (pid) {
		case -1:
			perror("fork failure");
			return 1;
		case 0:
			sprintf(str, "Hello from child");
			n = 5;
			break;
		default:
			sprintf(str, "Hello from parent");
			n = 10;
			break;
	}

	for (;n--;) {
		printf("%s\n", str);
	}

	if (pid) {
		wait(NULL);
	}
	
	return 0;
}