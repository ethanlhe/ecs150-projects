#include <fcntl.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#define BUFFER_SIZE 1024

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "wgrep: searchterm [file ...]" << std::endl;
        exit(1);
    }

    std::string searchTerm = argv[1];
    std::vector<int> fds;

    // If no files are provided, read from standard input
    if (argc == 2) {
        fds.push_back(STDIN_FILENO);
    } else {
        // Open each file and add its file descriptor to the vector
        for (int i = 2; i < argc; ++i) {
            int fd = open(argv[i], O_RDONLY);
            if (fd < 0) {
                std::cout << "wgrep: cannot open file" << std::endl;
                exit(1);
            }
            fds.push_back(fd);
        }
    }

    char buffer[BUFFER_SIZE];
    std::string line;
    ssize_t bytesRead;

    // Process each file descriptor using a traditional loop
    for (size_t idx = 0; idx < fds.size(); ++idx) {
        int fd = fds[idx];
        line.clear();
        while ((bytesRead = read(fd, buffer, BUFFER_SIZE)) > 0) {
            for (ssize_t i = 0; i < bytesRead; ++i) {
                char c = buffer[i];
                line += c;
                if (c == '\n') {
                    // Search for the term in the line
                    if (line.find(searchTerm) != std::string::npos) {
                        ssize_t bytesWritten = 0;
                        ssize_t lineLength = line.length();
                        const char* lineCStr = line.c_str();
                        // Write the line to standard output
                        while (bytesWritten < lineLength) {
                            ssize_t result = write(STDOUT_FILENO, lineCStr + bytesWritten, lineLength - bytesWritten);
                            if (result < 0) {
                                std::cerr << "wgrep: error writing to output" << std::endl;
                                exit(1);
                            }
                            bytesWritten += result;
                        }
                    }
                    line.clear();
                }
            }
        }
        if (bytesRead < 0) {
            std::cerr << "wgrep: error reading file" << std::endl;
            exit(1);
        }
        if (fd != STDIN_FILENO) {
            close(fd);
        }
    }

    return 0;
}
