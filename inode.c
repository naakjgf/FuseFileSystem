#include "inode.h"
#include "blocks.h"
#include "bitmap.h"
#include <string.h>

// The main inode.c implementations

const int INODE_COUNT = 256;

inode_t* get_inode(int inum) {
	void* inodes = blocks_get_block(1); // Assuming inode list is stored in block 1
	return (inode_t*)(inodes + (sizeof(inode_t) * inum));
}

int alloc_inode(int mode) {
    printf("Debug: alloc_inode called with mode: %o\n", mode);

    void* ibm = get_inode_bitmap();
    for (int ii = 1; ii < INODE_COUNT; ++ii) {
        if (!bitmap_get(ibm, ii)) {
            bitmap_put(ibm, ii, 1);
            printf("Debug: alloc_inode allocated inode number: %d\n", ii);

            inode_t* new_inode = get_inode(ii);
            new_inode->refs = 1;
            new_inode->mode = mode;
            new_inode->size = 0;
            new_inode->block = alloc_block();
            printf("Debug: Block allocated for inode: %d\n", new_inode->block);

            if (new_inode->block == -1) {
                printf("Debug: Block allocation failed, freeing inode bitmap\n");
                bitmap_put(ibm, ii, 0);
                return -1;
            }
            return ii;
        }
    }

    printf("Debug: No inode could be allocated\n");
    return -1;
}


void free_inode(int inum) {
	printf("+ free_inode(%d)\n", inum);
	void* ibm = get_inode_bitmap();
	bitmap_put(ibm, inum, 0);

	inode_t* node = get_inode(inum);
	free_block(node->block); // Freeing the block associated with the inode
	node->block = 0;
}

int grow_inode(inode_t* node, int size) {
	// Assuming size increase doesn't require new block allocation
	node->size += size;
	return node->size; 
}

int shrink_inode(inode_t* node, int size) {
	// Reducing size without affecting the block
	if (node->size > size) {
		node->size -= size;
	}
	return node->size;
}

void write_to_file(inode_t* node, const char *buf, size_t size, off_t offset) {
	void* block = blocks_get_block(node->block);
	memcpy(block + offset, buf, size);
}

void read_from_file(inode_t* node, char *buf, size_t size, off_t offset) {
	void* block = blocks_get_block(node->block);
	memcpy(buf, block + offset, size);
}

