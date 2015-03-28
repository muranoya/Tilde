#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hashmap.h"

static unsigned int calc_hash(unsigned int size, const char *key);
static void free_list(struct List *list);

struct Hashmap *
init_hashmap(unsigned int size)
{
    struct Hashmap *hmap;
    struct List *list;
    int i;

    hmap = (struct Hashmap*)malloc(sizeof(struct Hashmap));
    hmap->list = (struct List*)malloc(sizeof(struct List)*size);
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
fin_hashmap(struct Hashmap *hmap)
{
    int i;
    for (i = 0; i < hmap->size; ++i)
    {
        free_list(hmap->list+i);
    }
    free(hmap->list);
    free(hmap);
}

int
add_hashmap(struct Hashmap *hmap, const char *key, int x)
{
    unsigned int hash;
    char *str;
    struct Data *data;
    struct List *list;
    
    if (exists_hashmap(hmap, key)) return 0;

    hash = calc_hash(hmap->size, key);
    str = (char *)malloc(sizeof(char)*(strlen(key)+1));
    strcpy(str, key);
    data = (struct Data *)malloc(sizeof(struct Data));
    list = hmap->list+hash;

    data->k = str;
    data->x = x;
    data->next = NULL;

    if (list->len == 0)
    {
        list->head = data;
        list->tail = data;
    }
    else
    {
        list->tail->next = data;
        list->tail = data;
    }
    list->len++;
    return 1;
}

int
remove_hashmap(struct Hashmap *hmap, const char *key)
{
    unsigned int hash = calc_hash(hmap->size, key);
    struct List *list;
    struct Data *now, *prev = NULL;

    list = hmap->list+hash;
    now = list->head;
    for (;;)
    {
        if (now == NULL) return 0;
        if (strcmp(key, now->k) == 0)
        {
            if (list->head == now) list->head = now->next;
            if (list->tail == now) list->tail = prev;
            if (prev != NULL) prev->next = now->next;
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

int *
search_hashmap(struct Hashmap *hmap, const char *key)
{
    unsigned int hash = calc_hash(hmap->size, key);
    struct Data *data = (hmap->list+hash)->head;

    for (;;)
    {
        if (data == NULL) return NULL;
        if (strcmp(key, data->k) == 0)
        {
            return &data->x;
        }
        data = data->next;
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
free_list(struct List *list)
{
    struct Data *now, *next;
    if (list->len == 0) return;
    now = list->head;
    next = now->next;
    for (;;)
    {
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
    struct List *list;
    struct Data *data;
    int i;
    for (i = 0; i < hmap->size; ++i)
    {
        list = hmap->list+i;
        data = list->head;
        for (;;)
        {
            if (data != NULL)
            {
                printf("key=%s, val=%d, hash=%d\n",
                        data->k,
                        data->x,
                        calc_hash(hmap->size, data->k));
            }
            else
            {
                break;
            }
            data = data->next;
        }
    }
}

int
main(int argc, char *argv[])
{
    struct Hashmap *map = init_hashmap(16);
    const int BUFSIZE = 128;
    char buf[BUFSIZE], key[BUFSIZE];
    int val, *val2;

    for (;;)
    {
        fgets(buf, BUFSIZE, stdin);
        if (strncmp(buf, "add", 3) == 0)
        {
            puts("key="); fgets(key, BUFSIZE, stdin);
            puts("val="); fgets(buf, BUFSIZE, stdin);
            val = atoi(buf);
            printf("Add to hashmap (%s, %d)\n", key, val);
            add_hashmap(map, key, val) ? puts("Success") : puts("Failed");
        }
        else if (strncmp(buf, "del", 3) == 0)
        {
            puts("key="); fgets(key, BUFSIZE, stdin);
            printf("Delete from hashmap (%s)\n", key);
            remove_hashmap(map, key) ? puts("Success") : puts("Failed");
        }
        else if (strncmp(buf, "set", 3) == 0)
        {
            puts("key="); fgets(key, BUFSIZE, stdin);
            puts("val="); fgets(buf, BUFSIZE, stdin);
            val = atoi(buf);
            printf("Set new value to hashmap (%s, %d)\n", key, val);
            val2 = search_hashmap(map, key);
            if (val2 == NULL)
            {
                puts("Failed");
            }
            else
            {
                puts("Success");
                *val2 = val;
            }
        }
        else if (strncmp(buf, "prn", 3) == 0)
        {
            puts("key="); fgets(key, BUFSIZE, stdin);
            printf("Search from hashmap (%s)\n", key);
            val2 = search_hashmap(map, key);
            if (val2 == NULL)
            {
                puts("Failed");
            }
            else
            {
                puts("Success");
                printf("val = %d\n", *val2);
            }
        }
        else if (strncmp(buf, "all", 3) == 0)
        {
            printf("All items = %d\n", count(map));
            debug_print(map);
        }
        else
        {
            fin_hashmap(map);
            return EXIT_SUCCESS;
        }
    }
}
#endif
