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

rubic: y.tab.o lex.yy.o hashmap.o symtable.o stack.o
	$(CC) -o $@ $^ $(LDFLAGS)

symtable.o: symtable.c
	$(CC) $(CFLAGS) -c $^ -o $@

hashmap.o: hashmap.c hashmap.h
	$(CC) $(CFLAGS) -c $< -o $@

stack.o: stack.c stack.h
	$(CC) $(CFLAGS) -c $< -o $@

tests: tests.c stack.o hashmap.o
	$(CC) $(CFLAGS) $^ -o $@

clean:
	@rm -f *~ *.o lex.yy.c y.tab.c y.tab.h lex.yy.c

mrproper:
	@make clean
	@rm -f rubic 
