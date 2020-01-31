# Useful info on makefiles: https://www.cs.oberlin.edu/~kuperman/help/make.html
CC = gcc
CFLAGS=-Wall -Werror -std=c11 -pedantic-errors

SEEsh: SEEsh.c
	${CC} ${CFLAGS} -o SEEsh.o SEEsh.c
	