%{
#include <stdio.h>
#include <stdlib.h>
#include "symtable.h"
#include "hashmap.h"

// This must be the last include!
#include "y.tab.h"


void yyerror(char *);

// 3 namespaces
struct hashmap *vars;
struct hashmap *classes;
struct hashmap *functions;

struct stack * tmp_params;
struct function *tmp_func;

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
		printf("Error: super class %s not defined", $4);
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
{
	if ($1->tt != UND_T && $1->tc) {
		printf("warning: already initialized constant %s.", $1->vn);
	}
	// Give a name to the expr value
	if ($3->vn != NULL) free($3->vn);
	$3->vn = strdup($1->vn);
	$3->tc = $1->tc;
	hashmap_set(vars, $1->vn, $3);
	var_free($1);
}
                    | RETURN expr
{
	tmp_func->ret = $2;
}
                    | DEF ID opt_params term stmts terms END
{

	tmp_func->fn = $2;
	tmp_func->params = tmp_params;
	hashmap_set(functions, $2, tmp_func);

	// create new blank function
	tmp_func = function_new();
	// new param stack
	tmp_params = stack_new();

	// DO NOT FREE $2! It is used for the function's name ATM.
	//free($2);
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
	}

	$$ = var;
	free($1);
}
                    | ID '.' primary
{
	struct class *cla = hashmap_get(classes, $1);
	struct var *var = NULL;

    if (cla != NULL && $3->tt == FUN_T) {
        if (strcmp($3->vn, "new") == 0) {
			var = var_new("object");
			var->tt = OBJ_T;
			var->ob.cn = strdup($1);
        }
    }
	else {
		var = hashmap_get(vars, $1);
		if (var->tt == OBJ_T && $3->tt == FUN_T) {
			// Verify that the method exists for this object
			struct function *fun = hashmap_get(functions, $3->vn);
			var = var_new($3->vn);
			var->tt = fun->ret->tt;
		}
	}

	if (var == NULL) {
		printf("Error: %s.%s is undefined.\n", $1, $3->vn);
		exit(EXIT_FAILURE);
	}

	$$ = var;
	free($1);
}
                    | ID '(' exprs ')'
{
	struct function *fun = hashmap_get(functions, $1);
	$$ = var_new($1);

	// If the function is unknown, it's name is transmitted
	if (fun == NULL) {
		printf("Function not defined: %s\n", $1);
		$$->tt = FUN_T;
	}
	else {
		$$->tt = fun->ret->tt;
	}

	free($1);
}
                    | ID '(' ')'
{
	struct function *fun = hashmap_get(functions, $1);
	$$ = var_new($1);

	// If the function is unknown, it's name is transmitted
	if (fun == NULL) {
		printf("Function not defined: %s\n", $1);
		$$->tt = FUN_T;
	}
	else {
		$$->tt = fun->ret->tt;
	}

	free($1);
}
;
exprs               : exprs ',' expr
                    | expr
;
primary             : lhs
{
	if ($1->tt == UND_T) {
		printf("Error: %s is undefined\n", $1->vn);
		/* exit(EXIT_FAILURE); */
	}
	else {
		$$ = $1;
	}
}
                    | STRING
{
	$$ = var_new("string");
	$$->tt = STR_T;
	$$->st = $1;
}
                    | FLOAT
{
	$$ = var_new("float");
	$$->tt = FLO_T;
	$$->fl = $1;
}
                    | INT
{
	$$ = var_new("int");
	$$->tt = INT_T;
	$$->in = $1;
}
                    | BOOL
{
	$$ = var_new("bool");
	$$->tt = BOO_T;
	$$->bo = $1;
}
                    | '(' expr ')'
{
    $$ = $2;
}
;
expr                : expr AND comp_expr
{
	$$ = var_new("bool");
	$$->t = BOO_T;
	if ($1->t != BOO_T || $3->t != BOO_T) {
		yyerror("Error: AND operation with non-boolean member.");
		exit(EXIT_FAILURE);
	}
	$$->bo = $1->bo && $3->bo;
}
                    | expr OR comp_expr
{
	$$ = var_new("bool");
	$$->t = BOO_T;
	if ($1->t != BOO_T || $3->t != BOO_T) {
		yyerror("Error: OR operation with non-boolean member.");
		exit(EXIT_FAILURE);
	}
	$$->bo = $1->bo || $3->bo;
}
                    | comp_expr
{
	$$ = $1;
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
{
	$$ = $1;
}
                    | additive_expr '+' multiplicative_expr
                    | additive_expr '-' multiplicative_expr
;
multiplicative_expr : multiplicative_expr '*' primary
                    | multiplicative_expr '/' primary
                    | primary
{
	$$ = $1;
}
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
	tmp_func = function_new();

	yyparse(); 

	function_free(tmp_func);
	stack_free(&tmp_params, var_free);

	puts("");
	puts("Dumping vars:");
	hashmap_dump(vars, var_dump);

	puts("");
	puts("Dumping classes:");
	hashmap_dump(classes, class_dump);

	puts("");
	puts("Dumping functions:");
	hashmap_dump(functions, function_dump);

	// disabled because these hashes may share variables and they might get
	// free()'d multiple times
	//hashmap_free(&vars, var_free);
	//hashmap_free(&classes, class_free);
	hashmap_free(&functions, function_free);

	return 0;
}
