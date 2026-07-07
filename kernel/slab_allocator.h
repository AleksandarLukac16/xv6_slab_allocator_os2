
#define BLOCK_SIZE (4096)

struct cache_t;

typedef unsigned long size_t;
void cache_init(void *space, int block_num);
uint64 cache_create(const char *name, size_t size,
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