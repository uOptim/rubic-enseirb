CFLAGS= #-Wall -Wextra -g -O0  # -g, -O3 , ... par exemple
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
rubic: y.tab.o lex.yy.o hashmap.o symtable.o
	$(CC) -o $@ $^ $(LDFLAGS)

symtable.o: symtable.c
	$(CC) $(CFLAGS) -c $^ -o $@

hashmap.o: hashmap.c
	$(CC) $(CFLAGS) -c $^ -o $@

clean:
	@rm -f *~ *.o lex.yy.c y.tab.c y.tab.h lex.yy.c

mrproper:
	@make clean
	@rm -f rubic 
