%{
	#include <stdio.h>
	#include <string.h>
	#include <stdlib.h>

	#include "stack.h"
	#include "totrash.h"
	#include "hashmap.h"
	#include "symtable.h"

	#include "y.tab.h"

	struct class    *tmp_class;
	struct function *tmp_function;

	// scope blocks
	struct block {
		struct stack *classes;
		struct stack *variables;
		struct stack *functions;
	};

	// symbol table
	struct hashmap *classes;
	struct hashmap *variables;
	struct hashmap *functions;

	void yyerror(char *);

	unsigned int new_reg();

%}

%union {
	int n;
	char c;
	char *s;
	double f;

	struct var *v;
};

%token AND OR CLASS IF THEN ELSE END WHILE DO DEF LEQ GEQ 
%token FOR TO RETURN IN NEQ
%token COMMENT
%token <c> BOOL
%token <s> STRING
%token <f> FLOAT
%token <n> INT
%token <s> ID 

%type <v> expr

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
	if (hashmap_get(classes, $2) != NULL) {
		fprintf(stderr, "Class %s already exists\n", $2);
		exit(EXIT_FAILURE);
	}

	tmp_class->cn = $2;

	hashmap_set(classes, $2, tmp_class);
	tmp_class = class_new();
	//free($2);
}
                | CLASS ID '<' ID term stmts terms END
{
	struct class *super = hashmap_get(classes, $4);

	if (super == NULL) {
		fprintf(stderr, "Super class %s of %s not defined\n", $4, $2);
		exit(EXIT_FAILURE);
	}

	if (hashmap_get(classes, $2) != NULL) {
		fprintf(stderr, "Class %s already exists\n", $2);
		exit(EXIT_FAILURE);
	}

	tmp_class->cn = $2;
	tmp_class->super = super;

	hashmap_set(classes, $2, tmp_class);
	tmp_class = class_new();

	//free($2);
	free($4);
}
                | stmt
{
	// here: top statements
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

stmt		: IF expr THEN stmts terms END
                | IF expr THEN stmts terms ELSE stmts terms END 
                | FOR ID IN expr TO expr term stmts terms END
                | WHILE expr DO term stmts terms END 
                | lhs '=' expr
                | RETURN expr
{
	// uncomment when expr is defined
	//tmp_function->ret = $2;
}
                | DEF ID opt_params term stmts terms END
{
	if (hashmap_get(functions, $2) != NULL) {
		fprintf(stderr, "Function %s already defined\n", $2);
		exit(EXIT_FAILURE);
	}

	tmp_function->fn = $2;

	hashmap_set(functions, $2, tmp_function);
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
}
                | FLOAT
{
	struct var *v = var_new("float", new_reg());
}
                | INT
{
	struct var *v = var_new("integer", new_reg());
}
                | '(' expr ')'
;
expr            : expr AND comp_expr
                | expr OR comp_expr
                | comp_expr
;
comp_expr       : additive_expr '<' additive_expr
                | additive_expr '>' additive_expr
                | additive_expr LEQ additive_expr
                | additive_expr GEQ additive_expr
                | additive_expr EQ additive_expr
                | additive_expr NEQ additive_expr
                | additive_expr
;
additive_expr   : multiplicative_expr
                | additive_expr '+' multiplicative_expr
                | additive_expr '-' multiplicative_expr
;
multiplicative_expr : multiplicative_expr '*' primary
                    | multiplicative_expr '/' primary
                    | primary
;
opt_terms	: /* none */
		| terms
		;

terms		: terms ';'
                | terms '\n'
		| ';'
                | '\n'
		;
term            : ';'
                | '\n'
;
%%
int main() {
	tmp_class    = class_new();
	tmp_function = function_new();

	classes   = hashmap_new();
	functions = hashmap_new();
	variables = hashmap_new();

	yyparse(); 

	hashmap_dump(classes, class_dump);
	hashmap_dump(functions, function_dump);

	return 0;
}


unsigned int new_reg() {
	static unsigned int reg = 0;
	return reg++;
}
