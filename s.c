#include <unistd.h>
#include <sys/types.h>

int main() {
	setuid(0);
	printf("%d\n", getuid());
}
