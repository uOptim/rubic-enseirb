%{
#include <stdio.h>
#include <stdlib.h>
#include "symtable.h"
#include "hashmap.h"

// This should be the last include!
#include "y.tab.h"


void yyerror(char *);

// 3 namespaces
struct hashmap *vars;
struct hashmap *classes;
struct hashmap *functions;

struct stack * tmp_params;

%}

%union {
	int n;
	char c;
	char *s;
	double f;

	struct var *v;
}

%token AND OR CLASS IF THEN ELSE END WHILE DO DEF LEQ GEQ 
%token FOR TO RETURN IN NEQ
%token <s> STRING ID 
%token <n> INT BOOL 
%token <f> FLOAT 

%type <v> primary lhs expr comp_expr additive_expr multiplicative_expr

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
		exit(EXIT_FAILURE);
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
	stack_push(tmp_params, var_new($1));
	free($1);
}
                    | ID
{
	printf("param: %s\n", $1);
	stack_push(tmp_params, var_new($1));
	free($1);
}
; 
lhs                 : ID
{
	struct var *var = hashmap_get(vars, $1);

	if (var == NULL) {
		printf("New var: %s\n", $1);
		var = var_new($1);
		hashmap_set(vars, $1, var);
	}

	$$ = var;
	free($1);
}
                    | ID '.' primary
{
	free($1);
}
                    | ID '(' exprs ')'
{
	if (NULL == hashmap_get(functions, $1)) {
		printf("Function not defined: %s\n", $1);
	}
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
                    | STRING
{
	$$ = var_new("string");
	$$->t = STR_T;
	$$->st = $1;
}
                    | FLOAT
{
	$$ = var_new("float");
	$$->t = FLO_T;
	$$->fl = $1;
}
                    | INT
{
	$$ = var_new("int");
	$$->t = INT_T;
	$$->in = $1;
}
                    | BOOL
{
	$$ = var_new("bool");
	$$->t = BOO_T;
	$$->bo = $1;
}
                    | '(' expr ')'
;
expr                : expr AND comp_expr
{
	$$ = var_new("bool");
	$$->t = BOO_T;
	$$->bo = $1 && $3;
}
                    | expr OR comp_expr
{
	$$ = var_new("bool");
	$$->t = BOO_T;
	$$->bo = $1 || $3;
}
                    | comp_expr
{
	$$ = var_new("bool");
	$$->t = BOO_T;
	$$->bo = $1;
}
;
comp_expr           : additive_expr '<' additive_expr
{
	$$ = var_new("");
	$$->t = BOO_T;
	if ($1->t == FLO_T) {
		if ($3->t == FLO_T) {
			$$->bo = ($1->fl < $3->fl);
		} else {
			$$->bo = ($1->fl < $3->in);
		}
	} else {
		if ($3->t == FLO_T) {
			$$->bo = ($1->in < $3->fl);
		} else {
			$$->bo = ($1->in < $3->in);
		}
	}
}
                    | additive_expr '>' additive_expr
{
	$$ = var_new("");
	$$->t = BOO_T;
	if ($1->t == FLO_T) {
		if ($3->t == FLO_T) {
			$$->bo = ($1->fl > $3->fl);
		} else {
			$$->bo = ($1->fl > $3->in);
		}
	} else if ($1->t == BOO_T) {
		;
	} else {
		if ($3->t == FLO_T) {
			$$->bo = ($1->in > $3->fl);
		} else {
			$$->bo = ($1->in > $3->in);
		}
	}
}
                    | additive_expr LEQ additive_expr
{
	$$ = var_new("");
	$$->t = BOO_T;
	if ($1->t == FLO_T) {
		if ($3->t == FLO_T) {
			$$->bo = ($1->fl <= $3->fl);
		} else {
			$$->bo = ($1->fl <= $3->in);
		}
	} else if ($1->t == BOO_T) {
		;
	} else {
		if ($3->t == FLO_T) {
			$$->bo = ($1->in <= $3->fl);
		} else {
			$$->bo = ($1->in <= $3->in);
		}
	}
}
                    | additive_expr GEQ additive_expr
{
	$$ = var_new("");
	$$->t = BOO_T;
	if ($1->t == FLO_T) {
		if ($3->t == FLO_T) {
			$$->bo = ($1->fl >= $3->fl);
		} else {
			$$->bo = ($1->fl >= $3->in);
		}
	} else if ($1->t == BOO_T) {
		;
	} else {
		if ($3->t == FLO_T) {
			$$->bo = ($1->in >= $3->fl);
		} else {
			$$->bo = ($1->in >= $3->in);
		}
	}
}
                    | additive_expr EQ additive_expr
{
	$$ = var_new("");
	$$->t = BOO_T;
	if ($1->t == FLO_T) {
		if ($3->t == FLO_T) {
			$$->bo = ($1->fl == $3->fl);
		} else {
			$$->bo = ($1->fl == $3->in);
		}
	} else if ($1->t == BOO_T) {
		;
	} else {
		if ($3->t == FLO_T) {
			$$->bo = ($1->in == $3->fl);
		} else {
			$$->bo = ($1->in == $3->in);
		}
	}
}
                    | additive_expr NEQ additive_expr
{
	$$ = var_new("");
	$$->t = BOO_T;
	if ($1->t == FLO_T) {
		if ($3->t == FLO_T) {
			$$->bo = ($1->fl != $3->fl);
		} else {
			$$->bo = ($1->fl != $3->in);
		}
	} else if ($1->t == BOO_T) {
		;
	} else {
		if ($3->t == FLO_T) {
			$$->bo = ($1->in != $3->fl);
		} else {
			$$->bo = ($1->in != $3->in);
		}
	}
}
                    | additive_expr
{
	$$ = $1;
}
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

	stack_free(&tmp_params, var_free);

	hashmap_dump(vars, var_dump);
  	hashmap_free(&vars, var_free);
	puts("");

	hashmap_dump(classes, class_dump);
  	hashmap_free(&classes, class_free);
	puts("");

	hashmap_dump(functions, function_dump);
  	hashmap_free(&functions, function_free);

	return 0;
}
