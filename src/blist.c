#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "blist.h"

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

struct Blist *
make_blist()
{
    struct Blist *blist;
    blist = (struct Blist*)malloc(sizeof(struct Blist));
    blist->head = blist->tail = NULL;
    blist->len = 0;
    return blist;
}

void
free_blist(struct Blist *list, void (*liberator)(void *))
{
    struct Blist_body *now, *next;
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

int
count_blist(struct Blist *list)
{
    return list->len;
}

void
add_blist(struct Blist *list, void *data)
{
    struct Blist_body *newbody;
    newbody = (struct Blist_body*)malloc(sizeof(struct Blist_body));
    newbody->prev = newbody->next = NULL;
    newbody->data = data;

    if (list->len == 0)
    {
        list->head = list->tail = newbody;
    }
    else
    {
        newbody->prev = list->tail;
        newbody->prev->next = newbody;
        list->tail = newbody;
    }
    list->len++;
}

int
remove_blist(struct Blist *list, void (*liberator)(void *), int n)
{
    struct Blist_body *b;
    
    if (n >= list->len)
    {
        return 0;
    }
    else if (list->len == 1)
    {
        liberator(list->head->data);
        free(list->head);
        list->head = list->tail = NULL;
        list->len = 0;
        return 1;
    }
    else if (n == 0)
    {
        b = list->head;
        list->head = b->next;
        b->next->prev = NULL;
        liberator(b->data);
        free(b);
        list->len--;
        return 1;
    }
    else if (n == list->len-1)
    {
        b = list->tail;
        list->tail = b->prev;
        b->prev->next = NULL;
        liberator(b->data);
        free(b);
        list->len--;
        return 1;
    }

    if (n >= list->len / 2)
    {
        n = list->len-1-n;
        for (b = list->tail; ; --n)
        {
            if (n == 0)
            {
                b->prev->next = b->next;
                b->next->prev = b->prev;
                liberator(b->data);
                free(b);
                list->len--;
                return 1;
            }
            b = b->prev;
        }
    }
    else
    {
        for (b = list->head; ; --n)
        {
            if (n == 0)
            {
                b->prev->next = b->next;
                b->next->prev = b->prev;
                liberator(b->data);
                free(b);
                list->len--;
                return 1;
            }
            b = b->next;
        }
    }
}

int
remove_blist2(struct Blist *list, int (*cmp)(void*, void*), void (*liberator)(void*), void *data)
{
    struct Blist_body *b;

    for (b = list->head;;)
    {
        if (b == NULL) return 0;
        if (cmp(data, b->data) == 0)
        {
            if (b == list->head && b == list->tail)
            {
                list->head = list->tail = NULL;
            }
            else if (b == list->head)
            {
                list->head = b->next;
                if (b->next != NULL) b->next->prev = NULL;
            }
            else if (b == list->tail)
            {
                list->tail = b->prev;
                if (b->prev != NULL) b->prev->next = NULL;
            }
            else
            {
                b->prev->next = b->next;
                b->next->prev = b->prev;
            }
            liberator(b->data);
            free(b);
            list->len--;
            return 1;
        }
        b = b->next;
    }
    return 0;
}

void *
at_blist(struct Blist *list, int n)
{
    struct Blist_body *b;

    if (n >= list->len) return NULL;

    if (n >= list->len / 2)
    {
        b = list->tail;
        for (n = list->len-1-n;; --n)
        {
            if (n == 0) return b;
            b = b->prev;
        }
    }
    else
    {
        b = list->head;
        for (;; --n)
        {
            if (n == 0) return b;
            b = b->next;
        }
    }
}

void
init_iterator_blist(const struct Blist *list, struct iterator_blist *iter)
{
    iter->p = list->head;
}

int
hasNext_blist(const struct iterator_blist *iter)
{
    return iter->p != NULL;
}

void
next_blist(struct iterator_blist *iter)
{
    if (iter->p != NULL)
    {
        iter->p = iter->p->next;
    }
}

void *
value_blist(const struct iterator_blist *iter)
{
    return iter->p->data;
}

#ifdef TEST_BLIST
void
librator_debug(void *data){}

int
cmp_debug(void *a, void *b)
{
    return strcmp((char*)a, (char*)b);
}

int
main(int argc, char *argv[])
{
    struct Blist *list;
    struct iterator_blist iter;

    list = make_blist();
    add_blist(list, "a");
    add_blist(list, "b");
    add_blist(list, "c");

    // if correct, print [a, b, c, ]
    fprintf(stderr, "[");
    for (init_iterator_blist(list, &iter);
            hasNext_blist(&iter);
            next_blist(&iter))
    {
        fprintf(stderr, "%s, ", (char*)value_blist(&iter));
    }
    fprintf(stderr, "]");

    // if correct, print [a, c, ]
    fprintf(stderr, "[");
    remove_blist(list, librator_debug, 1);
    for (init_iterator_blist(list, &iter);
            hasNext_blist(&iter);
            next_blist(&iter))
    {
        fprintf(stderr, "%s, ", (char*)value_blist(&iter));
    }
    fprintf(stderr, "]");

    // if correct, print [a, ]
    fprintf(stderr, "[");
    remove_blist(list, librator_debug, 1);
    for (init_iterator_blist(list, &iter);
            hasNext_blist(&iter);
            next_blist(&iter))
    {
        fprintf(stderr, "%s, ", (char*)value_blist(&iter));
    }
    fprintf(stderr, "]");

    // if correct, print []
    fprintf(stderr, "[");
    remove_blist(list, librator_debug, 0);
    for (init_iterator_blist(list, &iter);
            hasNext_blist(&iter);
            next_blist(&iter))
    {
        fprintf(stderr, "%s, ", (char*)value_blist(&iter));
    }
    fprintf(stderr, "]");

    // if correct, print [test, ]
    fprintf(stderr, "[");
    add_blist(list, "test");
    for (init_iterator_blist(list, &iter);
            hasNext_blist(&iter);
            next_blist(&iter))
    {
        fprintf(stderr, "%s, ", (char*)value_blist(&iter));
    }
    fprintf(stderr, "]");

    // if correct, print []
    fprintf(stderr, "[");
    remove_blist2(list, cmp_debug, librator_debug, "test");
    for (init_iterator_blist(list, &iter);
            hasNext_blist(&iter);
            next_blist(&iter))
    {
        fprintf(stderr, "%s, ", (char*)value_blist(&iter));
    }
    fprintf(stderr, "]");
    
    free_blist(list, librator_debug);

    fprintf(stderr, "\n");

    return 0;
}
#endif

