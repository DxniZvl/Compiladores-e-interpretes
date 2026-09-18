CC = cc
LEX = flex

scanner: main.c lex.yy.c
	$(CC) main.c lex.yy.c -o scanner

lex.yy.c: scanner.l
	$(LEX) scanner.l

.PHONY: run clean
run: scanner
	./scanner < input.txt

clean:
	rm -f scanner lex.yy.c
