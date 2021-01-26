using System.Collections.Generic;
using Void = System.Object; // should be 'void', but C# doesn't allow generics to use that
using Any = System.Object;

public class AstInterpreter
	: Expr.IVisitor<Any>
	, Stmt.IVisitor<Void>
{
	private readonly Environment globals = new Environment();
	private Environment environment;

	public AstInterpreter()
	{
		environment = globals;
		globals.Define<LoxCallables.Clock>();
		globals.Define<LoxCallables.Print>();
	}

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

	private Any Evaluate(Expr expr) => expr.Accept(this);
	private void Execute(Stmt stmt) => stmt.Accept(this);

	internal void ExecuteBlock(List<Stmt> statements, Environment environment)
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
	private static void CheckNumberOperand(Token token, Any value)
	{
		if (value is double) { return; }
		throw new RuntimeErrorException(token, "expected a number");
	}

	private static void CheckNumberOperands(Token token, Any left, Any right)
	{
		if (left is double && right is double) { return; }
		throw new RuntimeErrorException(token, "expected numbers");
	}

	// Expr.IVisitor<Any>
	Any Expr.IVisitor<Any>.VisitAssignExpr(Expr.Assign expr)
	{
		Any value = Evaluate(expr.value);
		environment.Assign(expr.name, value);
		return value;
	}

	Any Expr.IVisitor<Any>.VisitBinaryExpr(Expr.Binary expr)
	{
		Any left = Evaluate(expr.left);
		Any right = Evaluate(expr.right);
		switch (expr.op.type) {
			case TokenType.PLUS: {
				if (left is string && right is string) { return (string)left + (string)right; }
				CheckNumberOperands(expr.op, left, right); return (double)left + (double)right;
			}
			case TokenType.MINUS: CheckNumberOperands(expr.op, left, right); return (double)left - (double)right;
			case TokenType.STAR:  CheckNumberOperands(expr.op, left, right); return (double)left * (double)right;
			case TokenType.SLASH: CheckNumberOperands(expr.op, left, right); return (double)left / (double)right;

			case TokenType.GREATER:       CheckNumberOperands(expr.op, left, right); return (double)left > (double)right;
			case TokenType.GREATER_EQUAL: CheckNumberOperands(expr.op, left, right); return (double)left >= (double)right;
			case TokenType.LESS:          CheckNumberOperands(expr.op, left, right); return (double)left < (double)right;
			case TokenType.LESS_EQUAL:    CheckNumberOperands(expr.op, left, right); return (double)left <= (double)right;

			case TokenType.BANG_EQUAL:  return !IsEqual(left, right);
			case TokenType.EQUAL_EQUAL: return IsEqual(left, right);
		}
		Lox.RuntimeError(expr.op, "wrong binary expression");
		return null;
	}

	Any Expr.IVisitor<Any>.VisitGroupingExpr(Expr.Grouping expr)
	{
		return Evaluate(expr.expression);
	}

	Any Expr.IVisitor<Any>.VisitCallExpr(Expr.Call expr)
	{
		Any callee = Evaluate(expr.callee);

		List<Any> arguments = new List<Any>();
		foreach (Expr argument in expr.arguments) {
			arguments.Add(Evaluate(argument));
		}

		if (!(callee is ILoxCallable)) {
			throw new RuntimeErrorException(expr.paren, "is not a callable");
		}

		ILoxCallable function = (ILoxCallable)callee;
		if (arguments.Count != function.Arity()) {
			throw new RuntimeErrorException(expr.paren, "expected " + function.Arity() + " arguments, got " + arguments.Count);
		}

		return function.Call(this, arguments);
	}

	Any Expr.IVisitor<Any>.VisitLiteralExpr(Expr.Literal expr)
	{
		return expr.value;
	}

	Any Expr.IVisitor<Any>.VisitLogicalExpr(Expr.Logical expr)
	{
		Any left = Evaluate(expr.left);

		switch (expr.op.type) {
			case TokenType.OR: {
				if (IsTrue(left)) { return left; }
			} break;

			case TokenType.AND: {
				if (!IsTrue(left)) { return left; }
			} break;
		}

		return Evaluate(expr.right);
	}

	Any Expr.IVisitor<Any>.VisitUnaryExpr(Expr.Unary expr)
	{
		Any value = Evaluate(expr.right);
		switch (expr.op.type) {
			case TokenType.BANG: return !IsTrue(value);
			case TokenType.MINUS: CheckNumberOperand(expr.op, value); return -(double)value;
		}
		Lox.RuntimeError(expr.op, "wrong unary expression");
		return null;
	}

	Any Expr.IVisitor<Any>.VisitVariableExpr(Expr.Variable expr)
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

	Void Stmt.IVisitor<Void>.VisitFunctionStmt(Stmt.Function stmt)
	{
		environment.Define(stmt.name.lexeme, new LoxCallables.Function(stmt, environment));
		return default;
	}

	Void Stmt.IVisitor<Void>.VisitIfStmt(Stmt.If stmt)
	{
		if (IsTrue(Evaluate(stmt.condition))) {
			Execute(stmt.thenBranch);
		}
		else if (stmt.elseBranch != null) {
			Execute(stmt.elseBranch);
		}
		return default;
	}

	Void Stmt.IVisitor<Void>.VisitReturnStmt(Stmt.Return stmt)
	{
		Any value = null;
		if (stmt.value != null) {
			value = Evaluate(stmt.value);
		}
		throw new ReturnException(value);
	}

	Void Stmt.IVisitor<Void>.VisitVarStmt(Stmt.Var stmt)
	{
		Any value = null;
		if (stmt.initializer != null) {
			value = Evaluate(stmt.initializer);
		}
		environment.Define(stmt.name.lexeme, value);
		return default;
	}

	Void Stmt.IVisitor<Void>.VisitWhileStmt(Stmt.While stmt)
	{
		while (IsTrue(Evaluate(stmt.condition))) {
			Execute(stmt.body);
		}
		return default;
	}

	// data
	private static bool IsTrue(Any value)
	{
		if (value == null) { return false; }
		if (value is bool) { return (bool)value; }
		return true;
	}
	
	private static bool IsEqual(Any left, Any right)
	{
		if (left == null && right == null) { return true; }
		if (left == null) { return false; }
		return left.Equals(right);
	}

	public static string Stringify(Any value)
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
