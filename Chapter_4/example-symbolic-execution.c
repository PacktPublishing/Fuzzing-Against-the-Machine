#include <stdio.h>
#include <stdlib.h>

int main (int argc, char**argv) 
{ 
	int a = atoi(argv[1]); 
	int b = atoi(argv[2]); 

	if (a % b == 0)   
		printf("Your numbers are multiples\n"); 
	else 
		printf("Your numbers are not multiples\n");

	return 0; 
}
