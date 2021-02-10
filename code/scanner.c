#include <stdio.h>
#include <string.h>

#include "common.h"
#include "scanner.h"

typedef struct {
	char const * start;
	char const * current;
	uint32_t line;
} Scanner;

static Scanner scanner;

void scanner_init(char const * source) {
	scanner.start = source;
	scanner.current = source;
	scanner.line = 1;
}

static bool is_at_end(void) {
	return *scanner.current == '\0';
}

static Token make_token(Token_Type type) {
	return (Token) {
		.type = type,
		.start = scanner.start,
		.length = (uint32_t)(scanner.current - scanner.start),
		.line = scanner.line,
	};
}

static Token make_error_token(char const * message) {
	return (Token) {
		.type = TOKEN_ERROR,
		.start = message,
		.length = (uint32_t)strlen(message),
		.line = scanner.line,
	};
}

static char scanner_advance(void) {
	return *(scanner.current++);
}

static bool match(char expected) {
	if (is_at_end()) { return false; }
	if (*scanner.current != expected) { return false; }
	scanner.current++;
	return true;
}

static char peek(void) {
	return *scanner.current;
}

static char peek_next(void) {
	if (is_at_end()) { return '\0'; }
	return *(scanner.current + 1);
}

static void skip_whitespace(void) {
	for (;;) {
		char c = peek();
		switch (c) {
			case ' ':
			case '\t':
			case '\r':
				scanner_advance();
				break;

			case '\n':
				scanner.line++;
				scanner_advance();
				break;

			case '/':
				if (peek_next() == '/') {
					while (!is_at_end() && peek() != '\n') { scanner_advance(); }
				}
				else {
					return;
				}
				break;

			default:
				return;
		}
	}
}

static Token make_string_token(void) {
	while (!is_at_end() && peek() != '"') {
		if (peek() == '\n') { scanner.line++; }
		scanner_advance();
	}

	if (is_at_end()) { return make_error_token("unterminated string"); }

	scanner_advance();
	return make_token(TOKEN_STRING);
}

static bool is_digit(char c) {
	return '0' <= c && c <= '9';
}

static bool is_alpha(char c) {
	return ('a' <= c && c <= 'z')
	    || ('A' <= c && c <= 'Z')
	    || c == '_';
}

static Token make_number_token(void) {
	while (is_digit(peek())) { scanner_advance(); }

	if (peek() == '.' && is_digit(peek_next())) {
		scanner_advance();
		while (is_digit(peek())) { scanner_advance(); }
	}

	return make_token(TOKEN_NUMBER);
}

static Token_Type check_keyword(uint32_t start, uint32_t length, char const * rest, Token_Type type) {
	if (scanner.current - scanner.start == start + length && memcmp(scanner.start + start, rest, length) == 0) {
		return type;
	}
	return TOKEN_IDENTIFIER;
}

static Token_Type identifier_type(void) {
	switch (scanner.start[0]) {
		case 'a': return check_keyword(1, 2, "nd", TOKEN_AND);
		case 'c': return check_keyword(1, 4, "lass", TOKEN_CLASS);
		case 'e': return check_keyword(1, 3, "lse", TOKEN_ELSE);
		case 'f':
			if (scanner.current - scanner.start > 1) {
				switch (scanner.start[1]) {
					case 'a': return check_keyword(2, 3, "lse", TOKEN_FALSE);
					case 'o': return check_keyword(2, 1, "r", TOKEN_FOR);
					case 'u': return check_keyword(2, 1, "n", TOKEN_FUN);
				}
			}
			break;
		case 'i': return check_keyword(1, 1, "f", TOKEN_IF);
		case 'n': return check_keyword(1, 2, "il", TOKEN_NIL);
		case 'o': return check_keyword(1, 1, "r", TOKEN_OR);
		case 'r': return check_keyword(1, 5, "eturn", TOKEN_RETURN);
		case 's': return check_keyword(1, 4, "uper", TOKEN_SUPER);
		case 't':
			if (scanner.current - scanner.start > 1) {
				switch (scanner.start[1]) {
					case 'h': return check_keyword(2, 2, "is", TOKEN_THIS);
					case 'r': return check_keyword(2, 2, "ue", TOKEN_TRUE);
				}
			}
			break;
		case 'v': return check_keyword(1, 2, "ar", TOKEN_VAR);
		case 'w': return check_keyword(1, 4, "hile", TOKEN_WHILE);
	}
	return TOKEN_IDENTIFIER;
}

static Token make_identifier_token(void) {
	while (is_alpha(peek()) || is_digit(peek())) { scanner_advance(); }
	return make_token(identifier_type());
}

Token scan_token(void) {
	skip_whitespace();

	scanner.start = scanner.current;

	if (is_at_end()) { return make_token(TOKEN_EOF); }

	char c = scanner_advance();
	if (is_alpha(c)) { return make_identifier_token(); }
	if (is_digit(c)) { return make_number_token(); }

	switch (c) {
		case '(': return make_token(TOKEN_LEFT_PAREN);
		case ')': return make_token(TOKEN_RIGHT_PAREN);
		case '{': return make_token(TOKEN_LEFT_BRACE);
		case '}': return make_token(TOKEN_RIGHT_BRACE);
		case ';': return make_token(TOKEN_SEMICOLON);
		case ',': return make_token(TOKEN_COMMA);
		case '.': return make_token(TOKEN_DOT);
		case '-': return make_token(TOKEN_MINUS);
		case '+': return make_token(TOKEN_PLUS);
		case '/': return make_token(TOKEN_SLASH);
		case '*': return make_token(TOKEN_STAR);

		case '!':
			return make_token(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
		case '=':
			return make_token(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
		case '<':
			return make_token(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
		case '>':
			return make_token(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);

		case '"': return make_string_token();
	}

	return make_error_token("unexpected character");
}

// data
