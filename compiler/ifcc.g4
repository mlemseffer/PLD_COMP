grammar ifcc;

prog : function_def+ EOF ;

function_def : type VAR '(' parameters? ')' '{' statement* '}' ;

parameters : type VAR (',' type VAR)* ;

type : 'int' | 'double' ;

statement : declaration ';' | affectation ';' | expr ';' | return_stmt | block | ifStmt | whileStmt ;

block : '{' statement* '}' ;

ifStmt : 'if' '(' expr ')' statement ('else' statement)? ;

whileStmt : 'while' '(' expr ')' statement ;

declaration : type VAR '=' expr              # declVar
           | type VAR '[' CONST ']'           # declArray
           ;

affectation : lvalue '=' expr;

lvalue : VAR '[' expr ']'    # lvalueArray
       | VAR                 # lvalueVar
       ;

expr : '-' expr              # unaryMinusExpr
     | expr ('*' | '/') expr  # mulDivExpr
     | expr ('+' | '-') expr  # addSubExpr
     | expr ('<' | '>' | '<=' | '>=') expr # relExpr
     | expr ('==' | '!=') expr # eqExpr
     | VAR '(' (expr (',' expr)*)? ')' # callExpr
     | VAR '[' expr ']'        # arrayAccessExpr
     | '(' expr ')'           # parenExpr
     | CONST_DOUBLE            # constDoubleExpr
     | CONST                   # constExpr
     | VAR                    # varExpr
     ;


return_stmt: RETURN expr ';' ;

RETURN : 'return' ;
VAR : [a-zA-Z_][a-zA-Z_0-9]* ;
CONST_DOUBLE : [0-9]+ '.' [0-9]* | '.' [0-9]+ ;
CONST : [0-9]+ ;
COMMENT : '/*' .*? '*/' -> skip ;
DIRECTIVE : '#' .*? '\n' -> skip ;
WS    : [ \t\r\n] -> channel(HIDDEN);
