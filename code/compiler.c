#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "object.h"
#include "compiler.h"
#include "scanner.h"

#if defined(DEBUG_PRINT_BYTECODE)
#include "debug.h"
#endif // DEBUG_PRINT_BYTECODE

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
	bool is_captured;
} Local;

typedef enum {
	TYPE_FUNCTION,
	TYPE_SCRIPT,
} Function_Type;

typedef struct {
	uint8_t index;
	bool is_local;
} Upvalue;

typedef struct Obj_Function Obj_Function;

typedef struct Compiler {
	struct Compiler * enclosing;
	Obj_Function * function;
	Function_Type type;
	Local locals[LOCALS_MAX];
	Upvalue upvalues[LOCALS_MAX];
	uint32_t local_count;
	uint32_t scope_depth;
} Compiler;

static Parser parser;
static Compiler * current_compiler = NULL;

typedef struct Chunk Chunk;

static Chunk * current_chunk(void) {
	return &current_compiler->function->chunk;
}

// errors
static void error_at(Token * token, char const * message) {
	DEBUG_BREAK();
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

static bool compiler_match(Token_Type type) {
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
	if (constant == LOCALS_MAX) {
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

static uint32_t emit_jump(Op_Code instruction) {
	emit_byte((uint8_t)instruction);
	emit_byte(0xff);
	emit_byte(0xff);
	return current_chunk()->count - 2;
}

static void patch_jump(uint32_t target) {
	uint32_t jump = current_chunk()->count - target - 2;
	if (jump > UINT16_MAX) {
		error("too much code to jump over");
	}
	current_chunk()->code[target] = (jump >> 8) & 0xff;
	current_chunk()->code[target + 1] = jump & 0xff;
}

static void emit_loop(uint32_t target) {
	emit_byte(OP_LOOP);
	uint32_t loop = current_chunk()->count - target + 2;
	if (loop > UINT16_MAX) {
		error("too much code to loop over");
	}
	emit_byte((loop >> 8) & 0xff);
	emit_byte(loop & 0xff);
}

static void compiler_init(Compiler * compiler, Function_Type type) {
	compiler->enclosing = current_compiler;
	compiler->type = type;
	compiler->local_count = 0;
	compiler->scope_depth = 0;

	// GC protection
	compiler->function = NULL;
	compiler->function = new_function();

	current_compiler = compiler;

	if (type != TYPE_SCRIPT) {
		compiler->function->name = copy_string(parser.previous.start, parser.previous.length);
	}

	Local * local = &compiler->locals[compiler->local_count++];
	local->depth = 0;
	local->is_captured = false;
	local->name.start = "";
	local->name.length = 0;
}

static Obj_Function * compiler_end(void) {
	emit_bytes(OP_NIL, OP_RETURN);

	Obj_Function * function = current_compiler->function;

#if defined(DEBUG_PRINT_BYTECODE)
	if (!parser.had_error) {
		chunk_disassemble(current_chunk(), function->name != NULL ? function->name->chars : "<script>");
		printf("\n");
	}
#endif // DEBUG_PRINT_BYTECODE

	current_compiler = current_compiler->enclosing;
	return function;
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

	if (can_assign && compiler_match(TOKEN_EQUAL)) {
		error("invalid assignment target");
	}
}

typedef struct Obj_String Obj_String;

static uint8_t identifier_constant(Token * name) {
	Obj_String * obj_name = copy_string(name->start, name->length);
	return make_constant(TO_OBJ(obj_name));
}

static uint32_t resolve_local(Compiler * compiler, Token * name) {
	for (uint32_t i = compiler->local_count; i-- > 0;) {
		Local * local = &compiler->locals[i];
		if (identifiers_equal(name, &local->name)) {
			if (local->depth == UINT32_MAX) {
				error("can't read local variable in its own initializer");
			}
			return i;
		}
	}
	return UINT32_MAX;
}

static uint32_t add_upvalue(Compiler * compiler, uint8_t index, bool is_local) {
	uint32_t upvalue_count = compiler->function->upvalue_count;
	for (uint32_t i = 0; i < upvalue_count; i++) {
		Upvalue * upvalue = &compiler->upvalues[i];
		if (upvalue->index == index && upvalue->is_local == is_local) {
			return i;
		}
	}
	if (upvalue_count == LOCALS_MAX) {
		error("too many closure variables");
		return 0;
	}
	compiler->upvalues[upvalue_count].index = index;
	compiler->upvalues[upvalue_count].is_local = is_local;
	return compiler->function->upvalue_count++;
}

static uint32_t resolve_upvalue(Compiler * compiler, Token * name) {
	if (compiler->enclosing == NULL) { return UINT32_MAX; }

	uint32_t local = resolve_local(compiler->enclosing, name);
	if (local != UINT32_MAX) {
		compiler->enclosing->locals[local].is_captured = true;
		return add_upvalue(compiler, (uint8_t)local, true);
	}

	uint32_t upvalue = resolve_upvalue(compiler->enclosing, name);
	if (upvalue != UINT32_MAX) {
		return add_upvalue(compiler, (uint8_t)upvalue, false);
	}

	return UINT32_MAX;
}

static void add_local(Token name) {
	if (current_compiler->local_count == LOCALS_MAX) {
		error("too many local variables");
		return;
	}
	Local * local = &current_compiler->locals[current_compiler->local_count++];
	local->name = name;
	local->depth = UINT32_MAX;
	local->is_captured = false;
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

static void mark_initialized(void) {
	if (current_compiler->scope_depth == 0) { return; }
	current_compiler->locals[current_compiler->local_count - 1].depth = current_compiler->scope_depth;
}

static void define_variable(uint8_t global) {
	if (current_compiler->scope_depth > 0) {
		mark_initialized();
		return;
	}
	emit_bytes(OP_DEFINE_GLOBAL, global);
}

static void do_expression(void);
static uint8_t argument_list(void) {
	uint8_t arg_count = 0;
	if (parser.current.type != TOKEN_RIGHT_PAREN) {
		do {
			if (arg_count == UINT8_MAX) {
				error("can't have more that 255 arguments");
			}
			arg_count++;
			do_expression();
		} while (compiler_match(TOKEN_COMMA));
	}
	consume(TOKEN_RIGHT_PAREN, "expected a ')'");
	return arg_count;
}

static void do_expression(void);
static void named_variable(Token name, bool can_assign) {
	Op_Code get_op, set_op;
	uint32_t arg;
	if ((arg = resolve_local(current_compiler, &name)) != UINT32_MAX) {
		get_op = OP_GET_LOCAL;
		set_op = OP_SET_LOCAL;
	}
	else if ((arg = resolve_upvalue(current_compiler, &name)) != UINT32_MAX) {
		get_op = OP_GET_UPVALUE;
		set_op = OP_SET_UPVALUE;
	}
	else {
		arg = identifier_constant(&name);
		get_op = OP_GET_GLOBAL;
		set_op = OP_SET_GLOBAL;
	}

	if (can_assign && compiler_match(TOKEN_EQUAL)) {
		do_expression();
		emit_bytes((uint8_t)set_op, (uint8_t)arg);
	}
	else {
		emit_bytes((uint8_t)get_op, (uint8_t)arg);
	}
}

static void begin_scope(void) {
	current_compiler->scope_depth++;
}

static void end_scope(void) {
	current_compiler->scope_depth--;

	while (current_compiler->local_count > 0 && current_compiler->locals[current_compiler->local_count - 1].depth > current_compiler->scope_depth) {
		if (current_compiler->locals[current_compiler->local_count - 1].is_captured) {
			emit_byte(OP_CLOSE_UPVALUE);
		}
		else {
			emit_byte(OP_POP);
		}
		current_compiler->local_count--;
	}
}

// expressions
static void do_expression(void) {
	parse_presedence(PREC_ASSIGNMENT);
}

static void do_number(bool can_assign) {
	(void)can_assign;
	double number = strtod(parser.previous.start, NULL);
	emit_constant(TO_NUMBER(number));
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

static void do_and(bool can_assign) {
	(void)can_assign;
	uint32_t end_jump = emit_jump(OP_JUMP_IF_FALSE);
	emit_byte(OP_POP);
	parse_presedence(PREC_AND);
	patch_jump(end_jump);
}

static void do_or(bool can_assign) {
	(void)can_assign;
	uint32_t else_jump = emit_jump(OP_JUMP_IF_FALSE);
	uint32_t end_jump = emit_jump(OP_JUMP);

	patch_jump(else_jump);
	emit_byte(OP_POP);

	parse_presedence(PREC_OR);
	patch_jump(end_jump);
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
	Obj_String * string = copy_string(parser.previous.start + 1, parser.previous.length - 2);
	emit_constant(TO_OBJ(string));
}

static void do_variable(bool can_assign) {
	named_variable(parser.previous, can_assign);
}

static void do_grouping(bool can_assign) {
	(void)can_assign;
	do_expression();
	consume(TOKEN_RIGHT_PAREN, "expected a ')");
}

static void do_call(bool can_assign) {
	(void)can_assign;
	uint8_t arg_count = argument_list();
	emit_bytes(OP_CALL, arg_count);
}

static void do_dot(bool can_assign) {
	consume(TOKEN_IDENTIFIER, "expected an identifier");
	uint8_t name = identifier_constant(&parser.previous);

	if (can_assign && compiler_match(TOKEN_EQUAL)) {
		do_expression();
		emit_bytes(OP_SET_PROPERTY, name);
	}
	else {
		emit_bytes(OP_GET_PROPERTY, name);
	}
}

static void do_block(void);
static void do_function(Function_Type type) {
	Compiler compiler;
	compiler_init(&compiler, type);
	begin_scope();

	consume(TOKEN_LEFT_PAREN, "expected a '(");
	if (parser.current.type != TOKEN_RIGHT_PAREN) {
		do {
			if (current_compiler->function->arity == UINT8_MAX) {
				error_at_current("can't have more that 255 parameters");
			}
			current_compiler->function->arity++;
			uint8_t param_constant = parse_variable("expected a parameter name");
			define_variable(param_constant);
		} while (compiler_match(TOKEN_COMMA));
	}
	consume(TOKEN_RIGHT_PAREN, "expected a ')");

	consume(TOKEN_LEFT_BRACE, "expected a '{");
	do_block();

	// redundant: OP_RETURN does this implicitly
	// end_scope();

	Obj_Function * function = compiler_end();

	uint8_t function_constant = make_constant(TO_OBJ(function));
	if (function->upvalue_count > 0) {
		emit_bytes(OP_CLOSURE, function_constant);
		for (uint32_t i = 0; i < function->upvalue_count; i++) {
			emit_bytes(
				compiler.upvalues[i].index,
				compiler.upvalues[i].is_local ? 1 : 0
			);
		}
	}
	else {
		emit_bytes(OP_CONSTANT, function_constant);
	}
}

// statements
static void  do_expression_statement(void) {
	do_expression();
	consume(TOKEN_SEMICOLON, "expected a ';'");
	emit_byte(OP_POP);
}

static void do_var_declaration(void) {
	uint8_t global = parse_variable("expected a variable name");

	if (compiler_match(TOKEN_EQUAL)) {
		do_expression();
	}
	else {
		emit_byte(OP_NIL);
	}

	consume(TOKEN_SEMICOLON, "expected a ';");

	define_variable(global);
}

static void do_fun_declaration(void) {
	uint8_t global = parse_variable("expected a function name");
	mark_initialized();
	do_function(TYPE_FUNCTION);
	define_variable(global);
}

static void do_class_declaration(void) {
	consume(TOKEN_IDENTIFIER, "expected a class name");
	uint8_t name_constant = identifier_constant(&parser.previous);
	declare_variable();

	emit_bytes(OP_CLASS, name_constant);
	define_variable(name_constant);

	consume(TOKEN_LEFT_BRACE, "expected a '{'");
	consume(TOKEN_RIGHT_BRACE, "expected a '}'");
}

static void do_declaration(void);
static void do_block(void) {
	while (parser.current.type != TOKEN_EOF && parser.current.type != TOKEN_RIGHT_BRACE) {
		do_declaration();
	}
	consume(TOKEN_RIGHT_BRACE, "expected a '}");
}

static void do_statement(void);
static void do_if_statement(void) {
	consume(TOKEN_LEFT_PAREN, "expected a '('");
	do_expression();
	consume(TOKEN_RIGHT_PAREN, "expected a ')'");

	uint32_t then_jump = emit_jump(OP_JUMP_IF_FALSE);
	emit_byte(OP_POP);
	do_statement();

	uint32_t else_jump = emit_jump(OP_JUMP);
	patch_jump(then_jump);

	emit_byte(OP_POP);
	if (compiler_match(TOKEN_ELSE)) { do_statement(); }
	patch_jump(else_jump);
}

static void do_while_statement(void) {
	consume(TOKEN_LEFT_PAREN, "expected a '('");

	uint32_t loop_start = current_chunk()->count;
	printf("jump to %d\n", (uint32_t)(current_chunk()->count));
	do_expression();

	consume(TOKEN_RIGHT_PAREN, "expected a ')'");

	uint32_t exit_jump = emit_jump(OP_JUMP_IF_FALSE);
	emit_byte(OP_POP);

	do_statement();
	printf("jump from1 %d\n", (uint32_t)(current_chunk()->count));
	emit_loop(loop_start);
	printf("jump from2 %d\n", (uint32_t)(current_chunk()->count));

	patch_jump(exit_jump);
	emit_byte(OP_POP);
}

static void do_for_statement(void) {
	begin_scope();

	consume(TOKEN_LEFT_PAREN, "expected a '('");

	if (compiler_match(TOKEN_SEMICOLON)) {
		// no initializer
	}
	else if (compiler_match(TOKEN_VAR)) {
		do_var_declaration();
	}
	else {
		do_expression_statement();
	}

	uint32_t loop_start = current_chunk()->count;

	uint32_t exit_jump = UINT32_MAX;
	if (!compiler_match(TOKEN_SEMICOLON)) {
		do_expression();
		consume(TOKEN_SEMICOLON, "expected a ';'");

		exit_jump = emit_jump(OP_JUMP_IF_FALSE);
		emit_byte(OP_POP);
	}

	if (!compiler_match(TOKEN_RIGHT_PAREN)) {
		uint32_t body_jump = emit_jump(OP_JUMP);
		uint32_t per_loop_start = current_chunk()->count;

		do_expression();
		emit_byte(OP_POP);
		consume(TOKEN_RIGHT_PAREN, "expected a ')'");

		emit_loop(loop_start);
		loop_start = per_loop_start;
		patch_jump(body_jump);
	}

	do_statement();
	emit_loop(loop_start);

	if (exit_jump != UINT32_MAX) {
		patch_jump(exit_jump);
		emit_byte(OP_POP);
	}

	end_scope();
}

static void do_return_statement(void) {
	if (current_compiler->type == TYPE_SCRIPT) {
		error("can't return from top-level code");
	}

	if (compiler_match(TOKEN_SEMICOLON)) {
		emit_bytes(OP_NIL, OP_RETURN);
	}
	else {
		do_expression();
		consume(TOKEN_SEMICOLON, "expected a ';'");
		emit_byte(OP_RETURN);
	}
}

static void do_statement(void) {
	if (compiler_match(TOKEN_IF)) {
		do_if_statement();
	}
	else if (compiler_match(TOKEN_WHILE)) {
		do_while_statement();
	}
	else if (compiler_match(TOKEN_FOR)) {
		do_for_statement();
	}
	else if (compiler_match(TOKEN_RETURN)) {
		do_return_statement();
	}
	else if (compiler_match(TOKEN_LEFT_BRACE)) {
		begin_scope();
		do_block();
		end_scope();
	}
	else {
		do_expression_statement();
	}
}

static void do_declaration(void) {
	if (compiler_match(TOKEN_VAR)) {
		do_var_declaration();
	}
	else if (compiler_match(TOKEN_FUN)) {
		do_fun_declaration();
	}
	else if (compiler_match(TOKEN_CLASS)) {
		do_class_declaration();
	}
	else {
		do_statement();
	}

	if (parser.panic_mode) { syncronize(); }
}

//
Obj_Function * compile(char const * source) {
	scanner_init(source);

	Compiler compiler;
	compiler_init(&compiler, TYPE_SCRIPT);

	parser.had_error = false;
	parser.panic_mode = false;

	compiler_advance();
	while (!compiler_match(TOKEN_EOF)) {
		do_declaration();
	}
	Obj_Function * function = compiler_end();
	return parser.had_error ? NULL : function;
}

void gc_mark_compiler_roots_grey(void) {
	for (Compiler * compiler = current_compiler; compiler != NULL; compiler = compiler->enclosing) {
		gc_mark_object_grey((Obj *)compiler->function);
	}
}

// data
static Parse_Rule rules[] = {
	[TOKEN_LEFT_PAREN]    = {do_grouping, do_call,   PREC_CALL},
	// [TOKEN_RIGHT_PAREN]   = {NULL,        NULL,      PREC_NONE},
	// [TOKEN_LEFT_BRACE]    = {NULL,        NULL,      PREC_NONE},
	// [TOKEN_RIGHT_BRACE]   = {NULL,        NULL,      PREC_NONE},
	// [TOKEN_COMMA]         = {NULL,        NULL,      PREC_NONE},
	[TOKEN_DOT]           = {NULL,        do_dot,    PREC_CALL},
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
	[TOKEN_AND]           = {NULL,        do_and,    PREC_AND},
	// [TOKEN_CLASS]         = {NULL,        NULL,      PREC_NONE},
	// [TOKEN_ELSE]          = {NULL,        NULL,      PREC_NONE},
	[TOKEN_FALSE]         = {do_literal,  NULL,      PREC_NONE},
	// [TOKEN_FOR]           = {NULL,        NULL,      PREC_NONE},
	// [TOKEN_FUN]           = {NULL,        NULL,      PREC_NONE},
	// [TOKEN_IF]            = {NULL,        NULL,      PREC_NONE},
	[TOKEN_NIL]           = {do_literal,  NULL,      PREC_NONE},
	[TOKEN_OR]            = {NULL,        do_or,     PREC_OR},
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
