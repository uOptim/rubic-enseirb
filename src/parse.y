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

	struct stack *scopes;
	struct stack *labels;

	const char * local2llvm_type(char);
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
	struct instruction *inst;
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

	instruction_dump($2);
}
				stmts terms endif
{
	free(stack_pop(labels));
}
                | FOR ID IN expr TO expr term stmts terms END
{
	free($2); // free ID
	cst_free($4);
}
                | WHILE expr term stmts terms END 
{
	stack_push(labels, new_label());

	printf("While: ");
	instruction_dump($2);
}
                | lhs '=' expr
{
	struct var *var = symbol_lookup(scopes, $1, VAR_T);

	if (var == NULL) {
		var = var_new($1);

		hashmap_set(
			((struct block *) stack_peak(scopes, 0))->variables,
			$1,
			var
		);

		//i_alloca(var);
	}
	
	instruction_dump($3);
	i_store(var, instruction_get_result($3));

	free($1);
}
                | RETURN expr
{
	if (tmp_function == NULL) {
		fprintf(stderr, "Unexpected 'return' token\n");
		exit(EXIT_FAILURE);
	}
	
	instruction_dump($2);
	//instruction_ret(instruction_get_result($2));

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
                | expr
;
primary         : lhs
{
	struct var *v = symbol_lookup(scopes, $1, VAR_T);

	if (v == NULL) {
		fprintf(stderr, "Error: undefined variable %s\n", $1);
		exit(EXIT_FAILURE);
	}

	$$ = instruction_get_result(i_load(v));
	
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
	instruction_dump($2);
	$$ = instruction_get_result($2);
}
;
expr            : expr AND comp_expr
{
	//$$ = craft_boolean_operation($1, $3, "and");
	//if ($$ == NULL) {
	//	fprintf(stderr, "Incompatible type for boolean operation\n");
	//	exit(EXIT_FAILURE);
	//}
	//cst_free($1);
	//cst_free($3);
}
                | expr OR comp_expr
{
	//$$ = craft_boolean_operation($1, $3, "or");
	//if ($$ == NULL) {
	//	fprintf(stderr, "Incompatible type for boolean operation\n");
	//	exit(EXIT_FAILURE);
	//}
	//cst_free($1);
	//cst_free($3);
}
                | comp_expr
{
	//$$ = $1;
}
;
comp_expr       : additive_expr '<' additive_expr
{
	//$$ = craft_operation($1, $3, "icmp slt", "fcmp slt");
	//if ($$ == NULL) {
	//	fprintf(stderr, "Incompatible type for comparison operation\n");
	//	exit(EXIT_FAILURE);
	//}
	//cst_free($1);
	//cst_free($3);
}
                | additive_expr '>' additive_expr
{
	//$$ = craft_operation($1, $3, "icmp sgt", "fcmp sgt");
	//if ($$ == NULL) {
	//	fprintf(stderr, "Incompatible type for comparison operation\n");
	//	exit(EXIT_FAILURE);
	//}
	//cst_free($1);
	//cst_free($3);
}
                | additive_expr LEQ additive_expr
{
	//$$ = craft_operation($1, $3, "icmp sle", "fcmp sle");
	//if ($$ == NULL) {
	//	fprintf(stderr, "Incompatible type for comparison operation\n");
	//	exit(EXIT_FAILURE);
	//}
	//cst_free($1);
	//cst_free($3);
}
                | additive_expr GEQ additive_expr
{
	//$$ = craft_operation($1, $3, "icmp sge", "fcmp sge");
	//if ($$ == NULL) {
	//	fprintf(stderr, "Incompatible type for comparison operation\n");
	//	exit(EXIT_FAILURE);
	//}
	//cst_free($1);
	//cst_free($3);
}
                | additive_expr EQ additive_expr
{
	//$$ = craft_operation($1, $3, "icmp eq", "fcmp eq");
	//if ($$ == NULL) {
	//	fprintf(stderr, "Incompatible type for comparison operation\n");
	//	exit(EXIT_FAILURE);
	//}
	//cst_free($1);
	//cst_free($3);
}
                | additive_expr NEQ additive_expr
{
	//$$ = craft_operation($1, $3, "icmp neq", "fcmp neq");
	//if ($$ == NULL) {
	//	fprintf(stderr, "Incompatible type for comparison operation\n");
	//	exit(EXIT_FAILURE);
	//}
	//cst_free($1);
	//cst_free($3);
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
	instruction_dump($1);
	instruction_dump($3);
	struct cst *c1 = instruction_get_result($1);
	struct cst *c2 = instruction_get_result($3);

	$$ = i3addr(I_ADD, c1, c2);
	//$$ = craft_operation($1, $3, "add", "fadd");
	//if ($$ == NULL) {
	//	fprintf(stderr, "Incompatible types for operator +\n");
	//	exit(EXIT_FAILURE);
	//}
	//cst_free($1);
	//cst_free($3);
}
                | additive_expr '-' multiplicative_expr
{
	instruction_dump($1);
	instruction_dump($3);
	struct cst *c1 = instruction_get_result($1);
	struct cst *c2 = instruction_get_result($3);

	$$ = i3addr(I_SUB, c1, c2);

	//$$ = craft_operation($1, $3, "sub", "fsub");
	//if ($$ == NULL) {
	//	fprintf(stderr, "Incompatible types for operator -\n");
	//	exit(EXIT_FAILURE);
	//}
	//cst_free($1);
	//cst_free($3);
}
;
multiplicative_expr : multiplicative_expr '*' primary
{
	instruction_dump($1);
	struct cst *c1 = instruction_get_result($1);

	$$ = i3addr(I_MUL, c1, $3);

	//$$ = craft_operation($1, $3, "mul", "fmul");
	//if ($$ == NULL) {
	//	fprintf(stderr, "Incompatible types for operator *\n");
	//	exit(EXIT_FAILURE);
	//}
	//cst_free($1);
	//cst_free($3);
}
                    | multiplicative_expr '/' primary
{
	instruction_dump($1);
	struct cst *c1 = instruction_get_result($1);

	$$ = i3addr(I_DIV, c1, $3);

	//$$ = craft_operation($1, $3, "sdiv", "fdiv");
	//if ($$ == NULL) {
	//	fprintf(stderr, "Incompatible types for operator /\n");
	//	exit(EXIT_FAILURE);
	//}
	//cst_free($1);
	//cst_free($3);
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
