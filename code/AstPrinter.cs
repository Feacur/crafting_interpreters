using System.Text;

public class AstPrinter : Expr.IVisitor<string>
{
	public string Print(Expr expr) => expr.Accept(this);

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
}