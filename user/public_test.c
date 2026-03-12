#include <stdio.h>

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

typedef unsigned long size_t;
#define RUN_NUM (5)
#define ITERATIONS (1000)

#define shared_size (7)
#define MASK (0xA5)

struct data_s {
    int id;
    int shared;
    int iterations;
};

const char * const CACHE_NAMES[] = {"tc_0",
                                    "tc_1",
                                    "tc_2",
                                    "tc_3",
                                    "tc_4"};

// void construct(void *data) {
//     static int i = 1;
//     printf("%d Shared object constructed.\n", i++);
//     memset(data, MASK, shared_size);
// }

int check(void *data, size_t size) {
    int ret = 1;
    for (int i = 0; i < size; i++) {
        if (((unsigned char *)data)[i] != MASK) {
            ret = 0;
        }
    }

    return ret;
}

struct objects_s {
    int cache;
    void *data;
};

void work(void* pdata) {
    struct data_s data = *(struct data_s*) pdata;
    printf("Ovde sam\n");
    int size = 0;
    int object_size = data.id + 1;
    int cache_handle = kmem_cache_create(CACHE_NAMES[data.id], object_size, 0, 0);
    printf("Ovde sam2\n");
    unsigned long addr =0;
    struct objects_s *objs = (struct objects_s*)(kmalloc(sizeof(struct objects_s) * data.iterations,(unsigned long)&addr));
    //printf("Ovde sam3\n");
    for (int i = 0; i < data.iterations; i++) {
        if (i % 100 == 0) {
            unsigned long addr1 =0;
            objs[size].data = (void*)kmem_cache_alloc(data.shared,(unsigned long)&addr1);
            //printf("Ovde sam4\n");
            objs[size].cache = data.shared;
            if (!check(objs[size].data, shared_size)) {
                printf("Value not correct!");
            }
        }
        else {
            unsigned long addr2=0;
            objs[size].data = (void*)kmem_cache_alloc(cache_handle,(unsigned long)&addr2);
            objs[size].cache = cache_handle;
            memset(objs[size].data, MASK, object_size);
        }
        size++;
    }

    kmem_cache_info(cache_handle);
    kmem_cache_info(data.shared);

    for (int i = 0; i < size; i++) {
        if (!check(objs[i].data, (cache_handle == objs[i].cache) ? object_size : shared_size)) {
            printf("Value not correct!");
        }
        kmem_cache_free(objs[i].cache, (unsigned long)objs[i].data);
    }

    kfree((unsigned long)objs,sizeof(struct objects_s) * data.iterations);
    kmem_cache_destroy(cache_handle);
}



void runs(void(*work)(void*), struct data_s* data, int num) {
    for (int i = 0; i < num; i++) {
        struct data_s private_data;
        private_data = *(struct data_s*) data;
        private_data.id = i;
        work(&private_data);
    }
}

void main() {
    //int num_of_blocks = 1024;
    //void* space = malloc(num_of_blocks * BLOCK_SIZE);
    kmem_init(0,0);
    int shared = kmem_cache_create("shared object", shared_size, 0, 0);

    struct data_s data;
    data.shared = shared;
    data.iterations = ITERATIONS;
    //printf("ovde sam");
    runs(work, &data, RUN_NUM);

    kmem_cache_destroy(shared);
    //free(space);
}

