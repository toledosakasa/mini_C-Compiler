CC = gcc

tiggerC: tigger.tab.o lex.yy.o util.o geneeyore.o gentrigger.o
	$(CC) tigger.tab.o lex.yy.o util.o geneeyore.o gentrigger.o -lfl -o tiggerC 

tigger.tab.c: mini_C.bison
	bison mini_C.bison
	mv mini_C.tab.bison tigger.tab.c

lex.yy.c: mini_C.flex
	flex mini_C.flex

tigger.tab.o: tigger.tab.c globals_.h util.h scan.h geneeyore.h gentrigger.h
	$(CC) tigger.tab.c -c

lex.yy.o: lex.yy.c globals.h util.h scan.h
	$(CC) lex.yy.c -c

util.o: util.c globals.h util.h
	$(CC) util.c -c

geneeyore.o: geneeyore.c globals.h util.h geneeyore.h
	$(CC) geneeyore.c -c

gentrigger.o: gentrigger.c globals.h geneeyore.h
	$(CC) gentrigger.c -c

clean:
	-rm -f *.o lex.yy.c tigger.tab.c
