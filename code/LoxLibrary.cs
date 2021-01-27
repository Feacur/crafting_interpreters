using System;
using System.Collections.Generic;
using Any = System.Object;

namespace LoxLibrary
{
	public class Clock : ILoxCallable
	{
		int ILoxCallable.Arity() { return 0; }

		Any ILoxCallable.Call(AstInterpreter interpreter, List<Any> arguments)
		{
			return (double)DateTimeOffset.UtcNow.ToUnixTimeMilliseconds();
		}

		public override string ToString() => "<lib-fn " + GetType().Name + ">";
	}

	public class Print : ILoxCallable
	{
		int ILoxCallable.Arity() => 1;

		Any ILoxCallable.Call(AstInterpreter interpreter, List<Any> arguments)
		{
			Console.WriteLine(AstInterpreter.Stringify(arguments[0]));
			return null;
		}

		public override string ToString() => "<lib-fn " + GetType().Name + ">";
	}

	public class ConsoleRead : ILoxCallable
	{
		int ILoxCallable.Arity() => 0;

		Any ILoxCallable.Call(AstInterpreter interpreter, List<Any> arguments)
		{
			return (double)Console.Read();
		}

		public override string ToString() => "<lib-fn " + GetType().Name + ">";
	}
}
