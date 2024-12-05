#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

const char error_message[] = "An error has occurred\n";

// Function to preprocess the command line and insert spaces around '>'
std::string preprocess_line(const std::string &line) {
    std::string new_line;
    for (size_t i = 0; i < line.size(); ++i) {
        if (line[i] == '>') {
            // Insert space before '>' if not at the beginning and previous char is not space
            if (i > 0 && line[i - 1] != ' ') {
                new_line += ' ';
            }
            new_line += '>';
            // Insert space after '>' if not at the end and next char is not space
            if (i + 1 < line.size() && line[i + 1] != ' ') {
                new_line += ' ';
            }
        } else {
            new_line += line[i];
        }
    }
    return new_line;
}

int main(int argc, char *argv[]) {
    // Check for correct number of arguments
    if (argc > 2) {
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1);
    }

    // Initialize the path with /bin
    std::vector<std::string> path;
    path.push_back("/bin");

    // Open batch file if provided
    FILE *input_stream;
    if (argc == 2) {
        input_stream = fopen(argv[1], "r");
        if (input_stream == NULL) {
            write(STDERR_FILENO, error_message, strlen(error_message));
            exit(1);
        }
    } else {
        input_stream = stdin;
    }

    while (true) {
        // Display prompt in interactive mode
        if (input_stream == stdin) {
            std::cout << "wish> ";
            std::cout.flush();  // Ensure prompt is displayed immediately
        }

        // Read input line
        char *line = NULL;
        size_t len = 0;
        ssize_t nread = getline(&line, &len, input_stream);
        if (nread == -1) {
            free(line);
            exit(0);
        }

        // Remove trailing newline character
        if (line[nread - 1] == '\n') {
            line[nread - 1] = '\0';
        }

        // Skip empty lines
        if (strlen(line) == 0) {
            free(line);
            continue;
        }

        // Split input line into parallel commands
        std::string line_str(line);
        free(line);
        std::vector<std::string> commands;
        std::stringstream ss(line_str);
        std::string cmd;
        while (std::getline(ss, cmd, '&')) {
            // Trim whitespace
            size_t first = cmd.find_first_not_of(" \t");
            size_t last = cmd.find_last_not_of(" \t");
            if (first == std::string::npos || last == std::string::npos) {
                continue;
            }
            cmd = cmd.substr(first, (last - first + 1));
            commands.push_back(cmd);
        }

        // Execute each command
        std::vector<pid_t> child_pids;
        std::vector<std::string>::iterator cmd_it;
        for (cmd_it = commands.begin(); cmd_it != commands.end(); ++cmd_it) {
            std::string cmd_str = *cmd_it;

            // Preprocess the command line to insert spaces around '>'
            std::string preprocessed_cmd_str = preprocess_line(cmd_str);

            // Tokenize the command
            std::istringstream cmd_ss(preprocessed_cmd_str);
            std::vector<std::string> tokens;
            std::string token;
            while (cmd_ss >> token) {
                tokens.push_back(token);
            }

            // Check for empty command
            if (tokens.empty()) {
                write(STDERR_FILENO, error_message, strlen(error_message));
                continue;
            }

            // Handle redirection
            size_t redirect_pos = std::string::npos;
            size_t i;
            int redirect_count = 0;
            for (i = 0; i < tokens.size(); ++i) {
                if (tokens[i] == ">") {
                    redirect_pos = i;
                    redirect_count++;
                }
            }

            if (redirect_count > 1) {
                // Error: Multiple redirection operators
                write(STDERR_FILENO, error_message, strlen(error_message));
                continue;
            }

            std::string output_file;
            if (redirect_pos != std::string::npos) {
                // Error if no file specified or too many arguments after '>'
                if (redirect_pos + 1 >= tokens.size() || tokens.size() != redirect_pos + 2) {
                    write(STDERR_FILENO, error_message, strlen(error_message));
                    continue;
                }
                output_file = tokens[redirect_pos + 1];
                tokens.resize(redirect_pos);
            }

            if (tokens.empty()) {
                write(STDERR_FILENO, error_message, strlen(error_message));
                continue;
            }

            std::string cmd_name = tokens[0];

            // Built-in commands
            if (cmd_name == "exit") {
                if (tokens.size() != 1) {
                    write(STDERR_FILENO, error_message, strlen(error_message));
                    continue;
                }
                exit(0);
            } else if (cmd_name == "cd") {
                if (tokens.size() != 2) {
                    write(STDERR_FILENO, error_message, strlen(error_message));
                    continue;
                }
                const char *dir = tokens[1].c_str();
                if (chdir(dir) != 0) {
                    write(STDERR_FILENO, error_message, strlen(error_message));
                }
                continue;
            } else if (cmd_name == "path") {
                path.clear();
                for (i = 1; i < tokens.size(); ++i) {
                    path.push_back(tokens[i]);
                }
                continue;
            } else {
                // External commands
                if (path.empty()) {
                    write(STDERR_FILENO, error_message, strlen(error_message));
                    continue;
                }
                char *cmd_path = NULL;
                for (i = 0; i < path.size(); ++i) {
                    std::string full_path = path[i] + "/" + cmd_name;
                    if (access(full_path.c_str(), X_OK) == 0) {
                        cmd_path = strdup(full_path.c_str());
                        break;
                    }
                }
                if (cmd_path == NULL) {
                    write(STDERR_FILENO, error_message, strlen(error_message));
                    continue;
                }

                // Prepare arguments for execv
                int argc_exec = tokens.size();
                char **argv_exec = new char *[argc_exec + 1];
                int j;
                for (j = 0; j < argc_exec; ++j) {
                    argv_exec[j] = strdup(tokens[j].c_str());
                }
                argv_exec[argc_exec] = NULL;

                // Fork and execute the command
                pid_t pid = fork();
                if (pid < 0) {
                    write(STDERR_FILENO, error_message, strlen(error_message));
                } else if (pid == 0) {
                    // Child process
                    if (!output_file.empty()) {
                        int fd = open(output_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
                        if (fd < 0) {
                            write(STDERR_FILENO, error_message, strlen(error_message));
                            exit(1);
                        }
                        if (dup2(fd, STDOUT_FILENO) < 0 || dup2(fd, STDERR_FILENO) < 0) {
                            write(STDERR_FILENO, error_message, strlen(error_message));
                            exit(1);
                        }
                        close(fd);
                    }
                    execv(cmd_path, argv_exec);
                    // If execv returns, there was an error
                    write(STDERR_FILENO, error_message, strlen(error_message));
                    exit(1);
                } else {
                    // Parent process
                    child_pids.push_back(pid);
                    // Free allocated memory
                    for (j = 0; j < argc_exec; ++j) {
                        free(argv_exec[j]);
                    }
                    delete[] argv_exec;
                    free(cmd_path);
                }
            }
        }

        // Wait for all child processes to finish
        std::vector<pid_t>::iterator pid_it;
        for (pid_it = child_pids.begin(); pid_it != child_pids.end(); ++pid_it) {
            pid_t pid = *pid_it;
            waitpid(pid, NULL, 0);
        }
    }
    return 0;
}
