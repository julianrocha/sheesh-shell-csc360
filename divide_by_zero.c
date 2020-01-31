#include <stdlib.h>
#include <stdio.h> 
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
	int x = 1;
	int y = 0;
	int z = x / y;
	printf("%d\n", z);
}

// gcc -std=c11 -pedantic-errors -Wall -Werror divide_by_zero.c -o divide_by_zero.o
