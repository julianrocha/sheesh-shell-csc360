#include <stdlib.h>
#include <stdio.h> 
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
	puts("Starting infinite_loop");
	while(1){
		sleep(1);
		puts("Still Alive!");
	}

}

// gcc -std=c11 -pedantic-errors -Wall -Werror infinite_loop.c -o infinite_loop
