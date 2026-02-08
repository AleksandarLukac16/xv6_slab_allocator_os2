#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"
#include "buddy_kalloc.h"




uint8 tree[2^(BUDDY_TREE_LEVEL)];



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


static inline uint16 get_level(long vindex) {
    return 63 - __builtin_clzll(vindex);
}

static inline uint8 is_left_son(long vindex) {
    return vindex%2 == 0 ? 1:0;
}

static inline long go_up(long vindex) {
    return vindex>>1;
}



void * buddy_kalloc(uint16 order) {
    if (order>BUDDY_TREE_LEVEL) return 0;

    uint16 local_order = 0;
    long virtual_index = 1;
    while (1) {
        if (local_order == order) {
            if (get_state(virtual_index)==SLOT_EMPTY) {

            }
        }

    }


}






















