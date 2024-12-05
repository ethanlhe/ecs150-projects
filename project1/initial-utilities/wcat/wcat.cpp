#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include <iostream>

int main(int argc, char *argv[]) {
    if (argc == 1) {
        return 0;
    }

    for (int i = 1; i < argc; ++i) {
        int fd = open(argv[i], O_RDONLY);
        if (fd < 0) {
            std::cout << "wcat: cannot open file" << std::endl;
            exit(1);
        }

        const int BUFFER_SIZE = 4096;
        char buffer[BUFFER_SIZE];
        ssize_t bytesRead;

        while ((bytesRead = read(fd, buffer, BUFFER_SIZE)) > 0) {
            ssize_t bytesWritten = 0;
            while (bytesWritten < bytesRead) {
                ssize_t result = write(STDOUT_FILENO, buffer + bytesWritten, bytesRead - bytesWritten);
                if (result < 0) {
                    close(fd);
                    exit(1);
                }
                bytesWritten += result;
            }
        }

        if (bytesRead < 0) {
            close(fd);
            exit(1);
        }

        close(fd);
    }

    return 0;
}
