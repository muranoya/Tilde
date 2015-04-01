#ifndef H_BLIST
#define H_BLIST

struct Blist;
struct iterator_blist;

struct Blist *make_blist();
void free_blist(struct Blist *list, void (*liberator)(void*));
int  count_blist(struct Blist *list);
void add_blist(struct Blist *list, void *data);
int  remove_blist(struct Blist *list, void (*liberator)(void*), int n);
int  remove_blist2(struct Blist *list, int (*cmp)(void*, void*), void (*liberator)(void*), void *data);
void *at_blist(struct Blist *list, int n);

void init_iterator_blist(const struct Blist *list, struct iterator_blist *iter);
int  hasNext_blist(const struct iterator_blist *iter);
void next_blist(struct iterator_blist *iter);
void *value_blist(const struct iterator_blist *iter);
#endif

