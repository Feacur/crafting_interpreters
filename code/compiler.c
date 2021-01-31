#include <stdio.h>

#include "common.h"
#include "compiler.h"
#include "scanner.h"

void compile(char const * source) {
	scanner_init(source);
	uint32_t line = 0;
	for (;;) {
		Token token = scan_token();
		if (token.line != line) {
			printf("%4d ", token.line);
			line = token.line;
		}
		else {
			printf("   | ");
		}
		printf("%2d '%.*s'\n", token.type, token.length, token.start);

		if (token.type == TOKEN_EOF) { break; }
	}
}
