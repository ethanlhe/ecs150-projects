#include <fcntl.h>
#include <unistd.h>

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "wunzip: file1 [file2 ...]" << std::endl;
        exit(1);
    }

    for (int i = 1; i < argc; ++i) {
        int fd = open(argv[i], O_RDONLY);
        if (fd < 0) {
            std::cout << "wunzip: cannot open file" << std::endl;
            exit(1);
        }

        while (true) {
            uint32_t count;
            ssize_t bytesRead = read(fd, &count, sizeof(count));
            if (bytesRead == 0) {
                break;
            } else if (bytesRead < 0) {
                std::cout << "wunzip: error reading file" << std::endl;
                close(fd);
                exit(1);
            } else if (bytesRead != sizeof(count)) {
                std::cout << "wunzip: malformed compressed file" << std::endl;
                close(fd);
                exit(1);
            }

            char ch;
            bytesRead = read(fd, &ch, sizeof(char));
            if (bytesRead < 0) {
                std::cout << "wunzip: error reading file" << std::endl;
                close(fd);
                exit(1);
            } else if (bytesRead == 0) {
                std::cout << "wunzip: malformed compressed file" << std::endl;
                close(fd);
                exit(1);
            }

            size_t toWrite = count;
            const size_t bufferSize = 4096;
            char buffer[bufferSize];
            memset(buffer, ch, bufferSize);

            while (toWrite > 0) {
                size_t writeSize = (toWrite > bufferSize) ? bufferSize : toWrite;
                ssize_t bytesWritten = write(STDOUT_FILENO, buffer, writeSize);
                if (bytesWritten < 0) {
                    std::cout << "wunzip: error writing to output" << std::endl;
                    close(fd);
                    exit(1);
                }
                toWrite -= bytesWritten;
            }
        }

        close(fd);
    }

    return 0;
}
