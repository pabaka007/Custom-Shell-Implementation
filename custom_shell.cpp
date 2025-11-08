#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <cstring>   // for strdup
#include <cstdlib>   // for free()

using namespace std;

void executeCommand(vector<string> &tokens);

// ignore Ctrl+C and Ctrl+Z in parent
void handleSignals() {
    signal(SIGINT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
}

// split input by spaces
vector<string> parseInput(string &input) {
    stringstream ss(input);
    string token;
    vector<string> tokens;
    while (ss >> token)
        tokens.push_back(token);
    return tokens;
}

int main() {
    string input;

    handleSignals();

    while (true) {
        cout << "mysh> " << flush;
        if (!getline(cin, input)) break;
        if (input.empty()) continue;
        if (input == "exit") break;

        vector<string> tokens = parseInput(input);
        executeCommand(tokens);
    }

    cout << "Exiting custom shell. Goodbye!\n";
    return 0;
}

void executeCommand(vector<string> &tokens) {
    if (tokens.empty()) return;

    bool background = false;
    int in_redirect = -1, out_redirect = -1;
    string in_file = "", out_file = "";

    // check for background process
    if (tokens.back() == "&") {
        background = true;
        tokens.pop_back();
    }

    // handle redirection operators
    for (size_t i = 0; i < tokens.size(); i++) {
        if (tokens[i] == "<") {
            if (i + 1 < tokens.size()) {
                in_file = tokens[i + 1];
                tokens.erase(tokens.begin() + i, tokens.begin() + i + 2);
                i--;
            }
        } else if (tokens[i] == ">") {
            if (i + 1 < tokens.size()) {
                out_file = tokens[i + 1];
                tokens.erase(tokens.begin() + i, tokens.begin() + i + 2);
                i--;
            }
        }
    }

    // convert tokens to char* array
    char *args[tokens.size() + 1];
    for (size_t i = 0; i < tokens.size(); i++) {
        args[i] = strdup(tokens[i].c_str());
    }
    args[tokens.size()] = NULL;

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork failed");
        return;
    }

    if (pid == 0) {
        // Child process
        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);

        // input redirection
        if (!in_file.empty()) {
            int fd = open(in_file.c_str(), O_RDONLY);
            if (fd < 0) {
                perror("input file error");
                exit(1);
            }
            dup2(fd, STDIN_FILENO);
            close(fd);
        }

        // output redirection
        if (!out_file.empty()) {
            int fd = open(out_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                perror("output file error");
                exit(1);
            }
            dup2(fd, STDOUT_FILENO);
            close(fd);
        }

        // execute command
        if (execvp(args[0], args) == -1) {
            perror("command not found");
        }
        exit(1);
    } else {
        // Parent process
        if (!background) {
            waitpid(pid, NULL, 0);
        } else {
            cout << "Started background job PID: " << pid << endl;
        }
    }

    // free memory
    for (size_t i = 0; i < tokens.size(); i++)
        free(args[i]);
}
