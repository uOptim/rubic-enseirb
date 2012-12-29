%{
#include <stdio.h>
#include "y.tab.h"
#include "symtable.h"
#include "hashmap.h"

// 3 namespaces
struct hashmap *vars;
struct hashmap *classes;
struct hashmap *functions;

struct stack *tmp_params;

%}

%union {
	int   n;
	float f;
	char  *s;
}

%token AND OR CLASS IF THEN ELSE END WHILE DO DEF LEQ GEQ 
%token FOR TO RETURN IN NEQ
%token <s> STRING ID 
%token <n> INT BOOL 
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
	hashmap_set(classes, $2, class_new($2, NULL));
	free($2);
}
                    | CLASS ID '<' ID term stmts terms END
{
	struct class *super = hashmap_get(classes, $4);
	if (NULL == super) {
		yyerror("super class not defined");
	} else {
		hashmap_set(classes, $2, class_new($2, super));
	}

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
	hashmap_set(functions, $2, function_new($2, tmp_params));
	tmp_params = stack_new(); // new param stack
	free($2);
}
; 

opt_params          : /* none */
                    | '(' ')'
                    | '(' params ')'
;
params              : ID ',' params
{
	printf("param: %s\n", $1);
	stack_push(tmp_params, $1); // TODO push a struct type instead
	//free($1);
}
                    | ID
{
	printf("param: %s\n", $1);
	stack_push(tmp_params, $1); // TODO push a struct type instead
	//free($1);
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
	vars = hashmap_new();
	classes = hashmap_new();
	functions = hashmap_new();

	tmp_params = stack_new();

  	yyparse(); 

	stack_free(&tmp_params, free);

	hashmap_dump(vars, &type_dump);
  	hashmap_free(&vars, &type_free);
	puts("");

	hashmap_dump(classes, &class_dump);
  	hashmap_free(&classes, &class_free);
	puts("");

	hashmap_dump(functions, &function_dump);
  	hashmap_free(&functions, &function_free);

	return 0;
}
