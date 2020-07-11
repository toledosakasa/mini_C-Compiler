CC = gcc

eeyore: eeyore.tab.o lex.yy.o util.o geneeyore.o
	$(CC) eeyore.tab.o lex.yy.o util.o geneeyore.o -lfl -o eeyore 

eeyore.tab.c: mini_C.bison
	bison mini_C.bison
	mv mini_C.tab.bison eeyore.tab.c

lex.yy.c: mini_C.flex
	flex mini_C.flex

eeyore.tab.o: eeyore.tab.c globals_.h util.h scan.h geneeyore.h
	$(CC) eeyore.tab.c -c

lex.yy.o: lex.yy.c globals.h util.h scan.h
	$(CC) lex.yy.c -c

util.o: util.c globals.h util.h
	$(CC) util.c -c

geneeyore.o: geneeyore.c globals.h util.h geneeyore.h
	$(CC) geneeyore.c -c
	
clean:
	-rm -f *.o lex.yy.c eeyore.tab.c
