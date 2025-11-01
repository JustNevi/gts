#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

int execute_command(char *const *command) {
	execvp(command[0], command);
	fprintf(stderr, "Execute command failed.");
	return 1;
}

int reverse_dup_pfd(int *fd, int std, 
					int end, int oth_end) {
    close(fd[oth_end]);
	if (dup2(fd[end], std) == -1) {
		fprintf(stderr, "Dup pipe failed.");
		return 1;
	}
    close(fd[end]);

	return 0;
}

int read_end(char *buff, int size, int end) {
    ssize_t br;
	while ((br = read(end, buff, 
				      size - 1)) > 0) {
		buff[br] = '\0'; 
	}
	if (br == -1) {
		return 1;	
	}
	return 0;
}

int read_command_stdout(char *buffer, int size,
						char *const *command) {
    int status;
	int pid;
    int fd[2];

    if (pipe(fd) == -1) {
		fprintf(stderr, "Pipe failed.");
        return 1;
    }

    pid = fork();
    if (pid == -1) {
		fprintf(stderr, "Fork failed.");
        return 1;
    } else if (pid == 0) {
        if (reverse_dup_pfd(fd, STDOUT_FILENO, 
							1, 0) != 0) {
			fprintf(stderr, "Dup failed.");
            exit(EXIT_FAILURE);
        }
		if (execute_command(command) != 0) {
        	exit(127);
		}
	} else { 
        close(fd[1]);
		read_end(buffer, size, fd[0]);
        close(fd[0]);

        if (waitpid(pid, &status, 0) == -1) {
            return 1;
        }

        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        } else {
            return 1;
		}
	}

	return 0;
}


int main() {
	int status = 0;

	int size = 500;
	char buffer[size];
	char *const command[] = {"git", "--version", NULL};

	status = read_command_stdout(buffer, size, command);

	printf("STDOUT: %s", buffer);
	printf("STATUS: %d\n", status);

    return 0;
}
