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

	// symbol table
	struct block *scope;
	struct stack *scopes;

	unsigned int new_reg();

	void            add_symbol(struct symbol *);
	struct symbol * symbol_lookup(char, const char *);

	void yyerror(char *);
%}

%union {
	int n;
	char c;
	char *s;
	double f;
	
	unsigned int reg;
	struct symbol *symbol;
};

%token AND OR CLASS IF THEN ELSE END WHILE DO DEF LEQ GEQ 
%token FOR TO RETURN IN NEQ
%token COMMENT
%token <c> BOOL
%token <s> STRING
%token <f> FLOAT
%token <n> INT
%token <s> ID 

%type <reg> lhs primary expr comp_expr additive_expr multiplicative_expr
%type <symbol> topstmt stmt

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
{
	if ($1 != NULL) {
		struct block *s = scope; // tmp save

		scope = stack_pop(scopes);
		add_symbol($1);
		stack_push(scopes, scope);

		scope = s; // restore scope
	}
	
	else {
		puts("ARGH!");
	}
}
				| topstmts terms topstmt
{
	if ($3 != NULL) {
		struct block *s = scope; // tmp save

		scope = stack_pop(scopes);
		add_symbol($3);
		stack_push(scopes, scope);

		scope = s; // restore scope
	}
	else {
		puts("ARGH!");
	}
}
;
topstmt	        : CLASS ID term stmts terms END 
{
	// error checking
	if (hashmap_get(scope->classes, $2) != NULL) {
		fprintf(stderr, "Class %s already exists\n", $2);
		exit(EXIT_FAILURE);
	}

	tmp_class->cn = $2;

	$$ = sym_new($2, CLA_T, tmp_class);
	tmp_class = class_new();

	//free($2);
}
                | CLASS ID '<' ID term stmts terms END
{
	// error checking
	struct symbol *s = symbol_lookup(CLA_T, $4);

	if (s == NULL) {
		fprintf(stderr, "Super class %s of %s not defined\n", $4, $2);
		exit(EXIT_FAILURE);
	}

	if (symbol_lookup(CLA_T, $2) != NULL) {
		fprintf(stderr, "Class %s already exists\n", $2);
		exit(EXIT_FAILURE);
	}

	struct class *super = (struct class *) s->ptr;

	tmp_class->cn = $2;
	tmp_class->super = super;

	$$ = sym_new($2, CLA_T, tmp_class);
	tmp_class = class_new();

	//free($2);
	free($4);
}
                | stmt
{
	$$ = $1;
}
;

stmts	        : /* none */
                | stmt
{
	if ($1 != NULL) {
		add_symbol($1);
	}
}
                | stmts terms stmt
{
	if ($3 != NULL) {
		add_symbol($3);
	}
}
                ;

stmt			: IF expr THEN stmts terms END
{
	$$ = NULL;
}
                | IF expr THEN stmts terms ELSE stmts terms END 
{
	$$ = NULL;
}
                | FOR ID IN expr TO expr term stmts terms END
{
	$$ = NULL;
	free($2);
}
                | WHILE expr DO term stmts terms END 
{
	$$ = NULL;
}
                | lhs '=' expr
{
	$$ = NULL;
}
                | RETURN expr
{
	$$ = NULL;
}
                | DEF ID opt_params term stmts terms END
{
	if (hashmap_get(scope->functions, $2) != NULL) {
		fprintf(stderr, "Function %s already defined\n", $2);
		exit(EXIT_FAILURE);
	}

	tmp_function->fn = $2;

	$$ = sym_new($2, FUN_T, tmp_function);
	tmp_function = function_new();

	//free($2);
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
;
exprs           : exprs ',' expr
                | expr
;
primary         : lhs
                | STRING 
{
	struct var *v = var_new("string", new_reg());
	$$ = v->reg;
}
                | FLOAT
{
	struct var *v = var_new("float", new_reg());
	$$ = v->reg;
}
                | INT
{
	struct var *v = var_new("integer", new_reg());
	$$ = v->reg;
}
                | '(' expr ')'
;
expr            : expr AND comp_expr
{
	struct var *v = var_new("AND result", new_reg());
	$$ = v->reg;

	// GEN CODE, $$ MUST STORE THE RESULT
}
                | expr OR comp_expr
{
	struct var *v = var_new("OR result", new_reg());
	$$ = v->reg;

	// GEN CODE, $$ MUST STORE THE RESULT
}
                | comp_expr
{
	$$ = $1;
}
;
comp_expr       : additive_expr '<' additive_expr
{
	struct var *v = var_new("LT result", new_reg());
	$$ = v->reg;

	// GEN CODE, $$ MUST STORE THE RESULT
}
                | additive_expr '>' additive_expr
{
	struct var *v = var_new("GT result", new_reg());
	$$ = v->reg;

	// GEN CODE, $$ MUST STORE THE RESULT
}
                | additive_expr LEQ additive_expr
{
	struct var *v = var_new("LEQ result", new_reg());
	$$ = v->reg;

	// GEN CODE, $$ MUST STORE THE RESULT
}
                | additive_expr GEQ additive_expr
{
	struct var *v = var_new("GEQ result", new_reg());
	$$ = v->reg;

	// GEN CODE, $$ MUST STORE THE RESULT
}
                | additive_expr EQ additive_expr
{
	struct var *v = var_new("EQ result", new_reg());
	$$ = v->reg;

	// GEN CODE, $$ MUST STORE THE RESULT
}
                | additive_expr NEQ additive_expr
{
	struct var *v = var_new("NEQ result", new_reg());
	$$ = v->reg;

	// GEN CODE, $$ MUST STORE THE RESULT
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
	struct var *v = var_new("'+' result", new_reg());
	$$ = v->reg;

	// GEN CODE, $$ MUST STORE THE RESULT
}
                | additive_expr '-' multiplicative_expr
{
	struct var *v = var_new("'-' result", new_reg());
	$$ = v->reg;

	// GEN CODE, $$ MUST STORE THE RESULT
}
;
multiplicative_expr : multiplicative_expr '*' primary
{
	struct var *v = var_new("'*' result", new_reg());
	$$ = v->reg;

	// GEN CODE, $$ MUST STORE THE RESULT
}
                    | multiplicative_expr '/' primary
{
	struct var *v = var_new("'/' result", new_reg());
	$$ = v->reg;

	// GEN CODE, $$ MUST STORE THE RESULT
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
	tmp_class    = class_new();
	tmp_function = function_new();

	scope = block_new();

	scopes = stack_new();
	stack_push(scopes, block_new());

	yyparse(); 

	do {
		puts("===================");
		puts("Dumping classes for current scope");
		hashmap_dump(scope->classes, sym_dump);
		puts("Dumping functions for current scope");
		hashmap_dump(scope->functions, sym_dump);
		puts("Dumping variables for current scope");
		hashmap_dump(scope->variables, sym_dump);

		block_free(scope);

	} while ((scope = stack_pop(scopes)) != NULL);

	stack_free(&scopes, block_free);

	return 0;
}


unsigned int new_reg() {
	static unsigned int reg = 0;
	return reg++;
}


void add_symbol(struct symbol *sym)
{
	printf("Adding symbol %s\n", sym->name);
	switch(sym->type) {
		case VAR_T:
			hashmap_set(scope->variables, sym->name, sym);
			break;
		case FUN_T:
			hashmap_set(scope->functions, sym->name, sym);
			break;
		case CLA_T:
			hashmap_set(scope->classes, sym->name, sym);
			break;
	}
}


struct symbol * symbol_lookup(char type, const char *name)
{
	unsigned int i;
	struct block *b;
	struct symbol *sym;

	// stack peak highly ineffective!
	// improve perfs later
	for (i = 0; (b = stack_peak(scopes, i)) != NULL; ++i) {
		switch (type) {
			case VAR_T:
				sym = hashmap_get(b->variables, name);
				break;
			case FUN_T:
				sym = hashmap_get(b->functions, name);
				break;
			case CLA_T:
				sym = hashmap_get(b->classes, name);
				break;
			default:
				break;
		}

		if (sym != NULL) return sym;
	}

	return NULL;
}
