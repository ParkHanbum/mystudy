#include <stdio.h>

int main()
{
	char *test = "test";
	int i = 0, j = 0;

	if (i == 0)
		j = 1;

	if (i == 0)
		printf("%s %d \n", test, i);
	else if (i == 1)
		printf("%s %d \n", test, i);
	else if (i == 2)
		printf("%s %d \n", test, i);
	else if (i == 3)
		printf("%s %d \n", test, i);
	else if (i == 4)
		printf("%s %d \n", test, i);

	printf("%s %d \n", test, i);

	return i;
}
