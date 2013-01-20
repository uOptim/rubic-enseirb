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

	// context and scopes stack
	struct stack *scopes;

	unsigned int new_reg();

	const char * local2llvm_type(char);
	struct var * param_lookup(struct function *, const char *);
	void       * symbol_lookup(struct stack *, const char *, char);

	struct var * craft_operation(
		const struct var *,
		const struct var *,
		const char *,
		const char *);

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
}
				term stmts terms END
{
	// delete scope block
	struct block *b = stack_pop(scopes);
	//block_dump(b);
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
	//block_dump(b);
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

stmt
/* if then */
				: IF expr THEN stmts terms END
				| IF expr THEN stmts terms ELSE stmts terms END
                | FOR ID IN expr TO expr term stmts terms END
{
	free($2);
}
                | WHILE expr DO term stmts terms END 
                | lhs '=' expr
{
	struct var *var = symbol_lookup(scopes, $1, VAR_T);

	if ((var = symbol_lookup(scopes, $1, VAR_T)) == NULL) {
		var = var_new($1, new_reg());
		var->tt = $3->tt;

		hashmap_set(
			((struct block *) stack_peak(scopes, 0))->variables,
			$1,
			var
		);

		printf("%%var%d = alloca %s", var->reg, local2llvm_type(var->tt));
		printf("\t\t\t; alloca %s\n", var->vn);
	}
	
	if (var->tt != UND_T && var->tt != $3->tt) {
		fprintf(stderr, "Incompatible types in assignment\n");
	} else {
		var->tt = $3->tt;
		printf("store %s %%r%d, %s* %%var%d",
			local2llvm_type(var->tt), $3->reg,
			local2llvm_type(var->tt), var->reg);
		printf("\t\t\t; store %s into %s\n", $3->vn, var->vn);
	}
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
	//block_dump(b);
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
	struct var *var = var_new($1, new_reg());
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
	struct var *var = var_new($1, new_reg());
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
	$$ = symbol_lookup(scopes, $1, VAR_T);
	if ($$ == NULL) {
		fprintf(stderr, "Error: undefined variable %s\n", $1);
	}
	
	free($1);
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
	printf("%%r%d = fadd double %f, 0.0\n", $$->reg, $1);
}
                | INT
{
	$$ = var_new("integer", new_reg());
	$$->tt = INT_T;
	printf("%%r%d = add i32 %d, 0\n", $$->reg, $1);
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
	$$ = craft_operation($1, $3, "add", "fadd");
}
                | additive_expr '-' multiplicative_expr
{
	$$ = craft_operation($1, $3, "sub", "fsub");
}
;
multiplicative_expr : multiplicative_expr '*' primary
{
	$$ = craft_operation($1, $3, "mul", "fmul");
}
                    | multiplicative_expr '/' primary
{
	$$ = craft_operation($1, $3, "sdiv", "fdiv");
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

	puts("define i32 @main () {");
	yyparse(); 
	puts("ret i32 1");
	puts("}");

	struct block *b;
	while ((b = stack_pop(scopes)) != NULL) {
		//block_dump(b);
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


struct var * craft_operation(
	const struct var *v1,
	const struct var *v2,
	const char *op,
	const char *fop)
{
	struct var *result = var_new(op, new_reg());

	if (v1->tt == INT_T && v2->tt == INT_T) {
		result->tt = INT_T;
		printf("%%r%d = %s i32 %%r%d, %%r%d\n",
			result->reg, op, v1->reg, v2->reg);
	}

	else if (v1->tt == FLO_T && v2->tt == FLO_T) {
		result->tt = FLO_T;
		printf("%%r%d = %s double %%r%d, %%r%d\n",
			result->reg, fop, v1->reg, v2->reg);
	}

	else if (v1->tt == INT_T && v2->tt == FLO_T) {
		result->tt = FLO_T;
		// conversion
		unsigned int r = new_reg();
		printf("%%r%d = sitofp i32 %%r%d to double\n", r, v1->reg);
		printf("%%r%d = %s double %%r%d, %%r%d\n",
			result->reg, fop, r, v2->reg);
	}

	else if (v1->tt == FLO_T && v2->tt == INT_T) {
		result->tt = FLO_T;
		// conversion
		unsigned int r = new_reg();
		printf("%%r%d = sitofp i32 %%r%d to double\n", r, v2->reg);
		printf("%%r%d = %s double %%r%d, %%r%d\n",
			result->reg, fop, v1->reg, r);
	}

	else {
		fprintf(stderr, "Incompatible types for operators %s or %s\n", op, fop);
		var_free(result);
		return NULL;
	}

	return result;
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
		case STR_T:
			// TODO
			break;
		default:
			break;
	}

	return "UNKNOWN TYPE";
}
