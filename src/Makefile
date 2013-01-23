CFLAGS=-Wall -Wextra -g # -O0
LDFLAGS=-lfl #-ly # les bibliotheques necessaires
CC=gcc

all: rubic

lex.yy.c: scanner.l
	lex $<

y.tab.c: parse.y
	yacc -d $<

y.tab.o: y.tab.c y.tab.h
	$(CC) $(CFLAGS) -c $<

lex.yy.o: lex.yy.c
	$(CC) $(CFLAGS) -c $<

rubic: y.tab.o lex.yy.o hashmap.o symtable.o stack.o block.o instruction.o gencode.o gencode.h types.o genfunc.o
	$(CC) -o $@ $^ $(LDFLAGS)

symtable.o: symtable.c symtable.h hashmap.h stack.h
	$(CC) $(CFLAGS) -c $< -o $@

hashmap.o: hashmap.c hashmap.h
	$(CC) $(CFLAGS) -c $< -o $@

stack.o: stack.c stack.h
	$(CC) $(CFLAGS) -c $< -o $@

block.o: block.c block.h symtable.h hashmap.h
	$(CC) $(CFLAGS) -c $< -o $@

genfunc.o: genfunc.c genfunc.h symtable.h stack.h instruction.h
	$(CC) $(CFLAGS) -c $< -o $@

types.o: types.c types.h symtable.h
	$(CC) $(CFLAGS) -c $< -o $@

instruction.o: instruction.c instruction.h symtable.h
	$(CC) $(CFLAGS) -c $< -o $@

gencode.o: gencode.c gencode.h symtable.h stack.h instruction.h
	$(CC) $(CFLAGS) -c $< -o $@

tests: tests.c stack.o hashmap.o
	$(CC) $(CFLAGS) $^ -o $@

clean:
	@rm -f *~ *.o lex.yy.c y.tab.c y.tab.h rubic
