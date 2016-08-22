#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

int main() {
	char string[] = "changeme";
	chmod(string, 0666);
	return 0;
}
