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
	void scope_add_symbol(struct block *, struct symbol *);

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
%type <symbol> topstmts topstmt stmts stmt

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
topstmt	        : CLASS ID term stmts terms END 
{
	// error checking
	if (hashmap_get(scope->classes, $2) != NULL) {
		fprintf(stderr, "Class %s already exists\n", $2);
		exit(EXIT_FAILURE);
	}

	tmp_class->cn = $2;

	hashmap_set(scope->classes, $2, tmp_class);
	tmp_class = class_new();
	//free($2);
}
                | CLASS ID '<' ID term stmts terms END
{
	// error checking
	struct class *super = hashmap_get(scope->classes, $4);

	if (super == NULL) {
		fprintf(stderr, "Super class %s of %s not defined\n", $4, $2);
		exit(EXIT_FAILURE);
	}

	if (hashmap_get(scope->classes, $2) != NULL) {
		fprintf(stderr, "Class %s already exists\n", $2);
		exit(EXIT_FAILURE);
	}

	tmp_class->cn = $2;
	tmp_class->super = super;

	hashmap_set(scope->classes, $2, tmp_class);
	tmp_class = class_new();

	//free($2);
	free($4);
}
                | stmt
{
	struct block *s = scope; // tmp save

	scope = stack_pop(scopes);
	// uncomment when stmt is defined
	//scope_add_symbol(scope, $1);
	stack_push(scopes, scope);

	scope = s; // restore scope
}
;

stmts	        : /* none */
                | stmt
{
}
                | stmts terms stmt
{
}
                ;

stmt			: IF expr THEN stmts terms END
                | IF expr THEN stmts terms ELSE stmts terms END 
                | FOR ID IN expr TO expr term stmts terms END
                | WHILE expr DO term stmts terms END 
                | lhs '=' expr
                | RETURN expr
{
}
                | DEF ID opt_params term stmts terms END
{
	if (hashmap_get(scope->functions, $2) != NULL) {
		fprintf(stderr, "Function %s already defined\n", $2);
		exit(EXIT_FAILURE);
	}

	tmp_function->fn = $2;

	hashmap_set(scope->functions, $2, tmp_function);
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
}
                | ID
{
	struct var *v = var_new($1, new_reg());
	v->tt = UND_T;

	stack_push(tmp_function->params, v);
}
; 
lhs             : ID
                | ID '.' primary
                | ID '(' exprs ')'
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

	scopes = stack_new();
	stack_push(scopes, block_new()); // top level scope

	scope = block_new(); // 2nd level scope

	yyparse(); 

	hashmap_dump(scope->classes, class_dump);
	hashmap_dump(scope->functions, function_dump);

	block_free(scope);
	stack_free(&scopes, block_free);

	return 0;
}


unsigned int new_reg() {
	static unsigned int reg = 0;
	return reg++;
}


void scope_add_symbol(struct block *s, struct symbol *sym)
{
	switch(sym->type) {
		case VAR_T:
			hashmap_set(s->variables, sym->name, sym->ptr);
			break;
		case FUN_T:
			hashmap_set(s->functions, sym->name, sym->ptr);
			break;
		case CLA_T:
			hashmap_set(s->classes, sym->name, sym->ptr);
			break;
	}
}

