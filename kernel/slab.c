#include "defs.h"
#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "buddy_kalloc.h"
#include "slab.h"

#define CACHES_ARR_SIZE 13

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
    uint16 buddy_order;
    struct spinlock lock; // every cache needs lock , cant let multiple threads use cache in same time
    
    struct slab* full_slab;
    struct slab* empty_slab;
    struct slab* partial_slab;
    void (*ctor)(void*);
    void (*dtor)(void*);


    struct kmem_cache_s *prev;
    struct kmem_cache_s *next;

};

struct kmem_cache_s *caches_head;

char*  names[] = {"size-32B","size-64B"
                ,"size-128B","size-256B"
                ,"size-512B","size-1024B"
                ,"size-2048B","size-4096B"
                ,"size-8192B","size-16384B"
                ,"size-32768B","size-65536B"
                ,"size-131072B"};





 void kmem_init(void *space, int block_num) {

     buddy_init();
     caches_head = (struct kmem_cache_s*)buddy_kalloc(0);
     caches_head->name = names[0];
     caches_head->size = 32;
     caches_head->buddy_order = 0;
     caches_head->full_slab = 0;
     caches_head->empty_slab = 0;
     caches_head->partial_slab = 0;
     caches_head->ctor=0;
     caches_head->dtor=0;
     caches_head->next=0;
     caches_head->prev=0;
     // maybe make all small memory caches upfront ??

 }
//

//     uint64 size = 32;
//     for (int i=0; i<CACHES_ARR_SIZE; i++) {
//         small_buffer_caches[i].name=names[i];
//         small_buffer_caches[i].size=size<<i;
//         small_buffer_caches[i].full_slab=0;
//         small_buffer_caches[i].empty_slab=0;
//         small_buffer_caches[i].partial_slab=0;
//         small_buffer_caches[i].ctor=0;
//         small_buffer_caches[i].dtor=0;
//     }
//
//
// }

kmem_cache_t *kmem_cache_create(const char *name, size_t size,
void (*ctor)(void *),void (*dtor)(void *)) {
    //ovde ih samo inicijalizujem i uvezujem u listu
}

int kmem_cache_shrink(kmem_cache_t *cachep){}
void *kmem_cache_alloc(kmem_cache_t *cachep){}
void kmem_cache_free(kmem_cache_t *cachep, void *objp){}
void *kmalloc(size_t size) // used to allocate space wia size-N caches
{

}

//change name after i remove old allocator from use
void s_kfree(const void *objp){}
void kmem_cache_destroy(kmem_cache_t *cachep){}
void kmem_cache_info(kmem_cache_t *cachep){}
int kmem_cache_error(kmem_cache_t *cachep){}