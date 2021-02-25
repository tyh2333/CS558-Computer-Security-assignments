#include <stdio.h>
#include <stdlib.h>

static int x = 8;

void prompt(){
	char buf[100];

	gets(buf);
	printf("You entered: %s\n", buf);

}

int main(){
	prompt();
	return 0;
}

void target(){
	printf("Haha! You got owned!\n");
	exit(0);
}
