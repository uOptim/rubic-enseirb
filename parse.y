%{
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "symtable.h"
#include "hashmap.h"

// This must be the last include!
#include "y.tab.h"


void yyerror(char *);

unsigned int new_reg();

// 3 namespaces
struct hashmap *vars;       // variables may not be global
struct hashmap *classes;    // classes are global
struct hashmap *functions;  // functions are global

struct stack * tmp_params;
struct class * tmp_class;
struct function *tmp_func;

struct hashmap *tmp_func_env;
// Pointer on the current environment.
// (either a function environment or the "normal" environment)
struct hashmap **env;

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
%token COMMENT
%token <s> STRING ID 
%token <n> INT BOOL 
%token <f> FLOAT 

%type <s> lhs
%type <v> primary expr comp_expr additive_expr multiplicative_expr

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
	tmp_class->cn = $2;
	
	hashmap_set(symtable, $2, tmp_class);

	tmp_class = class_new();
}
                    | CLASS ID '<' ID term stmts terms END
{
	struct class *super = hashmap_get(symtable, $4);

	if (NULL == super) {
		printf("Error: super class %s not defined", $4);
		exit(EXIT_FAILURE);
	}

	tmp_class->cn = $2;
	tmp_class->super = super;
	
	hashmap_set(symtable, $2, tmp_class);

	tmp_class = class_new();

	free($4);
}
                    | stmt
;

stmts               : /* none */
                    | stmt
                    | stmts terms stmt
;

stmt                : COMMENT
                    | IF expr THEN stmts terms END
                    | IF expr THEN stmts terms ELSE stmts terms END 
                    | FOR ID IN expr TO expr term stmts terms END
{
	free($2);
}
                    | WHILE expr DO term stmts terms END 
                    | lhs '=' expr
{
	// Useless for the momemt
	// $1 = $3;

	// Update var value
	switch ($3->tt) {
		case INT_T:
			$1->in = $3->in;
			break;
		case BOO_T:
			$1->bo = $3->bo;
			break;
		case FLO_T:
			$1->fl = $3->fl;
			break;
		case STR_T:
			$1->st = strdup($3->st);
			break;
		case OBJ_T:
			$1->ob.cn = strdup($3->ob.cn);
			break;
		default:
			printf("Right expression is of invalid type.");
			break;
	}

	if ($1->tt == UND_T) {
		// New variable is added to the symtable
		hashmap_set(*env, $1->vn, $1);
	}
	else if ($1->tc) {
		// Existing constant should not be modified
		printf("warning: already initialized constant %s.", $1->vn);
	}

	// Update var type
	$1->tt = $3->tt;
	var_free($3);
}
                    | RETURN expr
{
	tmp_func->ret = $2;
}
                    | DEF ID opt_params term stmts terms END
{
	tmp_func->fn = $2;
	tmp_func->params = tmp_params;
	hashmap_set(symtable, $2, tmp_func);

	// create new blank function
	tmp_func = function_new();
	// new param stack
	tmp_params = stack_new();
    // free the stacks but not its data, they are in the function params stack
    hashmap_free(&tmp_func_env, NULL);
    tmp_func_env = hashmap_new();

    // Reset the "normal environmenent"
    env = &vars;

	// DO NOT FREE $2! It is used for the function's name ATM.
	//free($2);
}
; 

opt_params          : /* none */
{
    env = &tmp_func_env;
}
                    | '(' ')'
{
    env = &tmp_func_env;
}
                    | '(' params ')'
{
    env = &tmp_func_env;
}
;
params              : ID ',' params
{
    struct var *param = var_new($1);
	printf("param: %s\n", $1);
	stack_push(tmp_params, var_new($1, new_reg()));
    hashmap_set(tmp_func_env, $1, param);
	free($1);
}
                    | ID
{
    struct var *param = var_new($1);
	printf("param: %s\n", $1);
	stack_push(tmp_params, var_new($1, new_reg()));
    hashmap_set(tmp_func_env, $1, param);
	free($1);
}
; 
lhs                 : ID
{
	struct var *var = hashmap_get(*env, $1);

	if (var == NULL) {
		printf("New var: %s\n", $1);
		var = var_new($1);
	}

	$$ = var;
	free($1);
}
                    | ID '.' primary
{
/* Never put exit in comment here, this may cause a segfault */

	struct class *cla = hashmap_get(classes, $1);
	struct var *var = NULL;

	if (cla != NULL && $3->tt == FUN_T) {
		if (strcmp($3->vn, "new") == 0) {
			var = var_new("object");
			var->tt = OBJ_T;
			var->ob.cn = strdup($1);
		}
		else {
			printf("Error: %s.%s is undefined.\n", $1, $3->vn);
			exit(EXIT_FAILURE);
		}
	}
	else {
		var = hashmap_get(*env, $1);
		if (var == NULL) {
			printf("Error: %s is undefined.\n", $1);
			exit(EXIT_FAILURE);
		}
		else if (var->tt == OBJ_T && $3->tt == FUN_T) {
			// Verify that the method exists for this object
			struct function *fun = hashmap_get(functions, $3->vn);
			if (fun == NULL) {
				printf("Error: %s.%s is undefined.\n", $1, $3->vn);
				exit(EXIT_FAILURE);
			}
			var = var_new($3->vn);
			var->tt = fun->ret->tt;
		}
	}

	$$ = var;

	free($3);
	free($1);
}
                    | ID '(' exprs ')'
{
	struct function *fun = hashmap_get(symtable, $1);
	$$ = var_new($1, new_reg());

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
	struct function *fun = hashmap_get(symtable, $1);
	$$ = var_new($1, new_reg());

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
	$$ = var_new("string", new_reg());
	$$->tt = STR_T;
	$$->st = $1;
}
                    | FLOAT
{
	$$ = var_new("float", new_reg());
	$$->tt = FLO_T;
	$$->fl = $1;

	printf("%%%d = add f64 %g, 0\n", $$->reg, $1);
}
                    | INT
{
	$$ = var_new("int", new_reg());
	$$->tt = INT_T;
	$$->in = $1;

	printf("%%%d = add i32 %d, 0\n", $$->reg, $1);
}
                    | BOOL
{
	$$ = var_new("bool", new_reg());
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
	$$ = var_new("bool", new_reg());
	$$->t = BOO_T;
	if ($1->t != BOO_T || $3->t != BOO_T) {
		yyerror("Error: AND operation with non-boolean member.");
		exit(EXIT_FAILURE);
	}
	$$->bo = $1->bo && $3->bo;
}
                    | expr OR comp_expr
{
	$$ = var_new("bool", new_reg());
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
	$$ = var_new("", new_reg());
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
	$$ = var_new("", new_reg());
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
	$$ = var_new("", new_reg());
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
	$$ = var_new("", new_reg());
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
	$$ = var_new("", new_reg());
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
	$$ = var_new("", new_reg());
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
{
	$$ = var_new("", new_reg());
	if ($1->tt == FLO_T) {
		if ($3->tt == FLO_T) {
			$$->tt = FLO_T;
			$$->fl = ($1->fl + $3->fl);
		} else if ($3->t == INT_T) {
			$$->tt = FLO_T;
			$$->fl = ($1->fl + $3->in);
		}
		else {
			$$->tt = UND_T;
		}
	}
	else if ($1->t == INT_T) {
		if ($3->t == FLO_T) {
			$$->t = FLO_T;
			$$->fl = ($1->in + $3->fl);
		} else if ($3->t == INT_T) {
			$$->t = INT_T;
			$$->in = ($1->in + $3->in);
		}
		else {
			$$->tt = UND_T;
		}
	}
	else {
		$$->tt = UND_T;
	}
}
                    | additive_expr '-' multiplicative_expr
{
	$$ = var_new("", new_reg());
	if ($1->t == FLO_T) {
		if ($3->t == FLO_T) {
			$$->t = FLO_T;
			$$->fl = ($1->fl - $3->fl);
		} else if ($3->t == INT_T) {
			$$->t = FLO_T;
			$$->fl = ($1->fl - $3->in);
		}
		else {
			$$->tt = UND_T;
		}
	}
	else if ($1->t == INT_T) {
		if ($3->t == FLO_T) {
			$$->t = FLO_T;
			$$->fl = ($1->in - $3->fl);
		} else if ($3->t == INT_T) {
			$$->t = INT_T;
			$$->in = ($1->in - $3->in);
		}
		else {
			$$->tt = UND_T;
		}
	}
	else {
		$$->tt = UND_T;
	}
}
;
multiplicative_expr : multiplicative_expr '*' primary
{
	$$ = var_new("", new_reg());
	if ($1->t == FLO_T) {
		if ($3->t == FLO_T) {
			$$->t = FLO_T;
			$$->fl = ($1->fl * $3->fl);
		} else if ($3->t == INT_T) {
			$$->t = FLO_T;
			$$->fl = ($1->fl * $3->in);
		}
		else {
			$$->tt = UND_T;
		}
	}
	else if ($1->t == INT_T) {
		if ($3->t == FLO_T) {
			$$->t = FLO_T;
			$$->fl = ($1->in * $3->fl);
		} else if ($3->t == INT_T) {
			$$->t = INT_T;
			$$->in = ($1->in * $3->in);
		}
		else {
			$$->tt = UND_T;
		}
	}
	else {
		$$->tt = UND_T;
	}
}
                    | multiplicative_expr '/' primary
{
	$$ = var_new("", new_reg());
	if ($1->t == FLO_T) {
		if ($3->t == FLO_T) {
			$$->t = FLO_T;
			$$->fl = ($1->fl / $3->fl);
		} else if ($3->t == INT_T) {
			$$->t = FLO_T;
			$$->fl = ($1->fl / $3->in);
		}
		else {
			$$->tt = UND_T;
		}
	}
	else if ($1->t == INT_T) {
		if ($3->t == FLO_T) {
			$$->t = FLO_T;
			$$->fl = ($1->in / $3->fl);
		} else if ($3->t == INT_T) {
			$$->t = INT_T;
			$$->in = ($1->in / $3->in);
		}
		else {
			$$->tt = UND_T;
		}
	}
	else {
		$$->tt = UND_T;
	}
}
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
	symtable = hashmap_new();

	tmp_class = class_new();
	tmp_params = stack_new();
	tmp_func = function_new();

    tmp_func_env = hashmap_new();
    env = &vars;

	yyparse(); 

	class_free(tmp_class);
	function_free(tmp_func);
	stack_free(&tmp_params, var_free);

	// disabled because these hashes may share variables and they might get
	// free()'d multiple times
	//hashmap_free(&vars, var_free);
	//hashmap_free(&classes, class_free);
	hashmap_free(&functions, function_free);
    hashmap_free(&tmp_func_env, NULL);

	return 0;
}


unsigned int new_reg() {
	static unsigned int reg = 0;
	return reg++;
}
