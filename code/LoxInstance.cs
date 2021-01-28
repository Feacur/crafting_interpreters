using System.Collections.Generic;
using Any = System.Object;

public class LoxInstance
{
	private readonly LoxClass loxClass;
	private readonly Dictionary<string, Any> fields = new Dictionary<string, Any>();

	public LoxInstance(LoxClass loxClass)
	{
		this.loxClass = loxClass;
	}

	public Any Get(Token name)
	{
		if (fields.TryGetValue(name.lexeme, out Any value)) {
			return value;
		}

		LoxFunction method = loxClass.FindMethod(name.lexeme);
		if (method != null) { return method.Bind(this); }

		throw new RuntimeErrorException(name, "undefined property '" + name.lexeme + "'");
	}

	public void Set(Token name, Any value)
	{
		fields[name.lexeme] = value; // allow redifinition
		// fields.Add(name, value);
	}

	public override string ToString() => loxClass.ToString() + " instance";
}
