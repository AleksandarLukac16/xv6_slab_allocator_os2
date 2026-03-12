#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"
#include "proc.h"
#include "slab.h"

#define RUN_NUM (5)
#define shared_size (7)
#define MASK (0xA5)

struct kmem_cache_s* test_caches[RUN_NUM];
int next_cache_id = 0;


uint64 sys_kmem_init(void)
{
    kmem_init(0,0); // i use whole space
    return 1;
}

void construct(void *data) {
    static int i = 1;
    printf("%d Shared object constructed.\n", i++);
    memset(data, MASK, shared_size);
}

uint64 sys_kmem_cache_create(void)
{
    // here i will return handle to created cache
    char name[32];
    int size;
    argstr(0,name,sizeof(name));
    argint(1,&size);
    struct kmem_cache_s* cache = kmem_cache_create(name,size,construct,0);
    uint64 my_handle = next_cache_id;
    next_cache_id++;
    test_caches[next_cache_id] = cache;
    return my_handle;
}

uint64 sys_kmem_cache_alloc(void)
{
    int handle;
    uint64 addr;
    argint(0,&handle);
    argaddr(1,&addr);
    void* mem = kmem_cache_alloc(test_caches[handle]);
    copyout(myproc()->pagetable,addr,(char*)mem,addr);
    return addr;
}

uint64 sys_kmem_cache_free(void)
{
    int handle;
    uint64 obj;
    argint(0, &handle);
    argaddr(1, &obj);
    kmem_cache_t *cache = test_caches[handle];
    if (!cache)
        return -1;
    kmem_cache_free(cache, (void*)obj);
    return 0;
}


uint64 sys_kmem_cache_destroy(void)
{
    int handle;
    argint(0,&handle);
    kmem_cache_destroy(test_caches[handle]);
    return 0;
}


uint64 sys_kmem_cache_info(void)
{
    int handle;
    argint(0,&handle);
    kmem_cache_info(test_caches[handle]);
    return 0;
}


uint64 sys_kmalloc(void)
{
    uint64 size;
    uint64 mem_addr;
    argaddr(0,&size);
    argaddr(1,&mem_addr);
    uint64  ker_space= (uint64)kmalloc(size);
    copyout(myproc()->pagetable,mem_addr,(char*)ker_space,size);
    return mem_addr;
}


uint64 sys_kfree(void)
{
    uint64 addr;
    int size;
    argaddr(0,&addr);
    argint(1,&size);
    uint64 here_to_copy =0;
    copyin(myproc()->pagetable,(char*)addr,(uint64)&here_to_copy,size);
    kfree((void*)addr);
    return 0;

}
