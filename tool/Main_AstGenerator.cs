using System.IO;

class AstGenerator
{
	static void Main(string[] args)
	{
		DefineAst("code", "Expr", new string[] {
			"Assign   : Token name, Expr value",
			"Binary   : Expr left, Token op, Expr right",
			"Call     : Expr callee, Token paren, List<Expr> arguments",
			"Get      : Expr obj, Token name",
			"Grouping : Expr expression",
			"Literal  : Any value",
			"Logical  : Expr left, Token op, Expr right",
			"Set      : Expr obj, Token name, Expr value",
			"Unary    : Token op, Expr right",
			"Variable : Token name",
		});

		DefineAst("code", "Stmt", new string[] {
			"Block      : List<Stmt> statements",
			"Class      : Token name, List<Stmt.Function> methods",
			"Expression : Expr expression",
			"Function   : Token name, List<Token> parameters, List<Stmt> body",
			"If         : Expr condition, Stmt thenBranch, Stmt elseBranch",
			"Return     : Token keyword, Expr value",
			"Var        : Token name, Expr initializer",
			"While      : Expr condition, Stmt body",
		});
	}

	private static void DefineAst(string outputPath, string baseName, string[] types)
	{
		using (StreamWriter writer = new StreamWriter(outputPath + "/" + baseName + ".cs")) {
			writer.WriteLine("// automatically generated by the ast_generator");
			writer.WriteLine();
			writer.WriteLine("using System.Collections.Generic;");
			writer.WriteLine("using Any = System.Object;");
			writer.WriteLine();
			writer.WriteLine("public abstract class " + baseName);
			writer.WriteLine("{");
			writer.WriteLine("\tpublic abstract R Accept<R>(IVisitor<R> visitor);");
			writer.WriteLine();
			DefineVisitor(writer, baseName, types);
			foreach (string type in types) {
				string[] typeParts = type.Split(':');
				DefineType(writer, baseName, typeParts[0].Trim(), typeParts[1].Trim());
			}
			writer.WriteLine("}");
		}
	}

	private static void DefineVisitor(StreamWriter writer, string baseName, string[] types)
	{
		writer.WriteLine("\tpublic interface IVisitor<R>");
		writer.WriteLine("\t{");
		foreach (string type in types) {
			string[] typeParts = type.Split(':');
			string className = typeParts[0].Trim();
			writer.WriteLine("\t\tR Visit" + className + baseName + "(" + className + " " + baseName.ToLower() + ");");
		}
		writer.WriteLine("\t}");
	}

	private static void DefineType(StreamWriter writer, string baseName, string className, string fields)
	{
		string[] fieldsArray = fields.Split(',');
		writer.WriteLine();
		writer.WriteLine("\tpublic class " + className + " : " + baseName);
		writer.WriteLine("\t{");
		foreach (string field in fieldsArray) {
			writer.WriteLine("\t\tpublic " + field.Trim() + ";");
		}
		writer.WriteLine();
		writer.WriteLine("\t\tpublic " + className + "(" + fields + ")");
		writer.WriteLine("\t\t{");
		foreach (string field in fieldsArray) {
			string[] fieldParts = field.Split(' ');
			string fieldName = fieldParts[fieldParts.Length - 1];
			writer.WriteLine("\t\t\tthis." + fieldName + " = " + fieldName + ";");
		}
		writer.WriteLine("\t\t}");
		writer.WriteLine();
		writer.WriteLine("\t\tpublic override R Accept<R>(IVisitor<R> visitor)");
		writer.WriteLine("\t\t{");
		writer.WriteLine("\t\t\treturn visitor.Visit" + className + baseName + "(this);");
		writer.WriteLine("\t\t}");
		writer.WriteLine("\t}");
	}
}
