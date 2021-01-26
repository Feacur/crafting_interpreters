using System.Collections.Generic;
using Any = System.Object;

public interface ILoxCallable
{
	int Arity();
	Any Call(AstInterpreter interpreter, List<Any> arguments);
}
