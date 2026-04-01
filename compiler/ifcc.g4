grammar ifcc;

prog : function_def+ EOF ;

function_def : type VAR '(' parameters? ')' '{' statement* '}' ;

parameters : type VAR (',' type VAR)* ;

type : 'int' | 'double' | 'void' | 'char' ;

statement : declaration ';' | expr ';' | return_stmt | return_void_stmt | block | ifStmt | whileStmt | doWhileStmt | forStmt | breakStmt | continueStmt | postIncStmt | postDecStmt ;

postIncStmt : VAR INC ';' ;
postDecStmt : VAR DEC ';' ;

block : '{' statement* '}' ;

ifStmt : 'if' '(' expr ')' statement ('else' statement)? ;

whileStmt : 'while' '(' expr ')' statement ;

doWhileStmt : 'do' statement 'while' '(' expr ')' ';' ;

forStmt : 'for' '(' (declaration | expr)? ';' expr? ';' expr? ')' statement ;

breakStmt : 'break' ';' ;

continueStmt : 'continue' ';' ;

declaration : type VAR '=' expr              # declVar
           | type VAR '[' CONST ']'           # declArray
           | type VAR                         # declVarUninit
           ;

lvalue : VAR '[' expr ']'    # lvalueArray
       | VAR                 # lvalueVar
       ;

expr : INC VAR                               # preIncExpr
     | DEC VAR                               # preDecExpr
     | '-' expr                              # unaryMinusExpr
     | '!' expr                              # logicalNotExpr
     | expr ('*' | '/' | '%') expr           # mulDivModExpr
     | expr ('+' | '-') expr                 # addSubExpr
     | expr (SHL | SHR) expr                 # shiftExpr
     | expr ('<' | '>' | LE | GE) expr       # relExpr
     | expr ('==' | '!=') expr               # eqExpr
     | expr '&' expr                         # bitAndExpr
     | expr '^' expr                         # bitXorExpr
     | expr '|' expr                         # bitOrExpr
     | expr AND expr                         # logicalAndExpr
     | expr OR expr                          # logicalOrExpr
     | <assoc=right> expr '?' expr ':' expr  # ternaryExpr
     | <assoc=right> lvalue '=' expr         # assignExpr
     | <assoc=right> lvalue PLUSEQ expr      # plusAssignExpr
     | <assoc=right> lvalue MINUSEQ expr     # minusAssignExpr
     | <assoc=right> lvalue MULEQ expr       # mulAssignExpr
     | <assoc=right> lvalue DIVEQ expr       # divAssignExpr
     | <assoc=right> lvalue MODEQ expr       # modAssignExpr
     | VAR '(' (expr (',' expr)*)? ')'       # callExpr
     | VAR '[' expr ']'                      # arrayAccessExpr
     | '(' expr ')'                          # parenExpr
     | CONST_DOUBLE                          # constDoubleExpr
     | CONST                                 # constExpr
     | CHAR_CONST                            # charExpr
     | VAR                                   # varExpr
     ;


return_stmt: RETURN expr ';' ;
return_void_stmt: RETURN ';' ;

// Multi-character operator tokens
INC     : '++' ;
DEC     : '--' ;
SHL     : '<<' ;
SHR     : '>>' ;
LE      : '<=' ;
GE      : '>=' ;
AND     : '&&' ;
OR      : '||' ;
PLUSEQ  : '+=' ;
MINUSEQ : '-=' ;
MULEQ   : '*=' ;
DIVEQ   : '/=' ;
MODEQ   : '%=' ;

RETURN : 'return' ;
VAR : [a-zA-Z_][a-zA-Z_0-9]* ;
CONST_DOUBLE : [0-9]+ '.' [0-9]* | '.' [0-9]+ ;
CONST : [0-9]+ ;
CHAR_CONST : '\'' ( '\\' [nrt0\\'] | ~['\\\r\n] ) '\'' ;
COMMENT : '/*' .*? '*/' -> skip ;
LINE_COMMENT : '//' ~[\r\n]* -> skip ;
DIRECTIVE : '#' .*? '\n' -> skip ;
WS    : [ \t\r\n] -> channel(HIDDEN);
