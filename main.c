#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <ctype.h>

#define BUFFER_SIZE 1000
#define POINTER_HISTORY 0

char* command;
char* command_pipe;
char* input;
char* output;
int command_from_history = 0;
char** possible_commands;

FILE *input_file = NULL;
FILE *output_file = NULL;

int pointer_history = POINTER_HISTORY;
char ** shell_history;

void run_command();
void history();
void save_history();
int setup();
void history_set_command(int index);
void write_command_on_file();
int build_pipe_string();
int build_output(char *c);
int build_input();
void open_output_file();
void open_input_file();
char *trimwhitespace(char *str);

void help() {
    printf("'help' command.\n");
}

int main() {
    
    int valid_setup = setup();
    if(valid_setup == 1) 
        return valid_setup;
    
    while(1) {
        input_file = NULL;
        output_file = NULL;
        printf("Digit a command:");
        fgets(command, BUFFER_SIZE, stdin);
        run_command();
    }
}

void run_command() {
    command = trimwhitespace(command);
    if(strcmp(command, possible_commands[0]) == 0) {
        help();
    } else if(strcmp(command, possible_commands[1]) == 0) {
        history();
    } else {
        if(command_from_history == 0)
            save_history();
        if(build_pipe_string() == 1) {
            //build piped , will modify the 'command' variable as weel.
            build_output(command_pipe);
        } else {
            build_output(command);
        }
        build_input();

         // Just to check if is getting right   
        if(input_file != NULL) {
            printf("Has Input: \%s\n", input);
        } else {
            printf("Don't have Input file\n");
        }
        // Just to check if is getting right    
        if(output_file != NULL) {
            printf("Has Output: \%s\n", output);
        } else {
            printf("Don't have Output file\n");
        }

        if(strlen(command) == 0) {
            printf("Error on command: %s\n", command);
            return;
        } else {
            command = trimwhitespace(command);
            if(command[strlen(command)-1] == '\n')
                command[strlen(command)-1] = '\0';
            printf("Command: %s\n", command);
            // run command

        }

        if(strlen(command_pipe) == 0) {
            printf("Don't have command pipe\n");
        } else {
            printf("Command pipe: %s\n", command_pipe);
            // fork and run second command;
        }

    }
    return;
}

int setup() {
    printf("Configuring environemnt...\n");
    command = (char*)malloc(sizeof(char)*BUFFER_SIZE);
    command_pipe = (char*)malloc(sizeof(char)*BUFFER_SIZE);
    input = (char*)malloc(sizeof(char)*BUFFER_SIZE);
    output = (char*)malloc(sizeof(char)*BUFFER_SIZE);

    shell_history = (char**)malloc(sizeof(char)*100);
    for(size_t i=0;i<100;i++) {
        shell_history[i] = (char*)malloc(sizeof(char)*BUFFER_SIZE);
    }

    possible_commands = (char**)malloc(sizeof(char**)*10);
    for(size_t i=0; i < 10;i++) {
        possible_commands[i] = (char*)malloc(sizeof(char*)*BUFFER_SIZE);
    }

    possible_commands[0] = "help";
    possible_commands[1] = "history";


    printf("This is a shell environment, feel free to use any command on Linux environment!\n");
    printf("You can digit 'help' to see the how this shell works and 'history' to check all the previous commands\n");

    return 0;
}

void history() {
    printf("Press enter to see history: \n");
    char *history_command = (char*)malloc(sizeof(char)*BUFFER_SIZE);
    int commands_show = 0;
    while(fgets(history_command, BUFFER_SIZE, stdin)) {
        if(strcmp(history_command, "quit\n") == 0)
            return;
        else if(strcmp(history_command, "\n") != 0) {
            int c = atoi(history_command);
            if(c > 0 && c <= commands_show) {
                history_set_command(c-1);
                command_from_history = 1;
                run_command();
                command_from_history = 0;
                return;
            } else {
                printf("Invalid command or out of range.\n");
            }
        } else {
            if(commands_show >= pointer_history) {
                printf("End of history...\n");
            } else {
                strcpy(history_command, shell_history[commands_show++]);
                printf("%d - %s\n", commands_show, history_command);
            }
        }
    }
    return;
}

void history_set_command(int index) {
    strcpy(command, shell_history[index]);
    printf("Running command from history: %s\n", command);
    return;
}

void save_history() {
    strcpy(shell_history[pointer_history], command);
    if(++pointer_history > 100)
        pointer_history = 0;
}

int build_pipe_string() {
    command_pipe = (char*)malloc(sizeof(char)*BUFFER_SIZE);
    int i=0;
    int switch_command = 0;
    while(command[i] != '\0') {
        if(command[i] == '|') {
            command[i] = '\0';
            switch_command = 1;
            i++;
            continue;
        }
        if(switch_command == 1) {
            strcpy(command_pipe, &(command[i]));
            break;
        }
        i++;
    }
    if(strlen(command_pipe) == 0)
        return 0;
    command_pipe = trimwhitespace(command_pipe);
    return 1;
}

int build_output(char *c) {
    output = (char*)malloc(sizeof(char)*BUFFER_SIZE);
    int i=0;
    int switch_command = 0;
    while(c[i] != '\0') {
        if(c[i] == '>') {
            c[i] = '\0';
            switch_command = 1;
            i++;
            continue;
        }
        if(switch_command == 1) {
            strcpy(output, &(c[i]));
            break;
        }
        i++;
    }

    if(strlen(output) == 0)
        return 0;
    output = trimwhitespace(output);
    open_output_file();
    return 1;
}

int build_input() {
    input = (char*)malloc(sizeof(char)*BUFFER_SIZE);
    int i=0;
    int switch_command = 0;
    while(command[i] != '\0') {
        if(command[i] == '<') {
            command[i] = '\0';
            switch_command = 1;
            i++;
            continue;
        }
        if(switch_command == 1) {
            strcpy(input, &(command[i]));
            break;
        }
        i++;
    }

    if(strlen(input) == 0)
        return 0;
    input = trimwhitespace(input);
    open_input_file();
    return 1;
}

void open_output_file() {
    output_file = fopen(output, "w");
    return;
}

void open_input_file() {
    input_file = fopen(input, "r");
    return;
}

char *trimwhitespace(char *str)
{
  char *end;

  // Trim leading space
  while(isspace((unsigned char)*str)) str++;

  if(*str == 0)  // All spaces?
    return str;

  // Trim trailing space
  end = str + strlen(str) - 1;
  while(end > str && isspace((unsigned char)*end)) end--;

  // Write new null terminator character
  end[1] = '\0';

  return str;
}