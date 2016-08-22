#include <string.h>
#include <unistd.h>

int main (int argc, char **argv) {
	if (argc < 3) {
		return 1;
	}
	setpgid(strtoul(argv[1]), strtoul(argv[2]));
}
