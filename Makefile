default: main.c
	${CC} $< -o snake -std=c23 -Wpedantic -Wall -Wextra -Wconversion -Wsign-conversion
