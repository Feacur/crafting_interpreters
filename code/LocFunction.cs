using System.Collections.Generic;
using Any = System.Object;

public class LoxFunction : ILoxCallable
{
	private readonly Stmt.Function declaration;
	private readonly Environment closure;

	public LoxFunction(Stmt.Function declaration, Environment closure)
	{
		this.declaration = declaration;
		this.closure = closure;
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
			return e.value;
		}
		return null;
	}

	public override string ToString() => "<fn " + declaration.name.lexeme + ">";
}
