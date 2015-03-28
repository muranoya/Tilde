#ifndef H_HASHMAP
#define H_HASHMAP

struct Data
{
    char *k;
    int x;
    struct Data *next;
};

struct List
{
    struct Data *head;
    struct Data *tail;
    int len;
};

struct Hashmap
{
    struct List *list;
    unsigned int size;
};

struct Hashmap *init_hashmap(unsigned int size);
void fin_hashmap(struct Hashmap *h);
int  add_hashmap(struct Hashmap *h, const char *key, int x);
int  remove_hashmap(struct Hashmap *h, const char *key);
int  exists_hashmap(struct Hashmap *h, const char *key);
int *search_hashmap(struct Hashmap *h, const char *key);
#endif

