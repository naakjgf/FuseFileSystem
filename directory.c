#include <string.h>
#include <assert.h>
#include "directory.h"
#include "inode.h"
#include "blocks.h"
#include "bitmap.h"
#include "slist.h"
#include "util.h"

int inode_path_lookup(const char* path) {
    if (strcmp(path, "/") == 0) {
        return 0; // Root directory assumed to be at inode 0
    }

    slist_t* dir_list = slist_explode(path, '/');
    slist_t* list_start = dir_list;

    int inum = 0; // Start from root inode
    while (dir_list != NULL && inum != -1) {
        inode_t* cur_dir = get_inode(inum);
        inum = directory_lookup(cur_dir, dir_list->data);
        dir_list = dir_list->next;
    }

    slist_free(list_start);
    return inum;
}

int directory_lookup(inode_t *dd, const char *name) {
    dirent_t *directory = (dirent_t *)blocks_get_block(dd->block);
    for (int ii = 0; ii < dd->size / sizeof(dirent_t); ++ii) {
        dirent_t *entry = &directory[ii];
        if (strreq(entry->name, name)) {
            return entry->inum;
        }
    }
    return -1;
}

int directory_put(inode_t *dd, const char *name, int inum) {
    int new_size = dd->size + sizeof(dirent_t);
    if (grow_inode(dd, sizeof(dirent_t)) == -1) {
        return -1;
    }

    dirent_t *directory = (dirent_t *)blocks_get_block(dd->block);
    dirent_t *new_entry = &directory[dd->size / sizeof(dirent_t)];
    memset(new_entry->name, 0, DIR_NAME_LENGTH);
    strncpy(new_entry->name, name, DIR_NAME_LENGTH);
    new_entry->inum = inum;
    dd->size = new_size;

    return 0;
}

int directory_delete(inode_t *dd, const char *name) {
    dirent_t *directory = (dirent_t *)blocks_get_block(dd->block);
    int entries = dd->size / sizeof(dirent_t);

    for (int ii = 0; ii < entries; ++ii) {
        dirent_t *entry = &directory[ii];
        if (strreq(entry->name, name)) {
            for (int jj = ii; jj < entries - 1; ++jj) {
                directory[jj] = directory[jj + 1];
            }
            dd->size -= sizeof(dirent_t);
            return 0;
        }
    }

    return -1;
}

slist_t* directory_list(const char* path) {
    return slist_explode(path, '/');
}

void print_directory(inode_t *dd) {
    slist_t *list = directory_list("..."); // Path to the directory
    for (slist_t *it = list; it != NULL; it = it->next) {
        printf("%s\n", it->data);
    }
    slist_free(list);
}

