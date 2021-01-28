using System.Collections;
using System.Runtime.CompilerServices;
using System.Text;

public class AstPrinter
	: Expr.IVisitor<string>
	, Stmt.IVisitor<string>
{
	public string Print(Expr expr) => expr.Accept(this);
	public string Print(Stmt stmt) => stmt.Accept(this);

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	private string Parenthesize(params object[] values)
	{
		StringBuilder builder = new StringBuilder();
		builder.Append("(");
		foreach (object value in values) {
			AppendBuilder(builder, value);
		}
		builder.Append(")");
		return builder.ToString();
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	private void AppendBuilder(StringBuilder builder, object value)
	{
		if (value is Expr) { builder.Append(Print((Expr)value)); return; }
		if (value is Stmt) { builder.Append(Print((Stmt)value)); return; }
		if (value is Token) { builder.Append(((Token)value).lexeme); return; }
		if (value is ICollection) { foreach (object it in (ICollection)value) {
			AppendBuilder(builder, it);
		} return; }
		builder.Append(value);
	}

	// Expr.IVisitor<string>
	string Expr.IVisitor<string>.VisitAssignExpr(Expr.Assign expr)
	{
		return Parenthesize("=", expr.name, expr.value);
	}

	string Expr.IVisitor<string>.VisitBinaryExpr(Expr.Binary expr)
	{
		return Parenthesize(expr.op.lexeme, expr.left, expr.right);
	}

	string Expr.IVisitor<string>.VisitCallExpr(Expr.Call expr)
	{
		return Parenthesize(expr.callee, expr.arguments);
	}

	string Expr.IVisitor<string>.VisitGetExpr(Expr.Get expr)
	{
		return Parenthesize("get", expr.obj, expr.name);
	}

	string Expr.IVisitor<string>.VisitGroupingExpr(Expr.Grouping expr)
	{
		return Parenthesize("group", expr.expression);
	}

	string Expr.IVisitor<string>.VisitLiteralExpr(Expr.Literal expr)
	{
		if (expr.value == null) { return "nil"; }
		return expr.value.ToString();
	}

	string Expr.IVisitor<string>.VisitLogicalExpr(Expr.Logical expr)
	{
		return Parenthesize(expr.op.lexeme, expr.left, expr.right);
	}

	string Expr.IVisitor<string>.VisitSetExpr(Expr.Set expr)
	{
		return Parenthesize("set", expr.name, expr.value);
	}

	string Expr.IVisitor<string>.VisitThisExpr(Expr.This expr)
	{
		return expr.keyword.lexeme;
	}

	string Expr.IVisitor<string>.VisitUnaryExpr(Expr.Unary expr)
	{
		return Parenthesize(expr.op.lexeme, expr.right);
	}

	string Expr.IVisitor<string>.VisitVariableExpr(Expr.Variable expr)
	{
		return expr.name.lexeme;
	}

	// Stmt.IVisitor<string>
	string Stmt.IVisitor<string>.VisitBlockStmt(Stmt.Block stmt)
	{
		return Parenthesize("block", stmt.statements);
	}

	string Stmt.IVisitor<string>.VisitClassStmt(Stmt.Class stmt)
	{
		if (stmt.superclass != null) {
			return Parenthesize("class", stmt.name, "<", stmt.superclass, stmt.methods);
		}
		return Parenthesize("class", stmt.name, stmt.methods);
	}

	string Stmt.IVisitor<string>.VisitExpressionStmt(Stmt.Expression stmt)
	{
		return Parenthesize(";", stmt.expression);
	}

	string Stmt.IVisitor<string>.VisitFunctionStmt(Stmt.Function stmt)
	{
		return Parenthesize(stmt.name, stmt.parameters, stmt.body);
	}

	string Stmt.IVisitor<string>.VisitIfStmt(Stmt.If stmt)
	{
		if (stmt.elseBranch == null) {
			return Parenthesize("if", stmt.condition, stmt.thenBranch);
		}
		return Parenthesize("if", stmt.condition, stmt.thenBranch, stmt.elseBranch);
	}

	string Stmt.IVisitor<string>.VisitReturnStmt(Stmt.Return stmt)
	{
		return Parenthesize("return", stmt.value);
	}

	string Stmt.IVisitor<string>.VisitVarStmt(Stmt.Var stmt)
	{
		if (stmt.initializer == null) {
			return Parenthesize("var", stmt.name);
		}
		return Parenthesize("var", stmt.name, "=", stmt.initializer);
	}

	string Stmt.IVisitor<string>.VisitWhileStmt(Stmt.While stmt)
	{
		return Parenthesize("while", stmt.condition, stmt.body);
	}
}
