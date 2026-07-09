#include "types.h"
#include "spinlock.h"
#include "defs.h"
#include "buddy_kalloc.h"

#define SMALL_MEM_BUFF_CNT 13
#define OBJS_IN_SLAB 8
#define MAX_SMALL_MEM_BUFF 131072
#define FREE_MAGIC_NUM 0xDEAD
#define MAX_CACHE_NAME 30

const char *names[] = {
    "size-32B", "size-64B", "size-128B", "size-256B", "size-512B", "size-1024B", "size-2048B", "size-4096B",
    "size-8192B", "size-16384B", "size-32768B", "size-65536B", "size-131072B"
};

const size_t sizes[] = {32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072};

extern char end[]; // first address after kernel.
// defined by kernel.ld.




struct free {
    struct free *next;
    int magic_num;
};

struct slab {
    size_t total; // how much objects can slab store
    size_t free_count; // free slots counter

    struct free *free_list; // list of free slots in slab
    struct slab *next; // next slab
};



struct cache_t *caches_head; // head of list of caches
struct cache_t caches_origin; // origin of all caches , this cache allocate other caches
struct cache_t* small_mem_caches[SMALL_MEM_BUFF_CNT];
struct spinlock raw_caches_lock;

int free_slab_list(struct slab**sl_list,size_t obj_size);

void cache_dtor(void* c) {
    // here i write destructor for caches and than i can destroy them in
    if (c == 0)return;
    struct cache_t* cache = (struct cache_t*)c;
    free_slab_list(&cache->full_slabs, cache->size);
    free_slab_list(&cache->empty_slabs, cache->size);
    free_slab_list(&cache->partial_slabs, cache->size);
}


void cache_init() {
    //buddy_init();
    caches_origin.name = "caches-origin";
    caches_origin.size = sizeof(struct cache_t);
    initlock(&caches_origin.lock, "caches-origin-lock");
    caches_origin.full_slabs = 0;
    caches_origin.empty_slabs = 0;
    caches_origin.partial_slabs = 0;
    caches_origin.ctor = 0;
    caches_origin.dtor = cache_dtor;
    caches_origin.next = 0;
    caches_origin.prev = 0;
    caches_origin.error_msg = 0;
    for (int i =0;i<SMALL_MEM_BUFF_CNT;i++)small_mem_caches[i]=0;
    initlock(&raw_caches_lock, "raw-caches-lock");
    caches_head = &caches_origin;
}

uint16 get_order_from_size(size_t obj_size, uint64 *slab_size) {
    //calculates slab size as side effect and returns order for buddy allocator
    *slab_size = BLOCK_SIZE;
    int target_objs;

    if (obj_size <= 256)
        target_objs = 8;
    else if (obj_size <= 4096)
        target_objs = 4;
    else if (obj_size <= 32768)
        target_objs = 2;
    else
        target_objs = 1;

    uint16 order = 0;
    while (*slab_size - sizeof(struct slab) < target_objs * obj_size) {
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
    slab->free_count = slab->total;
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
    if (*from_list == 0) panic("provided null as source list ");

    if (*from_list == slab) {
        *from_list = slab->next;
    } else {
        struct slab *prev = *from_list;
        while (prev != 0 && prev->next != slab) {
            prev = prev->next;
        }
        if (prev == 0 || prev->next != slab) panic("Wrong list provided!");
        prev->next = slab->next;
    }
    slab->next = *to_list;
    *to_list = slab;
}

struct cache_t* cache_create(const char *name, size_t size, // size of object in cache
                                void (*ctor)(void *), void (*dtor)(void *)) {
    struct cache_t *cache = (struct cache_t*) cache_alloc(&caches_origin);

    acquire(&caches_origin.lock);
    cache->name = name;
    initlock(&cache->lock, (char*)name);
    cache->ctor = ctor;
    cache->dtor = dtor;
    cache->size = size;
    cache->prev = 0;
    cache->full_slabs = 0;
    cache->empty_slabs = 0;
    cache->partial_slabs = 0;
    cache->error_msg = 0;

    cache->next = caches_head;
    caches_head->prev = cache;
    caches_head = cache;
    release(&caches_origin.lock);
    return cache;
}

int free_slab(struct slab *slab, size_t obj_size) {
    // here i need to calculate size and to provide buddy with address that it needs to free
    size_t slab_size;
    uint16 order = get_order_from_size(obj_size, &slab_size);
    if (!buddy_kfree(slab, order))panic("free_slab");
    return 1<<order;
}

int free_slab_list(struct slab**sl_list,size_t obj_size) {
    int freed_blocks =0;
    while (*sl_list != 0) {
        struct slab *slab_to_free = *sl_list;
        *sl_list = (*sl_list)->next;
        freed_blocks+=free_slab(slab_to_free,obj_size);
    }
    return freed_blocks;
}

int cache_shrink(struct cache_t *cachep) {
    // here i return memory that free list holds ,back to buddy allocator
    acquire(&cachep->lock);
    if (!cachep->empty_slabs) {
        release(&cachep->lock);
        return 0;
    }
    int released_blocks = free_slab_list(&cachep->empty_slabs,cachep->size);
    release(&cachep->lock);
    return released_blocks;
}

void *cache_alloc(struct cache_t *cachep) {
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
            // panic if cant alloc memory from buddy
            release(&cachep->lock);
            panic("cache_create failed");
        }
        obj = slab_alloc(slab); // dilema here , do i allocate always 8 and more slots ?
        if (obj != 0) {
            slab->next = cachep->partial_slabs;
            cachep->partial_slabs = slab;
        }
    } else if (cachep->partial_slabs != 0) {
        // if there is , pick one from partial slabs
        slab = cachep->partial_slabs;
        obj = slab_alloc(slab);
        if (obj != 0 && slab->free_count == 0) {
            move_slab(&cachep->partial_slabs, &cachep->full_slabs, slab);
        }
    } else {
        // no partial slabs , need to get mem from free_slabs
        slab = cachep->empty_slabs;
        obj = slab_alloc(slab);
        if (obj != 0) {
            move_slab(&cachep->empty_slabs, &cachep->partial_slabs, slab);
        }
    }
    //here i found slab and i need to alocate mem from it
    if (obj!=0) {
        struct free* guard = (struct free*) obj;
        guard->magic_num = 0;
    }
    if (cachep->ctor != 0 && obj != 0)cachep->ctor(obj);
    // i need to allocate space from slab and to provide ctor with it

    release(&cachep->lock);
    return obj;
}

int find_obj(struct slab *slab, const void *obj, size_t obj_size, struct slab **target_slab) {
    if (obj == 0|| slab ==0)return 0;
    while (slab) {
        uint64 start = (uint64) &slab[1];
        uint64 end_of_slab = start + obj_size * slab->total;
        uint64 target = (uint64) obj;

        if (target < end_of_slab && target >= start && (target - start) % obj_size == 0) {
            //condition (target-start)%obj_size insures that my obj starts at end of obj in front of it ,
            //it is not in middle of some obj
            *target_slab = slab;
            return 1;
        }

        slab = slab->next;
    }
    *target_slab = 0;
    return 0;
}

void slab_free(struct slab *slab, const void *obj) {
    struct free *free_obj = (struct free *) obj;
    free_obj->next = slab->free_list;
    slab->free_list = free_obj;
    slab->free_count++;
}

void cache_free(struct cache_t *cachep, void *objp) {
    //Here i call destructor on object and , move slabs to their new place
    if (objp == 0)return;

    acquire(&cachep->lock);
    struct free* guard = (struct free*) objp;
    if (guard->magic_num == FREE_MAGIC_NUM) {
        cachep->error_msg = "Double free!!!";
        release(&cachep->lock);
        return;
    }
    struct slab *target_slab;
    if (find_obj(cachep->partial_slabs, objp, cachep->size, &target_slab)) {
        //if (cachep->dtor != 0)cachep->dtor(objp);
        slab_free(target_slab, objp);
        guard->magic_num = FREE_MAGIC_NUM;
        if (target_slab->free_count == target_slab->total) {
            move_slab(&cachep->partial_slabs, &cachep->empty_slabs, target_slab);
        }
        release(&cachep->lock);
        return;
    }else if (find_obj(cachep->full_slabs, objp, cachep->size, &target_slab)) {
        //if (cachep->dtor != 0)cachep->dtor(objp);
        slab_free(target_slab, objp);
        guard->magic_num = FREE_MAGIC_NUM;
        move_slab(&cachep->full_slabs, &cachep->partial_slabs, target_slab);
        release(&cachep->lock);
        return;
    }
    cachep->error_msg = "object not found in any slab";
    release(&cachep->lock);
}

int size_to_index(size_t size) {
    if (size <= 32)return 0;
    // round up: 33→64, 65→128
    int bits = 64 - my_clz(size - 1);
    return bits - 5;  // 5 because 2^5 = 32 is index 0
}

void *cache_kalloc(size_t size) // used to allocate space wia size-N caches
{
    //pages larger than largest small-mem-cache is delegated directly to buddy
    if (size >MAX_SMALL_MEM_BUFF) {
        uint16 order =0;
        size_t block = BLOCK_SIZE;
        while (block<size) {
            block <<= 1;
            order++;
        }
        return buddy_kalloc(order);
    }
    int index = size_to_index(size);
    acquire(&raw_caches_lock);
    struct cache_t* cache = small_mem_caches[index];
    if (cache == 0) {
        // alloc new cache with this size and put it to array
        small_mem_caches[index] = cache_create(names[index],sizes[index],0,0);
        cache = small_mem_caches[index];
    }
    release(&raw_caches_lock);
    if (cache == 0) panic ("cache_kalloc: cant allocate cache");
    return cache_alloc(cache);

}

//change name after i remove old allocator from use
void cache_kfree(const void *objp) {
    if (objp == 0)return;
    struct free* guard = (struct free *) objp;
    if (guard->magic_num==FREE_MAGIC_NUM) return;
    for (int i =0;i<SMALL_MEM_BUFF_CNT;i++) {
        struct cache_t* cache = small_mem_caches[i];
        struct slab* slab;
        if (cache == 0) continue;
        acquire(&cache->lock);
        if (find_obj(cache->partial_slabs, objp, cache->size, &slab)) {
            slab_free(slab, objp);
            guard->magic_num=FREE_MAGIC_NUM;
            if (slab->free_count == slab->total) {
                move_slab(&cache->partial_slabs, &cache->empty_slabs, slab);
            }
            release(&cache->lock);
            return;
        }else if (find_obj(cache->full_slabs, objp, cache->size, &slab)) {
            slab_free(slab, objp);
            guard->magic_num=FREE_MAGIC_NUM;
            if (slab->free_count == slab->total) {
                move_slab(&cache->full_slabs, &cache->empty_slabs, slab);
            }else {
                move_slab(&cache->full_slabs, &cache->partial_slabs, slab);
            }
            release(&cache->lock);
            return;
        }
        release(&cache->lock);
    }
}

void cache_destroy(struct cache_t *cachep) {
    if (cachep == 0)return;
    acquire(&cachep->lock);

    if (cachep->full_slabs != 0 || cachep->partial_slabs != 0) {
        // Objects still in use — warn but proceed
        cachep->error_msg = "kmem_cache_destroy: %s has active objects";
        release(&cachep->lock);
        return;
    }

    release(&cachep->lock);

    acquire(&caches_origin.lock);

    //remove from cache list
    if (cachep == caches_head)caches_head = cachep->next;
    if (cachep->next) cachep->next->prev = cachep->prev;
    if (cachep->prev) cachep->prev->next = cachep->next;
    release(&caches_origin.lock);
    cache_free(&caches_origin, cachep);
    // here i call kmem_cache_free of origin cache

}

void cache_info(struct cache_t *cachep) {
    if (cachep == 0) return;
    acquire(&cachep->lock);

    int total_slabs = 0;
    int total_objs = 0;
    int used_objs = 0;
    size_t slab_size;
    get_order_from_size(cachep->size, &slab_size);

    struct slab *s = cachep->full_slabs;
    while (s) {
        total_slabs++;
        total_objs += s->total;
        used_objs += s->total;
        s = s->next;
    }

    // Count partial slabs
    s = cachep->partial_slabs;
    while (s) {
        total_slabs++;
        total_objs += s->total;
        used_objs += (s->total - s->free_count);
        s = s->next;
    }

    // Count empty slabs
    s = cachep->empty_slabs;
    while (s) {
        total_slabs++;
        total_objs += s->total;
        s = s->next;
    }

    int percentage = total_objs > 0 ? (used_objs * 100) / total_objs : 0;

    printf("cache: %s\n", cachep->name);
    printf("  obj size: %lu | slab size: %lu\n", cachep->size, slab_size);
    printf("  slabs: %d | objs: %d/%d | usage: %d%%\n",
           total_slabs, used_objs, total_objs, percentage);

    release(&cachep->lock);
}

int cache_error(struct cache_t *cachep) {
    if (cachep->error_msg != 0) {
        printf("cache %s: %s\n", cachep->name, cachep->error_msg);
        cachep->error_msg = 0;
        return 1;

    }
    return 0;
}

struct cache_t * id_to_cache(uint64 id) {
    struct cache_t* temp = caches_head;
    while (temp) {
        if (temp->id == id) {
            return temp;
        }
        temp = temp->next;
    }
    for (int i =0;i<SMALL_MEM_BUFF_CNT;i++) {
        if (small_mem_caches[i]->id ==id) {
            return small_mem_caches[i];
        }
    }
    return 0;
}

uint64 cache_to_id(struct cache_t *cache) {
    return cache->id;
}
