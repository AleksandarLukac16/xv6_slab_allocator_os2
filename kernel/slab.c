#include "defs.h"
#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "buddy_kalloc.h"
#include "slab.h"

#define SMALL_MEM_BUFF_CNT 13

extern char end[]; // first address after kernel.
// defined by kernel.ld.

struct run {
    struct run *next;
};

// struct {
//     struct spinlock lock;
//     struct run *freelist;
// } kmem;



struct kmem_cache_s {
    const char* name;
    size_t size;
    //uint16 buddy_order;
    struct spinlock lock; // every cache needs lock , cant let multiple threads use cache in same time
    
    struct slab* full_slabs;
    struct slab* empty_slabs;
    struct slab* partial_slabs;
    void (*ctor)(void*);
    void (*dtor)(void*);


    struct kmem_cache_s *prev;
    struct kmem_cache_s *next;

};

struct kmem_cache_s *caches_head;

const char*  names[] = {"size-32B","size-64B"
                ,"size-128B","size-256B"
                ,"size-512B","size-1024B"
                ,"size-2048B","size-4096B"
                ,"size-8192B","size-16384B"
                ,"size-32768B","size-65536B"
                ,"size-131072B"};

const size_t sizes[]={32,64,128,256,512,1024,2048,4096,8192,16384,32768,65536,131072};





 void kmem_init(void *space, int block_num) {

     buddy_init();
     caches_head = (struct kmem_cache_s*)buddy_kalloc(0);



 }


kmem_cache_t *kmem_cache_create(const char *name, size_t size,
void (*ctor)(void *),void (*dtor)(void *)) {

}

int kmem_cache_shrink(kmem_cache_t *cachep){}
void *kmem_cache_alloc(kmem_cache_t *cachep){}
void kmem_cache_free(kmem_cache_t *cachep, void *objp){}
void *kmalloc(size_t size) // used to allocate space wia size-N caches
{
    //pages larger than largest small-mem-cache is delegated directly to buddy
}

//change name after i remove old allocator from use
void s_kfree(const void *objp){}
void kmem_cache_destroy(kmem_cache_t *cachep){}
void kmem_cache_info(kmem_cache_t *cachep){}
int kmem_cache_error(kmem_cache_t *cachep){}