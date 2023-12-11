#include <string.h>
#include <assert.h>
#include "directory.h"
#include "inode.h"
#include "blocks.h"
#include "bitmap.h"
#include "slist.h"
#include "util.h"

// look through directory to find inode based on the given path
int inode_path_lookup(const char* path) {
    if (strcmp(path, "/") == 0) {
        return 0; // Root directory assumed to be at inode 0
    }
    slist_t* dir_list = slist_explode(path, '/');	// split by '/', add to linked list
    slist_t* list_start = dir_list;
    // directory lookup starting from root inode, stops if lookup fails
    int inum = 0;
    while (dir_list != NULL && inum != -1) {
        inode_t* cur_dir = get_inode(inum);
        inum = directory_lookup(cur_dir, dir_list->data);
        dir_list = dir_list->next;
    }
    // free linked list
    slist_free(list_start);
    return inum;
}

// searches through a directory to find an entry with the same name as name
int directory_lookup(inode_t *dd, const char *name) {
    // get block associated with *dd
    dirent_t *directory = (dirent_t *)blocks_get_block(dd->block);
    // iterate through directory
    for (int ii = 0; ii < dd->size / sizeof(dirent_t); ++ii) {
        dirent_t *entry = &directory[ii];
	// compare entry name with the name it's looking for
        if (strcmp(entry->name, name) == 0) {
            return entry->inum;
        }
    }
    // no such file exists
    return -1;
}

// adds a new entry to this directory with the given name and inode index
int directory_put(inode_t *dd, const char *name, int inum) {
    // calculate size after adding entry
    int new_size = dd->size + sizeof(dirent_t);
    // add the given inode
    if (grow_inode(dd, sizeof(dirent_t)) == -1) {
	    return -1;
    }
    // gets the block associated with *dd
    dirent_t *directory = (dirent_t *)blocks_get_block(dd->block);
    dirent_t *new_entry = &directory[dd->size / sizeof(dirent_t)];
    // assign a name and inode index to the directory
    strncpy(new_entry->name, name, DIR_NAME_LENGTH);
    new_entry->inum = inum;
    // update size
    dd->size = new_size;
    // sucessful
    return 0;
}

// deletes an entry with the specified name in the directory. Updates the other entries accordingly
int directory_delete(inode_t *dd, const char *name) {
    // gets the block associated with *dd
    dirent_t *directory = (dirent_t *)blocks_get_block(dd->block);
    // number of directory entries
    int entries = dd->size / sizeof(dirent_t);
    // iterate through entries
    for (int ii = 0; ii < entries; ++ii) {
        dirent_t *entry = &directory[ii];
	// if it finds the entry to delete, it is deleted
        if (strcmp(entry->name, name) == 0) {
	    // the remaining entries' are shifted
            for (int jj = ii; jj < entries - 1; ++jj) {
                directory[jj] = directory[jj + 1];
            }
	    // update size
            dd->size -= sizeof(dirent_t);
            return 0;
        }
    }
    // entry not found
    return -1;
}

// separate the path by a forward slash and create a list
slist_t* directory_list(const char* path) {
    return slist_explode(path, '/');
}

// prints a directory's data
void print_directory(inode_t *dd) {
    slist_t *list = directory_list("..."); // Path to the directory
    // iterate and print each entry's data
    for (slist_t *it = list; it != NULL; it = it->next) {
        printf("%s\n", it->data);
    }
    // free memory
    slist_free(list);
}

