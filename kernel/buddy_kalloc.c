#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"
#include "buddy_kalloc.h"


struct arrayhead
{
	uint8* array;
	uint64 size;
} ;

uint8 tree[2^(BUDDY_TREE_LEVEL)];



extern char end[];

struct {
     struct freeblock* next;
}freeblock ;


struct {
    struct spinlock lock;
    struct freeblock* freelist[];
} kmem;

























