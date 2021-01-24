using System.Collections.Generic;
using System.Runtime.CompilerServices;

public class Parser
{
	private readonly List<Token> tokens;
	private int current = 0;

	public Parser(List<Token> tokens)
	{
		this.tokens = tokens;
	}

	public Expr Parse()
	{
		try
		{
			return DoExpression();
		}
		catch (ParseErrorException)
		{
			return null;
		}
	}

	private class ParseErrorException : System.Exception
	{
		public ParseErrorException(string message) : base(message) { }
	}

	// rules
	private Expr DoExpression() => DoEquality();

	private Expr DoEquality() => DoBinary(DoComparison, TokenType.BANG_EQUAL, TokenType.EQUAL_EQUAL);

	private Expr DoComparison() => DoBinary(DoTerm, TokenType.GREATER, TokenType.GREATER_EQUAL, TokenType.LESS, TokenType.LESS_EQUAL);

	private Expr DoTerm() => DoBinary(DoFactor, TokenType.MINUS, TokenType.PLUS);

	private Expr DoFactor() => DoBinary(DoUnary, TokenType.SLASH, TokenType.STAR);

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	private Expr DoBinary(System.Func<Expr> rule, params TokenType[] types)
	{
		Expr expr = rule.Invoke();
		while (Match(types)) {
			Token token = PeekPrev();
			Expr right = rule.Invoke();
			expr = new Expr.Binary(expr, token, right);
		}
		return expr;
	}

	private Expr DoUnary()
	{
		if (Match(TokenType.BANG, TokenType.MINUS)) {
			Token token = PeekPrev();
			Expr right = DoUnary();
			return new Expr.Unary(token, right);
		}
		return DoPrimary();
	}

	private Expr DoPrimary()
	{
		if (Match(TokenType.FALSE)) { return new Expr.Literal(false); }
		if (Match(TokenType.TRUE)) { return new Expr.Literal(true); }
		if (Match(TokenType.NIL)) { return new Expr.Literal(null); }

		if (Match(TokenType.NUMBER, TokenType.STRING)) {
			return new Expr.Literal(PeekPrev().literal);
		}

		if (Match(TokenType.LEFT_PAREN)) {
			Expr expr = DoExpression();
			Consume(TokenType.RIGHT_PAREN, "expected ')' after expression");
			return new Expr.Grouping(expr);
		}

		throw Error(Peek(), "expected an expression");
	}

	// recovery
	private void Consume(TokenType type, string message)
	{
		if (Peek().type == type) { Advance(); return; }
		throw Error(Peek(), message);
	}

	private ParseErrorException Error(Token token, string message)
	{
		Lox.Error(token, message);
		return new ParseErrorException(message);
	}

	private void Synchronize()
	{
		Advance();

		while (!IsAtEnd()) {
			if (PeekPrev().type == TokenType.SEMICOLON) { break; }

			switch (Peek().type) {
				case TokenType.CLASS:
				case TokenType.FUN:
				case TokenType.VAR:
				case TokenType.FOR:
				case TokenType.IF:
				case TokenType.WHILE:
				case TokenType.PRINT:
				case TokenType.RETURN:
					break;
			}

			Advance();
		}
	}

	// primitives
	private bool Match(params TokenType[] types)
	{
		if (IsAtEnd()) { return false; }
		foreach (TokenType type in types) {
			if (Peek().type == type) { current++; return true; }
		}
		return false;
	}

	private Token Peek() => tokens[current];

	private Token PeekPrev() => tokens[current - 1];

	private Token Advance() => tokens[current++];

	private bool IsAtEnd() => Peek().type == TokenType.EOF;
}
