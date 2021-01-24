using System.Text;

public class AstPrinter
	: Expr.IVisitor<string>
	, Stmt.IVisitor<string>
{
	public string Print(Expr expr) => expr.Accept(this);
	public string Print(Stmt stmt) => stmt.Accept(this);

	private string Parenthesize(string name, params Expr[] exprs)
	{
		StringBuilder builder = new StringBuilder();
		builder.Append("(").Append(name);
		foreach (Expr expr in exprs)
		{
			builder.Append(" ").Append(Print(expr));
		}
		builder.Append(")");
		return builder.ToString();
	}

	// Expr.IVisitor<string>
	string Expr.IVisitor<string>.VisitAssignExpr(Expr.Assign expr)
	{
		return "(= " + expr.name.lexeme + " " + Print(expr.value) + ")";
	}

	string Expr.IVisitor<string>.VisitBinaryExpr(Expr.Binary expr)
	{
		return Parenthesize(expr.token.lexeme, expr.left, expr.right);
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

	string Expr.IVisitor<string>.VisitUnaryExpr(Expr.Unary expr)
	{
		return Parenthesize(expr.token.lexeme, expr.right);
	}

	string Expr.IVisitor<string>.VisitVariableExpr(Expr.Variable expr)
	{
		return expr.name.lexeme;
	}

	// Stmt.IVisitor<string>
	string Stmt.IVisitor<string>.VisitBlockStmt(Stmt.Block stmt)
	{
		StringBuilder builder = new StringBuilder();
		builder.Append("(block ");
		foreach (Stmt statement in stmt.statements)
		{
			builder.Append(Print(statement));
		}
		builder.Append(")");
		return builder.ToString();
	}

	string Stmt.IVisitor<string>.VisitExpressionStmt(Stmt.Expression stmt)
	{
		return Parenthesize(";", stmt.expression);
	}

	string Stmt.IVisitor<string>.VisitPrintStmt(Stmt.Print stmt)
	{
		return Parenthesize("print", stmt.expression);
	}

	string Stmt.IVisitor<string>.VisitVarStmt(Stmt.Var stmt)
	{
		if (stmt.initializer != null) {
			return "(var " + stmt.name.lexeme + "=" + Print(stmt.initializer) + ")";
		}
		return "(var " + stmt.name.lexeme + ")";
	}
}