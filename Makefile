all: mysh
mysh: mysh.c
	gcc -Wall -Werror -fsanitize=address,undefined -g mysh.c -o mysh
clear:
	rm -rf mysh *~
