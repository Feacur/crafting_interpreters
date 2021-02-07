#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chunk.h"
#include "object.h"
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

typedef void Parse_Fn(bool can_assign);

typedef struct {
	Parse_Fn * prefix;
	Parse_Fn * infix;
	Precedence precedence;
} Parse_Rule;

typedef struct {
	Token name;
	uint32_t depth;
} Local;

typedef struct {
	Local locals[LOCALS_MAX];
	uint32_t local_count;
	uint32_t scope_depth;
} Compiler;

static Parser parser;
static Compiler * current_compiler = NULL;

typedef struct Chunk Chunk;

static Chunk * compiling_chunk;
static Chunk * current_chunk(void) {
	return compiling_chunk;
}

// errors
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

// scanning
static void compiler_advance(void) {
	parser.previous = parser.current;

	for (;;) {
		parser.current = scan_token();
		if (parser.current.type != TOKEN_ERROR) { break; }
		error_at_current(parser.current.start);
	}
}

static void syncronize(void) {
	parser.panic_mode = false;

	while (parser.current.type != TOKEN_EOF) {
		if (parser.previous.type == TOKEN_SEMICOLON) { return; }

		switch (parser.current.type) {
			case TOKEN_CLASS:
			case TOKEN_FUN:
			case TOKEN_VAR:
			case TOKEN_FOR:
			case TOKEN_IF:
			case TOKEN_WHILE:
			case TOKEN_PRINT:
			case TOKEN_RETURN:
				return;
			default: break;
		}

		compiler_advance();
	}
}

static void consume(Token_Type type, char const * message) {
	if (parser.current.type == type) {
		compiler_advance();
		return;
	}

	error_at_current(message);
}

static bool match(Token_Type type) {
	if (parser.current.type != type) { return false; }
	compiler_advance();
	return true;
}

static bool identifiers_equal(Token * a, Token * b) {
	if (a->length != b->length) { return false; }
	return memcmp(a->start, b->start, sizeof(char) * a->length) == 0;
}

// emitting
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

static void compiler_init(Compiler * compiler) {
	compiler->local_count = 0;
	compiler->scope_depth = 0;
	current_compiler = compiler;
}

static void compiler_end(void) {
	emit_byte(OP_RETURN);
#if defined(DEBUG_PRINT_CODE)
	if (!parser.had_error) {
		chunk_disassemble(current_chunk(), "code");
	}
#endif // DEBUG_PRINT_CODE
}

// parsing
static Parse_Rule * get_rule(Token_Type type);
static void parse_presedence(Precedence precedence) {
	(void)precedence;
	compiler_advance();
	Parse_Fn * prefix_rule = get_rule(parser.previous.type)->prefix;
	if (prefix_rule == NULL) {
		error("expected an expression");
		return;
	}

	bool can_assign = precedence <= PREC_ASSIGNMENT;
	prefix_rule(can_assign);

	while (precedence <= get_rule(parser.current.type)->precedence) {
		compiler_advance();
		Parse_Fn * infix_rule = get_rule(parser.previous.type)->infix;
		infix_rule(can_assign);
	}

	if (can_assign && match(TOKEN_EQUAL)) {
		error("invalid assignment target");
	}
}

static uint8_t identifier_constant(Token * name) {
	return make_constant(TO_OBJ(copy_string(name->start, name->length)));
}

static void add_local(Token name) {
	if (current_compiler->local_count == LOCALS_MAX) {
		error("too many local variables");
		return;
	}
	Local * local = &current_compiler->locals[current_compiler->local_count++];
	local->name = name;
	local->depth = current_compiler->scope_depth;
}

static void declare_variable(void) {
	if (current_compiler->scope_depth == 0) { return; }
	Token * name = &parser.previous;
	for (uint32_t i = current_compiler->local_count; i-- > 0;) {
		Local * local = &current_compiler->locals[i];
		if (local->depth != UINT32_MAX && local->depth < current_compiler->scope_depth) {
			break;
		}

		if (identifiers_equal(name, &local->name)) {
			error("a varaible redeclaration");
		}
	}
	add_local(*name);
}

static uint8_t parse_variable(char const * error_message) {
	consume(TOKEN_IDENTIFIER, error_message);

	declare_variable();
	if (current_compiler->scope_depth > 0) { return 0; }

	return identifier_constant(&parser.previous);
}

static void define_variable(uint8_t global) {
	if (current_compiler->scope_depth > 0) { return; }
	emit_bytes(OP_DEFINE_GLOBAL, global);
}

static void do_expression(void);
static void named_variable(Token name, bool can_assign) {
	uint8_t arg = identifier_constant(&name);
	if (can_assign && match(TOKEN_EQUAL)) {
		do_expression();
		emit_bytes(OP_SET_GLOBAL, arg);
	}
	else {
		emit_bytes(OP_GET_GLOBAL, arg);
	}
}

// expressions
static void do_expression(void) {
	parse_presedence(PREC_ASSIGNMENT);
}

static void do_number(bool can_assign) {
	(void)can_assign;
	Value value = TO_NUMBER(strtod(parser.previous.start, NULL));
	emit_constant(value);
}

static void do_unary(bool can_assign) {
	(void)can_assign;
	Token_Type operator_type = parser.previous.type;
	// compile the operand
	parse_presedence(PREC_UNARY);
	// emit the operator instruction
	switch (operator_type) {
		case TOKEN_BANG:  emit_byte(OP_NOT); break;
		case TOKEN_MINUS: emit_byte(OP_NEGATE); break;
		default: return; // unreachable
	}
}

static void do_binary(bool can_assign) {
	(void)can_assign;
	Token_Type operator_type = parser.previous.type;
	// compile the right operand
	Parse_Rule * rule = get_rule(operator_type);
	parse_presedence((Precedence)(rule->precedence + 1));
	// emit the operator instruction
	switch (operator_type) {
		case TOKEN_BANG_EQUAL:    emit_bytes(OP_EQUAL, OP_NOT); break;
		case TOKEN_EQUAL_EQUAL:   emit_byte(OP_EQUAL); break;
		case TOKEN_GREATER:       emit_byte(OP_GREATER); break;
		case TOKEN_GREATER_EQUAL: emit_bytes(OP_LESS, OP_NOT); break;
		case TOKEN_LESS:          emit_byte(OP_LESS); break;
		case TOKEN_LESS_EQUAL:    emit_bytes(OP_GREATER, OP_NOT); break;

		case TOKEN_PLUS:  emit_byte(OP_ADD); break;
		case TOKEN_MINUS: emit_byte(OP_SUBTRACT); break;
		case TOKEN_STAR:  emit_byte(OP_MULTIPLY); break;
		case TOKEN_SLASH: emit_byte(OP_DIVIDE); break;
		default: return; // unreachable
	}
}

static void do_literal(bool can_assign) {
	(void)can_assign;
	Token_Type operator_type = parser.previous.type;
	switch (operator_type) {
		case TOKEN_NIL:   emit_byte(OP_NIL); break;
		case TOKEN_FALSE: emit_byte(OP_FALSE); break;
		case TOKEN_TRUE:  emit_byte(OP_TRUE); break;
		default: return; // unreachable
	}
}

typedef struct Obj Obj;

static void do_string(bool can_assign) {
	(void)can_assign;
	emit_constant(TO_OBJ(
		(Obj *)copy_string(parser.previous.start + 1, parser.previous.length - 2)
	));
}

static void do_variable(bool can_assign) {
	named_variable(parser.previous, can_assign);
}

static void do_grouping(bool can_assign) {
	(void)can_assign;
	do_expression();
	consume(TOKEN_RIGHT_PAREN, "expected a ')");
}

// statements
static void do_print_statement(void) {
	do_expression();
	consume(TOKEN_SEMICOLON, "expected a ';'");
	emit_byte(OP_PRINT);
}

static void  do_expression_statement(void) {
	do_expression();
	consume(TOKEN_SEMICOLON, "expected a ';'");
	emit_byte(OP_POP);
}

static void begin_scope(void) {
	current_compiler->scope_depth++;
}

static void end_scope(void) {
	current_compiler->scope_depth--;

	while (current_compiler->local_count > 0 && current_compiler->locals[current_compiler->local_count - 1].depth > current_compiler->scope_depth) {
		emit_byte(OP_POP);
		current_compiler->local_count--;
	}
}

static void do_declaration(void);
static void do_block(void) {
	while (parser.current.type != TOKEN_EOF && parser.current.type != TOKEN_RIGHT_BRACE) {
		do_declaration();
	}
	consume(TOKEN_RIGHT_BRACE, "expected a '}");
}

static void do_statement(void) {
	if (match(TOKEN_PRINT)) {
		do_print_statement();
	}
	else if (match(TOKEN_LEFT_BRACE)) {
		begin_scope();
		do_block();
		end_scope();
	}
	else {
		do_expression_statement();
	}
}

static void do_var_declaration(void) {
	uint8_t global = parse_variable("expected a variable name");

	if (match(TOKEN_EQUAL)) {
		do_expression();
	}
	else {
		emit_byte(OP_NIL);
	}

	consume(TOKEN_SEMICOLON, "expected a ';");

	define_variable(global);
}

static void do_declaration(void) {
	if (match(TOKEN_VAR)) {
		do_var_declaration();
	}
	else {
		do_statement();
	}

	if (parser.panic_mode) { syncronize(); }
}

//
bool compile(char const * source, Chunk * chunk) {
	scanner_init(source);

	Compiler compiler;
	compiler_init(&compiler);

	compiling_chunk = chunk;

	parser.had_error = false;
	parser.panic_mode = false;

	compiler_advance();
	while (!match(TOKEN_EOF)) {
		do_declaration();
	}
	compiler_end();

	return !parser.had_error;
}

// data
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
	[TOKEN_BANG]          = {do_unary,    NULL,      PREC_NONE},
	[TOKEN_BANG_EQUAL]    = {NULL,        do_binary, PREC_EQUALITY},
	// [TOKEN_EQUAL]         = {NULL,        NULL,      PREC_NONE},
	[TOKEN_EQUAL_EQUAL]   = {NULL,        do_binary, PREC_EQUALITY},
	[TOKEN_GREATER]       = {NULL,        do_binary, PREC_COMPARISON},
	[TOKEN_GREATER_EQUAL] = {NULL,        do_binary, PREC_COMPARISON},
	[TOKEN_LESS]          = {NULL,        do_binary, PREC_COMPARISON},
	[TOKEN_LESS_EQUAL]    = {NULL,        do_binary, PREC_COMPARISON},
	[TOKEN_IDENTIFIER]    = {do_variable, NULL,      PREC_NONE},
	[TOKEN_STRING]        = {do_string,   NULL,      PREC_NONE},
	[TOKEN_NUMBER]        = {do_number,   NULL,      PREC_NONE},
	// [TOKEN_AND]           = {NULL,        NULL,      PREC_NONE},
	// [TOKEN_CLASS]         = {NULL,        NULL,      PREC_NONE},
	// [TOKEN_ELSE]          = {NULL,        NULL,      PREC_NONE},
	[TOKEN_FALSE]         = {do_literal,  NULL,      PREC_NONE},
	// [TOKEN_FOR]           = {NULL,        NULL,      PREC_NONE},
	// [TOKEN_FUN]           = {NULL,        NULL,      PREC_NONE},
	// [TOKEN_IF]            = {NULL,        NULL,      PREC_NONE},
	[TOKEN_NIL]           = {do_literal,  NULL,      PREC_NONE},
	// [TOKEN_OR]            = {NULL,        NULL,      PREC_NONE},
	// [TOKEN_PRINT]         = {NULL,        NULL,      PREC_NONE},
	// [TOKEN_RETURN]        = {NULL,        NULL,      PREC_NONE},
	// [TOKEN_SUPER]         = {NULL,        NULL,      PREC_NONE},
	// [TOKEN_THIS]          = {NULL,        NULL,      PREC_NONE},
	[TOKEN_TRUE]          = {do_literal,  NULL,      PREC_NONE},
	// [TOKEN_VAR]           = {NULL,        NULL,      PREC_NONE},
	// [TOKEN_WHILE]         = {NULL,        NULL,      PREC_NONE},
	// [TOKEN_ERROR]         = {NULL,        NULL,      PREC_NONE},
	[TOKEN_EOF]           = {NULL,        NULL,      PREC_NONE},
};

static Parse_Rule * get_rule(Token_Type type) {
	return &rules[type];
}
