%{
#include <stdio.h>
#include "y.tab.h"
#include "symtable.h"
#include "hashmap.h"

struct hashmap *h;

%}

%union {
	int   n;
	float f;
	char  *s;
}

%token AND OR CLASS IF THEN ELSE END WHILE DO DEF LEQ GEQ 
%token FOR TO RETURN IN NEQ
%token <s> STRING BOOL ID 
%token <n> INT 
%token <f> FLOAT 
%left '*' 
%left '/'
%left '+' '-'
%left '<' '>' LEQ GEQ EQ
%left AND OR
%%
program             : topstmts opt_terms
;
topstmts            :      
                    | topstmt
                    | topstmts terms topstmt
;
topstmt             : CLASS ID term stmts terms END 
{
	hashmap_set(h, $2, type_new(CLA_T, $2));
	free($2);
}
                    | CLASS ID '<' ID term stmts terms END
{
	free($2);
	free($4);
}
                    | stmt
;

stmts               : /* none */
                    | stmt
                    | stmts terms stmt
;

stmt                : IF expr THEN stmts terms END
                    | IF expr THEN stmts terms ELSE stmts terms END 
                    | FOR ID IN expr TO expr term stmts terms END
{
	free($2);
}
                    | WHILE expr DO term stmts terms END 
                    | lhs '=' expr
                    | RETURN expr
                    | DEF ID opt_params term stmts terms END
{
	free($2);
}
; 

opt_params          : /* none */
                    | '(' ')'
                    | '(' params ')'
;
params              : ID ',' params
{
	free($1);
}
                    | ID
{
	free($1);
}
; 
lhs                 : ID
{
	free($1);
}
                    | ID '.' primary
{
	free($1);
}
                    | ID '(' exprs ')'
{
	free($1);
}
                    | ID '(' ')'
{
	free($1);
}
;
exprs               : exprs ',' expr
                    | expr
;
primary             : lhs
                    | STRING        { fprintf(stderr, "string\n"); }
                    | FLOAT         { fprintf(stderr, "float\n"); }
                    | INT           { fprintf(stderr, "int\n"); }
                    | BOOL          { fprintf(stderr, "boolean\n"); }
                    | '(' expr ')'
;
expr                : expr AND comp_expr
                    | expr OR comp_expr
                    | comp_expr
;
comp_expr           : additive_expr '<' additive_expr
                    | additive_expr '>' additive_expr
                    | additive_expr LEQ additive_expr
                    | additive_expr GEQ additive_expr
                    | additive_expr EQ additive_expr
                    | additive_expr NEQ additive_expr
                    | additive_expr
;
additive_expr       : multiplicative_expr
                    | additive_expr '+' multiplicative_expr
                    | additive_expr '-' multiplicative_expr
;
multiplicative_expr : multiplicative_expr '*' primary
                    | multiplicative_expr '/' primary
                    | primary
;
opt_terms           : /* none */
                    | terms
;

terms               : terms ';'
                    | terms '\n'
                    | ';'
                    | '\n'
;
term                : ';'
                    | '\n'
;
%%
int main() {
	h = hashmap_new();
  	yyparse(); 
	hashmap_dump(h, &type_dump);
  	hashmap_free(&h, &type_free);

	return 0;
}
