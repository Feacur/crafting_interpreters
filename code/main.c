#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "vm.h"

static char * read_file(char const * path) {
	FILE * file = fopen(path, "rb");
	if (file == NULL) {
		fprintf(stderr, "couldn't open file \"%s\"\n", path);
		exit(4);
	}

	fseek(file, 0L, SEEK_END);
	size_t file_size = (size_t)ftell(file);
	rewind(file);

	char * buffer = (char *)malloc(file_size + 1);
	if (buffer == NULL) {
		fprintf(stderr, "not enough memory to read \"%s\"\n", path);
		exit(5);
	}

	size_t bytes_read = (size_t)fread(buffer, sizeof(char), file_size, file);
	if (bytes_read < file_size) {
		fprintf(stderr, "couldn't read file \"%s\"\n", path);
		exit(6);
	}

	buffer[bytes_read] = '\0';

	fclose(file);
	return buffer;
}

static void run_file(char const * path) {
	char * source = read_file(path);
	Interpret_Result result = vm_interpret(source);
	free(source);

	if (result == INTERPRET_COMPILE_ERROR) { exit(2); }
	if (result == INTERPRET_RUNTIME_ERROR) { exit(3); }
}

static void repl(void) {
	char line[1024];
	for (;;) {
		printf("> ");
		if (!fgets(line, sizeof(line), stdin)) {
			printf("\n");
			break;
		}

		vm_interpret(line);
	}
}

int main (int argc, char * argv[]) {
	(void)argc; (void)argv;
	vm_init();

	if (argc == 1) {
		repl();
	}
	else if (argc == 2) {
		run_file(argv[1]);
	}
	else {
		fprintf(stderr, "usage: interpreter [path]\n");
	}

	vm_free();
	(void)getchar();
	return 0;
}
