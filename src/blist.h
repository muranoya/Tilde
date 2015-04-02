#ifndef H_BLIST
#define H_BLIST

#include "util.h"

struct Blist_body
{
    struct Blist_body *prev;
    struct Blist_body *next;
    void *data;
};

struct Blist
{
    struct Blist_body *head;
    struct Blist_body *tail;
    int len;
};

struct iterator_blist
{
    struct Blist_body *p;
};

struct Blist *make_blist();
void free_blist(struct Blist *list, void (*liberator)(void*));
int  count_blist(const struct Blist *list);
void add_blist(struct Blist *list, void *data);
bool remove_blist(struct Blist *list, void (*liberator)(void*), int n);
bool remove_blist2(struct Blist *list, int (*cmp)(void*, void*),
        void (*liberator)(void*), void *data);
void *at_blist(struct Blist *list, int n);

void init_iterator_blist(const struct Blist *list, struct iterator_blist *iter);
bool isend_iterator_blist(const struct iterator_blist *iter);
void next_iterator_blist(struct iterator_blist *iter);
void *value_iterator_blist(const struct iterator_blist *iter);
#endif

