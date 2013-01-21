%{
	#include <stdio.h>
	#include <string.h>
	#include <stdlib.h>

	#include "stack.h"
	#include "block.h"
	#include "hashmap.h"
	#include "symtable.h"

	// include y.tab.h after every type definition
	#include "y.tab.h"

	struct class    *tmp_class;
	struct function *tmp_function;

	struct stack *scopes;
	struct stack *labels;

	const char * local2llvm_type(char);
	struct var * param_lookup(struct function *, const char *);
	void       * symbol_lookup(struct stack *, const char *, char);

	struct cst * craft_operation(
		const struct cst *,
		const struct cst *,
		const char *,
		const char *);

	struct cst * craft_boolean_operation(
		const struct cst *,
		const struct cst *,
		const char *);

	int craft_store(struct var *, const struct cst *);

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
%type <cst> primary expr exprs comp_expr additive_expr multiplicative_expr

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
	// delete IF scope block
	struct block *b = stack_pop(scopes);
	block_free(b);

	// create new scope block
	stack_push(scopes, block_new());

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
stmt
				: IF 
{
	// create new scope block
	stack_push(scopes, block_new());
	stack_push(labels, new_label());
}
				expr opt_terms THEN
{
	// create new scope block
	stack_push(scopes, block_new());

	unsigned int lnum = *(unsigned int *)stack_peak(labels, 0);
	if ($3->reg > 0) {
		printf("br i1 %%r%d, label %%IfTrue%d, label %%IfFalse%d\n",
			$3->reg, lnum, lnum);
	} else {
		printf("br i1 %s, label %%IfTrue%d, label %%IfFalse%d\n",
			($3->c > 0) ? "true" : "false", lnum, lnum);
	}
	printf("IfTrue%d:\n", lnum);
}
				stmts terms endif
{
	struct block *b;
	// delete THEN/ELSE scope block
	b = stack_pop(scopes);
	block_free(b);
	// delete IF scope block
	b = stack_pop(scopes);
	block_free(b);

	stack_pop(labels);
}

                | FOR
{
	// create new scope block
	stack_push(scopes, block_new());
}
				ID IN expr TO expr term stmts terms END
{
	struct block *b;
	// delete scope block
	b = stack_pop(scopes);
	block_free(b);

	free($3); // free ID
}
                | WHILE
{
	// create new scope block
	stack_push(scopes, block_new());
}
				expr DO term stmts terms END 
{
	struct block *b;
	// delete scope block
	b = stack_pop(scopes);
	block_free(b);
}
                | lhs '=' expr
{
	struct var *var = symbol_lookup(scopes, $1, VAR_T);

	if (var == NULL) {
		var = var_new($1);
		var->tt = $3->type;

		hashmap_set(
			((struct block *) stack_peak(scopes, 0))->variables,
			$1,
			var
		);

		printf("%%%s = alloca %s\n", var->vn, local2llvm_type(var->tt));
	}
	
	int rv = craft_store(var, $3);
	if (rv == -1) {
		fprintf(stderr, "Incompatible types in assignment\n");
		exit(EXIT_FAILURE);
	}

	free($1);
	cst_free($3);
}
                | RETURN expr
{
	if (tmp_function == NULL) {
		fprintf(stderr, "Unexpected 'return' token\n");
		exit(EXIT_FAILURE);
	}
	
	tmp_function->ret = $2->type;

	if ($2->reg > 0) {
		printf("ret %s %%r%d\n", local2llvm_type($2->type), $2->reg);
	} else {
		switch ($2->type) {
			case INT_T:
				printf("ret i32 %d\n", $2->i);
				break;
			case BOO_T:
				printf("ret i1 %s\n", ($2->c > 0) ? "true" : "false");
				break;
			case FLO_T:
				printf("ret double %g\n", $2->f);
				break;
			default:
				fprintf(stderr, "Invalid return type?\n");
				exit(EXIT_FAILURE);
		}
	}

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
}
                | ID '(' exprs ')'
{
	$$ = $1;
}
;
exprs           : exprs ',' expr
                | expr
;
primary         : lhs
{
	struct var *v = symbol_lookup(scopes, $1, VAR_T);

	if (v == NULL) {
		fprintf(stderr, "Error: undefined variable %s\n", $1);
		exit(EXIT_FAILURE);
	}

	$$ = cst_new(v->tt, CST_OPRESULT);
	printf("%%r%d = load %s* %%%s\n", $$->reg, local2llvm_type(v->tt), v->vn);
	
	free($1);
}
                | STRING 
{
	$$ = cst_new(STR_T, CST_PURECST);
	$$->s = $1;
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
	$$ = $2;
}
;
expr            : expr AND comp_expr
{

	$$ = craft_boolean_operation($1, $3, "and");
	if ($$ == NULL) {
		fprintf(stderr, "Incompatible type for boolean operation\n");
		exit(EXIT_FAILURE);
	}
	cst_free($1);
	cst_free($3);
}
                | expr OR comp_expr
{
	$$ = craft_boolean_operation($1, $3, "or");
	if ($$ == NULL) {
		fprintf(stderr, "Incompatible type for boolean operation\n");
		exit(EXIT_FAILURE);
	}
	cst_free($1);
	cst_free($3);
}
                | comp_expr
{
	$$ = $1;
}
;
comp_expr       : additive_expr '<' additive_expr
{
	$$ = craft_operation($1, $3, "icmp slt", "fcmp slt");
	if ($$ == NULL) {
		fprintf(stderr, "Incompatible type for comparison operation\n");
		exit(EXIT_FAILURE);
	}
	cst_free($1);
	cst_free($3);
}
                | additive_expr '>' additive_expr
{
	$$ = craft_operation($1, $3, "icmp sgt", "fcmp sgt");
	if ($$ == NULL) {
		fprintf(stderr, "Incompatible type for comparison operation\n");
		exit(EXIT_FAILURE);
	}
	cst_free($1);
	cst_free($3);
}
                | additive_expr LEQ additive_expr
{
	$$ = craft_operation($1, $3, "icmp sle", "fcmp sle");
	if ($$ == NULL) {
		fprintf(stderr, "Incompatible type for comparison operation\n");
		exit(EXIT_FAILURE);
	}
	cst_free($1);
	cst_free($3);
}
                | additive_expr GEQ additive_expr
{
	$$ = craft_operation($1, $3, "icmp sge", "fcmp sge");
	if ($$ == NULL) {
		fprintf(stderr, "Incompatible type for comparison operation\n");
		exit(EXIT_FAILURE);
	}
	cst_free($1);
	cst_free($3);
}
                | additive_expr EQ additive_expr
{
	$$ = craft_operation($1, $3, "icmp eq", "fcmp eq");
	if ($$ == NULL) {
		fprintf(stderr, "Incompatible type for comparison operation\n");
		exit(EXIT_FAILURE);
	}
	cst_free($1);
	cst_free($3);
}
                | additive_expr NEQ additive_expr
{
	$$ = craft_operation($1, $3, "icmp neq", "fcmp neq");
	if ($$ == NULL) {
		fprintf(stderr, "Incompatible type for comparison operation\n");
		exit(EXIT_FAILURE);
	}
	cst_free($1);
	cst_free($3);
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
	$$ = craft_operation($1, $3, "add", "fadd");
	if ($$ == NULL) {
		fprintf(stderr, "Incompatible types for operator +\n");
		exit(EXIT_FAILURE);
	}
	cst_free($1);
	cst_free($3);
}
                | additive_expr '-' multiplicative_expr
{
	$$ = craft_operation($1, $3, "sub", "fsub");
	if ($$ == NULL) {
		fprintf(stderr, "Incompatible types for operator -\n");
		exit(EXIT_FAILURE);
	}
	cst_free($1);
	cst_free($3);
}
;
multiplicative_expr : multiplicative_expr '*' primary
{
	$$ = craft_operation($1, $3, "mul", "fmul");
	if ($$ == NULL) {
		fprintf(stderr, "Incompatible types for operator *\n");
		exit(EXIT_FAILURE);
	}
	cst_free($1);
	cst_free($3);
}
                    | multiplicative_expr '/' primary
{
	$$ = craft_operation($1, $3, "sdiv", "fdiv");
	if ($$ == NULL) {
		fprintf(stderr, "Incompatible types for operator /\n");
		exit(EXIT_FAILURE);
	}
	cst_free($1);
	cst_free($3);
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


int craft_store(struct var *var, const struct cst *c)
{
	if (var->tt == UND_T) {
		var->tt = c->type;
	} else {
		var->tt = compatibility_table[(int)var->tt][(int)c->type];
	}

	printf("store %s ", local2llvm_type(var->tt));
	if (c->reg > 0) {
		printf("%%r%d, ", c->reg);
	} else {
		switch (c->type) {
			case INT_T:
				printf("%d, ", c->i);
				break;
			case FLO_T:
				printf("%g, ", c->f);
				break;
			case BOO_T:
				printf("%s, ", (c->c == 0) ? "false" : "true");
				break;
		}
	}
	printf("%s* %%%s\n", local2llvm_type(var->tt), var->vn);
	return 0;
}

struct cst * craft_operation(
	const struct cst *c1,
	const struct cst *c2,
	const char *op,
	const char *fop)
{
	struct cst *result = NULL;

	if (c1->type == INT_T && c2->type == INT_T) {
		result = cst_new(INT_T, CST_OPRESULT);
		printf("%%r%d = %s i32 ", result->reg, op);
		if (c1->reg > 0) {
			printf("%%r%d", c1->reg);
		} else {
			printf("%d", c1->i);
		}
		printf(", ");
		if (c2->reg > 0) {
			printf("%%r%d", c2->reg);
		} else {
			printf("%d", c2->i);
		}
		puts("");
	}

	else if (c1->type == FLO_T && c2->type == FLO_T) {
		result = cst_new(FLO_T, CST_OPRESULT);
		printf("%%r%d = %s double ", result->reg, fop);
		if (c1->reg > 0) {
			printf("%%r%d", c1->reg);
		} else {
			printf("%#g", c1->f);
		}
		printf(", ");
		if (c2->reg > 0) {
			printf("%%r%d", c2->reg);
		} else {
			printf("%#g", c2->f);
		}
		puts("");
	}

	else if (c1->type == INT_T && c2->type == FLO_T) {
		unsigned int r;
		result = cst_new(FLO_T, CST_OPRESULT);

		if (c1->reg > 0) {
			// conversion needed
			r = new_reg();
			printf("%%r%d = sitofp i32 %%r%d to double\n", r, c1->reg);
		}

		printf("%%r%d = %s double ", result->reg, fop);

		if (c1->reg > 0) {
			printf("%%r%d", r);
		} else {
			printf("%d.0", c1->i);
		}
		printf(", ");
		if (c2->reg > 0) {
			printf("%%r%d", c2->reg);
		} else {
			printf("%#g", c2->f);
		}
		puts("");
	}

	else if (c1->type == FLO_T && c2->type == INT_T) {
		unsigned int r;
		result = cst_new(FLO_T, CST_OPRESULT);

		if (c2->reg > 0) {
			// conversion
			r = new_reg();
			printf("%%r%d = sitofp i32 %%r%d to double\n", r, c2->reg);
		}
		printf("%%r%d = %s double ", result->reg, fop);
		
		if (c1->reg > 0) {
			printf("%%r%d", c1->reg);
		} else {
			printf("%#g", c1->f);
		}
		printf(", ");
		if (c2->reg > 0) {
			printf("%%r%d", r);
		} else {
			printf("%d.0", c2->i);
		}
		puts("");
	}

	return result;
}

struct cst * craft_boolean_conversion(const struct cst *c1)
{
	struct cst *c = NULL;

	switch (c1->type) {
		case INT_T:
			c = cst_new(BOO_T, CST_OPRESULT);
			printf("%%r%d = icmp sgt i32 ", c->reg);
			if (c1->reg > 0) {
				printf("%%r%d ", c1->reg);
			} else {
				printf("%d ", c1->i);
			}
			printf(", 0\n");
			break;
		case FLO_T:
			c = cst_new(BOO_T, CST_OPRESULT);
			printf("%%r%d = fcmp sgt double ", c->reg);
			if (c1->reg > 0) {
				printf("%%r%d ", c1->reg);
			} else {
				printf("%#g ", c1->f);
			}
			printf(", 0.0\n");
			break;
		default:
			return NULL;
	}
	
	return c;
}

struct cst * craft_boolean_operation(
	const struct cst *c1,
	const struct cst *c2,
	const char *op)
{
	struct cst *c = cst_new(BOO_T, CST_OPRESULT);

	if (c1->type != BOO_T) {
		c1 = craft_boolean_conversion(c1);
		if (c1 == NULL) return NULL;
	}
	if (c2->type != BOO_T) {
		c2 = craft_boolean_conversion(c2);
		if (c2 == NULL) return NULL;
	}
	
	printf("%%r%d = icmp %s i1 ", c->reg, op);
	if (c1->reg > 0) {
		printf("%%r%d", c1->reg);
	} else {
		printf("%s", (c1->c > 0) ? "true" : "false");
	}
	printf(", ");
	if (c2->reg > 0) {
		printf("%%r%d", c2->reg);
	} else {
		printf("%s", (c2->c > 0) ? "true" : "false");
	}
	puts("");

	return c;
}

const char * local2llvm_type(char type)
{
	switch(type) {
		case INT_T:
			return "i32";
			break;
		case FLO_T:
			return "double";
			break;
		case BOO_T:
			return "i1";
			break;
		case STR_T:
			// TODO
			break;
		default:
			break;
	}

	return "UNKNOWN TYPE";
}


char convert2bool(struct cst *c)
{
	char v;

	if (c->type == INT_T) {
		if (c->i > 0) v = 1;
		else v = 0;
	} else if (c->type == FLO_T) {
		if (c->f > 0) v = 1;
		else v = 0;
	} else {
		fprintf(stderr, "Incompatible type for boolean conversion\n");
		exit(EXIT_FAILURE);
	}

	return v;
}
