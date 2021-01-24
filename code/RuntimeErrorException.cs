public class RuntimeErrorException : System.Exception
{
	public Token token;

	public RuntimeErrorException(Token token, string message) : base(message) {
		this.token = token;
	}
}
