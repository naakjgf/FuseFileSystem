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
    // Initialize the blocks system with the disk image file path
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
    // extract parent path and lookup the inode of the parent directory
    char parent_path[256];
    extract_parent_path(path, parent_path);
    int parent_inum = inode_path_lookup(parent_path);
    // check if parent directory exists
    if (parent_inum < 0) {
        return -1;
    }
    // get parent inode
    inode_t *parent_inode = get_inode(parent_inum);
    if (!parent_inode) {
        return -1;
    }
    // allocate new inode
    int inum = alloc_inode(mode);
    // check if new inode allocation is successful
    if (inum < 0) {
        return -1;
    }
    // get filename and add it to the parent directory
    const char *name = get_filename_from_path(path);
    if (directory_put(parent_inode, name, inum) < 0) {
        free_inode(inum);
        return -1;
    }
    // success
    return 0;
}

// Remove a file.
int storage_unlink(const char *path) {
    // get parent path
    char parent_path[256];
    extract_parent_path(path, parent_path);
    int parent_inum = inode_path_lookup(parent_path);
    // check if parent directory exists
    if (parent_inum < 0) {
        return -1; // Parent directory not found.
    }
    // get inode of parent directory
    inode_t *parent_inode = get_inode(parent_inum);
    // get filename from path
    const char *name = get_filename_from_path(path);
    // delete the link from parent directory
    if (directory_delete(parent_inode, name) < 0) {
        return -1; // File removal failed.
    }

    return 0; // Success.
}

// Create a link to a file.
int storage_link(const char *from, const char *to) {
    // get inode of source file
    int inum_from = inode_path_lookup(from);
    // check if source file exists
    if (inum_from < 0) {
        return -1; // Source file not found.
    }
    // get parent path from provided link
    char parent_path_to[256];
    extract_parent_path(to, parent_path_to);
    int parent_inum_to = inode_path_lookup(parent_path_to);
    // check if target directory exists
    if (parent_inum_to < 0) {
        return -1; // Target directory not found.
    }
    // get inode of target directory
    inode_t *parent_inode_to = get_inode(parent_inum_to);
    // get filename for new link
    const char *name_to = get_filename_from_path(to);
    // create a new link
    if (directory_put(parent_inode_to, name_to, inum_from) < 0) {
        return -1; // Link creation failed.
    }

    return 0; // Success.
}

// Rename or move a file or directory.
int storage_rename(const char *from, const char *to) {
    // Unlink the target if it exists.
    storage_unlink(to);
    // get inode of source file
    int inum_from = inode_path_lookup(from);
    if (inum_from < 0) {
        return -1; // Source file not found.
    }
    // get parent path from source path
    char parent_path_from[256];
    extract_parent_path(from, parent_path_from);
    // get inode of parent directory
    int parent_inum_from = inode_path_lookup(parent_path_from);
    // get pointer for inode of parent directory
    inode_t *parent_inode_from = get_inode(parent_inum_from);
    const char *name_from = get_filename_from_path(from);
    // delete file from sourec
    directory_delete(parent_inode_from, name_from);
    // get parent path from target path
    char parent_path_to[256];
    extract_parent_path(to, parent_path_to);
    int parent_inum_to = inode_path_lookup(parent_path_to);
    // get file name and inode of parent directory
    inode_t *parent_inode_to = get_inode(parent_inum_to);
    const char *name_to = get_filename_from_path(to);
    // add a new entry for the directory
    directory_put(parent_inode_to, name_to, inum_from);
    return 0; // Success.
}

// Set file access and modification times.
int storage_set_time(const char *path, const struct timespec ts[2]) {
    // get inode of file
    int inum = inode_path_lookup(path);
    if (inum < 0) {
        return -1; // File not found.
    }
    // get pointer to file's inode
    inode_t *inode = get_inode(inum);
    if (!inode) {
        return -1; // Inode not found.
    }
    return 0; // Success.
}

// List the contents of a directory.
slist_t* storage_list(const char* path) {
    // get inode of file
    int inum = inode_path_lookup(path);
    if (inum < 0) {
        return NULL; // Directory not found.
    }
    // get pointer to file's inode
    inode_t* inode = get_inode(inum);
    if (!inode || !(inode->mode & 040000)) { // Check if it's a directory.
        return NULL;
    }
    // return the list of directories
    return directory_list(path);
}

