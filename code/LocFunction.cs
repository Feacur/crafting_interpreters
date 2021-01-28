using System.Collections.Generic;
using Any = System.Object;

public class LoxFunction : ILoxCallable
{
	private readonly Stmt.Function declaration;
	private readonly Environment closure;
	private readonly bool isInitializer;

	public LoxFunction(Stmt.Function declaration, Environment closure, bool isInitializer)
	{
		this.declaration = declaration;
		this.closure = closure;
		this.isInitializer = isInitializer;
	}

	public LoxFunction Bind(LoxInstance instance)
	{
		Environment environment = new Environment(closure);
		environment.Define("this", instance);
		return new LoxFunction(declaration, environment, isInitializer);
	}

	int ILoxCallable.Arity() => declaration.parameters.Count;

	Any ILoxCallable.Call(AstInterpreter interpreter, List<Any> arguments)
	{
		Environment environment = new Environment(closure);
		for (int i = 0; i < declaration.parameters.Count; i++) {
			environment.Define(declaration.parameters[i].lexeme, arguments[i]);
		}
		try {
			interpreter.ExecuteBlock(declaration.body, environment);
		}
		catch (ReturnException e) {
			if (isInitializer) { return closure.GetAt(0, "this"); }
			return e.value;
		}
		if (isInitializer) { return closure.GetAt(0, "this"); }
		return null;
	}

	public override string ToString() => "<fn " + declaration.name.lexeme + ">";
}
