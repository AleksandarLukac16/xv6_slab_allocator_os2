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
}

extern char end[];

struct {
     struct freeblock* next;
}freeblock ;


struct {
    struct spinlock lock;
    struct freeblock* freelist[];
} kmem;

int address_to_index(void *ptr)
{

}

int address_to_bit(int index , void *ptr)
{

}

int split_block()
int merge_block()

//size is power of 2 of actual size
void* buddy_kalloc(uint16 size)
{
	if(size < 0 || size >BUDDY_TREE_LEVEL) return NULL;



}

int buddy_kfree(void *ptr)
{

}


























