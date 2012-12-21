%{
#include <stdio.h>
#include "y.tab.h"

int column=0; int lineno=0;

/* Function that counts the current column */
void count() {
  int i;
  for (i = 0; yytext[i] != '\0'; i++) {
    if (yytext[i] == '\n')
      column = 0;
    else if (yytext[i] == '\t')
      column += 8 - (column % 8);
    else
      column++;
  }
}
/* Function called in case of error during parsing */
void yyerror(char *s) {
  fprintf(stderr,"%s line %d col %d\n",s,lineno,column);
}

%}
F ([0-9]+\.[0-9]*)
I ([0-9]+)
S (\"[^\n\"]*\")
ID ([$@]?[a-zA-Z_][a-zA-Z_0-9]*)
%%
{F}      { count(); return(FLOAT); }
{I}      { count(); return(INT); }
{S}      { count(); return(STRING); }
"for"    { count(); return(FOR); }
"while"  { count(); return(WHILE);}
"if"     { count(); return(IF); }
"then"   { count(); return(THEN); }
"else"   { count(); return(ELSE); }
"return" { count(); return(RETURN); }
"class"  { count(); return(CLASS); }
"def"    { count(); return(DEF); }
"in"     { count(); return(IN); }
"end"    { count(); return(END); }
".."	 { count(); return(TO); }
"<="	 { count(); return(LEQ); }
">="     { count(); return(GEQ); }
"=="	 { count(); return(EQ); }
"!="	 { count(); return(NEQ); }
"&&"     { count(); return(AND); }
"||"     { count(); return(OR); }
";"	 { count(); return(';'); }
","	 { count(); return(','); }
"."	 { count(); return('.'); }
"="      { count(); return('='); }
"("	 { count(); return('('); }
")"	 { count(); return(')'); }
"-"	 { count(); return('-'); }
"+"	 { count(); return('+'); }
"*"	 { count(); return('*'); }
"/"	 { count(); return('/'); }
"<"	 { count(); return('<'); }
">"	 { count(); return('>'); }
{ID}     { count(); return(ID); }
"\n"     { count(); lineno++; return('\n'); }
[ \t\v\f]  { count(); }
.        ;
%%

