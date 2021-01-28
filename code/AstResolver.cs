using System.Collections.Generic;
using Void = System.Object; // should be 'void', but C# doesn't allow generics to use that
using Scope = System.Collections.Generic.Dictionary<string, bool>;

public class AstResolver
	: Expr.IVisitor<Void>
	, Stmt.IVisitor<Void>
{
	private readonly AstInterpreter interpreter;
	private readonly Stack<Scope> scopes = new Stack<Scope>();
	private FunctionType currentFunction = FunctionType.NONE;
	private ClassType currentClass = ClassType.NONE;

	public AstResolver(AstInterpreter interpreter)
	{
		this.interpreter = interpreter;
	}

	public void Resolve(List<Stmt> statements)
	{
		foreach (var statement in statements) {
			Resolve(statement);
		}
	}

	private enum FunctionType {
		NONE,
		FUNCTION,
		INITIALIZER,
		METHOD,
	}

	private enum ClassType {
		NONE,
		CLASS,
	}

	private void Resolve(Expr expr) => expr.Accept(this);
	private void Resolve(Stmt statement) => statement.Accept(this);

	private void ResolveLocal(Expr expr, Token name)
	{
		// iterate from the last to the first
		int index = 0;
		foreach (Scope scope in scopes) {
			if (scope.ContainsKey(name.lexeme)) {
				interpreter.Resolve(expr, index);
				break;
			}
			index++;
		}
	}

	private void ResolveFunction(Stmt.Function function, FunctionType type)
	{
		FunctionType enclosingFunction = currentFunction;
		currentFunction = type;
		BeginScope();
		foreach (Token parameter in function.parameters) {
			Declare(parameter);
			Define(parameter);
		}
		Resolve(function.body);
		EndScope();
		currentFunction = enclosingFunction;
	}

	private void Declare(Token name)
	{
		if (scopes.Count == 0) { return; }
		Scope scope = scopes.Peek();
		if (scope.ContainsKey(name.lexeme)) {
			Lox.Error(name, "redeclaration of a variable");
		}
		scope[name.lexeme] = false;
	}

	private void Define(Token name)
	{
		if (scopes.Count == 0) { return; }
		scopes.Peek()[name.lexeme] = true;
	}

	private void BeginScope() => scopes.Push(new Dictionary<string, bool>());

	private void EndScope() => scopes.Pop();
	
	// Expr.IVisitor<Void>
	Void Expr.IVisitor<Void>.VisitAssignExpr(Expr.Assign expr)
	{
		Resolve(expr.value);
		ResolveLocal(expr, expr.name);
		return default;
	}

	Void Expr.IVisitor<Void>.VisitBinaryExpr(Expr.Binary expr)
	{
		Resolve(expr.left);
		Resolve(expr.right);
		return default;
	}

	Void Expr.IVisitor<Void>.VisitCallExpr(Expr.Call expr)
	{
		Resolve(expr.callee);
		foreach (Expr arguments in expr.arguments) {
			Resolve(arguments);
		}
		return default;
	}

	Void Expr.IVisitor<Void>.VisitGetExpr(Expr.Get expr)
	{
		Resolve(expr.obj);
		return default;
	}

	Void Expr.IVisitor<Void>.VisitGroupingExpr(Expr.Grouping expr)
	{
		Resolve(expr.expression);
		return default;
	}

	Void Expr.IVisitor<Void>.VisitLiteralExpr(Expr.Literal expr)
	{
		// empty
		return default;
	}

	Void Expr.IVisitor<Void>.VisitLogicalExpr(Expr.Logical expr)
	{
		Resolve(expr.left);
		Resolve(expr.right);
		return default;
	}

	Void Expr.IVisitor<Void>.VisitSetExpr(Expr.Set expr)
	{
		Resolve(expr.value);
		Resolve(expr.obj);
		return default;
	}

	Void Expr.IVisitor<Void>.VisitThisExpr(Expr.This expr)
	{
		if (currentClass == ClassType.NONE) {
			Lox.Error(expr.keyword, "'this' is only valid inside a method");
		}
		ResolveLocal(expr, expr.keyword);
		return default;
	}

	Void Expr.IVisitor<Void>.VisitUnaryExpr(Expr.Unary expr)
	{
		Resolve(expr.right);
		return default;
	}

	Void Expr.IVisitor<Void>.VisitVariableExpr(Expr.Variable expr)
	{
		if (scopes.Count > 0 && scopes.Peek().TryGetValue(expr.name.lexeme, out bool defined) && !defined) {
			Lox.Error(expr.name, "can't read local variable in its own initializer");
		}
		ResolveLocal(expr, expr.name);
		return default;
	}

	// Stmt.IVisitor<Void>
	Void Stmt.IVisitor<Void>.VisitBlockStmt(Stmt.Block stmt)
	{
		BeginScope();
		Resolve(stmt.statements);
		EndScope();
		return default;
	}

	Void Stmt.IVisitor<Void>.VisitClassStmt(Stmt.Class stmt)
	{
		ClassType enclosingClass = currentClass;
		currentClass = ClassType.CLASS;
		Declare(stmt.name);
		Define(stmt.name);
		BeginScope();
		scopes.Peek()["this"] = true;
		foreach (Stmt.Function method in stmt.methods) {
			FunctionType type = FunctionType.METHOD;
			if (method.name.lexeme == "init") {
				type = FunctionType.INITIALIZER;
			}
			ResolveFunction(method, type);
		}
		EndScope();
		currentClass = enclosingClass;
		return default;
	}

	Void Stmt.IVisitor<Void>.VisitExpressionStmt(Stmt.Expression stmt)
	{
		Resolve(stmt.expression);
		return default;
	}

	Void Stmt.IVisitor<Void>.VisitFunctionStmt(Stmt.Function stmt)
	{
		Declare(stmt.name);
		Define(stmt.name);
		ResolveFunction(stmt, FunctionType.FUNCTION);
		return default;
	}

	Void Stmt.IVisitor<Void>.VisitIfStmt(Stmt.If stmt)
	{
		Resolve(stmt.condition);
		Resolve(stmt.thenBranch);
		if (stmt.elseBranch != null) { Resolve(stmt.elseBranch); }
		return default;
	}

	Void Stmt.IVisitor<Void>.VisitReturnStmt(Stmt.Return stmt)
	{
		if (currentFunction == FunctionType.NONE) {
			Lox.Error(stmt.keyword, "can't return from top-level code");
		}
		if (stmt.value != null) {
			if (currentFunction == FunctionType.INITIALIZER) {
				Lox.Error(stmt.keyword, "can't return a value from an initializer");
			}
			Resolve(stmt.value);
		}
		return default;
	}

	Void Stmt.IVisitor<Void>.VisitVarStmt(Stmt.Var stmt)
	{
		Declare(stmt.name);
		if (stmt.initializer != null) {
			Resolve(stmt.initializer);
		}
		Define(stmt.name);
		return default;
	}

	Void Stmt.IVisitor<Void>.VisitWhileStmt(Stmt.While stmt)
	{
		Resolve(stmt.condition);
		Resolve(stmt.body);
		return default;
	}
}
