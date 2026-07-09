#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "vm.h"
#include "../user/user.h"


uint64 sys_cache_init(void) {
    cache_init();

}
uint64 sys_cache_create(void) {
    char* name;
    size_t size;
    uint64 ctor;
    uint64 dtor;
    int len = argstr(0,name,30);
    argaddr(1,&size);
    argaddr(2,&ctor);
    argaddr(3,&dtor);
    struct cache_t* cache = cache_create(name
        ,size,(void(*)(void*))ctor,(void(*)(void*))dtor);
    return cache_to_id(cache);

}
uint64 sys_cache_shrink(void) {
    uint64 cache_id;
    argaddr(0,&cache_id);
    struct cache_t* cache = id_to_cache(cache_id);
    uint64 ret = cache_shrink(cache);
    return ret;

}
uint64 sys_cache_alloc(void) {
    uint64 cache_id;
    argaddr(0,&cache_id);
    struct cache_t* cache = id_to_cache(cache_id);
    if (cache == 0) {
        return -1;
    }
    struct proc* p = myproc();
    void* mem = cache_alloc(cache);
    if (mem ==0) {
        return -1;
    }
    uint64 old_size = PGROUNDUP(p->sz);
    uint64 new_space = PGROUNDUP(cache->size);

    int perm = PTE_R | PTE_W | PTE_U;
    int ret = mappages(p->pagetable,old,new_space,mem,perm);
    if (ret < 0) {
        return -1;
    }
    p->sz +=PGROUNDUP(cache->size);
    return old_size;

}
uint64 sys_cache_free(void);
uint64 sys_cache_kalloc(void);
uint64 sys_cache_kfree(void);
uint64 sys_cache_destroy(void);
uint64 sys_cache_info(void);
uint64 sys_cache_error(void);