#include <fcntl.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        // No files provided
        std::cout << "wunzip: file1 [file2 ...]" << std::endl;
        exit(1);
    }

    // Process each file
    for (int i = 1; i < argc; ++i) {
        int fd = open(argv[i], O_RDONLY);
        if (fd < 0) {
            // Error opening file
            std::cout << "wunzip: cannot open file" << std::endl;
            exit(1);
        }

        // Decompress the file
        while (true) {
            // Read the 4-byte integer (run length)
            int count;
            ssize_t bytesRead = read(fd, &count, sizeof(int));
            if (bytesRead == 0) {
                // End of file
                break;
            } else if (bytesRead < 0) {
                // Read error
                std::cout << "wunzip: error reading file" << std::endl;
                close(fd);
                exit(1);
            } else if (bytesRead != sizeof(int)) {
                // Incomplete integer read
                std::cout << "wunzip: malformed compressed file" << std::endl;
                close(fd);
                exit(1);
            }

            // Read the character
            char ch;
            bytesRead = read(fd, &ch, sizeof(char));
            if (bytesRead < 0) {
                // Read error
                std::cout << "wunzip: error reading file" << std::endl;
                close(fd);
                exit(1);
            } else if (bytesRead == 0) {
                // Unexpected end of file
                std::cout << "wunzip: malformed compressed file" << std::endl;
                close(fd);
                exit(1);
            }

            // Write the character 'count' times to standard output
            ssize_t toWrite = count;
            const size_t bufferSize = 4096;
            char buffer[bufferSize];
            memset(buffer, ch, bufferSize);

            while (toWrite > 0) {
                ssize_t writeSize = (toWrite > bufferSize) ? bufferSize : toWrite;
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
