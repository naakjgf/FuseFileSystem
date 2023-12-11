#include "inode.h"
#include "blocks.h"
#include "bitmap.h"
#include <string.h>

// The main inode.c implementations

const int INODE_COUNT = 256;

// gets an inode based on the given inum index
inode_t* get_inode(int inum) {
	void* inodes = blocks_get_block(1); // Assuming inode list is stored in block 1
	return (inode_t*)(inodes + (sizeof(inode_t) * inum));
}

// searches for a free inode, updates bitmap, initializes the inode and allocates a block
int alloc_inode(int mode) {
    // gets bitmap
    void* ibm = get_inode_bitmap();
    // iterate through inode numbers to look for a free inode
    for (int ii = 1; ii < INODE_COUNT; ++ii) {
        if (!bitmap_get(ibm, ii)) {
	    // sets the free inode's correspoinding bit in the bitmap
            bitmap_put(ibm, ii, 1);
	    // initializes the newly created inode
            inode_t* new_inode = get_inode(ii);
            new_inode->refs = 1;
            new_inode->mode = mode;
            new_inode->size = 0;
            new_inode->block = alloc_block();
	    // handle if inode allocation fails
            if (new_inode->block == -1) {
                bitmap_put(ibm, ii, 0);
                return -1;
            }
	    // return inode number (success)
            return ii;
        }
    }
    // no free inodes
    return -1;
}

// marks an inode as free, frees the block associated with the inode, and then resets the block pointer
void free_inode(int inum) {
	printf("+ free_inode(%d)\n", inum);
	// gets the inode bitmap
	void* ibm = get_inode_bitmap();
	// frees the inode
	bitmap_put(ibm, inum, 0);
	// gets the inode based on inum
	inode_t* node = get_inode(inum);
	free_block(node->block); // Freeing the block associated with the inode
	// reset block pointer
	node->block = 0;
}

// increases the size of an inode
int grow_inode(inode_t* node, int size) {
	// Assuming size increase doesn't require new block allocation
	node->size += size;
	return node->size; 
}

// reduces the size of an inode
int shrink_inode(inode_t* node, int size) {
	// Reducing size without affecting the block
	if (node->size > size) {
		node->size -= size;
	}
	return node->size;
}

// writes data into a file 
void write_to_file(inode_t* node, const char *buf, size_t size, off_t offset) {
	// gets the block associated with node
	void* block = blocks_get_block(node->block);
	// make a copy of size amount of node's data 
	memcpy(block + offset, buf, size);
}

// reads data from a file
void read_from_file(inode_t* node, char *buf, size_t size, off_t offset) {
	// gets the block associated with inode
	void* block = blocks_get_block(node->block);
	// copy size amount of data from offset into buffer
	memcpy(buf, block + offset, size);
}

