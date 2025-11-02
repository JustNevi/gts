#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

#define RECEIVE_FILE "rx"
#define TRANSFER_FILE "tx"

#define GIT_HASH_LEN 40 

void stdin_to_file(char *p) {
	FILE *f = fopen(p, "w");
	int b;
	while ((b = fgetc(stdin)) != EOF) {
		fputc(b, f);
	}
	fclose(f);
}

void file_to_stdout(char *p) {
	FILE *f = fopen(p, "r");
	int b;
	while ((b = fgetc(f)) != EOF) {
		fputc(b, stdout);
	}
	fclose(f);
}

int execute_command(char *const *command) {
	execvp(command[0], command);
	fprintf(stderr, "Execute command failed.");
	return 1;
}

int reverse_dup_pfd(int *fd,
					int end, int oth_end) {
    close(fd[oth_end]);
	if (dup2(fd[end], STDERR_FILENO) == -1) {
		fprintf(stderr, "Dup pipe failed.");
		return 1;
	}
	if (dup2(fd[end], STDOUT_FILENO) == -1) {
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
        if (reverse_dup_pfd(fd, 1, 0) != 0) {
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

int run_command(char *const *command) {
	int status;
	int size = 1000;
	char buffer[size];

	status = read_command_stdout(buffer, size, 
								 command);

	return status;
}

int get_first_commit(char *commit,
					 char *branch) {
	int status = 0;
	int size = GIT_HASH_LEN * 100;
	char buffer[size];

	char *const command[] = {
		"git", "log", 
		branch, 
		"--pretty=%H", 
		"--reverse", 
		NULL
	};

	status = read_command_stdout(buffer, size, 
							     command);

	memcpy(commit, buffer, GIT_HASH_LEN);
	commit[GIT_HASH_LEN] = '\0';

	if (commit[0] == '\0') {
		status = 1;
	}

	return status;
}


int get_first_msg_commit(char *commit, 
						 char *branch,
						 char *msg) {
	int status = 0;
	int size = GIT_HASH_LEN * 100;
	char buffer[size];

	char *const command[] = {
		"git", "log", 
		branch, 
		"--grep", msg,
		"--pretty=%H", 
		"--reverse", 
		NULL
	};

	status = read_command_stdout(buffer, size, 
							     command);

	memcpy(commit, buffer, GIT_HASH_LEN);
	commit[GIT_HASH_LEN] = '\0';

	if (commit[0] == '\0') {
		status = 1;
	}

	return status;
}

int now_is_last_commit() {
	char commit[GIT_HASH_LEN + 1];
	return get_first_commit(commit, 
						    "HEAD..origin/main");
}

int git_checkout(char *commit) {
	char *const command[] = {
		"git", 
		"checkout", 
		commit, 
		NULL
	};
	return run_command(command);
}

int git_add(char *fp) {
	char *const command[] = {
		"git", 
		"add",
		 fp,
		NULL
	};
	return run_command(command);
}

int git_commit(char *msg) {
	char *const command[] = {
		"git", 
		"commit",
		"-m", 
		 msg,
		NULL
	};
	return run_command(command);
}

int git_fetch() {
	char *const command[] = {
		"git", 
		"fetch",
		"origin",
		"main",
		NULL
	};
	return run_command(command);
}

int git_rebase() {
	char *const command[] = {
		"git", 
		"rebase",
		"origin/main",
		NULL
	};
	return run_command(command);
}

int git_push() {
	char *const command[] = {
		"git", 
		"push",
		"origin",
		"main",
		NULL
	};
	return run_command(command);
}

int go_next_msg_commit(char *msg) {
	char commit[GIT_HASH_LEN + 1];

	if (get_first_msg_commit(commit, 
					     "HEAD..main",
					  	 msg) != 0) {
		return 1;
	} 
	if (git_checkout(commit) != 0) {
		return 2;
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

void print_next_commit_file(char *path, char *msg) {
	if (go_next_msg_commit(msg) == 0) {
		file_to_stdout(path);
	}
}

void stdin_hook_commit(char *path) {
	git_checkout("main");
	stdin_to_file(path);
	git_add(path);
	if (git_commit(path) == 0) {
		go_past_commit(2);
	} else {
		go_past_commit(1);
	}
}

int detached_fetch() {
	git_fetch();
	git_checkout("main");
	int steps = 1;
	if (now_is_last_commit() == 0) {
		steps = 2;
	}
	git_rebase();
	go_past_commit(steps);
	return 0;
}

int receive(char *path) {
	detached_fetch();
	print_next_commit_file(path, path);
	return 0;
}

int send(char *path) {
	stdin_hook_commit(path);
	detached_fetch();
	git_push();
	return 0;
}

int arg_is(char *arg, char **funcs) {
	int i = 0;
	char *func;
	while ((func = funcs[i]) != NULL) {
		if (strcmp(arg, func) == 0) {
			return 1;
		}
		i++;
	}
	return 0;
}

int main(int argc, char *argv[]) {
	char *rx_file = RECEIVE_FILE;
	char *tx_file = TRANSFER_FILE;

	char rx = 0;
	char tx = 0;

	for (int i = 0; i < argc; i++) {
		char *arg = argv[i];
		if (arg_is(arg, 
			      (char *[]){"rx", NULL})) {
			rx_file = TRANSFER_FILE;
			tx_file = RECEIVE_FILE; 
		} else if (arg_is(arg, 
				   (char *[]){"recv", NULL})) {
			rx = 1;
		} else if (arg_is(arg, 
				   (char *[]){"send", NULL})) {
			tx = 1;
		}
	}

	if (rx == 1) {
		receive(rx_file);
	} else if (tx == 1) {
		send(tx_file);
	}

    return 0;
}
