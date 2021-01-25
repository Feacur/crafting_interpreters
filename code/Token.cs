using Any = System.Object;

public class Token
{
	public readonly TokenType type;
	public readonly string lexeme;
	public readonly Any literal;
	public readonly int line;

	public Token(TokenType type, string lexeme, Any literal, int line)
	{
		this.type = type;
		this.lexeme = lexeme;
		this.literal = literal;
		this.line = line;
	}

	public override string ToString()
	{
		return type + " " + lexeme + " " + literal;
	}
}
