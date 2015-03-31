#ifndef H_HASHMAP
#define H_HASHMAP

struct list;

struct Hashmap
{
    struct list *list;
    unsigned int size;
};

struct Hashmap *init_hashmap(unsigned int size);
void  free_hashmap(struct Hashmap *h, void (*)(void*));
int   add_hashmap(struct Hashmap *h, const char *key, void *data);
int   remove_hashmap(struct Hashmap *h, const char *key, void (*)(void*));
int   exists_hashmap(struct Hashmap *h, const char *key);
void *search_hashmap(struct Hashmap *h, const char *key);
#endif

