#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "compiler.h"
#include "scanner.h"

#if defined(DEBUG_PRINT_CODE)
#include "debug.h"
#endif // DEBUG_PRINT_CODE

typedef struct {
	Token current;
	Token previous;
	bool had_error;
	bool panic_mode;
} Parser;

typedef enum {
	PREC_NONE,
	PREC_ASSIGNMENT, // =
	PREC_OR,         // or
	PREC_AND,        // and
	PREC_EQUALITY,   // == !=
	PREC_COMPARISON, // < > <= =>
	PREC_TERM,       // + -
	PREC_FACTOR,     // * /
	PREC_UNARY,      // ! -
	PREC_CALL,       // . ()
	PREC_PRIMARY,    //
} Precedence;

typedef void Parse_Fn(void);

typedef struct {
	Parse_Fn * prefix;
	Parse_Fn * infix;
	Precedence precedence;
} Parse_Rule;

static Parser parser;
static Chunk * compiling_chunk;

static void do_number(void);
static void do_unary(void);
static void do_binary(void);
static void do_grouping(void);

static Parse_Rule rules[] = {
	[TOKEN_LEFT_PAREN]    = {do_grouping, NULL,      PREC_NONE},
	// [TOKEN_RIGHT_PAREN]   = {NULL,        NULL,      PREC_NONE},
	// [TOKEN_LEFT_BRACE]    = {NULL,        NULL,      PREC_NONE},
	// [TOKEN_RIGHT_BRACE]   = {NULL,        NULL,      PREC_NONE},
	// [TOKEN_COMMA]         = {NULL,        NULL,      PREC_NONE},
	// [TOKEN_DOT]           = {NULL,        NULL,      PREC_NONE},
	[TOKEN_MINUS]         = {do_unary,    do_binary, PREC_TERM},
	[TOKEN_PLUS]          = {NULL,        do_binary, PREC_TERM},
	// [TOKEN_SEMICOLON]     = {NULL,        NULL,      PREC_NONE},
	[TOKEN_SLASH]         = {NULL,        do_binary, PREC_FACTOR},
	[TOKEN_STAR]          = {NULL,        do_binary, PREC_FACTOR},
	// [TOKEN_BANG]          = {do_unary,    NULL,      PREC_NONE},
	// [TOKEN_BANG_EQUAL]    = {NULL,        NULL,      PREC_NONE},
	// [TOKEN_EQUAL]         = {NULL,        NULL,      PREC_NONE},
	// [TOKEN_EQUAL_EQUAL]   = {NULL,        NULL,      PREC_NONE},
	// [TOKEN_GREATER]       = {NULL,        NULL,      PREC_NONE},
	// [TOKEN_GREATER_EQUAL] = {NULL,        NULL,      PREC_NONE},
	// [TOKEN_LESS]          = {NULL,        NULL,      PREC_NONE},
	// [TOKEN_LESS_EQUAL]    = {NULL,        NULL,      PREC_NONE},
	// [TOKEN_IDENTIFIER]    = {NULL,        NULL,      PREC_NONE},
	// [TOKEN_STRING]        = {NULL,        NULL,      PREC_NONE},
	[TOKEN_NUMBER]        = {do_number,   NULL,      PREC_NONE},
	// [TOKEN_AND]           = {NULL,        NULL,      PREC_NONE},
	// [TOKEN_CLASS]         = {NULL,        NULL,      PREC_NONE},
	// [TOKEN_ELSE]          = {NULL,        NULL,      PREC_NONE},
	// [TOKEN_FALSE]         = {NULL,        NULL,      PREC_NONE},
	// [TOKEN_FOR]           = {NULL,        NULL,      PREC_NONE},
	// [TOKEN_FUN]           = {NULL,        NULL,      PREC_NONE},
	// [TOKEN_IF]            = {NULL,        NULL,      PREC_NONE},
	// [TOKEN_NIL]           = {NULL,        NULL,      PREC_NONE},
	// [TOKEN_OR]            = {NULL,        NULL,      PREC_NONE},
	// [TOKEN_PRINT]         = {NULL,        NULL,      PREC_NONE},
	// [TOKEN_RETURN]        = {NULL,        NULL,      PREC_NONE},
	// [TOKEN_SUPER]         = {NULL,        NULL,      PREC_NONE},
	// [TOKEN_THIS]          = {NULL,        NULL,      PREC_NONE},
	// [TOKEN_TRUE]          = {NULL,        NULL,      PREC_NONE},
	// [TOKEN_VAR]           = {NULL,        NULL,      PREC_NONE},
	// [TOKEN_WHILE]         = {NULL,        NULL,      PREC_NONE},
	// [TOKEN_ERROR]         = {NULL,        NULL,      PREC_NONE},
	[TOKEN_EOF]           = {NULL,        NULL,      PREC_NONE},
};

static Parse_Rule * get_rule(Token_Type type) {
	return &rules[type];
}

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

static void error(char const * message) {
	error_at(&parser.previous, message);
}

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

static uint8_t make_constant(Value value) {
	uint32_t constant = chunk_add_constant(current_chunk(), value);
	if (constant > UINT8_MAX) {
		error("too many constant in one chunk");
		return 0;
	}
	return (uint8_t)constant;
}

static void emit_byte(uint8_t byte) {
	chunk_write(current_chunk(), byte, parser.previous.line);
}

static void emit_bytes(uint8_t byte1, uint8_t byte2) {
	emit_byte(byte1);
	emit_byte(byte2);
}

static void emit_constant(Value value) {
	emit_bytes(OP_CONSTANT, make_constant(value));
}

static void parse_presedence(Precedence precedence) {
	(void)precedence;
	compiler_advance();
	Parse_Fn * prefix_rule = get_rule(parser.previous.type)->prefix;
	if (prefix_rule == NULL) {
		error("expected an expression");
		return;
	}

	prefix_rule();

	while (precedence <= get_rule(parser.current.type)->precedence) {
		compiler_advance();
		Parse_Fn * infix_rule = get_rule(parser.previous.type)->infix;
		infix_rule();
	}
}

static void do_expression(void) {
	parse_presedence(PREC_ASSIGNMENT);
}

static void do_number(void) {
	Value value = strtod(parser.previous.start, NULL);
	emit_constant(value);
}

static void do_unary(void) {
	Token_Type operator_type = parser.previous.type;
	// compile the operand
	parse_presedence(PREC_UNARY);
	// emit the operator instruction
	switch (operator_type) {
		case TOKEN_MINUS: emit_byte(OP_NEGATE); break;
		default: return; // unreachable
	}
}

static void do_binary(void) {
	Token_Type operator_type = parser.previous.type;
	// compile the right operand
	Parse_Rule * rule = get_rule(operator_type);
	parse_presedence((Precedence)(rule->precedence + 1));
	// emit the operator instruction
	switch (operator_type) {
		case TOKEN_PLUS:  emit_byte(OP_ADD); break;
		case TOKEN_MINUS: emit_byte(OP_SUBTRACT); break;
		case TOKEN_STAR:  emit_byte(OP_MULTIPLY); break;
		case TOKEN_SLASH: emit_byte(OP_DIVIDE); break;
		default: return; // unreachable
	}
}

static void do_grouping(void) {
	do_expression();
	consume(TOKEN_RIGHT_PAREN, "expected a ')");
}

static void compiler_end(void) {
	emit_byte(OP_RETURN);
#if defined(DEBUG_PRINT_CODE)
	if (!parser.had_error) {
		chunk_disassemble(current_chunk(), "code");
	}
#endif // DEBUG_PRINT_CODE
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
