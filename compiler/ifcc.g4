grammar ifcc;

prog : function_def+ EOF ;

function_def : type VAR '(' parameters? ')' '{' statement* '}' ;

parameters : type VAR (',' type VAR)* ;

type : 'int' | 'double' | 'void' ;

statement : declaration ';' | expr ';' | return_stmt | block | ifStmt | whileStmt ;

block : '{' statement* '}' ;

ifStmt : 'if' '(' expr ')' statement ('else' statement)? ;

whileStmt : 'while' '(' expr ')' statement ;

declaration : type VAR '=' expr              # declVar
           | type VAR '[' CONST ']'           # declArray
           ;

lvalue : VAR '[' expr ']'    # lvalueArray
       | VAR                 # lvalueVar
       ;

expr : '-' expr                              # unaryMinusExpr
     | '!' expr                              # logicalNotExpr
     | expr ('*' | '/' | '%') expr           # mulDivModExpr
     | expr ('+' | '-') expr                 # addSubExpr
     | expr ('<' | '>' | '<=' | '>=') expr   # relExpr
     | expr ('==' | '!=') expr               # eqExpr
     | expr '&' expr                         # bitAndExpr
     | expr '^' expr                         # bitXorExpr
     | expr '|' expr                         # bitOrExpr
     | <assoc=right> lvalue '=' expr         # assignExpr
     | VAR '(' (expr (',' expr)*)? ')'       # callExpr
     | VAR '[' expr ']'                      # arrayAccessExpr
     | '(' expr ')'                          # parenExpr
     | CONST_DOUBLE                          # constDoubleExpr
     | CONST                                 # constExpr
     | CHAR_CONST                            # charExpr
     | VAR                                   # varExpr
     ;


return_stmt: RETURN expr ';' ;

RETURN : 'return' ;
VAR : [a-zA-Z_][a-zA-Z_0-9]* ;
CONST_DOUBLE : [0-9]+ '.' [0-9]* | '.' [0-9]+ ;
CONST : [0-9]+ ;
CHAR_CONST : '\'' ( '\\' [nrt0\\'] | ~['\\\r\n] ) '\'' ;
COMMENT : '/*' .*? '*/' -> skip ;
DIRECTIVE : '#' .*? '\n' -> skip ;
WS    : [ \t\r\n] -> channel(HIDDEN);
