#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

#define RECEIVE_FILE "rx"
#define TRANSFER_FILE "tx"

#define GIT_HASH_LEN 40 

void print_file(FILE *f) {
	int br;	
    while ((br = fgetc(f)) != EOF) {
        printf("%c", br);
    };
}

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

int read_end(char *buffer, int size, int end) {
	char buff[1000];
    ssize_t br, bw = 0;
	while ((br = read(end, buff, 
				      size - 1)) > 0) {
		memcpy(buffer + bw, buff, br);
		bw += br;
	}
	buffer[bw] = '\0';

	if (br == -1) {
		return 1;	
	}
	return 0;
}

int read_command_stdout(char *buffer, int size,
						char *const *command, int std) {
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
        if (reverse_dup_pfd(fd, std, 
							1, 0) != 0) {
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

int get_first_commit(char *commit) {
	int status = 0;
	int size = GIT_HASH_LEN * 100;
	char buffer[size];

	char *const command[] = {
		"git", "log", 
		"HEAD..main", 
		"--pretty=%H", 
		"--reverse", 
		NULL
	};

	status = read_command_stdout(buffer, size, 
							     command, 
							     STDOUT_FILENO);

	memcpy(commit, buffer, GIT_HASH_LEN);
	commit[GIT_HASH_LEN] = '\0';

	if (commit[0] == '\0') {
		status = 1;
	}

	return status;
}

int git_checkout(char *commit) {
	int status;
	int size = 1000;
	char buffer[size];

	char *const command[] = {
		"git", 
		"checkout", 
		commit, 
		NULL
	};

	status = read_command_stdout(buffer, size, 
								 command, 
								 STDERR_FILENO);
	return status;
}


int go_next_commit() {
	char commit[GIT_HASH_LEN + 1];

	if (get_first_commit(commit) != 0) {
		return 1;
	} 
	if (git_checkout(commit) != 0) {
		return 1;
	} 

	return 0;
}

int go_past_commit(int step) {
	int status = 0;

	char commit[GIT_HASH_LEN + 1];
	snprintf(commit, sizeof(commit), 
		     "HEAD@{%d}", step);
	commit[GIT_HASH_LEN] = '\0';

	status = git_checkout(commit);

	return status;

}

int git_commit(char *msg) {
	int status;
	int size = 100;
	char buffer[size];

	char *const command[] = {
		"git", 
		"commit",
		"-m", 
		 msg,
		NULL
	};

	status = read_command_stdout(buffer, size, 
								 command, 
								 STDERR_FILENO);

	printf("%s", buffer);
	return status;
}

void print_next_commit_file(char *file) {
	if (go_next_commit() == 0) {
		FILE *f = fopen(file, "r");
		print_file(f);
		fclose(f);
	}
}

void stdin_to_file(char *p) {
	FILE *f = fopen(p, "w");
	int b;
	while((b = fgetc(stdin)) != EOF) {
		fputc(b, f);
	}
	fclose(f);
}

int main() {
	// print_next_commit_file(RECEIVE_FILE);

	stdin_to_file(TRANSFER_FILE);
    return 0;
}
