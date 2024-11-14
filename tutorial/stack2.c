#include <unistd.h>
#include <stdio.h>


int main(int argc, char** argv){
	setvbuf(stdout, NULL, _IONBF, 0);

	char name[10];
	printf("Wie heißt du?: ");
	read(0, name, 10*sizeof(char));
	printf("Hallo, \n");
	printf(name); //%p
	printf("\n");
	int buffer[10];
	read(0, buffer, 50*sizeof(int));
	return 0;
}
