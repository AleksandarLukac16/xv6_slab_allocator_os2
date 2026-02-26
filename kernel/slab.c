#include "defs.h"
#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "buddy_kalloc.h"
#include "slab.h"

#define SMALL_MEM_BUFF_CNT 13
#define OBJS_IN_SLAB 8

const char *names[] = {
    "size-32B", "size-64B", "size-128B", "size-256B", "size-512B", "size-1024B", "size-2048B", "size-4096B",
    "size-8192B", "size-16384B", "size-32768B", "size-65536B", "size-131072B"
};

const size_t sizes[] = {32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072};

extern char end[]; // first address after kernel.
// defined by kernel.ld.

struct run {
    struct run *next;
};

// struct {
//     struct spinlock lock;
//     struct run *freelist;
// } kmem;

struct free {
    struct free *next;
};

struct slab {
    size_t total;
    size_t free_count;

    struct free *free_list;
    struct slab *next;
};


struct kmem_cache_s {
    const char *name;
    size_t size;
    //uint16 buddy_order;
    struct spinlock lock; // every cache needs lock , cant let multiple threads use cache in same time

    struct slab *full_slabs;
    struct slab *empty_slabs;
    struct slab *partial_slabs;

    void (*ctor)(void *);

    void (*dtor)(void *);

    struct kmem_cache_s *prev;
    struct kmem_cache_s *next;
};

struct kmem_cache_s *caches_head;
struct kmem_cache_s caches_origin;


void kmem_init(void *space, int block_num) {
    buddy_init();
    caches_origin.name = "caches-origin";
    caches_origin.size = sizeof(struct kmem_cache_s);
    initlock(&caches_origin.lock, "caches-origin-lock");
    caches_origin.full_slabs = 0;
    caches_origin.empty_slabs = 0;
    caches_origin.partial_slabs = 0;
    caches_origin.ctor = 0;
    caches_origin.dtor = 0;
    caches_origin.next = 0;
    caches_origin.prev = 0;
    caches_head = &caches_origin;
}

uint16 get_order_from_size(size_t obj_size, uint64 *slab_size) {
    *slab_size = BLOCK_SIZE;
    uint16 order = 0;
    while (*slab_size - sizeof(struct slab) < OBJS_IN_SLAB * obj_size) {
        *slab_size <<= 1;
        order++;
    }
    return order;
}

struct slab *slab_init(void *mem, size_t obj_size, uint64 slab_size) {
    // here i cut memory and make obj slots from it
    if (mem == 0)return 0;
    struct slab *slab = (struct slab *) mem;
    slab->free_list = (struct free *) (&slab[1]);

    struct free *prev = slab->free_list;
    struct free *temp = (struct free *) ((uint64) slab->free_list + obj_size);
    prev->next = 0;
    while ((uint64) temp + obj_size <= (uint64) slab + slab_size) {
        // populating free list
        prev->next = temp;
        prev = temp;
        prev->next = 0;
        temp = (struct free *) ((uint64) temp + obj_size);
    }

    slab->total = (slab_size - sizeof(struct slab)) / obj_size;
    return slab;
}

void *slab_alloc(struct slab *slab) {
    //i will provide this function only with empty and partly empty slabs
    if (slab == 0 || slab->free_count <= 0)panic("Invalid slab provided");
    slab->free_count--;
    if (slab->free_list == 0)panic("slab_alloc failed");
    void *obj = slab->free_list;
    slab->free_list = slab->free_list->next;

    return obj;
}

void move_slab(struct slab **from_list, struct slab **to_list, struct slab *slab) {
    struct slab *temp = *from_list;
    struct slab *prev = *from_list;
    while (temp != slab) {
        prev = temp;
        temp = temp->next;
    }
    if (temp != slab) panic("Wrong list provided!");
    // here i need to remove from from_list and wire to to_list
}

kmem_cache_t *kmem_cache_create(const char *name, size_t size, // size of object in cache
                                void (*ctor)(void *), void (*dtor)(void *)) {
    acquire(&caches_origin.lock);
    kmem_cache_t *cache = (kmem_cache_t *) kmem_cache_alloc(&caches_origin);
    cache->name = name;
    initlock(&cache->lock, name);
    cache->ctor = ctor;
    cache->dtor = dtor;
    cache->size = size;
    cache->next = 0;
    cache->prev = 0;
    cache->full_slabs = 0;
    cache->empty_slabs = 0;
    cache->partial_slabs = 0;
    // need to add insert in list of caches
    release(&caches_origin.lock);
    return cache;
}

int kmem_cache_shrink(kmem_cache_t *cachep) {
}

void *kmem_cache_alloc(kmem_cache_t *cachep) {
    if (cachep == 0)return 0;
    acquire(&cachep->lock);

    struct slab *slab;
    void *obj = 0;
    if (cachep->partial_slabs == 0 && cachep->empty_slabs == 0) {
        // if there is no slabs at all allocate new slab
        uint64 slab_size;
        uint16 order = get_order_from_size(cachep->size, &slab_size);
        slab = slab_init(buddy_kalloc(order), cachep->size, slab_size);
        if (slab == 0) {
            // panic if cant alloc cache
            release(&cachep->lock);
            panic("kmem_cache_create failed");
        }
    } else if (cachep->partial_slabs != 0) {
        // if there is , pick one from partial slabs
        slab = cachep->partial_slabs;
        obj = slab_alloc(slab);
        if (slab->free_count == 0) {
        }
    } else {
        slab = cachep->empty_slabs;
    }
    //here i found slab and i need to alocate mem from it

    if (cachep->ctor != 0 && obj != 0)cachep->ctor(obj);
    // i need to allocate space from slab and to provide ctor with it

    release(&cachep->lock);
}

void kmem_cache_free(kmem_cache_t *cachep, void *objp) {
}

void *kmalloc(size_t size) // used to allocate space wia size-N caches
{
    //pages larger than largest small-mem-cache is delegated directly to buddy
}

//change name after i remove old allocator from use
void s_kfree(const void *objp) {
}

void kmem_cache_destroy(kmem_cache_t *cachep) {
}

void kmem_cache_info(kmem_cache_t *cachep) {
}

int kmem_cache_error(kmem_cache_t *cachep) {
}
