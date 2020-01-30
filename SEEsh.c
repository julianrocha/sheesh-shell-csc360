#include <stdlib.h> // exit
#include <stdio.h> // printing
#include <string.h> // string tokenizing/comparisons
#include <unistd.h> // fork(), getcwd()

extern char **environ;

#define MAX_INPUT_LENGTH 514 // as defined in assingment description. 512 characters + '\n' + '\0'
#define WHITE_SPACE_DELIM " \t\r\n\a"

// forward function declarations to break cyclic dependency between help command and built_in arrays
int cd(char **command_and_arguments);
int pwd(char **command_and_arguments);
int help(char **command_and_arguments);
int seesh_exit(char **command_and_arguments);
int set(char **command_and_arguments);
int unset(char **command_and_arguments);
int history(char **command_and_arguments);

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

int cd(char **command_and_arguments){
	char* dir = command_and_arguments[1];
	if(dir == NULL){
		dir = getenv("HOME");
	}
	if(chdir(dir) != 0){
		perror("Error: Problem changing working directory! Terminating...");
		exit(EXIT_FAILURE);
    }
	return 1;
}

int pwd(char **command_and_arguments){
	char pwd[MAX_INPUT_LENGTH]; // https://stackoverflow.com/questions/3642050/chdir-not-affecting-environment-variable-pwd
	if(getcwd(pwd, sizeof(pwd)) == NULL){
		perror("Error: Problem finding current working directory! Terminating...");
        exit(EXIT_FAILURE);
	}
	puts(pwd);
	return 1;
}
int help(char **command_and_arguments){
	for(int i = 0; i < sizeof(seesh_built_in_str_commands) / sizeof(char *); i++){
		printf("* %s%s\n", seesh_built_in_str_commands[i], seesh_built_in_descriptions[i]);
	}
	return 1;
}

int seesh_exit(char **command_and_arguments){
	return 0;
}

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
		perror("Error: Problem setting env variable! Terminating...");
        exit(EXIT_FAILURE);
	}
	return 1;
}

int unset(char **command_and_arguments){
	char *var = command_and_arguments[1];
	if(var != NULL){
		if(unsetenv(var) != 0){
			perror("Error: Problem unsetting env variable! Terminating...");
	        exit(EXIT_FAILURE);
		}
	}
	return 1;
}

int history(char **command_and_arguments){
	return 1;
}

void get_user_input(char line[]) {
	// read stdin line into fixed size char array
    if(fgets(line, MAX_INPUT_LENGTH, stdin) == NULL){
        perror("Error: Problem reading input! Terminating...");
        exit(EXIT_FAILURE); // https://stackoverflow.com/questions/13667364/exit-failure-vs-exit1
    }
    // remove '\n' from end of line by shifting '\0' left
    int end_of_line = strlen(line) - 1;
    if(*line && line[end_of_line] == '\n'){
    	line[end_of_line] = '\0';
    } else {
    	perror("Error: Input was too long for buffer! Terminating...");
        exit(EXIT_FAILURE);
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

int run_executable(char **command_and_arguments) {
	int pid;
	int status;
	pid = fork();
	if(pid == 0) {
		// child process
		execvp(command_and_arguments[0], command_and_arguments); // https://www.geeksforgeeks.org/exec-family-of-functions-in-c/
		exit(EXIT_FAILURE);
	} else if (pid > 0) {
		// parent process
		// wait in parent process until child process has completed
		waitpid(pid, &status, WUNTRACED); // https://linux.die.net/man/2/wait
	} else {
		// still in parent but could not create child
		perror("Error: Problem starting child process! Terminating..."); // https://www.tutorialspoint.com/c_standard_library/c_function_perror.htm
        exit(EXIT_FAILURE);
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
        free(command_and_arguments);
    }
}

void seesh_config(){
	char *home = getenv("HOME");
	char *file_name = "/.SEEshrc";
	FILE *file_pointer = fopen(strcat(home, file_name), "r");
	if(file_pointer == NULL) {
		perror("Error: Problem opening .SEEshrc! Terminating...");
        exit(EXIT_FAILURE);
	}

	char file_line[MAX_INPUT_LENGTH];
	while(fgets(file_line, MAX_INPUT_LENGTH, file_pointer) != NULL) {
		// print and execute each line of file
	}
}

int main(int argc, char *argv[]) {
    // load config
    seesh_config();
    // loop for shell
    seesh_loop();

    return EXIT_SUCCESS;
}

// gcc -std=c11 -pedantic-errors -Wall -Werror SEEsh.c -o SEEsh

