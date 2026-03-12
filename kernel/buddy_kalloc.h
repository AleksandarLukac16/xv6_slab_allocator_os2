#include "types.h"

#define BUDDY_TREE_LEVEL 15
#define SLOT_EMPTY 0
#define SLOT_SPLIT 1
#define SLOT_FULL 3 //must be 3
#define MEM_SIZE 128*1024*1024
#define TREE_SIZE ((2<<BUDDY_TREE_LEVEL)/4)


void *buddy_kalloc(uint16 order);

int buddy_kfree(void *adr,uint16 order);

int buddy_init();

int my_clz(uint64 x);
