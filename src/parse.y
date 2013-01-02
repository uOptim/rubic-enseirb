%{
  #include <stdio.h>
  #include "y.tab.h"

%}
%token AND OR CLASS IF THEN ELSE END WHILE DO DEF LEQ GEQ 
%token STRING FLOAT INT ID FOR TO RETURN IN NEQ
%left '*' 
%left '/'
%left '+' '-'
%left '<' '>' LEQ GEQ EQ
%left AND OR
%%
program		:  topstmts opt_terms
;
topstmts        :      
| topstmt
| topstmts terms topstmt
;
topstmt	        : CLASS ID term stmts END 
                | CLASS ID '<' ID term stmts END
                | stmt
;

stmts	        : /* none */
                | stmt
                | stmts terms stmt
                ;

stmt		: IF expr THEN stmts terms END
                | IF expr THEN stmts terms ELSE stmts terms END 
                | FOR ID IN expr TO expr term stmts terms END
                | WHILE expr DO term stmts terms END 
                | lhs '=' expr
                | RETURN expr
                | DEF ID opt_params term stmts terms END
; 

opt_params      : /* none */
                | '(' ')'
                | '(' params ')'
;
params          : ID ',' params
                | ID
; 
lhs             : ID
                | ID '.' primary
                | ID '(' exprs ')'
;
exprs           : exprs ',' expr
                | expr
;
primary         : lhs
                | STRING 
                | FLOAT
                | INT
                | '(' expr ')'
;
expr            : expr AND comp_expr
                | expr OR comp_expr
                | comp_expr
;
comp_expr       : additive_expr '<' additive_expr
                | additive_expr '>' additive_expr
                | additive_expr LEQ additive_expr
                | additive_expr GEQ additive_expr
                | additive_expr EQ additive_expr
                | additive_expr NEQ additive_expr
                | additive_expr
;
additive_expr   : multiplicative_expr
                | additive_expr '+' multiplicative_expr
                | additive_expr '-' multiplicative_expr
;
multiplicative_expr : multiplicative_expr '*' primary
                    | multiplicative_expr '/' primary
                    | primary
;
opt_terms	: /* none */
		| terms
		;

terms		: terms ';'
                | terms '\n'
		| ';'
                | '\n'
		;
term            : ';'
                | '\n'
;
%%
int main() {
  yyparse(); 
  return 0;
}
