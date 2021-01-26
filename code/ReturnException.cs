using Any = System.Object;

public class ReturnException : System.Exception
{
	public Any value;

	public ReturnException(Any value)
	{
		this.value = value;
	}
}
