#include "inode.h"
#include "blocks.h"
#include "bitmap.h"

// The main inode.c implementations

const int INODE_COUNT = 256;

inode* get_inode(int inum) {
	void* inodes = blocks_get_block(1); // Assuming inode list is stored in block 1
	return (inode*)(inodes + (sizeof(inode) * inum));
}

int alloc_inode(int mode) {
	void* ibm = get_inode_bitmap();

	for (int ii = 1; ii < INODE_COUNT; ++ii) {
		if (!bitmap_get(ibm, ii)) {
			bitmap_put(ibm, ii, 1);
			printf("+ alloc_inode() -> %d\n", ii);

			inode* new_inode = get_inode(ii);
			new_inode->refs = 1;
			new_inode->mode = mode;
			new_inode->size = 0;
			new_inode->block = alloc_block(); // Allocating a block for the inode
			if (new_inode->block == -1) {
				bitmap_put(ibm, ii, 0); // Freeing up the inode bitmap if block allocation fails
				return -1;
			}
			return ii;
		}
	}

	return -1;
}

void free_inode(int inum) {
	printf("+ free_inode(%d)\n", inum);
	void* ibm = get_inode_bitmap();
	bitmap_put(ibm, inum, 0);

	inode* node = get_inode(inum);
	free_block(node->block); // Freeing the block associated with the inode
	node->block = 0;
}

int grow_inode(inode* node, int size) {
	// Assuming size increase doesn't require new block allocation
	node->size += size;
	return node->size; 
}

int shrink_inode(inode* node, int size) {
	// Reducing size without affecting the block
	if (node->size > size) {
		node->size -= size;
	}
	return node->size;
}

void write_to_file(inode* node, const char *buf, size_t size, off_t offset) {
	void* block = blocks_get_block(node->block);
	memcpy(block + offset, buf, size);
}

void read_from_file(inode* node, char *buf, size_t size, off_t offset) {
	void* block = blocks_get_block(node->block);
	memcpy(buf, block + offset, size);
}

