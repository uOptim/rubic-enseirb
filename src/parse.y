%{
	#include <stdio.h>
	#include <string.h>
	#include <stdlib.h>

	#include "stack.h"
	#include "block.h"
	#include "hashmap.h"
	#include "gencode.h"
	#include "symtable.h"
	#include "instruction.h"

	// include y.tab.h after every type definition
	#include "y.tab.h"

	struct class    *tmp_class;
	struct function *tmp_function;

	struct stack *scopes; // variables scope
	struct stack *labels; // labels for br`s
	struct stack *istack; // instrstack

	struct var * param_lookup(struct function *, const char *);
	void       * symbol_lookup(struct stack *, const char *, char);

	unsigned int *new_label() {
		static unsigned int labelnum = 1;

		unsigned int *l = malloc(sizeof *l);
		*l = labelnum++;

		return l;
	}

	void yyerror(char *);
%}

%union {
	int n;
	char c;
	char *s;
	double f;
	
	struct cst *cst;
	struct instr*inst;
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
%type <cst> primary
%type <inst> expr comp_expr additive_expr multiplicative_expr

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
}
				term stmts terms END
{
	// delete scope block
	struct block *b = stack_pop(scopes);
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

/* Super bidouille! */
endif			: ELSE
{
	unsigned int lnum = *(unsigned int *)stack_peak(labels, 0);
	printf("br label %%EndIf%d\n", lnum);
	printf("IfFalse%d:\n", lnum);
}
				stmts terms END
{
	unsigned int lnum = *(unsigned int *)stack_peak(labels, 0);
	printf("br label %%EndIf%d\n", lnum);
	printf("EndIf%d:\n", lnum);
}
				| END
{
	unsigned int lnum = *(unsigned int *)stack_peak(labels, 0);
	printf("br label %%EndIf%d\n", lnum);
	printf("IfFalse%d:\n", lnum);
	printf("br label %%EndIf%d\n", lnum);
	printf("EndIf%d:\n", lnum);
}
				;

stmt 			: IF expr opt_terms THEN
{
	stack_push(labels, new_label());

}
				stmts terms endif
{
	free(stack_pop(labels));
}
                | FOR ID IN expr TO expr term stmts terms END
{
	free($2); // free ID
}
                | WHILE expr term stmts terms END 
{
	stack_push(labels, new_label());

	printf("While: ");
}
                | lhs '=' expr
{
	struct var *var;
	struct instr* i;
	
	var = symbol_lookup(scopes, $1, VAR_T);

	if (var == NULL) {
		var = var_new($1);

		hashmap_set(
			((struct block *) stack_peak(scopes, 0))->variables,
			$1,
			var
		);

		ialloca(var);
	}
	
	i = istore(var, instr_get_result($3));

	free($1);
}
                | RETURN expr
{
	if (tmp_function == NULL) {
		fprintf(stderr, "Unexpected 'return' token\n");
		exit(EXIT_FAILURE);
	}
	
	iret(instr_get_result($2));
	cst_free($2);
}

/* DEF ID opt_params term stmts terms END */
                | DEF ID
{
	tmp_function = function_new($2);

	if (symbol_lookup(scopes, $2, FUN_T) != NULL) {
		fprintf(stderr, "Function %s already defined\n", $2);
		exit(EXIT_FAILURE);
	}

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
	struct var *var = var_new($1);
	var->tt = UND_T;

	// masks poossible variables with the same name in the parrent block
	hashmap_set(
		((struct block *) stack_peak(scopes, 0))->variables,
		$1,
		var
	);

	stack_push(tmp_function->params, var);
	free($1);
}
                | ID
{
	struct var *var = var_new($1);
	var->tt = UND_T;

	// masks poossible variables with the same name in the parrent block
	hashmap_set(
		((struct block *) stack_peak(scopes, 0))->variables,
		$1,
		var
	);

	stack_push(tmp_function->params, var);
	free($1);
}
; 
lhs             : ID
{
	$$ = $1;
}
                | ID '.' primary
{
	$$ = $1;
	cst_free($3);
}
                | ID '(' exprs ')'
{
	$$ = $1;
}
;
exprs           : exprs ',' expr
{
}
                | expr
{
}
;
primary         : lhs
{
	struct var *v = symbol_lookup(scopes, $1, VAR_T);

	if (v == NULL) {
		fprintf(stderr, "Error: undefined variable %s\n", $1);
		exit(EXIT_FAILURE);
	}

	$$ = instr_get_result(iload(v));
	
	free($1);
}
                | STRING 
{
	$$ = NULL;
}
                | FLOAT
{
	$$ = cst_new(FLO_T, CST_PURECST);
	$$->f = $1;
}
                | BOOL
{
	$$ = cst_new(BOO_T, CST_PURECST);
	$$->c = $1;
}
                | INT
{
	$$ = cst_new(INT_T, CST_PURECST);
	$$->i = $1;
}
                | '(' expr ')'
{
	$$ = instr_get_result($2);
}
;
expr            : expr AND comp_expr
{
	$$ = i3addr(
		I_AND, 
		instr_get_result($1),
		instr_get_result($3)
	);
}
                | expr OR comp_expr
{
	$$ = i3addr(
		I_OR, 
		instr_get_result($1),
		instr_get_result($3)
	);
}
                | comp_expr
{
	$$ = $1;
}
;
comp_expr       : additive_expr '<' additive_expr
{
	$$ = i3addr(
		I_LT, 
		instr_get_result($1),
		instr_get_result($3)
	);
}
                | additive_expr '>' additive_expr
{
	$$ = i3addr(
		I_GT, 
		instr_get_result($1),
		instr_get_result($3)
	);
}
                | additive_expr LEQ additive_expr
{
	$$ = i3addr(
		I_LEQ, 
		instr_get_result($1),
		instr_get_result($3)
	);
}
                | additive_expr GEQ additive_expr
{
	$$ = i3addr(
		I_GEQ, 
		instr_get_result($1),
		instr_get_result($3)
	);
}
                | additive_expr EQ additive_expr
{
	$$ = i3addr(
		I_EQ, 
		instr_get_result($1),
		instr_get_result($3)
	);
}
                | additive_expr NEQ additive_expr
{
	$$ = i3addr(
		I_NEQ, 
		instr_get_result($1),
		instr_get_result($3)
	);
}
                | additive_expr
{
	$$ = $1;
}
;
additive_expr   : additive_expr '+' multiplicative_expr
{
	struct cst *c1 = instr_get_result($1);
	struct cst *c2 = instr_get_result($3);

	$$ = i3addr(I_ADD, c1, c2);
}
                | additive_expr '-' multiplicative_expr
{
	struct cst *c1 = instr_get_result($1);
	struct cst *c2 = instr_get_result($3);

	$$ = i3addr(I_SUB, c1, c2);
}
				| multiplicative_expr
{
	$$ = $1;
}
;

multiplicative_expr : multiplicative_expr '*' primary
{
	struct cst *c1 = instr_get_result($1);

	$$ = i3addr(I_MUL, c1, $3);
}
                    | multiplicative_expr '/' primary
{
	struct cst *c1 = instr_get_result($1);

	$$ = i3addr(I_DIV, c1, $3);
}
                    | primary
{
	struct cst *c = cst_new(INT_T, CST_PURECST);
	c->i = 0;

	$$ = i3addr(I_ADD, $1, c);
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
	labels = stack_new();

	stack_push(scopes, block_new());

	puts("define i32 @main () {");
	yyparse(); 
	puts("}");

	struct block *b;
	while ((b = stack_pop(scopes)) != NULL) {
		block_free(b);
	}

	stack_free(&scopes, NULL);
	stack_free(&labels, free);

	return 0;
}


void * symbol_lookup(
	struct stack *scopes,
	const char *name,
	char type)
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

