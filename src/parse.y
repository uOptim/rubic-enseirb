%{
	#include <stdio.h>
	#include <string.h>
	#include <stdlib.h>

	#include "stack.h"
	#include "block.h"
	#include "types.h"
	#include "hashmap.h"
	#include "gencode.h"
	#include "genfunc.h"
	#include "symtable.h"
	#include "instruction.h"

	// include y.tab.h after every type definition
	#include "y.tab.h"

	// used for string manipulations
	#define BUFSZ 128
	ssize_t size;
	char buf[BUFSZ];

	struct class    *tmp_class;
	struct function *tmp_function;

	struct stack *scopes; // variables scope
	struct stack *labels; // labels for br`s
	struct stack *istack; // current instruction stack
	struct stack *gistack; // global instruction stack
	struct stack *fistack; // function instruction stack
	struct stack *tstack; // types stack
	struct stack *tmp_args = NULL; // function call arguments

	struct var * param_lookup(struct function *, const char *);
	void       * symbol_lookup(struct stack *, const char *, char);

	unsigned int *new_label() {
		static unsigned int labelnum = 1;

		unsigned int *l = malloc(sizeof *l);
		*l = labelnum++;

		return l;
	}

	void exit_cleanly(int code)
	{
		stack_free(&labels, free);
		stack_free(&scopes, block_free);
		stack_free(&gistack, instr_free);
		stack_free(&fistack, instr_free);

		stack_free(&tstack, NULL);

		exit(code);
	}

	void yyerror(char *);
	void yylex_destroy();
	int yylex();
%}

%union {
	int n;
	char c;
	char *s;
	double f;
	
	struct instr *inst;
};

%token AND OR CLASS IF THEN ELSE END WHILE DO DEF LEQ GEQ 
%token FOR TO RETURN IN NEQ PUTS
%token COMMENT %token <c> BOOL
%token <s> STRING
%token <f> FLOAT
%token <n> INT
%token <s> ID 

%type <s> lhs
%type <inst> primary expr comp_expr additive_expr multiplicative_expr

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
	tmp_class = class_new($2);
	stack_push(tstack, &tmp_class->typenum);

	if (symbol_lookup(scopes, $2, CLA_T) != NULL) {
		class_free(tmp_class);
		fprintf(stderr, "Class %s already exists\n", $2);
		exit_cleanly(EXIT_FAILURE);
	}

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
	tmp_class = class_new($2);

	// error checking
	if ((super = symbol_lookup(scopes, $4, CLA_T)) == NULL) {
		fprintf(stderr, "Super class %s of %s not defined\n", $4, $2);
		exit_cleanly(EXIT_FAILURE);
	}

	if (symbol_lookup(scopes, $2, CLA_T) != NULL) {
		fprintf(stderr, "Class %s already exists\n", $2);
		exit_cleanly(EXIT_FAILURE);
	}

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

	size = snprintf(buf, BUFSZ, "br label %%EndIf%d", lnum);
	stack_push(istack, iraw(buf));

	size = snprintf(buf, BUFSZ, "IfFalse%d:", lnum);
	stack_push(istack, iraw(buf));
}
				stmts terms END
{
	unsigned int lnum = *(unsigned int *)stack_peak(labels, 0);

	size = snprintf(buf, BUFSZ, "br label %%EndIf%d", lnum);
	stack_push(istack, iraw(buf));

	size = snprintf(buf, BUFSZ, "EndIf%d:", lnum);
	stack_push(istack, iraw(buf));
}
				| END
{
	unsigned int lnum = *(unsigned int *)stack_peak(labels, 0);

	size = snprintf(buf, BUFSZ, "br label %%EndIf%d", lnum);
	stack_push(istack, iraw(buf));

	size = snprintf(buf, BUFSZ, "IfFalse%d:", lnum);
	stack_push(istack, iraw(buf));

	size = snprintf(buf, BUFSZ, "br label %%EndIf%d", lnum);
	stack_push(istack, iraw(buf));

	size = snprintf(buf, BUFSZ, "EndIf%d:", lnum);
	stack_push(istack, iraw(buf));
}
				;

stmt 			: COMMENT
                | IF expr opt_terms THEN
{
	stack_push(labels, new_label());

	struct elt *elt;
	struct instr *cast;

	unsigned int lnum = *(unsigned int *)stack_peak(labels, 0);

	// cast expr into bool if necessary
	if ( ! ($2->optype & I_BOO || $2->optype & I_CMP)) {
		cast = icast(casttobool, instr_get_result($2), BOO_T);
		stack_push(istack, cast);
		elt = instr_get_result(cast); 
	} else {
		elt = instr_get_result($2); 
	}

	if (elt->elttype == E_REG) {
		size = snprintf(buf, BUFSZ,
			"br i1 %%r%d, label %%IfTrue%d, label %%IfFalse%d",
			elt->reg->num, lnum, lnum
		);
	} else {
		size = snprintf(buf, BUFSZ,
			"br i1 %s, label %%IfTrue%d, label %%IfFalse%d",
			(elt->cst->c == 0) ? "false": "true", lnum, lnum
		);
	}
	stack_push(istack, iraw(buf));

	sprintf(buf, "IfTrue%d:", lnum);
	stack_push(istack, iraw(buf));

	elt_free(elt);
}
				stmts terms endif
{
	free(stack_pop(labels));
}
                | FOR ID IN expr
{
	stack_push(labels, new_label());

	struct var *var;
	struct instr *i;
	
	var = symbol_lookup(scopes, $2, VAR_T);

	if (var == NULL) {
		var = var_new($2);

		hashmap_set(
			((struct block *) stack_peak(scopes, 0))->variables,
			$2,
			var
		);

		i = ialloca(var);
		if (i == NULL) exit_cleanly(EXIT_FAILURE);
		stack_push(istack, i);
	}
	
	else if (var_isconst(var)) {
		fprintf(stderr, "Warning: %s is suposed to be a const.\n", var->vn);
	}

	i = istore(var, instr_get_result($4));
	if (i == NULL) exit_cleanly(EXIT_FAILURE);
	stack_push(istack, i);
}
				TO expr
{
	struct var *v;
	struct instr *i;
	unsigned int lnum = *(unsigned int *)stack_peak(labels, 0);

	size = snprintf(buf, BUFSZ, "br label %%loop%d", lnum);
	stack_push(istack, iraw(buf));

	size = snprintf(buf, BUFSZ, "loop%d:", lnum);
	stack_push(istack, iraw(buf));

	v = symbol_lookup(scopes, $2, VAR_T);
	i = iload(v);
	if (i == NULL) exit_cleanly(EXIT_FAILURE);
	stack_push(istack, i);

	i = i3addr(I_LEQ, instr_get_result(i), instr_get_result($7));
	if (i == NULL) exit_cleanly(EXIT_FAILURE);
	stack_push(istack, i);

	struct elt *elt = instr_get_result(i);
	if (elt->elttype == E_REG) {
		size = snprintf(buf, BUFSZ,
			"br i1 %%r%d, label %%cond%d, label %%endloop%d",
			elt->reg->num, lnum, lnum
		);
	} else {
		size = snprintf(buf, BUFSZ,
			"br i1 %s, label %%cond%d, label %%endloop%d",
			(elt->cst->c == 0) ? "false" : "true", lnum, lnum
		);
	}
	stack_push(istack, iraw(buf));

	size = snprintf(buf, BUFSZ, "cond%d:", lnum);
	stack_push(istack, iraw(buf));

	elt_free(elt);
}
				term stmts terms END
{
	unsigned int lnum = *(unsigned int *)stack_peak(labels, 0);

	struct var *v;
	struct cst *one;
	struct instr *i;
	
	v = symbol_lookup(scopes, $2, VAR_T);

	i = iload(v);
	if (i == NULL) exit_cleanly(EXIT_FAILURE);
	stack_push(istack, i);

	one = cst_new(INT_T); one->i = 1;
	i = i3addr(I_ADD, instr_get_result(i), elt_new(E_CST, one));
	if (i == NULL) exit_cleanly(EXIT_FAILURE);
	stack_push(istack, i);

	i = istore(v, instr_get_result(i));
	if (i == NULL) exit_cleanly(EXIT_FAILURE);
	stack_push(istack, i);

	size = snprintf(buf, BUFSZ, "br label %%loop%d", lnum);
	stack_push(istack, iraw(buf));

	size = snprintf(buf, BUFSZ, "endloop%d:", lnum);
	stack_push(istack, iraw(buf));

	free(stack_pop(labels));
	
	free($2);
}
                | WHILE 
{
	stack_push(labels, new_label());

	unsigned int lnum = *(unsigned int *)stack_peak(labels, 0);

	size = snprintf(buf, BUFSZ, "br label %%loop%d", lnum);
	stack_push(istack, iraw(buf));

	size = snprintf(buf, BUFSZ, "loop%d:", lnum);
	stack_push(istack, iraw(buf));
}
				expr 
{
	struct elt *elt;
	struct instr *cast;
	unsigned int lnum = *(unsigned int *)stack_peak(labels, 0);
	
	// cast expr into bool if necessary
	if ( ! ($3->optype & I_BOO || $3->optype & I_CMP)) {
		cast = icast(casttobool, instr_get_result($3), BOO_T);
		stack_push(istack, cast);
		elt = instr_get_result(cast); 
	} else {
		elt = instr_get_result($3);
	}

	if (elt->elttype == E_REG) {
		size = snprintf(buf, BUFSZ,
			"br i1 %%r%d, label %%cond%d, label %%endloop%d",
			elt->reg->num, lnum, lnum
		);
	} else {
		size = snprintf(buf, BUFSZ,
			"br i1 %s, label %%cond%d, label %%endloop%d",
			(elt->cst->c == 0) ? "false" : "true", lnum, lnum
		);
	}
	stack_push(istack, iraw(buf));

	size = snprintf(buf, BUFSZ, "cond%d:", lnum);
	stack_push(istack, iraw(buf));

	elt_free(elt);
}
				term stmts terms END 
{
	unsigned int lnum = *(unsigned int *)stack_peak(labels, 0);

	size = snprintf(buf, BUFSZ, "br label %%loop%d", lnum);
	stack_push(istack, iraw(buf));

	size = snprintf(buf, BUFSZ, "endloop%d:", lnum);
	stack_push(istack, iraw(buf));

	free(stack_pop(labels));
}
                | lhs '=' expr
{
	struct var *var;
	struct instr *i;
	
	var = symbol_lookup(scopes, $1, VAR_T);

	if (var == NULL) {
		var = var_new($1);

		hashmap_set(
			((struct block *) stack_peak(scopes, 0))->variables,
			$1,
			var
		);

		i = ialloca(var);
		if (i == NULL) exit_cleanly(EXIT_FAILURE);
		stack_push(istack, i);
	}

	else if (var_isconst(var)) {
		fprintf(stderr, "Warning: %s is suposed to be a const.\n", var->vn);
	}
	
	i = istore(var, instr_get_result($3));
	if (i == NULL) exit_cleanly(EXIT_FAILURE);
	stack_push(istack, i);

	free($1);
}
                | lhs
{
	struct instr *i;

	// function call
	if (tmp_args != NULL) {
		if (symbol_lookup(scopes, $1, FUN_T) == NULL) {
			fprintf(stderr, "Function %s not defined\n", $1);
			exit_cleanly(EXIT_FAILURE);
		}

		i = icall($1, tmp_args);
		if (i == NULL) exit_cleanly(EXIT_FAILURE);
		stack_push(istack, i);
	}
}
                | RETURN expr
{
	struct instr *i;

	if (tmp_function == NULL) {
		fprintf(stderr, "Unexpected 'return' token\n");
		exit_cleanly(EXIT_FAILURE);
	}
	
	i = iret(instr_get_result($2));
	if (i == NULL) exit_cleanly(EXIT_FAILURE);
	tmp_function->ret = instr_get_result($2);
	stack_push(istack, i);
}

/* DEF ID opt_params term stmts terms END */
                | DEF ID
{

	tmp_function = function_new($2);

	if (symbol_lookup(scopes, $2, FUN_T) != NULL) {
		fprintf(stderr, "Function %s already defined\n", $2);
		exit_cleanly(EXIT_FAILURE);
	}

	hashmap_set(
		((struct block *) stack_peak(scopes, 0))->functions,
		$2,
		tmp_function
	);

	// create new scope block
	stack_push(scopes, block_new());
	// change instruction scope
	istack = fistack;
}
				opt_params term stmts terms END
{
	func_gen_codes(tmp_function,
                   fistack,
                   ((struct block *)stack_peak(scopes, 1))->functions);

	stack_clear(fistack, instr_free);

	// change instruction scope
	istack = gistack;
	// delete scope block
	struct block *b = stack_pop(scopes);
	block_free(b);

	tmp_function = NULL;

	free($2);
}
                | PUTS expr
{
	struct instr *i;
	struct elt *elt = instr_get_result($2);
	i = iputs(elt);
	if (i == NULL) exit_cleanly(EXIT_FAILURE);
	stack_push(istack, i);
}
; 

opt_params      : /* none */
                | '(' ')'
                | '(' params ')'
;
params          : ID ',' params
{
	struct var *var = var_new($1);

	// masks possible variables with the same name in the parent block
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

	// masks possible variables with the same name in the parent block
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
	elt_free($3);
}
                | ID '(' ')'
{
	$$ = $1;
	tmp_args = stack_new();
}
                | ID '(' exprs ')'
{
	$$ = $1;
	tmp_args = stack_new();
}
;
exprs           : exprs ',' expr
{
	stack_push(tmp_args, instr_get_result($3));
}
                | expr
{
	stack_push(tmp_args, instr_get_result($1));
}
;
primary         : lhs
{
	// not a function call
	if (tmp_args == NULL) {
		struct var *v = symbol_lookup(scopes, $1, VAR_T);

		if (v == NULL) {
			fprintf(stderr, "Error: undefined variable %s\n", $1);
			exit_cleanly(EXIT_FAILURE);
		}

		$$ = iload(v);
		if ($$ == NULL) exit_cleanly(EXIT_FAILURE);
		stack_push(istack, $$);

		free($1);
	}
	// function call
	else {
		if (symbol_lookup(scopes, $1, FUN_T) == NULL) {
			fprintf(stderr, "Function %s not defined\n", $1);
			exit_cleanly(EXIT_FAILURE);
		}

		$$ = icall($1, tmp_args);
		if ($$ == NULL) exit_cleanly(EXIT_FAILURE);
	}
}
                | STRING 
{
	fprintf(stderr, "Strings are not supported.");
	exit_cleanly(EXIT_FAILURE);
	$$ = NULL;
}
                | FLOAT
{
	struct cst *c, *zero;
	
	c = cst_new(FLO_T);
	c->f = $1;

	zero = cst_new(FLO_T);
	zero->f = 0.0;

	$$ = i3addr(I_ADD, elt_new(E_CST, c), elt_new(E_CST, zero));
	if ($$ == NULL) exit_cleanly(EXIT_FAILURE);
	stack_push(istack, $$);
}
                | BOOL
{
	struct cst *c, *nottrue;
	
	c = cst_new(BOO_T);
	c->c = $1;

	nottrue = cst_new(BOO_T);
	nottrue->c = 0;

	$$ = i3addr(I_OR, elt_new(E_CST, c), elt_new(E_CST, nottrue));
	if ($$ == NULL) exit_cleanly(EXIT_FAILURE);
	stack_push(istack, $$);
}
                | INT
{
	struct cst *c, *zero;
	
	c = cst_new(INT_T);
	c->i = $1;

	zero = cst_new(INT_T);
	zero->i = 0;

	$$ = i3addr(I_ADD, elt_new(E_CST, c), elt_new(E_CST, zero));
	if ($$ == NULL) exit_cleanly(EXIT_FAILURE);
	stack_push(istack, $$);
}
                | '(' expr ')'
{
	$$ = $2;
}
;
expr            : expr AND comp_expr
{
	struct instr *i1 = icast(casttobool, instr_get_result($1), BOO_T);
	struct instr *i3 = icast(casttobool, instr_get_result($3), BOO_T);
	stack_push(istack, i1);
	stack_push(istack, i3);

	$$ = i3addr(I_AND, instr_get_result(i1), instr_get_result(i3));
	if ($$ == NULL) exit_cleanly(EXIT_FAILURE);
	stack_push(istack, $$);
}
                | expr OR comp_expr
{
	struct instr *i1 = icast(casttobool, instr_get_result($1), BOO_T);
	struct instr *i3 = icast(casttobool, instr_get_result($3), BOO_T);
	stack_push(istack, i1);
	stack_push(istack, i3);

	$$ = i3addr(I_OR, instr_get_result(i1), instr_get_result(i3));
	if ($$ == NULL) exit_cleanly(EXIT_FAILURE);
	stack_push(istack, $$);
}
                | comp_expr
{
	$$ = $1;
}
;
comp_expr       : additive_expr '<' additive_expr
{
	$$ = i3addr(I_LT, instr_get_result($1), instr_get_result($3));
	if ($$ == NULL) exit_cleanly(EXIT_FAILURE);
	stack_push(istack, $$);
}
                | additive_expr '>' additive_expr
{
	$$ = i3addr(I_GT, instr_get_result($1), instr_get_result($3));
	if ($$ == NULL) exit_cleanly(EXIT_FAILURE);
	stack_push(istack, $$);
}
                | additive_expr LEQ additive_expr
{
	$$ = i3addr(I_LEQ, instr_get_result($1), instr_get_result($3));
	if ($$ == NULL) exit_cleanly(EXIT_FAILURE);
	stack_push(istack, $$);
}
                | additive_expr GEQ additive_expr
{
	$$ = i3addr(I_GEQ, instr_get_result($1), instr_get_result($3));
	if ($$ == NULL) exit_cleanly(EXIT_FAILURE);
	stack_push(istack, $$);
}
                | additive_expr EQ additive_expr
{
	$$ = i3addr(I_EQ, instr_get_result($1), instr_get_result($3));
	if ($$ == NULL) exit_cleanly(EXIT_FAILURE);
	stack_push(istack, $$);
}
                | additive_expr NEQ additive_expr
{
	$$ = i3addr(I_NEQ, instr_get_result($1), instr_get_result($3));
	if ($$ == NULL) exit_cleanly(EXIT_FAILURE);
	stack_push(istack, $$);
}
                | additive_expr
{
	$$ = $1;
}
;
additive_expr   : additive_expr '+' multiplicative_expr
{
	$$ = i3addr(I_ADD, instr_get_result($1), instr_get_result($3));
	if ($$ == NULL) exit_cleanly(EXIT_FAILURE);
	stack_push(istack, $$);
}
                | additive_expr '-' multiplicative_expr
{
	$$ = i3addr(I_SUB, instr_get_result($1), instr_get_result($3));
	if ($$ == NULL) exit_cleanly(EXIT_FAILURE);
	stack_push(istack, $$);
}
				| multiplicative_expr
{
	$$ = $1;
}
;

multiplicative_expr : multiplicative_expr '*' primary
{
	$$ = i3addr(I_MUL, instr_get_result($1), instr_get_result($3));
	if ($$ == NULL) exit_cleanly(EXIT_FAILURE);
	stack_push(istack, $$);
}
                    | multiplicative_expr '/' primary
{
	$$ = i3addr(I_DIV, instr_get_result($1), instr_get_result($3));
	if ($$ == NULL) exit_cleanly(EXIT_FAILURE);
	stack_push(istack, $$);
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
	labels = stack_new();
	tstack = stack_new();
	fistack = stack_new();
	gistack = stack_new();
	istack = gistack;

	type_init(tstack);

	stack_push(scopes, block_new());

	yyparse(); 

//	hashmap_dump(((struct block *)stack_peak(scopes, 0))->functions,
//					function_dump);
//
	gencode_main(gistack,
				   ((struct block *)stack_peak(scopes, 0))->functions);

	yylex_destroy();
	exit_cleanly(EXIT_SUCCESS);

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

	stack_move(tmp, scopes);
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

