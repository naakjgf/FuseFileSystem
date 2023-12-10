// based on cs3650 starter code

#include <assert.h>
#include <bsd/string.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "storage.h"

#define FUSE_USE_VERSION 26
#include <fuse.h>

// implementation for: man 2 access
// Checks if a file exists.
int nufs_access(const char *path, int mask) {
    struct stat st;
    int res = storage_stat(path, &st);
    return (res == 0) ? 0 : -ENOENT;
}

// Gets an object's attributes (type, permissions, size, etc).
int nufs_getattr(const char *path, struct stat *st) {
    int res = storage_stat(path, st);
    if (res != 0) {
        return -ENOENT;
    }
    return 0;
}

// Lists the contents of a directory
int nufs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    struct stat st;
    if (nufs_getattr(path, &st) != 0 || !S_ISDIR(st.st_mode)) {
        return -ENOENT;
    }

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    slist_t *names = storage_list(path);
    for (slist_t *it = names; it != NULL; it = it->next) {
        filler(buf, it->data, NULL, 0);
    }

    slist_free(names);
    return 0;
}

// mknod makes a filesystem object like a file or directory
// called for: man 2 open, man 2 link
// Note, for this assignment, you can alternatively implement the create
// function.
// Create a file node
int nufs_mknod(const char *path, mode_t mode, dev_t rdev) {
    return storage_mknod(path, mode);
}

// most of the following callbacks implement
// another system call; see section 2 of the manual
// Create a directory
int nufs_mkdir(const char *path, mode_t mode) {
    return storage_mknod(path, mode | S_IFDIR);
}

// Remove a file
int nufs_unlink(const char *path) {
    return storage_unlink(path);
}

// Create a hard link
int nufs_link(const char *from, const char *to) {
    return storage_link(from, to);
}

int nufs_rmdir(const char *path) {
  int rv = -1;
  printf("rmdir(%s) -> %d\n", path, rv);
  return rv;
}

// Rename a file
int nufs_rename(const char *from, const char *to) {
    return storage_rename(from, to);
}

// Change permissions.
int nufs_chmod(const char *path, mode_t mode) {
    return -ENOSYS; // Didn't implement
}

// Change file size
int nufs_truncate(const char *path, off_t size) {
    return storage_truncate(path, size);
}

// This is called on open, but doesn't need to do much
// since FUSE doesn't assume you maintain state for
// open files.
// You can just check whether the file is accessible. The File open operation
int nufs_open(const char *path, struct fuse_file_info *fi) {
    return 0; // Nothing done.
}

// Read data from a file
int nufs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    return storage_read(path, buf, size, offset);
}

// Write data to a file
int nufs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    return storage_write(path, buf, size, offset);
}

// Set file access and modification times
int nufs_utimens(const char *path, const struct timespec ts[2]) {
    return storage_set_time(path, ts);
}

// Extended operations
int nufs_ioctl(const char *path, int cmd, void *arg, struct fuse_file_info *fi,
               unsigned int flags, void *data) {
  int rv = -1;
  printf("ioctl(%s, %d, ...) -> %d\n", path, cmd, rv);
  return rv;
}

void nufs_init_ops(struct fuse_operations *ops) {
  memset(ops, 0, sizeof(struct fuse_operations));
  ops->access = nufs_access;
  ops->getattr = nufs_getattr;
  ops->readdir = nufs_readdir;
  ops->mknod = nufs_mknod;
  // ops->create   = nufs_create; // alternative to mknod
  ops->mkdir = nufs_mkdir;
  ops->link = nufs_link;
  ops->unlink = nufs_unlink;
  ops->rmdir = nufs_rmdir;
  ops->rename = nufs_rename;
  ops->chmod = nufs_chmod;
  ops->truncate = nufs_truncate;
  ops->open = nufs_open;
  ops->read = nufs_read;
  ops->write = nufs_write;
  ops->utimens = nufs_utimens;
  ops->ioctl = nufs_ioctl;
};

struct fuse_operations nufs_ops;

int main(int argc, char *argv[]) {
  assert(argc > 2 && argc < 6);
  printf("TODO: mount %s as data file\n", argv[--argc]);
  // storage_init(argv[--argc]);
  nufs_init_ops(&nufs_ops);
  return fuse_main(argc, argv, &nufs_ops, NULL);
}
