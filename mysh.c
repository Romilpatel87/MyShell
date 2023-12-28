#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>
#include <glob.h>

#define MAX_COMMAND_LENGTH 1024
#define MAX_ARGUMENTS 100

// Function reads a command from the user
int read_command(char* command) {
	printf("\n> ");
	if (fgets(command, MAX_COMMAND_LENGTH, stdin) != NULL) {
    	command[strlen(command) - 1] = '\0';
    	return 0;
	} else {
    	return 1;
	}
}

// Function executes the command
void execute_command(char** arguments) {
	int catid = fork();
	if (catid < 0) {
    	printf("Error: Failed to fork a child process!\n");
    	return;
	} else if (catid == 0) {
    	if (execvp(arguments[0], arguments) < 0) {
        	printf("Error: Failed to execute command!\n");
        	exit(0);
    	}
	} else {
    	wait(NULL);
    	return;
	}
}
 
// Function executes two commands that are piped together
void execute_command_piped(char** arguments, char** argumentspipe) {
	int fd[2]; // 2 file descriptors: 1 is write end and 0 is read end
	int p1, p2;

	if (pipe(fd) < 0) {
    	printf("Error: Failed to intialize pipe\n");
    	return;
	}
	p1 = fork();
	if (p1 < 0) {
    	printf("Error: Failed to fork\n");
    	return;
	}
 
	if (p1 == 0) {
    	dup2(fd[1], STDOUT_FILENO); // child process writes to pipe
    	close(fd[0]); // close excess file descriptors
    	close(fd[1]); // close excess file descriptors

    	if (execvp(arguments[0], arguments) < 0) {
        	printf("Error: Failed to execute command p1\n");
        	exit(0);
    	}
	} else {
    	p2 = fork();
    	if (p2 < 0) {
        	printf("Error: Failed to fork\n");
        	return;
    	}
 
	if (p2 == 0) {
    	dup2(fd[0], STDIN_FILENO); //child process reads from pipe
    	close(fd[0]); // close excess file descriptors
    	close(fd[1]); // close excess file descriptors
   	 
    	if (execvp(argumentspipe[0], argumentspipe) < 0) {
        	printf("Error: Failed to execute command p2\n");
        	exit(0);
    	}
	} else {
    	wait(NULL);
    	wait(NULL);
    	}
	}
}

// Function is used to print the current working directory
void print_cwd() {
   char cwd[1024];
   if (getcwd(cwd, sizeof(cwd)) != NULL) {
  	printf("\nDir: %s", cwd);
   } else {
  	perror("Error: Failed to getcwd()\n");
   }
}

// Function for running built-in commands
int builtin_commands(char** arguments) {  
	char* commands[3];
	int commands_arguments = 0;
	char cwd[1024];
 
	commands[0] = "pwd"; // Present Work Directory
	commands[1] = "cd"; // Change Directory
	commands[2] = "exit"; // Exit from my shell

	for (int i = 0; i < 3; i++) {
    	if (strcmp(arguments[0], commands[i]) == 0) {
        	commands_arguments = i + 1;
        	break;
    	}
	}
    
	switch (commands_arguments) {
	case 1:
    	print_cwd();
    	return 1;
	case 2:
    	if ((arguments[1]) == NULL){
        	printf("%s",cwd);
        	chdir(getenv("HOME"));
        	print_cwd();
    	}
    	else{
        	chdir(arguments[1]);
    	}
    	return 1;
	case 3:
    	printf("\nExiting...\n");
    	exit(0);
	default:
    	break;
	}
	return 0;
}

// Function is used to split the command into two separate commands
int parse_pipe(char* input, char** commands) {
	const char* PIPE_DELIM = "|";
	char* token;
	int i = 0;
	token = strtok(input, PIPE_DELIM);
	while (token != NULL) {
    	commands[i++] = token;
    	token = strtok(NULL, PIPE_DELIM);
	}
	return i > 1; // return true if at least one pipe was found
}
// Function parses the input string into individual arguments and checks for wildcard
void parse_command_args(char* input_str, char** parsed_args) {
	int arg_index = 0;
	int glob_index, glob_count;
	glob_t glob_results;

	for (arg_index = 0; arg_index < MAX_ARGUMENTS; arg_index++) {
    	parsed_args[arg_index] = strsep(&input_str, " ");
    	if (parsed_args[arg_index] == NULL) {
        	break;
    	}
    	if (strlen(parsed_args[arg_index]) == 0) {
        	arg_index--;
    	} else {
        	// check for wildcard *
        	if (strchr(parsed_args[arg_index], '*')) {
            	glob(parsed_args[arg_index], GLOB_NOCHECK | GLOB_TILDE, NULL, &glob_results);
            	glob_count = glob_results.gl_pathc;
            	if (glob_count > 0) {
                	for (glob_index = 0; glob_index < glob_count; glob_index++) {
                    	parsed_args[arg_index+glob_index] = glob_results.gl_pathv[glob_index];
                	}
                	parsed_args[arg_index+glob_index] = NULL;
                	arg_index += glob_index;
            	} else {
                	// no matches found
                	printf("Error: No matches found for %s\n", parsed_args[arg_index]);
                	parsed_args[arg_index] = NULL;
            	}
        	}
    	}
	}
}

// Function handles more complex input that may contain piped commands and built-in commands.
int process_Input_String(char* inputString, char** parsedArguments, char** parsedPipeArguments)
{
	char* pipedCommands[2];
	int isPiped = 0;
 
	isPiped = parse_pipe(inputString, pipedCommands);
	if (isPiped) {
    	parse_command_args(pipedCommands[0], parsedArguments);
    	parse_command_args(pipedCommands[1], parsedPipeArguments);
	} else {
    	parse_command_args(inputString, parsedArguments);
	}
	if (builtin_commands(parsedArguments)) {
    	return 0;
	} else {
    	return 1 + isPiped;
	}
}

int main() {
	char inputString[MAX_COMMAND_LENGTH];
	char* parsedArgs[MAX_ARGUMENTS];
	char* parsedArgsPiped[MAX_ARGUMENTS];
	int execute_Flag = 0;
	int shouldExit = 0;

	while (!shouldExit) {
    	if (!read_command(inputString)) {
       	 
        	execute_Flag = process_Input_String(inputString, parsedArgs, parsedArgsPiped);
        	switch (execute_Flag) {
            	case 0:
                	// Empty input, do nothing
                	break;
            	case 1:
                	execute_command(parsedArgs);
                	break;
            	case 2:
                	execute_command_piped(parsedArgs, parsedArgsPiped);
                	break;
            	case -1:
                	printf("Error: failed to parse command\n");
                	break;
            	default:
                	printf("Error: unknown execute_Flag value: %d\n", execute_Flag);
                	break;
        	}
    	}
    	else {
        	shouldExit = 1;
    	}
	}
	return 0;
}