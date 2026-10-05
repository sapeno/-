
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>


static char SERVER_PROGRAM_NAME[] = "sss";

int main(int argc, char *argv[]) {
    if (argc == 1) {
        char msg [1024];
        uint32_t len = snprintf(msg, sizeof(msg) - 1, "usage: %s filename\n", argv[0]);
        write(STDERR_FILENO, msg, len);
        exit(EXIT_SUCCESS);
    }

    char progpath[1024];
    {
        ssize_t len = readlink("/proc/self/exe", progpath, sizeof(progpath) - 1);

        if (len == -1) {
            const char msg[] = "error: failed to read full program path\n";
            write(STDERR_FILENO, msg, sizeof(msg));
            exit(EXIT_FAILURE);
        }

        while (progpath[len] != '/')
            --len;

        progpath[len] = '\0';

    }

    int client_to_server[2];
    if(pipe(client_to_server) == -1) {
        const char msg[] = "error: failed to create pipe\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }

    int server_to_client[2];
    if(pipe(server_to_client) == -1) {
        const char msg[] = "error: failed to create pipe\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }

    const pid_t child = fork(); 

    switch (child) {
        case -1: {
            const char msg[] = "error: failed to spawn new process\n";
            write(STDERR_FILENO, msg, sizeof(msg));
            exit(EXIT_FAILURE);
        } break;

        case 0: {
            {
                pid_t pid = getpid();

                const char msg[] = "Child process started\n";
                write(STDERR_FILENO, msg, sizeof(msg) - 1); 
            }

            close(client_to_server[1]); 
            close(server_to_client[0]);

            dup2(client_to_server[0], STDIN_FILENO);

            close(client_to_server[0]);

            dup2(server_to_client[1], STDOUT_FILENO);
            close(server_to_client[1]);

            {
                char path[1024];
                strcpy(path, progpath);
                strcat(path, "/");
                strcat(path, SERVER_PROGRAM_NAME);
                // snprintf(path, sizeof(path) - 1, "%s/%s", progpath, SERVER_PROGRAM_NAME);

                char *const args[] = {SERVER_PROGRAM_NAME, argv[1], NULL};

                int32_t status = execv(path, args);

                if (status == -1) {
                    const char msg[] = "error: failed to exec into new exectuable image\n";
                    write(STDERR_FILENO, msg, sizeof(msg));
                    exit(EXIT_FAILURE);
                }
            }
        }break;

        default: {
            {
                pid_t pid = getpid();

                const char msg[] = "Parent process: successfully spawned child\n";
                write(STDERR_FILENO, msg, sizeof(msg) - 1);
            }

            close(client_to_server[0]);
            close(server_to_client[1]);

            char buf[4096];
            ssize_t bytes;

            while (bytes = read(STDIN_FILENO, buf, sizeof(buf))) {
			if (bytes < 0) {
				const char msg[] = "error: failed to read from stdin\n";
				write(STDERR_FILENO, msg, sizeof(msg));
				exit(EXIT_FAILURE);
			} else if (buf[0] == '\n') {
				break;
			}

			write(client_to_server[1], buf, bytes);

			bytes = read(server_to_client[0], buf, sizeof(buf));
			write(STDOUT_FILENO, buf, bytes);
		}

		close(client_to_server[1]);
		close(server_to_client[0]);

		wait(NULL);
	} break;
    }
}