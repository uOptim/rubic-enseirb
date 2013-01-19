%{
	#include <stdio.h>
	#include <string.h>
	#include <stdlib.h>
	#include <assert.h>

	#include "stack.h"
	#include "block.h"
	#include "hashmap.h"
	#include "symtable.h"

	// include y.tab.h after every type definition
	#include "y.tab.h"

	struct class    *tmp_class;
	struct function *tmp_function;

	// context and scopes stack
	struct stack *scopes;

	unsigned int new_reg();

	struct var * param_lookup(struct function *, const char *);
	void       * symbol_lookup(struct stack *, const char *, char);

	void yyerror(char *);
%}

%union {
	int n;
	char c;
	char *s;
	double f;
	
	struct var *var;
};

%token AND OR CLASS IF THEN ELSE END WHILE DO DEF LEQ GEQ 
%token FOR TO RETURN IN NEQ
%token COMMENT
%token <c> BOOL
%token <s> STRING
%token <f> FLOAT
%token <n> INT
%token <s> ID 

%type <s> lhs
%type <var> primary expr exprs comp_expr additive_expr multiplicative_expr

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
topstmt	        
/* CLASS ID term stmts terms END */
				: CLASS ID
{
	tmp_class = class_new();

	if (symbol_lookup(scopes, $2, CLA_T) != NULL) {
		fprintf(stderr, "Class %s already exists\n", $2);
		exit(EXIT_FAILURE);
	}

	tmp_class->cn = strdup($2);

	hashmap_set(
		((struct block *) stack_peak(scopes, 0))->classes,
		$2,
		tmp_class
	);

	// create new scope block
	stack_push(scopes, block_new());
	printf("New block for class %s\n", $2);
}
				term stmts terms END
{
	// delete scope block
	struct block *b = stack_pop(scopes);
	block_dump(b);
	block_free(b);

	tmp_class = NULL;

	free($2);
}

/* CLASS ID '<' ID term stmts terms END */
                | CLASS ID '<' ID 
{
	struct class *super;
	tmp_class = class_new();

	// error checking
	if ((super = symbol_lookup(scopes, $4, CLA_T)) == NULL) {
		fprintf(stderr, "Super class %s of %s not defined\n", $4, $2);
		exit(EXIT_FAILURE);
	}

	if (symbol_lookup(scopes, $2, CLA_T) != NULL) {
		fprintf(stderr, "Class %s already exists\n", $2);
		exit(EXIT_FAILURE);
	}

	tmp_class->cn = strdup($2);
	tmp_class->super = super;

	hashmap_set(
		((struct block *) stack_peak(scopes, 0))->classes,
		$2,
		tmp_class
	);

	// create new scope block
	stack_push(scopes, block_new());

}
				term stmts terms END
{
	// delete scope block
	struct block *b = stack_pop(scopes);
	block_dump(b);
	block_free(b);

	tmp_class = NULL;

	free($2);
	free($4);
}
                | stmt
;

stmts	        : /* none */
                | stmt
                | stmts terms stmt
                ;

stmt 			: IF expr THEN stmts terms END
                | IF expr THEN stmts terms ELSE stmts terms END 
                | FOR ID IN expr TO expr term stmts terms END
{
	free($2);
}
                | WHILE expr DO term stmts terms END 
                | lhs '=' expr
{
}
                | RETURN expr
{
	if (tmp_function != NULL) {
		tmp_function->ret = $2;
	}

	else {
		fprintf(stderr, "Unexpected 'return' token\n");
	}
}

/* DEF ID opt_params term stmts terms END */
                | DEF ID
{
	tmp_function = function_new();

	if (symbol_lookup(scopes, $2, FUN_T) != NULL) {
		fprintf(stderr, "Function %s already defined\n", $2);
		exit(EXIT_FAILURE);
	}

	tmp_function->fn = strdup($2);

	hashmap_set(
		((struct block *) stack_peak(scopes, 0))->functions,
		$2,
		tmp_function
	);

	// create new scope block
	stack_push(scopes, block_new());

}
				opt_params term stmts terms END
{
	// GENERATE CODE FOR FUNCTION HERE

	// delete scope block
	struct block *b = stack_pop(scopes);
	block_dump(b);
	block_free(b);

	tmp_function = NULL;

	free($2);
}
; 

opt_params      : /* none */
                | '(' ')'
                | '(' params ')'
;
params          : ID ',' params
{
	struct var *v = var_new($1, new_reg());
	v->tt = UND_T;

	stack_push(tmp_function->params, v);
	free($1);
}
                | ID
{
	struct var *v = var_new($1, new_reg());
	v->tt = UND_T;

	stack_push(tmp_function->params, v);
	free($1);
}
; 
lhs             : ID
                | ID '.' primary
{
	$$ = NULL;
	fprintf(stderr, "WARNING: SEGFAULT HAZARD!\n");

	free($1);
}
                | ID '(' exprs ')'
{
	$$ = NULL;
	fprintf(stderr, "WARNING: SEGFAULT HAZARD!\n");

	free($1);
}
;
exprs           : exprs ',' expr
                | expr
;
primary         : lhs
{

}
                | STRING 
{
	$$ = var_new("string", new_reg());
	$$->tt = STR_T;

	free($1);
}
                | FLOAT
{
	$$ = var_new("float", new_reg());
	$$->tt = FLO_T;
}
                | INT
{
	$$ = var_new("integer", new_reg());
	$$->tt = INT_T;
}
                | '(' expr ')'
{
	$$ = $2;
}
;
expr            : expr AND comp_expr
{
	$$ = var_new("AND result", new_reg());
	$$->tt = BOO_T;
}
                | expr OR comp_expr
{
	$$ = var_new("OR result", new_reg());
	$$->tt = BOO_T;
}
                | comp_expr
{
	$$ = $1;
}
;
comp_expr       : additive_expr '<' additive_expr
{
	$$ = var_new("LT result", new_reg());
	$$->tt = BOO_T;
}
                | additive_expr '>' additive_expr
{
	$$ = var_new("GT result", new_reg());
	$$->tt = BOO_T;
}
                | additive_expr LEQ additive_expr
{
	$$ = var_new("LEQ result", new_reg());
	$$->tt = BOO_T;
}
                | additive_expr GEQ additive_expr
{
	$$ = var_new("GEQ result", new_reg());
	$$->tt = BOO_T;
}
                | additive_expr EQ additive_expr
{
	$$ = var_new("EQ result", new_reg());
	$$->tt = BOO_T;
}
                | additive_expr NEQ additive_expr
{
	$$ = var_new("NEQ result", new_reg());
	$$->tt = BOO_T;
}
                | additive_expr
{
	$$ = $1;
}
;
additive_expr   : multiplicative_expr
{
	$$ = $1;
}
                | additive_expr '+' multiplicative_expr
{
	$$ = var_new("'+' result", new_reg());
	$$->tt = BOO_T;
}
                | additive_expr '-' multiplicative_expr
{
	$$ = var_new("'-' result", new_reg());
	$$->tt = BOO_T;
}
;
multiplicative_expr : multiplicative_expr '*' primary
{
	$$ = var_new("'*' result", new_reg());
	$$->tt = BOO_T;
}
                    | multiplicative_expr '/' primary
{
	$$ = var_new("'/' result", new_reg());
	$$->tt = BOO_T;
}
                    | primary
{
	$$ = $1;
}
;
opt_terms	: /* none */
			| terms
;
terms		: terms ';'
			| terms '\n'
			| ';'
			| '\n'
;
term		: ';'
			| '\n'
;

%%

int main() {

	scopes = stack_new();
	stack_push(scopes, block_new());

	yyparse(); 

	struct block *b;
	while ((b = stack_pop(scopes)) != NULL) {
		block_dump(b);
		block_free(b);
	}

	return 0;
}


unsigned int new_reg() {
	static unsigned int reg = 0;
	return reg++;
}


void * symbol_lookup(
	struct stack *scopes,
	const char *name,
	char type
)
{
	void *sym = NULL;

	struct block *b;
	struct hashmap *h = NULL;
	struct stack *tmp = stack_new();

	while ((b = stack_pop(scopes)) != NULL) {
		stack_push(tmp, b);

		switch (type) {
			case CLA_T:
				h = b->classes;
				break;
			case FUN_T:
				h = b->functions;
				break;
			case VAR_T:
				h = b->variables;
				break;
			default:
				return NULL;
		}

		if ((sym = hashmap_get(h, name)) != NULL) {
			break;
		}
	}

	while ((b = stack_pop(tmp)) != NULL) {
		stack_push(scopes, b);
	}

	stack_free(&tmp, block_free);

	return sym;
}


struct var * param_lookup(struct function *f, const char *name)
{
	struct var *v, *sym = NULL;
	struct stack *tmp = stack_new();

	while ((v = stack_pop(f->params)) != NULL) {
		stack_push(tmp, v);
		if (strcmp(v->vn, name) == 0) {
			sym = v;
			break;
		}
	}

	while ((v = stack_pop(tmp)) != NULL) {
		stack_push(f->params, v);
	}

	return sym;
}
