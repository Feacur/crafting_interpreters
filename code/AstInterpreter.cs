using System.Collections.Generic;
using Void = System.Object; // should be 'void', but C# doesn't allow generics to use that

public class AstInterpreter
	: Expr.IVisitor<object>
	, Stmt.IVisitor<Void>
{
	private Environment environment = new Environment();

	public void Interpret(List<Stmt> statements)
	{
		try {
			foreach (Stmt statement in statements) {
				Execute(statement);
			}
		}
		catch (RuntimeErrorException e) {
			Lox.RuntimeError(e.token, e.Message);
		}
	}

	private object Evaluate(Expr expr) => expr.Accept(this);
	private void Execute(Stmt stmt) => stmt.Accept(this);

	private void ExecuteBlock(List<Stmt> statements, Environment environment)
	{
		Environment previous = this.environment;
		try {
			this.environment = environment;
			foreach (var statement in statements) {
				Execute(statement);
			}
		}
		finally {
			this.environment = previous;
		}
	}

	// recovery
	private void CheckNumberOperand(Token token, object value)
	{
		if (value is double) { return; }
		throw new RuntimeErrorException(token, "expected a number");
	}

	private void CheckNumberOperands(Token token, object left, object right)
	{
		if (left is double && right is double) { return; }
		throw new RuntimeErrorException(token, "expected numbers");
	}

	// Expr.IVisitor<object>
	object Expr.IVisitor<object>.VisitAssignExpr(Expr.Assign expr)
	{
		object value = Evaluate(expr.value);
		environment.Assign(expr.name, value);
		return value;
	}

	object Expr.IVisitor<object>.VisitBinaryExpr(Expr.Binary expr)
	{
		object left = Evaluate(expr.left);
		object right = Evaluate(expr.right);
		switch (expr.token.type) {
			case TokenType.PLUS: {
				if (left is string && right is string) { return (string)left + (string)right; }
				CheckNumberOperands(expr.token, left, right); return (double)left + (double)right;
			}
			case TokenType.MINUS: CheckNumberOperands(expr.token, left, right); return (double)left - (double)right;
			case TokenType.STAR:  CheckNumberOperands(expr.token, left, right); return (double)left * (double)right;
			case TokenType.SLASH: CheckNumberOperands(expr.token, left, right); return (double)left / (double)right;

			case TokenType.GREATER:       CheckNumberOperands(expr.token, left, right); return (double)left > (double)right;
			case TokenType.GREATER_EQUAL: CheckNumberOperands(expr.token, left, right); return (double)left >= (double)right;
			case TokenType.LESS:          CheckNumberOperands(expr.token, left, right); return (double)left < (double)right;
			case TokenType.LESS_EQUAL:    CheckNumberOperands(expr.token, left, right); return (double)left <= (double)right;

			case TokenType.BANG_EQUAL:  return !IsEqual(left, right);
			case TokenType.EQUAL_EQUAL: return IsEqual(left, right);
		}
		Lox.RuntimeError(expr.token, "wrong binary expression");
		return null;
	}

	object Expr.IVisitor<object>.VisitGroupingExpr(Expr.Grouping expr)
	{
		return Evaluate(expr.expression);
	}

	object Expr.IVisitor<object>.VisitLiteralExpr(Expr.Literal expr)
	{
		return expr.value;
	}

	object Expr.IVisitor<object>.VisitUnaryExpr(Expr.Unary expr)
	{
		object value = Evaluate(expr.right);
		switch (expr.token.type) {
			case TokenType.BANG: return !IsTrue(value);
			case TokenType.MINUS: CheckNumberOperand(expr.token, value); return -(double)value;
		}
		Lox.RuntimeError(expr.token, "wrong unary expression");
		return null;
	}

	object Expr.IVisitor<object>.VisitVariableExpr(Expr.Variable expr)
	{
		return environment.Get(expr.name);
	}

	// Stmt.IVisitor<string>
	Void Stmt.IVisitor<Void>.VisitBlockStmt(Stmt.Block stmt)
	{
		ExecuteBlock(stmt.statements, new Environment(environment));
		return default;
	}

	Void Stmt.IVisitor<Void>.VisitExpressionStmt(Stmt.Expression stmt)
	{
		Evaluate(stmt.expression);
		return default;
	}

	Void Stmt.IVisitor<Void>.VisitPrintStmt(Stmt.Print stmt)
	{
		object value = Evaluate(stmt.expression);
		System.Console.WriteLine(Stringify(value));
		return default;
	}

	Void Stmt.IVisitor<Void>.VisitVarStmt(Stmt.Var stmt)
	{
		object value = null;
		if (stmt.initializer != null) {
			value = Evaluate(stmt.initializer);
		}
		environment.Define(stmt.name.lexeme, value);
		return default;
	}

	// data
	private static bool IsTrue(object value)
	{
		if (value == null) { return false; }
		if (value is bool) { return (bool)value; }
		return true;
	}
	
	private static bool IsEqual(object left, object right)
	{
		if (left == null && right == null) { return true; }
		if (left == null) { return false; }
		return left.Equals(right);
	}

	private static string Stringify(object value)
	{
		if (value == null) { return "nil"; }
		if (value is double) {
			double number = (double)value;
			if (double.IsNaN(number)) { return "nan"; }
			if (double.IsNegativeInfinity(number)) { return "-inf"; }
			if (double.IsPositiveInfinity(number)) { return "inf"; }
			return number.ToString("0.######");
		}
		return value.ToString();
	}
}
