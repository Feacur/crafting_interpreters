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

	public List<Stmt> Parse()
	{
		List<Stmt> statements = new List<Stmt>();
		while (!IsAtEnd()) {
			statements.Add(DoStatement());
		}
		return statements;
	}

	private class ParseErrorException : System.Exception
	{
		public ParseErrorException(string message) : base(message) { }
	}

	// statement rules
	private Stmt DoStatement()
	{
		try {
			if (Match(TokenType.CLASS)) { return DoClassStatement(); }
			if (Match(TokenType.FUN)) { return DoFunStatement("function"); }
			if (Match(TokenType.VAR)) { return DoVarStatement(); }
			if (Match(TokenType.FOR)) { return DoForStatement(); }
			if (Match(TokenType.IF)) { return DoIfStatement(); }
			if (Match(TokenType.RETURN)) { return DoReturnStatement(); }
			if (Match(TokenType.WHILE)) { return DoWhileStatement(); }
			if (Match(TokenType.LEFT_BRACE)) { return DoBlockStatement(); }
			return DoExpressionStatement();
		}
		catch (ParseErrorException) {
			Synchronize();
			return null;
		}
	}

	private Stmt DoClassStatement()
	{
		Token name = Consume(TokenType.IDENTIFIER, "expected a class name");

		Expr.Variable superclass = null;
		if (Match(TokenType.LESS)) {
			Consume(TokenType.IDENTIFIER, "expected a superclass name");
			superclass = new Expr.Variable(PeekPrev());
		}

		Consume(TokenType.LEFT_BRACE, "expected a '{'");

		List<Stmt.Function> methods = new List<Stmt.Function>();
		while (!IsAtEnd() && Peek().type != TokenType.RIGHT_BRACE) {
			methods.Add(DoFunStatement("method"));
		}

		Consume(TokenType.RIGHT_BRACE, "expected a '}'");

		return new Stmt.Class(name, superclass, methods);
	}

	private Stmt.Function DoFunStatement(string kind)
	{
		Token name = Consume(TokenType.IDENTIFIER, "expected a " + kind + " name");

		Consume(TokenType.LEFT_PAREN, "expected a '(");
		List<Token> parameters = new List<Token>();
		if (!IsAtEnd() && Peek().type != TokenType.RIGHT_PAREN) {
			do {
				if (parameters.Count >= 255) {
					Error(Peek(), "parameters count is limited to 255");
				}
				parameters.Add(Consume(TokenType.IDENTIFIER, "expected a parameter name"));
			} while (Match(TokenType.COMMA));
		}
		Consume(TokenType.RIGHT_PAREN, "expected a ')");

		Consume(TokenType.LEFT_BRACE, "expected a '{");
		List<Stmt> body = DoBlock();

		return new Stmt.Function(name, parameters, body);
	}

	private Stmt DoVarStatement()
	{
		Token name = Consume(TokenType.IDENTIFIER, "expected a variable name");

		Expr initializer = null;
		if (Match(TokenType.EQUAL)) {
			initializer = DoExpression();
		}

		Consume(TokenType.SEMICOLON, "expected a ';'");
		return new Stmt.Var(name, initializer);
	}

	private Stmt DoWhileStatement()
	{
		Consume(TokenType.LEFT_PAREN, "expected a '('");
		Expr condition = DoExpression();
		Consume(TokenType.RIGHT_PAREN, "expected a ')'");

		Stmt body = DoStatement();

		return new Stmt.While(condition, body);
	}

	private Stmt DoForStatement()
	{
		/*
		for (initializer; condition; expression) {
			body;
		}
		==
		{
			initializer;
			while (condition) {
				body;
				expression;
			}
		}
		*/
		Consume(TokenType.LEFT_PAREN, "expected a '('");

		Stmt initializer;
		if (Match(TokenType.SEMICOLON)) {
			initializer = null;
		}
		else if (Match(TokenType.VAR)) {
			initializer = DoVarStatement();
		}
		else {
			initializer = DoExpressionStatement();
		}

		Expr condition = null;
		if (!IsAtEnd() && Peek().type != TokenType.SEMICOLON) {
			condition = DoExpression();
		}
		Consume(TokenType.SEMICOLON, "expected a ';'");

		Expr expression = null;
		if (!IsAtEnd() && Peek().type != TokenType.SEMICOLON) {
			expression = DoExpression();
		}

		Consume(TokenType.RIGHT_PAREN, "expected a ')'");

		Stmt body = DoStatement();

		if (expression != null) {
			body = new Stmt.Block(new List<Stmt> {
				body, new Stmt.Expression(expression)
			});
		}

		if (condition == null) { condition = new Expr.Literal(true); }
		body = new Stmt.While(condition, body);

		if (initializer != null) {
			body = new Stmt.Block(new List<Stmt> {
				initializer, body
			});
		}

		return body;
	}

	private Stmt DoIfStatement()
	{
		Consume(TokenType.LEFT_PAREN, "expected a '('");
		Expr condition = DoExpression();
		Consume(TokenType.RIGHT_PAREN, "expected a ')'");

		Stmt thenBranch = DoStatement();

		Stmt elseBranch = null;
		if (Match(TokenType.ELSE)) {
			elseBranch = DoStatement();
		}

		return new Stmt.If(condition, thenBranch, elseBranch);
	}

	private Stmt DoReturnStatement()
	{
		Token keyword = PeekPrev();
		Expr value = null;
		if (!IsAtEnd() && Peek().type != TokenType.SEMICOLON) {
			value = DoExpression();
		}

		Consume(TokenType.SEMICOLON, "expected a ';'");
		return new Stmt.Return(keyword, value);
	}

	private Stmt DoBlockStatement() => new Stmt.Block(DoBlock());

	private List<Stmt> DoBlock()
	{
		List<Stmt> statements = new List<Stmt>();
		while (!IsAtEnd() && Peek().type != TokenType.RIGHT_BRACE) {
			statements.Add(DoStatement());
		}
		Consume(TokenType.RIGHT_BRACE, "expected a '}'");
		return statements;
	}

	private Stmt DoExpressionStatement()
	{
		Expr expr = DoExpression();
		Consume(TokenType.SEMICOLON, "expected a ';'");
		return new Stmt.Expression(expr);
	}

	// expression rules
	private Expr DoExpression() => DoAssignment();

	private Expr DoAssignment()
	{
		Expr expr = DoOr();

		if (Match(TokenType.EQUAL)) {
			Token equals = PeekPrev();
			Expr value = DoAssignment();

			if (expr is Expr.Variable) {
				Token name = ((Expr.Variable)expr).name;
				return new Expr.Assign(name, value);
			}
			else if (expr is Expr.Get) {
				Expr.Get get = (Expr.Get)expr;
				return new Expr.Set(get.obj, get.name, value);
			}

			Error(equals, "invalid assignment target"); // no throw
		}

		return expr;
	}

	private Expr DoOr() => DoLogical(DoAnd, TokenType.OR);

	private Expr DoAnd() => DoLogical(DoEquality, TokenType.AND);

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	private Expr DoLogical(System.Func<Expr> rule, params TokenType[] types)
	{
		Expr expr = rule.Invoke();
		while (Match(types)) {
			Token token = PeekPrev();
			Expr right = rule.Invoke();
			expr = new Expr.Logical(expr, token, right);
		}
		return expr;
	}

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
		return DoCall();
	}

	private Expr DoCall()
	{
		Expr expr = DoPrimary();

		while (true) {
			if (Match(TokenType.LEFT_PAREN)) {
				List<Expr> arguments = new List<Expr>();
				if (!IsAtEnd() && Peek().type != TokenType.RIGHT_PAREN) {
					do {
						if (arguments.Count >= 255) {
							Error(Peek(), "arguments count is limited to 255");
						}
						arguments.Add(DoExpression());
					} while (Match(TokenType.COMMA));
				}
				Token paren = Consume(TokenType.RIGHT_PAREN, "expected a ')");
				expr = new Expr.Call(expr, paren, arguments);
			}
			else if (Match(TokenType.DOT)) {
				Token name = Consume(TokenType.IDENTIFIER, "expected a property name");
				expr = new Expr.Get(expr, name);
			}
			else {
				break;
			}
		}

		return expr;
	}

	private Expr DoPrimary()
	{
		if (Match(TokenType.FALSE)) { return new Expr.Literal(false); }
		if (Match(TokenType.TRUE)) { return new Expr.Literal(true); }
		if (Match(TokenType.NIL)) { return new Expr.Literal(null); }

		if (Match(TokenType.NUMBER, TokenType.STRING)) {
			return new Expr.Literal(PeekPrev().literal);
		}

		if (Match(TokenType.THIS)) {
			return new Expr.This(PeekPrev());
		}

		if (Match(TokenType.IDENTIFIER)) {
			return new Expr.Variable(PeekPrev());
		}

		if (Match(TokenType.LEFT_PAREN)) {
			Expr expr = DoExpression();
			Consume(TokenType.RIGHT_PAREN, "expected ')' after expression");
			return new Expr.Grouping(expr);
		}

		throw Error(Peek(), "expected an expression");
	}

	// recovery
	private Token Consume(TokenType type, string message)
	{
		if (Peek().type == type) { return Advance(); }
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

	private bool IsAtEnd() => (current >= tokens.Count) || (Peek().type == TokenType.EOF);
}
