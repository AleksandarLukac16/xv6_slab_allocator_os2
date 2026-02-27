#include "buddy_kalloc.h"
#include "defs.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"


struct{
    struct spinlock buddy_lock;
    uint8 tree[TREE_SIZE];
} mem_tree;


struct buddy_stack {
    long vindex_stack[BUDDY_TREE_LEVEL];
    int top;
};

uint8 empty(struct buddy_stack *stack) {
    return stack->top == 0;
}


uint8 push(struct buddy_stack *stack,long vindex) {
    if (stack->top == BUDDY_TREE_LEVEL) return 0;
    stack->vindex_stack[stack->top] = vindex;
    stack->top++;
    return 1;
}
uint8 pop(struct buddy_stack *stack,long* vindex) {
    if (stack->top == 0) return 0;
    stack->top--;
    *vindex = stack->vindex_stack[stack->top];
    return 1;
}

void init_buddy_stack(struct buddy_stack *stack) {
    for (int i=0;i<BUDDY_TREE_LEVEL;i++) {
        stack->vindex_stack[i]=0;
    }
    stack->top=0;
}

extern char end[]; // linker will provide this with last mem location of kernel code

// static inline long rindex_to_vindex(long rindex) {
//     return rindex<<2;
// }

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
    uint8 state  = mem_tree.tree[rindex];
    return (state >>shift) & 0x3;
}

static inline void set_state(long vindex, uint8 state) {
    long rindex = vindex_to_rindex(vindex);
    uint8 shift = (vindex%4) << 1;
    mem_tree.tree[rindex] &= ~(0x3<<shift);
    mem_tree.tree[rindex] |=  (state & 0x3)<<shift;
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
    if (vindex == 0) panic("vindex problem in get_adr");
    long level = get_level(vindex);
    long mask = 1ULL << level;
    long offset = vindex ^ mask;  // removing leading 1 , used as delimiter
    uint64 block_size = MEM_SIZE >> level; // calculating size of block
    return (void*)(KERNBASE + (offset * block_size));
}

static inline long get_vindex(void* adr , uint16 level) {
    uint64 rel_addr = (uint64)adr - KERNBASE;
    uint64 block_size = MEM_SIZE >> level;
    return (1ULL << level) + (rel_addr / block_size);
}

static inline uint8 is_brother_full(long vindex,long (*mover)(long)) {
    if (get_state(mover(vindex))==SLOT_FULL)return 1;
    return 0;
}
// static inline uint8 is_brother_split(long vindex,long(*mover)(long)) {
//     if (get_state(mover(vindex))==SLOT_SPLIT)return 1;
//     return 0;
// }

static inline uint8 is_brother_empty(long vindex,long(*mover)(long)) {
    if (get_state(mover(vindex))==SLOT_EMPTY)return 1;
    return 0;
}

static inline void update_upstairs_alloc(long vindex) {

    //starts at virtual_index that is allocated , goes up and update its parents
    while (vindex !=1) {
        long (*mover)(long) ;
        uint8 state = get_state(vindex);
        if (is_left_son(vindex))
            mover = go_right;
        else mover = go_left;
        vindex = go_up(vindex);
        if (is_brother_full(vindex,mover) && state ==SLOT_FULL)
            set_state(vindex,SLOT_FULL);
        else
            set_state(vindex,SLOT_SPLIT);

    }
}

static inline void update_upstairs_free(long vindex) {
    //starts at last freed virtual_index , goes up and updates its parents

    while (vindex !=1) {
        long (*mover)(long);
        if (is_left_son(vindex))mover = go_right;
        else mover = go_left;
        vindex = go_up(vindex);
        uint8 state = get_state(vindex);
        if (state == SLOT_FULL) {
            set_state(vindex,SLOT_SPLIT);
        }else if (state == SLOT_SPLIT) {
            if (is_brother_empty(vindex,mover)) {
                set_state(vindex,SLOT_EMPTY);
            }else return;

        }
    }
}

static inline uint16 locate_free_block(long* vindex,uint16 level) {
    struct buddy_stack stack;
    init_buddy_stack(&stack);

    uint16 curr_level = 0;
    long virtual_index = *vindex;
    uint8 found = 0;

    while (1) {
        while (curr_level<=level) {
            uint8 state = get_state(virtual_index);
            if (state == SLOT_FULL) {
                break;  // backtrack
            }
            if (curr_level == level && state == SLOT_EMPTY) {
                found = 1;
                break;
            }else if (curr_level==level) {
                break;
            }

            if (state == SLOT_EMPTY) {
                virtual_index = go_left(virtual_index);
                curr_level++;

            }else if (state == SLOT_SPLIT) {
                uint8 ls = get_state(go_left(virtual_index));
                uint8 rs = get_state(go_right(virtual_index));
                //need to update this
                if (ls == SLOT_SPLIT && rs == SLOT_SPLIT) {
                    if (!push(&stack, virtual_index)) return 0;
                    // if both sons are split try to go left
                    virtual_index = go_left(virtual_index);
                } else if (ls == SLOT_EMPTY) {
                    //prefer to go on empty sides to lower backtracking
                    virtual_index = go_left(virtual_index);
                } else if (rs == SLOT_EMPTY) {
                    virtual_index = go_right(virtual_index);
                } else if (ls == SLOT_FULL && rs == SLOT_SPLIT) {
                    //no empty slots , go to splited
                    virtual_index = go_right(virtual_index);
                } else if (rs == SLOT_FULL && ls == SLOT_SPLIT) {
                    virtual_index = go_left(virtual_index);
                }else break;
                curr_level++;
            }else break;
        }
        if (found)break; // if found breaks
        if (empty(&stack)) break; // if there is nothing to go back and check , means there is no enough mem
        pop(&stack, &virtual_index);
        virtual_index = go_right(virtual_index);
        curr_level = get_level(virtual_index);
    }
    if (curr_level == level) {
        *vindex=virtual_index;
        return curr_level;
    }
    return 0;

}

void * buddy_kalloc(uint16 order) {

    if (order>BUDDY_TREE_LEVEL) panic("Order too big");
    uint16 level = BUDDY_TREE_LEVEL - order;

    acquire(&mem_tree.buddy_lock);

    long virtual_index = 1;
    uint16 local_level = locate_free_block(&virtual_index,level);
    if (local_level==0) {
        release(&mem_tree.buddy_lock);
        return 0;
    }
    if (get_state(virtual_index)==SLOT_EMPTY) {
        void* adr = get_adr(virtual_index);
        set_state(virtual_index,SLOT_FULL);
        update_upstairs_alloc(virtual_index);
        release(&mem_tree.buddy_lock);
        return adr;
    }
    release(&mem_tree.buddy_lock);
    return 0;
}

int buddy_kfree(void* adr , uint16 order) {

    uint64 addr = (uint64)adr;
    if (order>BUDDY_TREE_LEVEL || addr > PHYSTOP ||(char*)addr<end) panic("Invalid address");
    uint16 level = BUDDY_TREE_LEVEL - order;

    acquire(&mem_tree.buddy_lock);

    long virtual_index = get_vindex(adr,level);
    // see if freeing is valid
    if (get_state(virtual_index)!=SLOT_FULL) {
        release(&mem_tree.buddy_lock);
        return 0;
    }

    set_state(virtual_index,SLOT_EMPTY);
    //update all nodes in tree after this node is freed
    update_upstairs_free(virtual_index);
    release(&mem_tree.buddy_lock);
    return 1;

}

int buddy_init() {
    //here i save kernel code from getting runned over
    initlock(&mem_tree.buddy_lock,"buddy_lock");

    for (int i = 0; i < TREE_SIZE; i++) {
        mem_tree.tree[i]= ~((uint8)0);
    }

    // rounding up end to be at full page
    char* p = (char*)PGROUNDUP((uint64)end);


    //freeing 4KB until i free whole memory
    for(; p + PGSIZE <= (char*)PHYSTOP; p += PGSIZE)
        buddy_kfree(p,0);

    return 1;
}
































