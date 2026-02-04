#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"
#include "buddy_kalloc.h"

uint8 buddy_bits_level0 [NUM_OF_4KB_SIZE],
uint8 buddy_bits_level1 [NUM_OF_8KB_SIZE],
uint8 buddy_bits_level2 [NUM_OF_16KB_SIZE],
uint8 buddy_bits_level3 [NUM_OF_32KB_SIZE],
uint8 buddy_bits_level4 [NUM_OF_64KB_SIZE],
uint8 buddy_bits_level5 [NUM_OF_128KB_SIZE],
uint8 buddy_bits_level6 [NUM_OF_256KB_SIZE],
uint8 buddy_bits_level7 [NUM_OF_512KB_SIZE],
uint8 buddy_bits_level8 [NUM_OF_1MB_SIZE],
uint8 buddy_bits_level9 [NUM_OF_2MB_SIZE],
uint8 buddy_bits_level10 [NUM_OF_4MB_SIZE],
uint8 buddy_bits_level11 [NUM_OF_8MB_SIZE],
uint8 buddy_bits_level12 [NUM_OF_16MB_SIZE],
uint8 buddy_bits_level13 [NUM_OF_32MB_SIZE],
uint8 buddy_bits_level14 [NUM_OF_64MB_SIZE]



struct arrayhead tree[] =
{
	{ buddy_bits_level0,  NUM_OF_4KB_SIZE },
    { buddy_bits_level1,  NUM_OF_8KB_SIZE },
    { buddy_bits_level2,  NUM_OF_16KB_SIZE },
    { buddy_bits_level3,  NUM_OF_32KB_SIZE },
    { buddy_bits_level4,  NUM_OF_64KB_SIZE },
    { buddy_bits_level5,  NUM_OF_128KB_SIZE },
    { buddy_bits_level6,  NUM_OF_256KB_SIZE },
    { buddy_bits_level7,  NUM_OF_512KB_SIZE },
    { buddy_bits_level8,  NUM_OF_1MB_SIZE },
    { buddy_bits_level9,  NUM_OF_2MB_SIZE },
    { buddy_bits_level10, NUM_OF_4MB_SIZE },
    { buddy_bits_level11, NUM_OF_8MB_SIZE },
    { buddy_bits_level12, NUM_OF_16MB_SIZE },
    { buddy_bits_level13, NUM_OF_32MB_SIZE },
    { buddy_bits_level14, NUM_OF_64MB_SIZE }
};

struct arrayhead
{
	uint8* array;
	uint64 size;
	long index_free=0; // if that level is full index is -1
}

extern char end[];

struct {
     struct freeblock* next;
}freeblock ;


struct {
    struct spinlock lock;
    struct freeblock* freelist[];
} kmem;

uint64 address_to_index(void *ptr , uint16 order)
{
	return (uint64)(adr>>(12 + order + 1)); // here i add 12 because order 0 is 4KB ...
}
void* index_to_address(uint64 index , uint16 order)
{
	return (void*)(index<<(12 + order + 1));
}




int split_block(uint64 index ,uint16 order)
{

}
int merge_block(uint64 index ,uint16 order)
{

}

uint64 find_fitting(uint16 order)
{
	while(tree[order].index_free == -1)
	{
		order++; // add break here if memory is full
	}
	return tree[order].index_free;
}

//order is power of 2 of actual size
void* buddy_kalloc(uint16 order)
{
	if(order < 0 || order >BUDDY_TREE_LEVEL) return NULL;



}

int buddy_kfree(void *ptr)
{

}


























