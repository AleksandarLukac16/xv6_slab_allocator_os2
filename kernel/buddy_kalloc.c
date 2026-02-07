#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"
#include "buddy_kalloc.h"

uint8 buddy_bits_level0 [NUM_OF_4KB_SIZE];
uint8 buddy_bits_level1 [NUM_OF_8KB_SIZE];
uint8 buddy_bits_level2 [NUM_OF_16KB_SIZE];
uint8 buddy_bits_level3 [NUM_OF_32KB_SIZE];
uint8 buddy_bits_level4 [NUM_OF_64KB_SIZE];
uint8 buddy_bits_level5 [NUM_OF_128KB_SIZE];
uint8 buddy_bits_level6 [NUM_OF_256KB_SIZE];
uint8 buddy_bits_level7 [NUM_OF_512KB_SIZE];
uint8 buddy_bits_level8 [NUM_OF_1MB_SIZE];
uint8 buddy_bits_level9 [NUM_OF_2MB_SIZE];
uint8 buddy_bits_level10 [NUM_OF_4MB_SIZE];
uint8 buddy_bits_level11 [NUM_OF_8MB_SIZE];
uint8 buddy_bits_level12 [NUM_OF_16MB_SIZE];
uint8 buddy_bits_level13 [NUM_OF_32MB_SIZE];
uint8 buddy_bits_level14 [NUM_OF_64MB_SIZE];

struct arrayhead
{
	uint8* array;
	uint64 size;
} ;

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



extern char end[];

struct {
     struct freeblock* next;
}freeblock ;


struct {
    struct spinlock lock;
    struct freeblock* freelist[];
} kmem;

uint64 address_to_index(void *adr , uint16 order)
{
	return ((uint64)adr>>(12 + order + 1)); // here i add 12 because order 0 is 4KB ...
}
void* index_to_address(long index , uint16 order)
{
	return (void*)(index<<(12 + order + 1));
}

uint8 is_empty(uint16 to_update_order,long to_update) {
	return tree[to_update_order].array[to_update] == SLOT_EMPTY;
}

uint8 is_full(uint16 to_update_order,long to_update) {
	return tree[to_update_order].array[to_update] == SLOT_FULL;
}

uint8 is_buddy(uint16 to_update_order,long to_update) {
	return tree[to_update_order].array[to_update] == SLOT_BUDDY;
}

void alloc_update(uint16 to_update_order,long to_update) {
	tree[to_update_order].array[to_update] = (tree[to_update_order].array[to_update]<<1)|1;
}

void free_update(uint16 to_update_order,long to_update) {
	tree[to_update_order].array[to_update] >>= 1 ;
}



void update_downstairs(uint16 order,void *adr,void(*update)(uint16,long))
{
	uint16 to_update_order = order-1;
	uint64 power_of_iter = 0;
	while(to_update_order > 0)
	{
		long to_update = address_to_index(adr,to_update_order); // index is always lower

		long margin = to_update;
		for(; to_update < margin+(1<<power_of_iter) ; to_update++)
		{
			update(to_update_order,to_update);
			update(to_update_order,to_update);
		}
		power_of_iter ++;
		to_update_order --;

	}
}

void update_upstairs(uint16 order,void *adr,void(*update)(uint16,long))
{
	uint16 to_update_order = order+1;
	while(to_update_order < BUDDY_TREE_LEVEL)
	{
		long to_update = address_to_index(adr,to_update_order);
		update(to_update_order,to_update);
		to_update_order++;
	}
}

//order is power of 2 of actual size
void* buddy_kalloc(uint16 order)
{
	if(order < 0 || order >BUDDY_TREE_LEVEL) return 0;

	for (long i = 0; i< tree[order].size; i++)
	{
		if(tree[order].array[i]==SLOT_BUDDY)
		{
		 	// here i need to go in level down(if not 4kb and update)
			void* adr  = index_to_address(i,order);
			tree[order].array[i]=SLOT_FULL;
			update_downstairs(order,adr,alloc_update);
			return adr;
		}else if(tree[order].array[i]==SLOT_EMPTY)
		{
			void* adr = index_to_address(i,order);
			tree[order].array[i]=SLOT_BUDDY;
			update_downstairs(order,adr,alloc_update);
			update_upstairs(order,adr,alloc_update);
			return adr;
		}
	}

	return 0;

}

int buddy_kfree(void *ptr , uint16 order)
{
	if(order < 0 || order >BUDDY_TREE_LEVEL) return -1;

	long to_free = address_to_index(ptr,order);
	if (tree[order].array[to_free] == SLOT_BUDDY)
	{
		tree[order].array[to_free] = SLOT_EMPTY;
		update_downstairs(order,ptr,free_update);
		update_upstairs(order,ptr,free_update);

	}else if (tree[order].array[to_free] == SLOT_FULL)
	{
		tree[order].array[to_free] = SLOT_BUDDY;
		update_downstairs(order,ptr,free_update);
	}

	return 1;
}


























