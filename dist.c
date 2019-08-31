#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

int main(int argc, char **argv) {
	unsigned long b[256] = { 0 };
	if (argc != 2) {
		fprintf(stderr, "usage: %s file\n", argv[0]);
		return 1;
	}

	errno = 0;
	FILE *in = fopen(argv[1], "rb");
	if (!in) {
		fprintf(stderr, "unable to open %s: %s\n", argv[1], strerror(errno));
		return 1;
	}

	for (int ch = 0; (ch = fgetc(in)) != EOF;)
		b[ch]++;

	for (int i = 0; i < 256; i++) {
		if (!(i % 8))
			fprintf(stdout, "\n");
		fprintf(stdout, "%02x: %-4lu ", i, b[i]);
	}
	fprintf(stdout, "\n");

	fclose(in);
	return 0;
}
