#include <fcntl.h>   // For open()
#include <stdlib.h>  // For exit()
#include <unistd.h>  // For read(), write(), close()

#include <iostream>

int main(int argc, char *argv[]) {
    if (argc == 1) {
        // No files specified, exit with status code 0
        return 0;
    }

    // Iterate over each file provided in the command line arguments
    for (int i = 1; i < argc; ++i) {
        int fd = open(argv[i], O_RDONLY);
        if (fd < 0) {
            // Error opening file
            std::cout << "wcat: cannot open file" << std::endl;
            exit(1);
        }

        // Read from the file and write to stdout
        const int BUFFER_SIZE = 4096;
        char buffer[BUFFER_SIZE];
        ssize_t bytesRead;

        while ((bytesRead = read(fd, buffer, BUFFER_SIZE)) > 0) {
            ssize_t bytesWritten = 0;
            while (bytesWritten < bytesRead) {
                ssize_t result = write(STDOUT_FILENO, buffer + bytesWritten, bytesRead - bytesWritten);
                if (result < 0) {
                    // Error writing to stdout
                    close(fd);
                    exit(1);
                }
                bytesWritten += result;
            }
        }

        if (bytesRead < 0) {
            // Error reading from file
            close(fd);
            exit(1);
        }

        close(fd);
    }

    return 0;
}
