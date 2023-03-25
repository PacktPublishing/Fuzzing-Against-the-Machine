#include <stdio.h>
#include <string.h>

void
test_fuzz(char *str)
{
	int size = strlen(str);

	if (size < 40)
		return;

	if (strncmp(&str[1],"622b6f721088950153f52e4cecc49513", strlen("622b6f721088950153f52e4cecc49513")))
		return;

	printf("You have reached the crash!\n");
	printf("Doing last test\n");
	
	if (*((unsigned int*)&str[34]) == 0x70707070)
	{
		int *ptr = (int *)0x90909090;
		*ptr = 1;
	}

	printf("You shouldn't arrive here\n");
}

int
main(int argc, char *argv[])
{
	FILE * ptr;
	char ch;
	int index = 0;
	char buff[250] = {0};

	if (argc != 2)
	{
		printf("[-] Usage: %s <file>\n",argv[0]);
		return 1;
	}

	ptr = fopen(argv[1], "r");
	
	while(!feof(ptr) && index < 250)
	{
		ch = fgetc(ptr);
		buff[index++] = ch;
	}

	test_fuzz(buff);
	
	fclose(ptr);

	return 0;
}
