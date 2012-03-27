#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define POOL_SIZE (1<<30) // bytes
#define NEXT(chunk) ((void *)chunk + chunk->size)

struct my_chunk {
    size_t size;
    struct my_chunk * prev;
    struct my_chunk * next;
};

struct my_chunk_rsv {
    size_t size;
};

static void my_init_hook(void);
static void * my_malloc(size_t, const void *);
static void my_free(void *, const void *);
static void * my_realloc(void *, size_t, const void *);
static void * _alloc(struct my_chunk *, size_t);
static void * _locate_rsv_chunk(void *);
static struct my_chunk * _try_merge(struct my_chunk *);

void (*__malloc_initialize_hook)(void) = my_init_hook;
static void * mem;
static struct my_chunk aval[1];

static void my_init_hook(void) {
    if (POOL_SIZE < sizeof(struct my_chunk)) {
        exit(1);
    }
    mem = sbrk(POOL_SIZE);
    __malloc_hook = my_malloc;
    __free_hook = my_free;
    __realloc_hook = my_realloc;
    aval->next = mem;
    aval->prev = mem;
    ((struct my_chunk *)mem)->prev = aval;
    ((struct my_chunk *)mem)->next = aval;
    ((struct my_chunk *)mem)->size = POOL_SIZE;
};

static void * my_malloc(size_t size, const void * caller) {
    size += sizeof(struct my_chunk_rsv);
    struct my_chunk * p;
    for (p = aval->next; p != aval; p = p->next) {
        if (p->size >= size) {
            return _alloc(p, size);
        }
    }
    return NULL;
};

static void my_free(void * ptr, const void * caller) {
    ptr -= sizeof(struct my_chunk_rsv);
    struct my_chunk * left;
    if ((left = _locate_rsv_chunk(ptr)) == NULL) {
        exit(1);
    }
    struct my_chunk * p = ptr;
    p->prev = left;
    p->next = left->next;
    p->prev->next = p;
    p->next->prev = p;
    _try_merge(_try_merge(left));
    return;
};

static void * my_realloc(void * ptr, size_t size, const void * caller) {
    if (ptr == NULL) {
        return my_malloc(size, caller);
    }
    if (size == 0) {
        my_free(ptr, caller);
    }
    size += sizeof(struct my_chunk_rsv);
    struct my_chunk * left;
    if ((left = _locate_rsv_chunk(ptr)) == NULL) {
        exit(1);
    }
    struct my_chunk_rsv * p = ptr;
    { // TODO in-place realloc
    }
    void * ret = malloc(size - sizeof(struct my_chunk_rsv)); 
    if (ret != NULL) {
        memcpy(ret, ptr, size - sizeof(struct my_chunk_rsv));
        free(ptr);
    }
    return ret;
};

static void * _locate_rsv_chunk(void * ptr) {
    if (ptr < mem || ptr > mem + POOL_SIZE - sizeof(struct my_chunk_rsv)) {
        return NULL;
    }
    struct my_chunk * left = aval;
    while (left->next != aval && (void *)left->next <= ptr) {
        left = left->next;
    }
    struct my_chunk_rsv * p;
    if (left == aval) {
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

static void * _alloc(struct my_chunk * ptr, size_t size) {
    size_t size_left = ptr->size - size;
    if (size_left < sizeof(struct my_chunk_rsv)) {
        ptr->prev->next = ptr->next;
        ptr->next->prev = ptr->prev;
        return (struct my_chunk_rsv *)ptr + 1;
    }
    ptr->size = size;
    struct my_chunk * new_chunk = NEXT(ptr);
    new_chunk->size = size_left;
    new_chunk->prev = ptr->prev;
    new_chunk->next = ptr->next;
    new_chunk->prev->next = new_chunk;
    new_chunk->next->prev = new_chunk;
    return (struct my_chunk_rsv *)ptr + 1;
};

static struct my_chunk * _try_merge(struct my_chunk * left) {
    if (left != aval && left->next != aval && NEXT(left) == left->next) {
        left->size += left->next->size;
        struct my_chunk * t = left->next;
        t->prev->next = t->next;
        t->next->prev = t->prev;
        return left;
    }
    return left->next;
};
