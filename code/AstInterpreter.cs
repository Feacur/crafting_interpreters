public class AstInterpreter : Expr.IVisitor<object>
{
	public void Interpret(Expr expression)
	{
		try
		{
			object value = Evaluate(expression);
			System.Console.WriteLine(Stringify(value));
		}
		catch (RuntimeErrorException e)
		{
			Lox.RuntimeError(e.token, e.Message);
		}
	}

	private object Evaluate(Expr expr) => expr.Accept(this);

	private class RuntimeErrorException : System.Exception
	{
		public Token token;

		public RuntimeErrorException(Token token, string message) : base(message) {
			this.token = token;
		}
	}

	// recovery
	private void CheckNumberOperand(Token token, object value)
	{
		if (value is double) { return; }
		throw new RuntimeErrorException(token, "value must be a number");
	}

	private void CheckNumberOperands(Token token, object left, object right)
	{
		if (left is double && right is double) { return; }
		throw new RuntimeErrorException(token, "values must be numbers");
	}

	// Expr.IVisitor<object>
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
