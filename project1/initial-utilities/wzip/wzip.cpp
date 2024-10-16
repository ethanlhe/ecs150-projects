#include <fcntl.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <iostream>

#define BUFFER_SIZE 4096

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "wzip: file1 [file2 ...]" << std::endl;
        exit(1);
    }

    char currentChar = '\0';
    int count = 0;
    bool firstChar = true;

    for (int i = 1; i < argc; ++i) {
        int fd = open(argv[i], O_RDONLY);
        if (fd < 0) {
            std::cout << "wzip: cannot open file" << std::endl;
            exit(1);
        }

        char buffer[BUFFER_SIZE];
        ssize_t bytesRead;

        while ((bytesRead = read(fd, buffer, BUFFER_SIZE)) > 0) {
            for (ssize_t j = 0; j < bytesRead; ++j) {
                char ch = buffer[j];
                if (firstChar) {
                    currentChar = ch;
                    count = 1;
                    firstChar = false;
                } else if (ch == currentChar) {
                    count++;
                } else {
                    ssize_t bytesWritten = write(STDOUT_FILENO, &count, sizeof(int));
                    if (bytesWritten != sizeof(int)) {
                        std::cout << "wzip: error writing to output" << std::endl;
                        close(fd);
                        exit(1);
                    }
                    bytesWritten = write(STDOUT_FILENO, &currentChar, sizeof(char));
                    if (bytesWritten != sizeof(char)) {
                        std::cout << "wzip: error writing to output" << std::endl;
                        close(fd);
                        exit(1);
                    }
                    currentChar = ch;
                    count = 1;
                }
            }
        }

        if (bytesRead < 0) {
            std::cout << "wzip: error reading file" << std::endl;
            close(fd);
            exit(1);
        }

        close(fd);
    }

    if (!firstChar && count > 0) {
        ssize_t bytesWritten = write(STDOUT_FILENO, &count, sizeof(int));
        if (bytesWritten != sizeof(int)) {
            std::cout << "wzip: error writing to output" << std::endl;
            exit(1);
        }
        bytesWritten = write(STDOUT_FILENO, &currentChar, sizeof(char));
        if (bytesWritten != sizeof(char)) {
            std::cout << "wzip: error writing to output" << std::endl;
            exit(1);
        }
    }

    return 0;
}
