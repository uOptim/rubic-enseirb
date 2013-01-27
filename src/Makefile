CFLAGS=-Wall -Wextra -g # -O0
LDFLAGS=-lfl #-ly # les bibliotheques necessaires
CC=gcc

HEADERS=block.h gencode.h genfunc.h hashmap.h instruction.h stack.h symtable.h types.h

all: rubic

rubic: y.tab.o block.o builtins.o gencode.o genfunc.o hashmap.o instruction.o lex.yy.o stack.o symtable.o types.o 
	gcc $^ -o rubic2llvm $(LDFLAGS) 

lex.yy.c: scanner.l 
	lex $<

y.tab.c: parse.y $(HEADERS)
	yacc -d $<

y.tab.o: y.tab.c y.tab.h

lex.yy.o: lex.yy.c

stack.o: stack.c stack.h

hashmap.o: hashmap.c hashmap.h

builtins.o: builtins.c

types.o: types.c $(HEADERS)

genfunc.o: genfunc.c $(HEADERS)

gencode.o: gencode.c $(HEADERS)

symtable.o: symtable.c $(HEADERS)

block.o: block.c block.h $(HEADERS)

instruction.o: instruction.c $(HEADERS)


clean:
	@rm -f *~ *.o lex.yy.c y.tab.c y.tab.h rubic2llvm
