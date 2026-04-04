#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>

#include "antlr4-runtime.h"
#include "generated/ifccLexer.h"
#include "generated/ifccParser.h"
#include "generated/ifccBaseVisitor.h"

#include "CodeGenVisitor.h"
#include "SymbolTableVisitor.h"
#include "IRGenVisitor.h"
#include "IR.h"

using namespace antlr4;
using namespace std;

int main(int argn, const char **argv)
{
  // Déterminer le fichier source et l'architecture cible
  string target = "";
  string sourceFile = "";

  for (int i = 1; i < argn; i++) {
      string arg = argv[i];
      if (arg == "--target=x86") target = "x86";
      else if (arg == "--target=arm64") target = "arm64";
      else sourceFile = arg;
  }

  if (sourceFile.empty()) {
      cerr << "usage: ifcc [--target=x86|arm64] path/to/file.c" << endl;
      exit(1);
  }

  // Auto-détection si pas spécifié
  if (target.empty()) {
      #if defined(__aarch64__) || defined(__arm64__)
      target = "arm64";
      #else
      target = "x86";
      #endif
  }

  stringstream in;
  {
     ifstream lecture(sourceFile);
     if( !lecture.good() )
     {
         cerr<<"error: cannot read file: " << sourceFile << endl ;
         exit(1);
     }
     in << lecture.rdbuf();
  }
  
  ANTLRInputStream input(in.str());

  ifccLexer lexer(&input);
  CommonTokenStream tokens(&lexer);

  tokens.fill();

  ifccParser parser(&tokens);
  tree::ParseTree* tree = parser.prog();

  if(parser.getNumberOfSyntaxErrors() != 0)
  {
      cerr << "error: syntax error during parsing" << endl;
      exit(1);
  }

  // Passe 1 : analyse sémantique (table des symboles + vérifications)
  SymbolTableVisitor stv;
  stv.visit(tree);

  if (stv.hasError)
  {
      cerr << "error: semantic analysis failed" << endl;
      exit(1);
  }

  // Passe 2 : construction de l'IR
  IRGenVisitor irv;
  irv.visit(tree);

  // Passe 3 : génération de code assembleur depuis l'IR
  for (auto cfg : irv.getCFGs()) {
      cfg->target = target;
      cfg->gen_asm(cout);
  }

  return 0;
}
