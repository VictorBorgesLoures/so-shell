#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <regex.h>

#define MAX_INPUT 1024
#define MAX_ARGS 128
#define HISTORY_SIZE 100

char *history[HISTORY_SIZE];
int history_count = 0;
char *input_file = NULL, *output_file = NULL;

void add_to_history(const char *command)
{
    if (history_count < HISTORY_SIZE)
    {
        history[history_count++] = strdup(command);
    }
    else
    {
        free(history[0]);
        for (int i = 1; i < HISTORY_SIZE; i++)
        {
            history[i - 1] = history[i];
        }
        history[HISTORY_SIZE - 1] = strdup(command);
    }
}

void help()
{
    printf("Este programa é um shell que executará quaisquer comandos intríscecos ao sistema operacional Linux.\nDigite 'history' para listar os comandos já feitos e 'history x' para executar o comando da lista.\nTal que 'x' é um número entre 1 e 100.\n");
}

void print_history()
{
    for (int i = 0; i < history_count; i++)
    {
        printf("%d: %s\n", i + 1, history[i]);
    }
}

void execute_command(char *cmdline)
{
    char *args[MAX_ARGS];
    int background = 0, pipe_fd[2], pipe_index = -1;
    pid_t pid;

    add_to_history(cmdline);

    char *token = strtok(cmdline, " ");
    int arg_count = 0;

    while (token != NULL)
    {
        if (strcmp(token, "&") == 0)
        {
            background = 1;
        }
        else if (strcmp(token, "<") == 0)
        {
            token = strtok(NULL, " ");
            input_file = token;
        }
        else if (strcmp(token, ">") == 0)
        {
            token = strtok(NULL, " ");
            output_file = token;
        }
        else if (strcmp(token, "|") == 0)
        {
            pipe_index = arg_count;
            args[arg_count++] = NULL;
        }
        else
        {
            args[arg_count++] = token;
        }
        token = strtok(NULL, " ");
    }

    args[arg_count] = NULL;

    int stdout_copy = dup(1);
    int stdin_copy = dup(0);

    pipe(pipe_fd);

    if (input_file)
    {
        int fd = open(input_file, O_RDONLY);
        if (fd < 0)
        {
            perror("Erro na entrada de arquivo");
            exit(1);
        }
        dup2(fd, STDIN_FILENO);
        close(fd);
    }

    if (output_file)
    {
        int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0)
        {
            perror("Erro na saida de arquivo");
            exit(1);
        }
        dup2(fd, STDOUT_FILENO);
        close(fd);
    }

    pid = fork();

    if (pid == 0)
    { // Processo filho

        if (pipe_index != -1)
        {
            dup2(pipe_fd[1], STDOUT_FILENO);
            close(pipe_fd[0]);
            close(pipe_fd[1]);
        }

        if (execvp(args[0], args) < 0)
        {
            perror("Erro exec");
            exit(1);
        }
    }
    else if (pid > 0)
    { // Parent process
        if (!background)
        {
            waitpid(pid, NULL, 0);
        }
        else
        {
            if(!output_file)
                printf("Processo %d rodando no background\n", pid);
        }

        if (pipe_index != -1)
        {
            close(pipe_fd[1]);
            pid = fork();

            if (pid == 0)
            {
                dup2(pipe_fd[0], STDIN_FILENO);
                close(pipe_fd[0]);

                if (execvp(args[pipe_index + 1], &args[pipe_index + 1]) < 0)
                {
                    perror("Erro na execucao do pipe");
                    exit(1);
                }
            }
            else if (pid > 0)
            {
                close(pipe_fd[0]);
                waitpid(pid, NULL, 0);
            }
        }
    }
    else
    {
        perror("Erro no fork");
    }

    int ret;
    waitpid(pid, &ret, 0);

    dup2(stdin_copy, 0);
    close(stdin_copy);
    dup2(stdout_copy, 1);
    close(stdout_copy);
    close(pipe_fd[0]);
    close(pipe_fd[1]);
    output_file = NULL;
    input_file = NULL;
}

void signal_handler(int signo)
{
    if (signo == SIGCHLD)
    {
        while (waitpid(-1, NULL, WNOHANG) > 0)
        {
            if(!output_file)
                printf("Processo de background completado!\n");
        }
    }
}

int main()
{
    char cmdline[MAX_INPUT];

    signal(SIGCHLD, signal_handler);

    regex_t regex_history;
    regcomp(&regex_history, "^(history )([1-9][0-9]?|100)$", REG_EXTENDED | REG_NOSUB);
    char history_tester[MAX_INPUT];

    while (1)
    {
        printf("shell> ");
        if (fgets(cmdline, MAX_INPUT, stdin) == NULL)
        {
            break;
        }

        cmdline[strcspn(cmdline, "\n")] = '\0';
        strcpy(history_tester, cmdline);

        if (strcmp(cmdline, "exit") == 0)
        {
            break;
        }
        else if (strcmp(cmdline, "help") == 0)
        {
            help();
        }
        else if (strcmp(cmdline, "history") == 0)
        {
            print_history();
        }
        else if (regexec(&regex_history, history_tester, 0, NULL, 0) == 0)
        {
            char *number = strtok(history_tester, " ");
            number = strtok(NULL, " ");
            if (atoi(number) <= history_count)
                execute_command(history[atoi(number) - 1]);
        }
        else
        {
            execute_command(cmdline);
        }
    }

    for (int i = 0; i < history_count; i++)
    {
        free(history[i]);
    }

    return 0;
}
