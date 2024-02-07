#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>

char error_message[30] = "An error has occurred\n";

void myPrint(char *msg) {
    write(STDOUT_FILENO, msg, strlen(msg));
}

void create_process(char *argv[]) {
    pid_t pid;
    int status;

    if ((pid = fork()) < 0) {
        myPrint(error_message);
        exit(0);
    }
    if (pid == 0) {
        if (execvp(*argv, argv) < 0) {
            myPrint(error_message);
            exit(0);
        }
    } else {
        waitpid(pid, &status, WUNTRACED);
    }
}

char *remove_spaces(char *cmd) {
    char *ptr;
    char *copy = strdup(cmd);
    return strtok_r(copy, "\t\n ", &ptr);
}

char **make_argv(char *cmd) {
    char **argv = (char **)calloc(100, sizeof(char *));
    char *arg = strtok(cmd, " ");
    int i;

    for (i = 0; arg; i++) {
        argv[i] = arg;
        arg = strtok(NULL, " ");
    }
    argv[i] = NULL;

    return argv;
}

void pwd() {
    char buffer[1000];
    getcwd(buffer, sizeof(buffer));
    myPrint(buffer);
    myPrint("\n");
}

int cd(char *argv[]) {
    if (argv[2] != NULL) {
        return 1;
    }
    if (argv[1] == NULL) {
        char *home = getenv("HOME");
        if (chdir(home) < 0) {
            return 1;
        }
        return 0;
    }
    char cwd[500];
    getcwd(cwd, sizeof(cwd));
    char *dest = argv[1];
    if (chdir(dest) < 0) {
        return 1;
    }
    setenv(dest, cwd, 1);
    return 0;
}

int main(int argc, char *argv[]) {
    char cmd_buff[4000];
    int fd = dup(1);
    char *pinput;
    char *check;
    FILE *batch = fopen(argv[1], "r");
    int batch_print;
    char* redirect;
    int redirect_file;

    if ((argv[1] != NULL) && (batch == NULL)) {
        myPrint(error_message);
        exit(0);
    }

    while (1) {
        char delim[] = "\n\t;";
        char *saveptr;
        char *parse;
        char **proc_argv;
        batch_print = 0;

        if (argv[1] != NULL) {
            pinput = fgets(cmd_buff, 4000, batch);
            batch_print = 1;
        } else {
            myPrint("myshell> ");
            pinput = fgets(cmd_buff, 4000, stdin);
        }
        if (!pinput) {
            exit(0);
        }

        check = remove_spaces(cmd_buff);
        if (check != NULL) {
            if (batch_print) {
                myPrint(cmd_buff);
            }
        } else {
            continue;
        }

        check = strtok(cmd_buff, "\0\n");
        if (strlen(check) > 512) {
            myPrint(error_message);
            continue;
        }

        parse = strtok_r(cmd_buff, delim, &saveptr);
        while (parse) {
            redirect = strstr(parse, ">");

            check = remove_spaces(parse);
            if (!strcmp(check, "exit")) {
                check = strtok(parse, "\n\t ");
                if (strtok(NULL, "\n\t ") != NULL) {
                    myPrint(error_message);
                    parse = strtok_r(NULL, delim, &saveptr);
                    continue;
                }
                exit(0);
            }
            if (!strcmp(check, "pwd")) {
                check = strtok(parse, "\n\t ");
                if (strtok(NULL, "\n\t ") != NULL) {
                    myPrint(error_message);
                    parse = strtok_r(NULL, delim, &saveptr);
                    continue;
                }
                pwd();
                parse = strtok_r(NULL, delim, &saveptr);
                continue;
            }
            if (!strcmp(check, "cd")) {
                if (redirect != NULL) {
                    myPrint(error_message);
                    parse = strtok_r(NULL, delim, &saveptr);
                    continue;
                }
                proc_argv = make_argv(parse);
                if (cd(proc_argv)) {
                    myPrint(error_message);
                    parse = strtok_r(NULL, delim, &saveptr);
                    continue;
                }
                free(proc_argv);
                parse = strtok_r(NULL, delim, &saveptr);
                continue;
            }
            if (redirect != NULL) {
                int count = 0;
                
                for (int i = 0; i < strlen(parse); i++) {
                    if (parse[i] == '>') {
                        count++;
                    }
                }
                if (count > 1) {
                    myPrint(error_message);
                    parse = strtok_r(NULL, delim, &saveptr);
                    continue;
                }

                parse = strtok(parse, ">");

                char *file_string = strtok(NULL, ">");
                if (file_string == NULL) {
                    myPrint(error_message);
                    parse = strtok_r(NULL, delim, &saveptr);
                    continue;
                }
                redirect_file = open(remove_spaces(file_string), O_CREAT | O_EXCL | O_WRONLY | O_TRUNC, 0664);
                if (redirect_file == -1) {
                    myPrint(error_message);
                    parse = strtok_r(NULL, delim, &saveptr);
                    continue;
                }
                dup2(redirect_file, STDOUT_FILENO);
            }

            proc_argv = make_argv(parse);
            create_process(proc_argv);

            if (redirect != NULL) {
                dup2(fd, STDOUT_FILENO);
            }

            free(proc_argv);
            parse = strtok_r(NULL, delim, &saveptr);
        }
    }
}
