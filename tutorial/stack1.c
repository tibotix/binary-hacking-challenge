#include <unistd.h>
#include <stdio.h>

void win() {
	printf("you win\n");
}

int main(int argc, char** argv){
	int buffer[10];
	read(0, buffer, 16*sizeof(int));
	return 0;
}
