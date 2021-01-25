using System.Collections.Generic;
using Any = System.Object;

class Environment
{
	private readonly Environment enclosing;
	private readonly Dictionary<string, Any> variables = new Dictionary<string, Any>();

	public Environment()
	{
		enclosing = null;
	}

	public Environment(Environment enclosing)
	{
		this.enclosing = enclosing;
	}

	public void Define(string name, Any value)
	{
		variables[name] = value; // allow redifinition
		// variables.Add(name, value);
	}

	public void Assign(Token name, Any value)
	{
		if (variables.ContainsKey(name.lexeme)) {
			variables[name.lexeme] = value;
			return;
		}
		if (enclosing != null) { enclosing.Assign(name, value); return; }
		throw new RuntimeErrorException(name, "undefined variable '" + name.lexeme + "'");
	}

	public Any Get(Token name)
	{
		if (variables.TryGetValue(name.lexeme, out Any value)) {
			return value;
		}
		if (enclosing != null) { return enclosing.Get(name); }
		throw new RuntimeErrorException(name, "undefined variable '" + name.lexeme + "'");
	}
}
