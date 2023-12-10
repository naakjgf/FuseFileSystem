#include <sys/stat.h>
#include "storage.h"
#include "inode.h"
#include "directory.h"
#include "blocks.h"
#include "util.h"

// functions from inode
int inode_path_lookup(const char *path);
void read_from_file(int inode, char *buf, size_t size, off_t offset);
void write_to_file(int inode, const char *buf, size_t size, off_t offset);

// Initialize the storage system.
void storage_init(const char *path) {
    // Initialize the blocks system with the disk image file path.
	printf("Debug: Initializing storage with path: %s\n", path);
    	blocks_init(path);

    // Additional initialization steps can be added here if needed.
    // For example, initializing inodes or directories if required.
}

// Retrieve file or directory metadata.
int storage_stat(const char *path, struct stat *st) {
    // Find the inode number for the given path.
    int inum = inode_path_lookup(path);
    if (inum < 0) {
        return -1; // Path not found.
    }

    // Get the inode using the inode number.
    inode_t *inode = get_inode(inum);
    if (!inode) {
        return -1; // Inode not found.
    }

    // Populate the stat structure with inode data.
    st->st_mode = inode->mode;
    st->st_nlink = inode->refs;
    st->st_size = inode->size;
    st->st_uid = getuid();  // Assuming the file belongs to the current user.
    st->st_gid = getgid();  // Assuming the file belongs to the current user's group.
    // Other stat fields can be set here if needed.

    return 0; // Success.
}

// Read data from a file.
int storage_read(const char *path, char *buf, size_t size, off_t offset) {
    // Find the inode for the given path.
    int inum = inode_path_lookup(path);
    if (inum < 0) {
        return -1; // File not found.
    }

    inode_t *inode = get_inode(inum);
    if (!inode) {
        return -1; // Inode not found.
    }

    // Adjust size if it exceeds the file size from the offset.
    if (offset + size > inode->size) {
        size = inode->size - offset;
    }

    // Read data into buffer.
    read_from_file(inum, buf, size, offset);

    return size; // Number of bytes read.
}

// Write data to a file.
int storage_write(const char *path, const char *buf, size_t size, off_t offset) {
    // Find the inode for the given path.
    int inum = inode_path_lookup(path);
    if (inum < 0) {
        return -1; // File not found.
    }

    inode_t *inode = get_inode(inum);
    if (!inode) {
        return -1; // Inode not found.
    }

    // Ensure the inode is large enough to accommodate the write.
    if (offset + size > inode->size) {
        int grow_size = offset + size - inode->size;
        if (grow_inode(inode, grow_size) < 0) {
            return -1; // Failed to grow inode.
        }
    }

    // Write data from buffer to file.
    write_to_file(inum, buf, size, offset);

    return size; // Number of bytes written.
}

// Truncate or extend a file to a specified length.
int storage_truncate(const char *path, off_t size) {
    // Find the inode for the given path.
    int inum = inode_path_lookup(path);
    if (inum < 0) {
        return -1; // File not found.
    }

    inode_t *inode = get_inode(inum);
    if (!inode) {
        return -1; // Inode not found.
    }

    // Adjust the file size.
    if (size > inode->size) {
        // Extend the file.
        int grow_size = size - inode->size;
        if (grow_inode(inode, grow_size) < 0) {
            return -1; // Failed to grow inode.
        }
    } else if (size < inode->size) {
        // Truncate the file.
        int shrink_size = inode->size - size;
        if (shrink_inode(inode, shrink_size) < 0) {
            return -1; // Failed to shrink inode.
        }
    }

    return 0; // Success.
}

// Create a new file or directory.
int storage_mknod(const char *path, int mode) {
    printf("Debug: storage_mknod called with path: %s, mode: %o\n", path, mode);
    
    char parent_path[256];
    extract_parent_path(path, parent_path);
    int parent_inum = inode_path_lookup(parent_path);
    printf("Debug: Parent inode number: %d\n", parent_inum);

    if (parent_inum < 0) {
        printf("Debug: Parent directory not found\n");
        return -1;
    }

    inode_t *parent_inode = get_inode(parent_inum);
    if (!parent_inode) {
        printf("Debug: Parent inode not found\n");
        return -1;
    }

    int inum = alloc_inode(mode);
    printf("Debug: Allocated inode number: %d\n", inum);

    if (inum < 0) {
        printf("Debug: Failed to allocate inode\n");
        return -1;
    }

    const char *name = get_filename_from_path(path);
    if (directory_put(parent_inode, name, inum) < 0) {
        printf("Debug: Failed to add entry to directory\n");
        free_inode(inum);
        return -1;
    }

    printf("Debug: storage_mknod completed successfully\n");
    return 0;
}

// Remove a file.
int storage_unlink(const char *path) {
    char parent_path[256];
    extract_parent_path(path, parent_path);
    int parent_inum = inode_path_lookup(parent_path);
    if (parent_inum < 0) {
        return -1; // Parent directory not found.
    }

    inode_t *parent_inode = get_inode(parent_inum);
    const char *name = get_filename_from_path(path);

    if (directory_delete(parent_inode, name) < 0) {
        return -1; // File removal failed.
    }

    return 0; // Success.
}

// Create a link to a file.
int storage_link(const char *from, const char *to) {
    int inum_from = inode_path_lookup(from);
    if (inum_from < 0) {
        return -1; // Source file not found.
    }

    char parent_path_to[256];
    extract_parent_path(to, parent_path_to);
    int parent_inum_to = inode_path_lookup(parent_path_to);
    if (parent_inum_to < 0) {
        return -1; // Target directory not found.
    }

    inode_t *parent_inode_to = get_inode(parent_inum_to);
    const char *name_to = get_filename_from_path(to);

    if (directory_put(parent_inode_to, name_to, inum_from) < 0) {
        return -1; // Link creation failed.
    }

    return 0; // Success.
}

// Rename or move a file or directory.
int storage_rename(const char *from, const char *to) {
    // Unlink the target if it exists.
    storage_unlink(to);

    int inum_from = inode_path_lookup(from);
    if (inum_from < 0) {
        return -1; // Source file not found.
    }

    char parent_path_from[256];
    extract_parent_path(from, parent_path_from);
    int parent_inum_from = inode_path_lookup(parent_path_from);

    inode_t *parent_inode_from = get_inode(parent_inum_from);
    const char *name_from = get_filename_from_path(from);

    directory_delete(parent_inode_from, name_from);

    char parent_path_to[256];
    extract_parent_path(to, parent_path_to);
    int parent_inum_to = inode_path_lookup(parent_path_to);

    inode_t *parent_inode_to = get_inode(parent_inum_to);
    const char *name_to = get_filename_from_path(to);

    directory_put(parent_inode_to, name_to, inum_from);

    return 0; // Success.
}

// Set file access and modification times.
int storage_set_time(const char *path, const struct timespec ts[2]) {
    int inum = inode_path_lookup(path);
    if (inum < 0) {
        return -1; // File not found.
    }

    inode_t *inode = get_inode(inum);
    if (!inode) {
        return -1; // Inode not found.
    }

    // Update times in the inode. (Assuming inode structure has these fields)
//    inode->atime = ts[0].tv_sec;
//    inode->mtime = ts[1].tv_sec;

    return 0; // Success.
}

// List the contents of a directory.
slist_t* storage_list(const char* path) {
    int inum = inode_path_lookup(path);
    if (inum < 0) {
        return NULL; // Directory not found.
    }

    inode_t* inode = get_inode(inum);
    if (!inode || !(inode->mode & 040000)) { // Check if it's a directory.
        return NULL;
    }

    return directory_list(path);
}

