#include <stdlib.h> // exit, etc...
#include <stdio.h> // printing, file opening, etc...
#include <string.h> // string tokenizing/comparisons, etc...
#include <unistd.h> // fork(), getcwd(), etc...
#include <signal.h> // signal handler

#define MAX_INPUT_LENGTH 514 // as defined in assingment description. 512 characters + '\n' + '\0'
#define WHITE_SPACE_DELIM " \t\r\n\a" // each char considered a delimiter for commands in user input

// forward function declarations to break cyclic dependency between help command and built_in arrays
int cd(char **command_and_arguments);
int pwd(char **command_and_arguments);
int help(char **command_and_arguments);
int seesh_exit(char **command_and_arguments);
int set(char **command_and_arguments);
int unset(char **command_and_arguments);
int history(char **command_and_arguments);

extern char **environ; // POSIX standard for accessing the environement variables passed from parent
// https://stackoverflow.com/questions/4291080/print-the-environment-variables-using-environ

int seesh_pid;
int child_pid;

// built in commands that user can type into seesh
char *seesh_built_in_str_commands[] = {
	"cd",
  	"pwd",
  	"help",
  	"exit",
  	"set",
  	"unset",
  	"history"
};

// function pointers to relate string commands to C functions
int (*seesh_built_in_func_pointers[])(char **) = {
	&cd,
  	&pwd,
  	&help,
  	&seesh_exit,
  	&set,
  	&unset,
  	&history
};

// descriptions of built in commands for help command
char *seesh_built_in_descriptions[] = {
	" [dir] : change SEEsh’s working directory to dir, or to the HOME directory if dir is omitted.",
  	" : print the full filename of the current working directory.",
  	" : display information about builtin commands.",
  	" : exit SEEsh.",
  	" var [value] : Create or update value of var environment variable. var will be empty string if value is omitted. If both var and value are omitted, set displays all environment variables and values.",
  	" var : SEEsh destroys the environment variable var.",
  	" : print a list of all previously issued commands. re-execute a previously issued command by typing a prefix of that command preceded by an exclamation point (!commandprefix)."
};

// Change working direcotry to the passed absolute OR relative path
// If no path is passed, change to the directory specified in the HOME env var
int cd(char **command_and_arguments){
	char* dir = command_and_arguments[1];
	if(dir == NULL){
		dir = getenv("HOME");
	}
	if(chdir(dir) != 0){
		perror("Problem changing working directory");
    }
	return 1;
}

// Print the current working direcotry to stdout
// Assumption that current working directory can't be longer than MAX_INPUT_LENGTH
int pwd(char **command_and_arguments){
	char pwd[MAX_INPUT_LENGTH]; // https://stackoverflow.com/questions/3642050/chdir-not-affecting-environment-variable-pwd
	if(getcwd(pwd, sizeof(pwd)) == NULL){
		perror("Problem getting current working directory");
	}
	puts(pwd);
	return 1;
}

// Print all built in commands and their desctiptions to stddout
int help(char **command_and_arguments){
	for(int i = 0; i < sizeof(seesh_built_in_str_commands) / sizeof(char *); i++){
		printf("* %s%s\n", seesh_built_in_str_commands[i], seesh_built_in_descriptions[i]);
	}
	return 1;
}

// Terminate the seesh shell and return to parent process (likely the linux shell)
int seesh_exit(char **command_and_arguments){
	exit(EXIT_SUCCESS);
	return 0;
}

// Note, there is an assumption that var cannot contain '='
// Including '=' will result in unsuccesful set
// https://www.lemoda.net/c/set-get-env/
int set(char **command_and_arguments){
	char *var = command_and_arguments[1];
	char *val = command_and_arguments[2];
	if(var == NULL){
		int i = 0;
		while(environ[i]){
			printf("%s\n", environ[i++]);
		}
		return 1;
	}
	if(val == NULL){
		val = "";
	}
	if(setenv(var, val, 1) != 0){
		perror("Problem setting env variable");
	}
	return 1;
}

// Note, there is an assumption that var cannot contain '='
// Including '=' will result in unsuccesful unset
// If var does not exist, then unset will succeed and env will remain unchanged
int unset(char **command_and_arguments){
	char *var = command_and_arguments[1];
	if(var != NULL){
		if(unsetenv(var) != 0){
			perror("Problem unsetting env variable");
		}
	}
	return 1;
}

// use a singly linked list with head and tail pointers
// only add commands when they were successful
int history(char **command_and_arguments){
	return 1;
}

void get_user_input(char line[]) {
	// read stdin line into fixed size char array
    if(fgets(line, MAX_INPUT_LENGTH, stdin) == NULL){
    	if(feof(stdin)){
    		printf("\n");
    		exit(EXIT_SUCCESS); // terminate shell when EOF (^D) is entered
    	}
        // else there was an unknown issue reading input
        perror("Problem reading input!");
        line[0] = '\0';
        return;
    }
    // remove '\n' from end of line by shifting '\0' left
    int end_of_line = strlen(line) - 1;
    if(*line && line[end_of_line] == '\n'){
    	line[end_of_line] = '\0';
    } else {
    	// input was too long, flush input, then set to empty input
    	while(fgets(line, MAX_INPUT_LENGTH, stdin) != NULL){
    		if(line[strlen(line) - 1] == '\n') break;
    	}
    	line[0] = '\0';
    	puts("Input was too long for buffer");
    }
}

char **parse_user_input(char input[]) {
	// array of string tokens
	char **tokens = calloc(MAX_INPUT_LENGTH / 2 + 1, sizeof(char*));
	// calloc instead of malloc to insure all non initialized indices are zerod
    char *token = strtok(input, WHITE_SPACE_DELIM);
    int i = 0;
    while(token != NULL) {
    	tokens[i] = token;
    	i++;
    	token = strtok(NULL, WHITE_SPACE_DELIM);
    }
    return tokens;
}

// TODO, if divide_by_zero fails, need to display this failure to user
int run_executable(char **command_and_arguments) {
	int pid;
	int status;
	pid = fork();
	if(pid == 0) {
		// child process
		if(execvp(command_and_arguments[0], command_and_arguments) == -1){ // https://www.geeksforgeeks.org/exec-family-of-functions-in-c/
			perror("Problem executing program in child process");
			exit(EXIT_FAILURE); // terminate child process
		}
	} else if (pid > 0) {
		// parent process
		child_pid = pid;
		// wait in parent process until child process has completed
		waitpid(pid, &status, WUNTRACED); // https://linux.die.net/man/2/wait
	} else {
		// still in parent but could not create child
		perror("Problem starting child process"); // https://www.tutorialspoint.com/c_standard_library/c_function_perror.htm
	}
	return 1;
}

int execute(char **command_and_arguments) {
	// empty user_input
	if(command_and_arguments[0] == NULL) {
		return 1;
	}
	// if builtin command, execute in this process and return
	// loop over built-in commands, compare with first arg, call func if matching
	for(int i = 0; i < sizeof(seesh_built_in_str_commands) / sizeof(char *); i++){
		if(strcmp(command_and_arguments[0], seesh_built_in_str_commands[i]) == 0){
			return (*seesh_built_in_func_pointers[i])(command_and_arguments);
		}
	}
	// else, fork a child process and return when child finishes
	return run_executable(command_and_arguments);
}

void seesh_loop() {
    char user_input[MAX_INPUT_LENGTH];
    char ** command_and_arguments;
    int alive = 1;
    while(alive){
        printf("? ");   										// command prompt
        get_user_input(user_input);  							// grab user_input
        command_and_arguments = parse_user_input(user_input); 	// parse_user_input
        alive = execute(command_and_arguments);   				// execute_user_input
        child_pid = seesh_pid;									// reset pids for sig interrupt handling
        free(command_and_arguments);							// clean slate for next command
    }
}

// as specified in assignment, .SEEshrc is expected to be located in HOME dir of the user who started seesh
// as specified in assignment, it is assumed that no line of .SEEshrc will be longer than MAX_INPUT_LENGTH
void seesh_config(){
	char *home = getenv("HOME");
	char *home_copy = malloc((strlen(home) + 1) * sizeof(char));
	strcpy(home_copy, home);
	char *file_name = "/.SEEshrc";
	FILE *file_pointer = fopen(strcat(home_copy, file_name), "r");
	free(home_copy);
	if(file_pointer == NULL) {
		perror("Problem finding/opening .SEEshrc!");
        return;
	}

	char file_line[MAX_INPUT_LENGTH];
	char ** command_and_arguments;
	int alive = 1;
	while(fgets(file_line, MAX_INPUT_LENGTH, file_pointer) != NULL && alive) {
		int end_of_line = strlen(file_line) - 1;
		if(file_line[end_of_line] == '\n'){
			file_line[end_of_line] = '\0';
		}
		printf("$ %s\n", file_line);
		command_and_arguments = parse_user_input(file_line);
		alive = execute(command_and_arguments);
		free(command_and_arguments);
	}
}

void signal_interrupt_handler(int sig){
	if(child_pid == seesh_pid) exit(EXIT_SUCCESS); // if invoked from parent, exit
	kill(child_pid, SIGTERM); // if invoked while child is running, kill child
	child_pid = getpid(); // 
}

// TODO:
// need to make a SEEshrc
// need to make a makefile
// need to test with valgrind for memory leaks
// need to test for errors which crash seesh
// need to add some testing for swap in doubly linked list
// need to see why PWD env var isn't changing
// need to test on linux servers

// need to implement history
int main(int argc, char *argv[]) {

    // load config
    seesh_config();

    // initialize signal handler for ^C entered by user
    seesh_pid = getpid();
	child_pid = getpid();
	signal(SIGINT, signal_interrupt_handler);

    // loop for shell
    seesh_loop();

    return EXIT_SUCCESS;
}

// gcc -std=c11 -pedantic-errors -Wall -Werror SEEsh.c -o SEEsh

