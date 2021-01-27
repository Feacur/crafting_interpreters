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

		public override string ToString() { return "<lib-fn " + GetType().Name + ">"; }
	}

	public class Print : ILoxCallable
	{
		int ILoxCallable.Arity() { return 1; }

		Any ILoxCallable.Call(AstInterpreter interpreter, List<Any> arguments)
		{
			Console.WriteLine(AstInterpreter.Stringify(arguments[0]));
			return null;
		}

		public override string ToString() { return "<lib-fn " + GetType().Name + ">"; }
	}
}

namespace LoxDynamic
{
	public class Function : ILoxCallable
	{
		private readonly Stmt.Function declaration;
		private readonly Environment closure;

		public Function(Stmt.Function declaration, Environment closure)
		{
			this.declaration = declaration;
			this.closure = closure;
		}

		int ILoxCallable.Arity() { return declaration.parameters.Count; }

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

		public override string ToString() { return "<fn " + declaration.name.lexeme + ">"; }
	}
}
