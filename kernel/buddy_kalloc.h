#include "types.h"
#define BUDDY_TREE_LEVEL 14
#define SLOT_EMPTY 0
#define SLOT_SPLIT 1
#define SLOT_FULL 3
#define MEM_SIZE 128*1024*1024


void *buddy_kalloc(uint16 order);

int buddy_kfree(void *adr,uint16 order);

int binit();