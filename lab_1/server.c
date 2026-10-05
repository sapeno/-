
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <float.h>
#include <errno.h>
#include <string.h>

void myFun(const char *str, char * buf) {
    float num = 0.0f;
    char *end;
    const char *p = str;

    while(*p != '\0') {
        while (isspace((unsigned char)*p)) {
            p++;
        }
        if (*p == '\0') {
            break;
        }

        errno = 0;
        float tmp = strtof(p, &end);
        
        if (end == p) {
            const char msg[] = "error: failed to read number\n";
            write(STDERR_FILENO, msg, sizeof(msg));
            exit(EXIT_FAILURE); 
        }

        if (errno == ERANGE) {
            if (tmp >= FLT_MAX || tmp <= -FLT_MAX){
                const char msg[] = "error: overflow\n";
                write(STDERR_FILENO, msg, sizeof(msg));
            } else{
                const char msg[] = "error: number is too small\n";
                write(STDERR_FILENO, msg, sizeof(msg));
            }
            exit(EXIT_FAILURE);
            
        }
        num += tmp;
        p = end;
    }

    int i = 0;
    if (num < 0) {
            buf[i++] = '-';
            num = -num;
        }

        
        int whole = (int)num;
        int por = 1;
        while (whole / por >= 10) {
            por *= 10;
        }

        while (por > 0)
        {
            int dig = whole / por;
            buf[i++] = '0' + dig;
            whole %= por;
            por /=10;
        }
        

        buf[i++] = '.';

        float fract = num - (int)num;
        for (int j = 0; j < 6; j++) {
            fract *= 10;
            int dig = (int)fract;
            buf[i++] = '0' + dig;
            fract -= dig;
        } 
    
        buf[i] = '\0';
}

int main(int argc, char *argv[]) {
    char buf[4096];
    ssize_t bytes;

    pid_t pid = getpid();

    int32_t file = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0660);

    if (file == -1) {
        const char msg[] = "error: failed to open requested file\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
    }

    while ((bytes = read(STDIN_FILENO, buf, sizeof(buf) - 1)) > 0) {
        buf[bytes] = '\0';
		if (bytes < 0) {
			const char msg[] = "error: failed to read from stdin\n";
			write(STDERR_FILENO, msg, sizeof(msg));
			exit(EXIT_FAILURE);
		}

        // while ((n = getline))
        char newbuf [33];
        myFun(buf, newbuf);

        size_t len = strlen(newbuf);
        {

            int32_t written = write(file, newbuf, len);
            if (written != (ssize_t)len) {
                const char msg[] = "error: failed to write to file\n";
                write(STDERR_FILENO, msg, sizeof(msg));
                exit(EXIT_FAILURE);
            }

            {
                const char msg[] = "Server received: ";
                write(STDERR_FILENO, msg, sizeof(msg) - 1);
            }

            written = write(STDOUT_FILENO, newbuf, len);
            if (written != (ssize_t)len) {
                const char msg[] = "error: failed to echo\n";
                write(STDERR_FILENO, msg, sizeof(msg));
                exit(EXIT_FAILURE);
            }
        }

    }

    if (bytes == 0) {
        const char term = '\0';
		write(file, &term, sizeof(term));
    }

    close(file);

}
