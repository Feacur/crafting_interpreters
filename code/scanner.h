#if !defined(LOX_SCANNER)
#define LOX_SCANNER

#include "common.h"

typedef enum {
	TOKEN_NONE,

	// single-character tokens
	TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN,
	TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,
	TOKEN_COMMA, TOKEN_DOT, TOKEN_MINUS, TOKEN_PLUS,
	TOKEN_SEMICOLON, TOKEN_SLASH, TOKEN_STAR,

	// one or two character tokens
	TOKEN_BANG, TOKEN_BANG_EQUAL,
	TOKEN_EQUAL, TOKEN_EQUAL_EQUAL,
	TOKEN_GREATER, TOKEN_GREATER_EQUAL,
	TOKEN_LESS, TOKEN_LESS_EQUAL,

	// literals
	TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_NUMBER,

	// keywords
	TOKEN_AND, TOKEN_CLASS, TOKEN_ELSE, TOKEN_FALSE,
	TOKEN_FOR, TOKEN_FUN, TOKEN_IF, TOKEN_NIL, TOKEN_OR,
	TOKEN_PRINT, TOKEN_RETURN, TOKEN_SUPER, TOKEN_THIS,
	TOKEN_TRUE, TOKEN_VAR, TOKEN_WHILE,

	//
	TOKEN_ERROR,
	TOKEN_EOF,
} Token_Type;

typedef struct {
	Token_Type type;
	char const * start;
	uint32_t length;
	uint32_t line;
} Token;

void scanner_init(char const * source);
Token scan_token(void);

#endif
