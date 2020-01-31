#include <stdlib.h>
#include <stdio.h> 
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
	puts("Hello World! sleeping for 2 seconds...");
	sleep(2);
	puts("Hello World again! Program is now finished.");
}

// gcc -std=c11 -pedantic-errors -Wall -Werror hello_world.c -o hello_world.o
