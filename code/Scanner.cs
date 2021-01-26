using System.Collections.Generic;
using Any = System.Object;

public class Scanner
{
	private readonly string source;
	private readonly List<Token> tokens = new List<Token>();
	private int start = 0;
	private int current = 0;
	private int line = 0;

	public Scanner(string source)
	{
		this.source = source;
	}

	public List<Token> ScanTokens()
	{
		while (!IsAtEnd()) {
			start = current;
			ScanToken();
		}

		tokens.Add(new Token(TokenType.EOF, string.Empty, null, line));
		return tokens;
	}

	private void ScanToken()
	{
		char c = Advance();
		switch (c) {
			//
			case '(': AddToken(TokenType.LEFT_PAREN); break;
			case ')': AddToken(TokenType.RIGHT_PAREN); break;
			case '{': AddToken(TokenType.LEFT_BRACE); break;
			case '}': AddToken(TokenType.RIGHT_BRACE); break;
			case ',': AddToken(TokenType.COMMA); break;
			case '.': AddToken(TokenType.DOT); break;
			case '-': AddToken(TokenType.MINUS); break;
			case '+': AddToken(TokenType.PLUS); break;
			case ';': AddToken(TokenType.SEMICOLON); break;
			case '*': AddToken(TokenType.STAR); break;

			//
			case '!': AddToken(Match('=') ? TokenType.BANG_EQUAL : TokenType.BANG); break;
			case '=': AddToken(Match('=') ? TokenType.EQUAL_EQUAL : TokenType.EQUAL); break;
			case '<': AddToken(Match('=') ? TokenType.LESS_EQUAL : TokenType.LESS); break;
			case '>': AddToken(Match('=') ? TokenType.GREATER_EQUAL : TokenType.GREATER); break;

			//
			case '/': {
				if (Match('/')) {
					while (Peek() != '\n' && !IsAtEnd()) { Advance(); }
				}
				else {
					AddToken(TokenType.SLASH);
				}
			} break;

			//
			case ' ':
			case '\t':
			case '\r':
				break;

			case '\n': line++; break;

			//
			case '"': AddString(); break;

			default: {
				if (IsDigit(c)) { AddNumber(); break; }
				if (IsAlpha(c)) { AddIdentifier(); break; }
				Lox.Error(line, "unexpected character");
			} break;
		}
	}

	// tokenizers
	private void AddString()
	{
		while (Peek() != '"' && !IsAtEnd()) {
			if (Peek() == '\n') { line++; }
			Advance();
		}

		if (IsAtEnd()) {
			Lox.Error(line, "unterminated string");
			return;
		}

		Advance(); // consume closing quote

		string literal = source.Substring(start + 1, current - start - 2);
		AddToken(TokenType.STRING, literal);
	}

	private void AddNumber()
	{
		while (IsDigit(Peek())) { Advance(); }

		if (Peek() == '.' && IsDigit(PeekNext())) {
			Advance();
			while (IsDigit(Peek())) { Advance(); }
		}

		string literalString = source.Substring(start, current - start);
		double literal = double.Parse(literalString);
		AddToken(TokenType.NUMBER, literal);
	}

	private void AddIdentifier()
	{
		while (IsAlphaNumeric(Peek())) { Advance(); }

		string text = source.Substring(start, current - start);
		if (!keywords.TryGetValue(text, out TokenType type)) {
			type = TokenType.IDENTIFIER;
		}

		AddToken(type);
	}

	private void AddToken(TokenType type) => AddToken(type, null);

	private void AddToken(TokenType type, Any literal)
	{
		string lexeme = source.Substring(start, current - start);
		tokens.Add(new Token(type, lexeme, literal, line));
	}

	// primitives
	private bool Match(char expected)
	{
		if (IsAtEnd()) { return false; }
		if (source[current] == expected) { current++; return true; }
		return false;
	}

	private char Peek() => IsAtEnd() ? '\0' : source[current];

	private char PeekNext()
	{
		int index = current + 1;
		if (index >= source.Length) { return '\0'; }
		return source[index];
	}

	private char Advance() => source[current++];

	private bool IsAtEnd() => current >= source.Length;

	// data
	private static bool IsDigit(char c) => '0' <= c && c <= '9';

	private static bool IsAlpha(char c) => 'a' <= c && c <= 'z'
	                                    || 'A' <= c && c <= 'Z'
	                                    || c == '_';

	private static bool IsAlphaNumeric(char c) => IsAlpha(c) || IsDigit(c);

	private static Dictionary<string, TokenType> keywords = new Dictionary<string, TokenType> {
		{"and",    TokenType.AND},
		{"class",  TokenType.CLASS},
		{"else",   TokenType.ELSE},
		{"false",  TokenType.FALSE},
		{"for",    TokenType.FOR},
		{"fun",    TokenType.FUN},
		{"if",     TokenType.IF},
		{"nil",    TokenType.NIL},
		{"or",     TokenType.OR},
		{"return", TokenType.RETURN},
		{"super",  TokenType.SUPER},
		{"this",   TokenType.THIS},
		{"true",   TokenType.TRUE},
		{"var",    TokenType.VAR},
		{"while",  TokenType.WHILE},
	};
}
