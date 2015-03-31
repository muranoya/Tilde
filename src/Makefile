CC=cc
CFLAGS=-O2 -Wall

blist: blist.c blist.h
	$(CC) $(CFLAGS) -o blist blist.c -DTEST_BLIST

hashmap: hashmap.c hashmap.h
	$(CC) $(CFLAGS) -o hashmap hashmap.c -DTEST_HASHMAP

lex: lex.c tilde.h
	$(CC) $(CFLAGS) -o lex lex.c -DTEST_LEX

genparser: genparser.c tilde.h hashmap.c hashmap.h lex.c
	$(CC) $(CFLAGS) -o genparser genparser.c hashmap.c lex.c

clean:

.PHONY: all clean

