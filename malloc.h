#include <malloc.h>
#include <stdlib.h>
#include <unistd.h>

#define DEBUG 0

#if DEBUG
#define assert(c) do { if (!(c)) { fprintf(stderr, "%s !\n", #c); exit(1); } } while (0)
#else
#define assert(c)
#endif

#define POOL_SIZE (1<<30) // bytes
#define NEXT(chunk) ((void *)chunk + chunk->size)

struct my_chunk {
    size_t size;
#if DEBUG
    char rsv;
#endif
    struct my_chunk * prev;
    struct my_chunk * next;
};

static void my_init_hook(void);
static void * my_malloc(size_t, const void *);
static void my_free(void *, const void *);

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
    aval->next = mem;
    aval->prev = mem;
    ((struct my_chunk *)mem)->prev = aval;
    ((struct my_chunk *)mem)->next = aval;
    ((struct my_chunk *)mem)->size = POOL_SIZE;
#if DEBUG
    ((struct my_chunk *)mem)->rsv = 0;
    aval->rsv = 0;
#endif
};

static void * my_malloc(size_t size, const void * caller) {
    size += sizeof(struct my_chunk);
    struct my_chunk * temp;
    for (temp = aval->next; temp != aval; temp = temp->next) {
        if (temp->size < size) {
            continue;
        }
        size_t new_size = temp->size - size;
        if (new_size < sizeof(struct my_chunk)) {
            temp->prev->next = temp->next;
            temp->next->prev = temp->prev;
#if DEBUG
            temp->rsv = 1;
#endif
            return temp + 1;
        }
        temp->size = new_size;
        struct my_chunk * new_chunk = NEXT(temp);
        new_chunk->size = size;
#if DEBUG
        new_chunk->rsv = 1;
#endif
        return new_chunk + 1;
    }
    return NULL;
};

static void my_free(void * ptr, const void * caller) {
#if DEBUG
    char failed = 0;
#endif
    if (ptr < sizeof(struct my_chunk) + mem || ptr > mem + POOL_SIZE) {
#if DEBUG
        failed = 1;
#endif
        goto _failed;
    }
    ptr -= sizeof(struct my_chunk);
    struct my_chunk * temp = aval;
#if DEBUG
    for (temp = mem; (void *)temp < mem + POOL_SIZE; temp = NEXT(temp)) {
        if ((void *)temp == ptr && temp->rsv) {
            goto _succ;
        }
    }
    failed = 1;
_succ:
#endif
    while (temp->next != aval && (void *)temp->next <= ptr) {
        temp = temp->next;
    }
    struct my_chunk * left = temp;
    if (temp == aval) {
        temp = mem;
    } else {
        temp = NEXT(temp);
    }
    while ((void *)temp < ptr) {
        temp = NEXT(temp);
    }
    assert(ptr != mem + POOL_SIZE || temp->size == sizeof(struct my_chunk));
    if (temp != ptr) {
        goto _failed;
    }
    assert(!failed);
    temp->prev = left;
    temp->next = left->next;
    temp->prev->next = temp;
    temp->next->prev = temp;
#if DEBUG
    temp->rsv = 0;
#endif
    if (NEXT(left) == temp) {
        left->size += temp->size;
        temp->prev->next = temp->next;
        temp->next->prev = temp->prev;
        temp = left;
    }
    if (NEXT(temp) == temp->next) {
        temp->size += temp->next->size;
        struct my_chunk * t = temp->next;
        t->prev->next = t->next;
        t->next->prev = t->prev;
    }
    return;
_failed:
    assert(failed);
    exit(1);
};
