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
  stringstream in;
  if (argn==2)
  {
     ifstream lecture(argv[1]);
     if( !lecture.good() )
     {
         cerr<<"error: cannot read file: " << argv[1] << endl ;
         exit(1);
     }
     in << lecture.rdbuf();
  }
  else
  {
      cerr << "usage: ifcc path/to/file.c" << endl ;
      exit(1);
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
      cfg->gen_asm(cout);
  }

  return 0;
}
