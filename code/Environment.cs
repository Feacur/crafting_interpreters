using System.Collections.Generic;

class Environment
{
	private readonly Environment enclosing;
	private readonly Dictionary<string, object> variables = new Dictionary<string, object>();

	public Environment()
	{
		enclosing = null;
	}

	public Environment(Environment enclosing)
	{
		this.enclosing = enclosing;
	}

	public void Define(string name, object value)
	{
		variables[name] = value; // allow redifinition
		// variables.Add(name, value);
	}

	public void Assign(Token name, object value)
	{
		if (variables.ContainsKey(name.lexeme)) {
			variables[name.lexeme] = value;
			return;
		}
		if (enclosing != null) { enclosing.Assign(name, value); return; }
		throw new RuntimeErrorException(name, "undefined variable '" + name.lexeme + "'");
	}

	public object Get(Token name)
	{
		if (variables.TryGetValue(name.lexeme, out object value)) {
			return value;
		}
		if (enclosing != null) { return enclosing.Get(name); }
		throw new RuntimeErrorException(name, "undefined variable '" + name.lexeme + "'");
	}
}
