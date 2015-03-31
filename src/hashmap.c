#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hashmap.h"

struct container
{
    char *key;
    void *data;
    struct container *next;
};

struct list
{
    struct container *head;
    struct container *tail;
    int len;
};

static unsigned int calc_hash(unsigned int size, const char *key);
static void free_list(struct list *list, void (*)(void*));

struct Hashmap *
init_hashmap(unsigned int size)
{
    struct Hashmap *hmap;
    struct list *list;
    int i;

    hmap = (struct Hashmap*)malloc(sizeof(struct Hashmap));
    hmap->list = (struct list*)malloc(sizeof(struct list)*size);
    hmap->size = size;

    for (i = 0; i < size; ++i)
    {
        list = hmap->list+i;
        list->head = list->tail = NULL;
        list->len = 0;
    }
    return hmap;
}

void
free_hashmap(struct Hashmap *hmap, void (*liberator)(void*))
{
    int i;
    for (i = 0; i < hmap->size; ++i)
    {
        free_list(hmap->list+i, liberator);
    }
    free(hmap->list);
    free(hmap);
}

int
add_hashmap(struct Hashmap *hmap, const char *key, void *data)
{
    unsigned int hash;
    char *str;
    struct container *container;
    struct list *list;
    
    if (exists_hashmap(hmap, key)) return 0;

    hash = calc_hash(hmap->size, key);
    str = (char *)malloc(sizeof(char)*(strlen(key)+1));
    strcpy(str, key);
    container = (struct container *)malloc(sizeof(struct container));
    list = hmap->list+hash;

    container->key = str;
    container->data = data;
    container->next = NULL;

    if (list->len == 0)
    {
        list->head = container;
        list->tail = container;
    }
    else
    {
        list->tail->next = container;
        list->tail = container;
    }
    list->len++;
    return 1;
}

int
remove_hashmap(struct Hashmap *hmap, const char *key, void (*liberator)(void*))
{
    unsigned int hash = calc_hash(hmap->size, key);
    struct list *list;
    struct container *now, *prev = NULL;

    list = hmap->list+hash;
    now = list->head;
    for (;;)
    {
        if (now == NULL) return 0;
        if (strcmp(key, now->key) == 0)
        {
            if (list->head == now) list->head = now->next;
            if (list->tail == now) list->tail = prev;
            if (prev != NULL) prev->next = now->next;
            liberator(now->data);
            free(now);
            list->len--;
            return 1;
        }
        prev = now;
        now = now->next;
    }
}

int
exists_hashmap(struct Hashmap *hmap, const char *key)
{
    int *ret = search_hashmap(hmap, key);
    return ret == NULL ? 0 : 1;
}

void *
search_hashmap(struct Hashmap *hmap, const char *key)
{
    unsigned int hash = calc_hash(hmap->size, key);
    struct container *container = (hmap->list+hash)->head;

    for (;;)
    {
        if (container == NULL) return NULL;
        if (strcmp(key, container->key) == 0)
        {
            return container->data;
        }
        container = container->next;
    }
}

static unsigned int
calc_hash(unsigned int size, const char *key)
{
    // FNV Hash
    unsigned int hash;
    int i;
    int len = strlen(key);

    hash = 2166136261u;
    for(i = 0 ; i < len; ++i)
    {
        hash = (16777619u*hash)^(*key+i);
    }
    return hash % size;
}

static void
free_list(struct list *list, void (*liberator)(void*))
{
    struct container *now, *next;
    
    if (list->len == 0) return;
    now = list->head;
    next = now->next;
    
    for (;;)
    {
        liberator(now->data);
        free(now);
        if (next == NULL) return;
        now = next;
        next = next->next;
    }
}

#ifdef TEST_HASHMAP
static int
count(const struct Hashmap *hmap)
{
    int s, i;
    for (s = i = 0; i < hmap->size; ++i)
    {
        s += (hmap->list+i)->len;
    }
    return s;
}

static void
debug_print(const struct Hashmap *hmap)
{
    struct list *list;
    struct container *container;
    int i;
    for (i = 0; i < hmap->size; ++i)
    {
        list = hmap->list+i;
        container = list->head;
        for (;;)
        {
            if (container != NULL)
            {
                printf("key=%s, val=%d, hash=%d\n",
                        container->key,
                        *(int*)container->data,
                        calc_hash(hmap->size, container->key));
            }
            else
            {
                break;
            }
            container = container->next;
        }
    }
}

static void
liberator_int(void *data)
{
    free(data);
}

int
main(int argc, char *argv[])
{
    struct Hashmap *map = init_hashmap(16);
    const int BUFSIZE = 128;
    char buf[BUFSIZE], key[BUFSIZE];
    int val, *valp;

    for (;;)
    {
        fgets(buf, BUFSIZE, stdin);
        if (strncmp(buf, "add", 3) == 0)
        {
            puts("key="); fgets(key, BUFSIZE, stdin);
            puts("val="); fgets(buf, BUFSIZE, stdin);
            valp = (int*)malloc(sizeof(int));
            *valp = atoi(buf);
            printf("Add to hashmap (%s, %d)\n", key, *valp);
            add_hashmap(map, key, valp) ? puts("Success") : puts("Failed");
        }
        else if (strncmp(buf, "del", 3) == 0)
        {
            puts("key="); fgets(key, BUFSIZE, stdin);
            printf("Delete from hashmap (%s)\n", key);
            remove_hashmap(map, key, liberator_int) ? puts("Success") : puts("Failed");
        }
        else if (strncmp(buf, "set", 3) == 0)
        {
            puts("key="); fgets(key, BUFSIZE, stdin);
            puts("val="); fgets(buf, BUFSIZE, stdin);
            val = atoi(buf);
            printf("Set new value to hashmap (%s, %d)\n", key, val);
            valp = search_hashmap(map, key);
            if (valp == NULL)
            {
                puts("Failed");
            }
            else
            {
                puts("Success");
                *valp = val;
            }
        }
        else if (strncmp(buf, "prn", 3) == 0)
        {
            puts("key="); fgets(key, BUFSIZE, stdin);
            printf("Search from hashmap (%s)\n", key);
            valp = search_hashmap(map, key);
            if (valp == NULL)
            {
                puts("Failed");
            }
            else
            {
                puts("Success");
                printf("val = %d\n", *(int*)valp);
            }
        }
        else if (strncmp(buf, "all", 3) == 0)
        {
            printf("All items = %d\n", count(map));
            debug_print(map);
        }
        else
        {
            free_hashmap(map, liberator_int);
            return EXIT_SUCCESS;
        }
    }
}
#endif

