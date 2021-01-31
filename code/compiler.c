#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "compiler.h"
#include "scanner.h"

typedef struct {
	Token current;
	Token previous;
	bool had_error;
	bool panic_mode;
} Parser;

static Parser parser;
static Chunk * compiling_chunk;

static Chunk * current_chunk(void) {
	return compiling_chunk;
}

static void error_at(Token * token, char const * message) {
	if (parser.panic_mode) { return; }
	parser.panic_mode = true;

	fprintf(stderr, "[line %d] error", token->line);

	if (token->type == TOKEN_EOF) {
		fprintf(stderr, " at end");
	}
	else if (token->type == TOKEN_ERROR) {
		// empty
	}
	else {
		fprintf(stderr, " at '%.*s'", token->length, token->start);
	}

	fprintf(stderr, ": %s\n", message);
	parser.had_error = true;
}

static void error_at_current(char const * message) {
	error_at(&parser.current, message);
}

// static void error(char const * message) {
// 	error_at(&parser.previous, message);
// }

static void compiler_advance(void) {
	parser.previous = parser.current;

	for (;;) {
		parser.current = scan_token();
		if (parser.current.type != TOKEN_ERROR) { break; }
		error_at_current(parser.current.start);
	}
}

static void consume(Token_Type type, char const * message) {
	if (parser.current.type == type) {
		compiler_advance();
		return;
	}

	error_at_current(message);
}

static void emit_byte(uint8_t byte) {
	chunk_write(current_chunk(), byte, parser.previous.line);
}

// static void emit_bytes(uint8_t byte1, uint8_t byte2) {
// 	emit_byte(byte1);
// 	emit_byte(byte2);
// }

static void do_expression(void) {
}

static void compiler_end(void) {
	emit_byte(OP_RETURN);
}

bool compile(char const * source, Chunk * chunk) {
	scanner_init(source);
	compiling_chunk = chunk;

	parser.had_error = false;
	parser.panic_mode = false;

	compiler_advance();
	do_expression();
	consume(TOKEN_EOF, "expected the end of an expression");
	compiler_end();
	return !parser.had_error;
}
