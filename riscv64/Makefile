CC = gcc

riscv64C: riscv64.tab.o lex.yy.o util.o geneeyore.o genriscv.o
	$(CC) riscv64.tab.o lex.yy.o util.o geneeyore.o genriscv.o -lfl -o riscv64C 

riscv64.tab.c: mini_C.bison
	bison mini_C.bison
	mv mini_C.tab.bison riscv64.tab.c

lex.yy.c: mini_C.flex
	flex mini_C.flex

riscv64.tab.o: riscv64.tab.c globals_.h util.h scan.h geneeyore.h genriscv.h
	$(CC) riscv64.tab.c -c

lex.yy.o: lex.yy.c globals.h util.h scan.h
	$(CC) lex.yy.c -c

util.o: util.c globals.h util.h
	$(CC) util.c -c

geneeyore.o: geneeyore.c globals.h util.h geneeyore.h
	$(CC) geneeyore.c -c

genriscv.o: genriscv.c globals.h geneeyore.h
	$(CC) genriscv.c -c

clean:
	-rm -f *.o lex.yy.c riscv64.tab.c
