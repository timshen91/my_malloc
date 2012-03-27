#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define POOL_SIZE (1<<30) // bytes
#define NEXT(chunk) ((void *)(chunk) + (chunk)->size)
#define min(a, b) ((a) < (b) ? (a) : (b))

struct my_chunk {
    size_t size;
    struct my_chunk * prev;
    struct my_chunk * next;
};

static void my_init(void);
static void * my_malloc(size_t, const void *);
static void my_free(void *, const void *);
static void * my_realloc(void *, size_t, const void *);
static void _alloc(struct my_chunk *, size_t);
static void * _malloc_algo(size_t);
static struct my_chunk * _locate_rsv_chunk(void *);
static struct my_chunk * _try_merge(struct my_chunk *);
static void _insert_behind(struct my_chunk *, struct my_chunk *);
static void _delete(struct my_chunk *);

void (*__malloc_initialize_hook)(void) = my_init;
static void * mem;
static struct my_chunk avail[1];

static void my_init(void) {
    if (POOL_SIZE < sizeof(struct my_chunk)) {
        exit(1);
    }
    mem = sbrk(POOL_SIZE);
    __malloc_hook = my_malloc;
    __free_hook = my_free;
    __realloc_hook = my_realloc;
    avail->next = mem;
    avail->prev = mem;
    ((struct my_chunk *)mem)->prev = avail;
    ((struct my_chunk *)mem)->next = avail;
    ((struct my_chunk *)mem)->size = POOL_SIZE;
};

static void * my_malloc(size_t size, const void * caller) {
    size += sizeof(struct my_chunk);
    struct my_chunk * p;
    if ((p = _malloc_algo(size)) == NULL) {
        return NULL;
    }
    _alloc((void *)p, size);
    return p + 1;
};

static void my_free(void * ptr, const void * caller) {
    ptr -= sizeof(struct my_chunk);
    struct my_chunk * left;
    if ((left = _locate_rsv_chunk(ptr)) == NULL) {
        exit(1);
    }
    _insert_behind(left, ptr);
    _try_merge(_try_merge(left));
    return;
};

static void * my_realloc(void * ptr, size_t size, const void * caller) {
    if (ptr == NULL) {
        return my_malloc(size, caller);
    }
    if (size == 0) {
        my_free(ptr, caller);
        return NULL;
    }
    ptr -= sizeof(struct my_chunk);
    size += sizeof(struct my_chunk);
    struct my_chunk * left;
    if ((left = _locate_rsv_chunk(ptr)) == NULL) {
        exit(1);
    }

    struct my_chunk * p = ptr;
    if (size <= p->size || (NEXT(p) == left->next && size <= p->size + left->next->size)) {
        _insert_behind(left, p);
        _try_merge(p);
        _alloc(p, size);
        return p + 1;
    }
    struct my_chunk * ret;
    _insert_behind(left, p);
    _try_merge(_try_merge(left));
    if ((ret = _malloc_algo(size)) == NULL) {
        return NULL;
    }
    memmove(ret + 1, p + 1, p->size - sizeof(struct my_chunk));
    _alloc(ret, size);
    return ret + 1;
};

static void * _malloc_algo(size_t size) {
    struct my_chunk * p;
    for (p = avail->next; p != avail; p = p->next) {
        if (p->size >= size) {
            return p;
        }
    }
    return NULL;
};

static struct my_chunk * _locate_rsv_chunk(void * ptr) {
    if (ptr < mem || ptr > mem + POOL_SIZE - sizeof(struct my_chunk)) {
        return NULL;
    }
    struct my_chunk * left = avail;
    while (left->next != avail && (void *)left->next <= ptr) {
        left = left->next;
    }
    struct my_chunk * p;
    if (left == avail) {
        p = mem;
    } else {
        p = NEXT(left);
    }
    while ((void *)p < ptr) {
        p = NEXT(p);
    }
    if (p != ptr) {
        return NULL;
    }
    return left;
};

static void _alloc(struct my_chunk * ptr, size_t size) {
    size_t size_left = ptr->size - size;
    if (size_left < sizeof(struct my_chunk)) {
        _delete(ptr);
    } else {
        ptr->size = size;
        struct my_chunk * new_chunk = NEXT(ptr);
        new_chunk->size = size_left;
        _insert_behind(ptr, new_chunk);
        _delete(ptr);
    }
    return;
};

static struct my_chunk * _try_merge(struct my_chunk * left) {
    if (left != avail && left->next != avail && NEXT(left) == left->next) {
        left->size += left->next->size;
        _delete(left->next);
        return left;
    }
    return left->next;
};

static void _insert_behind(struct my_chunk * left, struct my_chunk * ptr) {
    ptr->prev = left;
    ptr->next = left->next;
    ptr->prev->next = ptr;
    ptr->next->prev = ptr;
};

static void _delete(struct my_chunk * ptr) {
    ptr->prev->next = ptr->next;
    ptr->next->prev = ptr->prev;
};
