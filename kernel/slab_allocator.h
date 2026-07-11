#include "types.h"
#include "spinlock.h"

#define BLOCK_SIZE (4096)
typedef unsigned long size_t;

struct cache_t {
    uint64 id;
    const char *name; // cache name
    const char *error_msg;
    size_t size; // size of objects that are beeing cached
    //uint16 buddy_order;
    struct spinlock lock; // every cache needs lock , cant let multiple threads use cache in same time

    struct slab *full_slabs; // list of full slabs
    struct slab *empty_slabs; // list of empty slabs
    struct slab *partial_slabs; // list of slabs that has one or more allocation

    void (*ctor)(void *); // object constructor

    void (*dtor)(void *); // object destructor

    struct cache_t *prev;
    struct cache_t *next;
};

void cache_init();
struct cache_t* cache_create(const char *name, size_t size,
void (*ctor)(void *),
void (*dtor)(void *)); // Allocate cache
int cache_shrink(struct cache_t *cachep); // Shrink cache
void *cache_alloc(struct cache_t *cachep); // Allocate one object from cache
void cache_free(struct cache_t *cachep, void *objp); // Deallocate one object from cache
void *cache_kalloc(size_t size); // Alloacate one small memory buffer
void cache_kfree(const void *objp); // Deallocate one small memory buffer
void cache_destroy(struct cache_t *cachep); // Deallocate cache
void cache_info(struct cache_t *cachep); // Print cache info
int cache_error(struct cache_t *cachep); // Print error message

struct cache_t* id_to_cache(uint64 id);
uint64 cache_to_id(struct cache_t* cache);