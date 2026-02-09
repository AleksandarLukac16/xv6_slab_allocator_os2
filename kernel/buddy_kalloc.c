#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"
#include "buddy_kalloc.h"

uint8 tree[2<<(BUDDY_TREE_LEVEL)];

extern char end[];

struct {
     struct freeblock* next;
}freeblock ;


struct {
    struct spinlock lock;
    struct freeblock* freelist[];
} kmem;

static inline long rindex_to_vindex(long rindex) {
    return rindex<<2;
}

static inline long vindex_to_rindex(long vindex) {
    return vindex>>2;
}

static inline long go_left(long vindex) {
    return vindex<<1;
}
static inline long go_right(long vindex) {
    return (vindex<<1)|1;
}
static inline uint8 get_state(long vindex) {
    long rindex = vindex_to_rindex(vindex);
    uint8 shift = (vindex%4) << 1;
    uint8 state  = tree[rindex];
    return (state >>shift) & 0x3;
}

static inline void set_state(long vindex, uint8 state) {
    long rindex = vindex_to_rindex(vindex);
    uint8 shift = (vindex%4) << 1;
    tree[rindex] &= ~(0x3<<shift);
    tree[rindex] |=  (state & 0x3)<<shift;
}


static inline long get_level(long vindex) {
    return 63 - __builtin_clzll(vindex); // this function deletes leading zeros
}

static inline uint8 is_left_son(long vindex) {
    return vindex%2 == 0 ? 1:0;
}

static inline long go_up(long vindex) {
    return vindex>>1;
}

static inline void *get_adr(long vindex) {
    if (vindex == 0) return 0;
    long level = get_level(vindex);
    long mask = 1ULL << level;
    long offset = vindex ^ mask;  // removing leading 1 , used as delimiter
    uint64 block_size = MEM_SIZE >> level; // calculating size of block
    return (void*)(KERNBASE + (offset * block_size));
}

static inline long get_vindex(void* adr , uint16 order) {
    uint64 rel_addr = (uint64)adr - KERNBASE;
    uint64 block_size = MEM_SIZE >> order;
    return (1ULL << order) + (rel_addr / block_size);
}

static inline void update_upstairs_alloc(long vindex) {
    while (vindex !=1) {
        if (is_left_son(vindex)) {
            vindex = go_up(vindex);
            set_state(vindex,LEFT_USED);
        }else {
            vindex = go_up(vindex);
            set_state(vindex,RIGHT_USED);
        }
    }
}

static inline void update_upstairs_free(long vindex) {
    while (vindex !=1) {
        if (is_left_son(vindex)) {
            vindex = go_up(vindex);
            if (get_state(vindex)==LEFT_USED) {
                set_state(vindex,SLOT_EMPTY);
            }else {
                set_state(vindex,RIGHT_USED);
                return;
            }
        }else {
            vindex = go_up(vindex);
            if (get_state(vindex)==RIGHT_USED) {
                set_state(vindex,SLOT_EMPTY);
            }else {
                set_state(vindex,LEFT_USED);
                return;
            }
        }
    }
}

static inline uint16 locate_free_block(long* vindex,uint16 order) { // add check for half used blocks
    uint16 local_order = 0;
    long virtual_index = *vindex;
    while (local_order < order) {
        if (get_state(virtual_index)==LEFT_USED) {
            virtual_index= go_right(virtual_index);
        }else if (get_state(virtual_index)==RIGHT_USED|| get_state(virtual_index)==SLOT_EMPTY) {
            virtual_index= go_left(virtual_index);
        }
        local_order++;
    }
    *vindex=virtual_index;
    return local_order;
}

void * buddy_kalloc(uint16 order) {
    if (order>BUDDY_TREE_LEVEL) return 0;

    long virtual_index = 1;
    uint16 local_order = locate_free_block(&virtual_index,order);
    if (local_order==0)return 0;
    if (get_state(virtual_index)==SLOT_EMPTY) {
        void* adr = get_adr(virtual_index);
        set_state(virtual_index,SLOT_FULL);
        update_upstairs_alloc(virtual_index);
        return adr;
    }
    return 0;
}

int buddy_kfree(void * adr , uint16 order) {

    if (order>BUDDY_TREE_LEVEL || (uint64)adr > PHYSTOP) return 0;

    long virtual_index = get_vindex(adr,order);
    set_state(virtual_index,SLOT_EMPTY);
    update_upstairs_free(virtual_index);
    return 1;

}
