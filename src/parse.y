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

	// symbols stack
	struct stack *scope;

	unsigned int new_reg();

	struct symbol * symbol_lookup(const char *);

	void yyerror(char *);
%}

%union {
	int n;
	char c;
	char *s;
	double f;
	
	unsigned int cnt;
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
%type <cnt> topstmts topstmt stmts stmt params opt_params

%left '*' 
%left '/'
%left '+' '-'
%left '<' '>' LEQ GEQ EQ
%left AND OR
%%

program		:  topstmts opt_terms
;
topstmts        :      
{
	$$ = 0;
}
				| topstmt
{
	$$ = 0;
	if ($1 != 0) $$ = 1;
}
				| topstmts terms topstmt
{
	$$ = $1;
	if ($1 != 0) ++$$;
}
;
topstmt	        : CLASS ID term stmts terms END 
{
	struct symbol *s;
	unsigned int n = $4;

	// pop symbols from stmts
	printf("%s declared %d symbols:\n", $2, n);
	while (n--) {
		s = stack_pop(scope);
		printf("Symbol: %s\n", s->name);
	}

	// error checking
	if (symbol_lookup($2) != NULL) {
		fprintf(stderr, "Class %s already exists\n", $2);
		exit(EXIT_FAILURE);
	}

	tmp_class->cn = $2;
	stack_push(scope, sym_new($2, CLA_T, tmp_class));
	tmp_class = class_new();

	$$ = 1;

	//free($2);
}
                | CLASS ID '<' ID term stmts terms END
{
	struct symbol *s;
	unsigned int n = $6;

	// pop symbols from stmts
	printf("%s declared %d symbols\n", $2, n);
	while (n--) {
		s = stack_pop(scope);
		printf("Symbol: %s\n", s->name);
	}

	// error checking
	s = symbol_lookup($4);

	if (s == NULL) {
		fprintf(stderr, "Super class %s of %s not defined\n", $4, $2);
		exit(EXIT_FAILURE);
	}

	if (symbol_lookup($2) != NULL) {
		fprintf(stderr, "Class %s already exists\n", $2);
		exit(EXIT_FAILURE);
	}

	struct class *super = (struct class *) s->ptr;

	tmp_class->cn = $2;
	tmp_class->super = super;
	stack_push(scope, sym_new($2, CLA_T, tmp_class));
	tmp_class = class_new();

	$$ = 1;

	//free($2);
	free($4);
}
                | stmt
{
	$$ = 0;
	if ($1 != 0) $$ = 1;
}
;

stmts	        : /* none */
{
	$$ = 0;
}
                | stmt
{
	$$ = 0;
	if ($1 != 0) $$ = 1;
}
                | stmts terms stmt
{
	$$ = $1;
	if ($3 != 0) ++$$;
}
                ;

stmt			: IF expr THEN stmts terms END
{
	$$ = 0;
}
                | IF expr THEN stmts terms ELSE stmts terms END 
{
	$$ = 0;
}
                | FOR ID IN expr TO expr term stmts terms END
{
	$$ = 0;
	free($2);
}
                | WHILE expr DO term stmts terms END 
{
	$$ = 0;
}
                | lhs '=' expr
{
	$$ = 1;
}
                | RETURN expr
{
	$$ = 0;
}
                | DEF ID opt_params term stmts terms END
{
	printf("%s declared %d symbols\n", $2, $3+$5);

	if (symbol_lookup($2) != NULL) {
		fprintf(stderr, "Function %s already defined\n", $2);
		exit(EXIT_FAILURE);
	}

	tmp_function->fn = $2;
	stack_push(scope, sym_new($2, FUN_T, tmp_function));
	tmp_function = function_new();

	$$ = 1;
	//free($2);
}
; 

opt_params      : /* none */
{
	$$ = 0;
}
                | '(' ')'
{
	$$ = 0;
}
                | '(' params ')'
{
	$$ = $2;
}
;
params          : ID ',' params
{
	$$ = 1 + $3;

	struct var *v = var_new($1, new_reg());
	v->tt = UND_T;

	stack_push(tmp_function->params, v);
	free($1);
}
                | ID
{
	$$ = 1;

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

	scope = stack_new();

	yyparse(); 

	struct symbol *s;
	printf("Top level symbols:\n");
	while ((s = stack_pop(scope)) != NULL) {
		printf("Symbol: %s\n", s->name);
		sym_free(s);
	};

	return 0;
}


unsigned int new_reg() {
	static unsigned int reg = 0;
	return reg++;
}


struct symbol * symbol_lookup(const char *name)
{
	unsigned int i;
	struct symbol *sym;

	// stack peak highly ineffective!
	// improve perfs later
	for (i = 0; (sym = stack_peak(scope, i)) != NULL; ++i) {
		if (strcmp(sym->name, name) == 0) return sym;
	}

	return NULL;
}
