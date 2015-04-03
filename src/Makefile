CC=cc
CFLAGS=-O2 -Wall

string: string.c tilde.h
	$(CC) $(CFLAGS) -o string string.c -DTEST_STRING

list: list.c tilde.h
	$(CC) $(CFLAGS) -o list list.c -DTEST_LIST

hashmap: hashmap.c tilde.h
	$(CC) $(CFLAGS) -o hashmap hashmap.c -DTEST_HASHMAP

lex: lex.c tilde.h
	$(CC) $(CFLAGS) -o lex lex.c string.c -DTEST_LEX

genparser: genparser.c tilde.h hashmap.c lex.c
	$(CC) $(CFLAGS) -o genparser genparser.c hashmap.c list.c lex.c

clean:

.PHONY: all clean

