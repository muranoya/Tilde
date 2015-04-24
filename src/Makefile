CC=cc
CFLAGS=-O2 -Wall

llparser: llparser.c string.c lex.c hashmap.c list.c error.c
	$(CC) $(CFLAGS) -o llparser $^ -DTEST_LL_PARSER

string: string.c error.c
	$(CC) $(CFLAGS) -o string $^ -DTEST_STRING

list: list.c error.c
	$(CC) $(CFLAGS) -o list $^ -DTEST_LIST

hashmap: hashmap.c string.c error.c
	$(CC) $(CFLAGS) -o hashmap $^ -DTEST_HASHMAP

lex: lex.c string.c error.c
	$(CC) $(CFLAGS) -o lex $^ -DTEST_LEX

genparser: genparser.c hashmap.c lex.c list.c string.c
	$(CC) $(CFLAGS) -o genparser $^

clean:

.PHONY: all clean

