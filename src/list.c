#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tilde.h"

struct List_body
{
    struct List_body *prev;
    struct List_body *next;
    void *data;
};

void liberator_void(void *data) { }

List *
make_list()
{
    List *list;

    list = (List*)malloc(sizeof(List));
    list->head = list->tail = NULL;
    list->len = 0;
    return list;
}

void
free_list(List **list, void (*liberator)(void *))
{
    struct List_body *now, *next;

    if ((*list)->len > 0)
    {
        now = (*list)->head;
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
    free(*list);
    *list = NULL;
}

void
add_list(List *list, void *data)
{
    struct List_body *newbody;

    newbody = (struct List_body*)malloc(sizeof(struct List_body));
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

bool
remove_list(List *list, void (*liberator)(void *), int n)
{
    struct List_body *b;
    
    if (n >= list->len) return false;
    if (list->len == 1)
    {
        liberator(list->head->data);
        free(list->head);
        list->head = list->tail = NULL;
        list->len = 0;
        return true;
    }
    if (n == 0 || n == list->len-1)
    {
        if (n == 0)
        {
            b = list->head;
            list->head = b->next;
            b->next->prev = NULL;
        }
        else
        {
            b = list->tail;
            list->tail = b->prev;
            b->prev->next = NULL;
        }
        liberator(b->data);
        free(b);
        list->len--;
        return true;
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
                return true;
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

void *
at_list(List *list, int n)
{
    struct List_body *b;

    if (n >= list->len) return NULL;

    if (n >= list->len / 2)
    {
        b = list->tail;
        for (n = list->len-1-n; ; --n)
        {
            if (n == 0) return b->data;
            b = b->prev;
        }
    }
    else
    {
        b = list->head;
        for (;; --n)
        {
            if (n == 0) return b->data;
            b = b->next;
        }
    }
}

void *
pop_list(List *list)
{
    void *ret;
    int len = list->len;
    if (len == 0) return NULL;
    ret = at_list(list, len-1);
    remove_list(list, liberator_void, len-1);
    return ret;
}

void
init_iter_list(const List *list, iter_list *iter) { iter->p = list->head; }

bool
hasnext_iter_list(const iter_list *iter) { return iter->p != NULL; }

void
next_iter_list(iter_list *iter) { if (iter->p != NULL) iter->p = iter->p->next; }

void *
value_iter_list(const iter_list *iter) { return iter->p->data; }

#ifdef TEST_LIST
void
debug_print(const List *list)
{
    iter_list iter;

    printf("[");
    for (init_iter_list(list, &iter);
         hasnext_iter_list(&iter);
         next_iter_list(&iter))
    {
        printf("%s, ", (char*)value_iter_list(&iter));
    }
    printf("]\n");
}

int
main(int argc, char *argv[])
{
    List *list;

    list = make_list();
    add_list(list, "a");
    add_list(list, "b");
    add_list(list, "c");

    // if correct, print [a, b, c, ]
    debug_print(list);

    // if correct, print c
    printf("%s\n", (char*)at_list(list, list->len-1));

    // if correct, print c
    printf("%s\n", (char*)pop_list(list));
    // if correct, print [a, b, ]
    debug_print(list);

    // if correct, print [a, b, ]
    debug_print(list);

    // if correct, print [b, ]
    remove_list(list, liberator_void, 0);
    debug_print(list);

    // if correct, print []
    remove_list(list, liberator_void, 0);
    debug_print(list);

    // if correct, print [test, ]
    add_list(list, "test");
    debug_print(list);

    free_list(&list, liberator_void);

    fprintf(stderr, "\n");

    return 0;
}
#endif

