using System.Collections.Generic;
using Any = System.Object;

public class LoxClass : ILoxCallable
{
	private readonly string name;
	private readonly Dictionary<string, LoxFunction> methods;

	public LoxClass(string name, Dictionary<string, LoxFunction> methods)
	{
		this.name = name;
		this.methods = methods;
	}

	public LoxFunction FindMethod(string name)
	{
		methods.TryGetValue(name, out LoxFunction method);
		return method;
	}

	int ILoxCallable.Arity() => 0;

	Any ILoxCallable.Call(AstInterpreter interpreter, List<Any> arguments)
	{
		LoxInstance instance = new LoxInstance(this);
		return instance;
	}

	public override string ToString() => name;
}
