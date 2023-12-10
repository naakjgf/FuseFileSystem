// Extracts the parent path from a given file path.
void extract_parent_path(const char* path, char* parent_path) {
    strcpy(parent_path, path);
    char* last_slash = strrchr(parent_path, '/');
    if (last_slash) {
        *last_slash = '\0'; // Truncate the string at the last slash.
    }
}

// Extracts the file name from a given file path.
const char* get_filename_from_path(const char* path) {
    const char* last_slash = strrchr(path, '/');
    return last_slash ? last_slash + 1 : path;
}
