using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

class Lox
{
	private static bool hadError;

	static void Main(string[] args)
	{
		// Console.WriteLine(new AstPrinter().Print(new Expr.Binary(
		// 	new Expr.Unary(
		// 		new Token(TokenType.MINUS, "-", null, 1),
		// 		new Expr.Literal(123)
		// 	),
		// 	new Token(TokenType.STAR, "*", null, 1),
		// 	new Expr.Grouping(
		// 		new Expr.Literal(45.67)
		// 	)
		// )));

		if (args.Length == 0) {
			RunPrompt();
			return;
		}

		if (args.Length > 1) {
			Console.WriteLine("usage: interpreter [script]");
			Environment.Exit(1); return;
		}

		RunFile(args[0]);
	}

	private static void RunFile(string path)
	{
		string source = File.ReadAllText(path, Encoding.Default);
		RunSource(source);
		if (hadError) { Environment.Exit(2); return; }
	}

	private static void RunPrompt()
	{
		for (;;) {
			Console.Write("> ");
			string line = Console.ReadLine();
			if (line == null) { break; }
			RunSource(line);
			hadError = false;
		}
	}

	private static void RunSource(string source)
	{
		if (string.IsNullOrEmpty(source)) { return; }
		Scanner scanner = new Scanner(source);
		List<Token> tokens = scanner.ScanTokens();

		foreach (Token token in tokens)
		{
			Console.WriteLine(token);
		}
	}

	public static void Error(int line, string message) => Report(line, string.Empty, message);

	public static void Report(int line, string where, string message)
	{
		System.Console.WriteLine("[line " + line + "] Error" + where + ": " + message);
		hadError = true;
	}
}
