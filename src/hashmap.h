#ifndef H_HASHMAP
#define H_HASHMAP

#include "util.h"

struct list;

struct Hashmap
{
    struct list *list;
    unsigned int size;
};

struct Hashmap *make_hashmap(unsigned int size);
void  free_hashmap(struct Hashmap *h, void (*)(void*));
bool  add_hashmap(struct Hashmap *h, const char *key, void *data);
bool  remove_hashmap(struct Hashmap *h, const char *key, void (*)(void*));
bool  exists_hashmap(struct Hashmap *h, const char *key);
void *search_hashmap(struct Hashmap *h, const char *key);
#endif

