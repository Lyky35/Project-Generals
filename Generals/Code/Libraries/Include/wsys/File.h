#ifndef _WSYS_FILE_H_
#define _WSYS_FILE_H_

#include <stdio.h>
#include <stdint.h>

// File system stubs
typedef FILE File;

static inline File* File_Open(const char *path, const char *mode) {
    return fopen(path, mode);
}

static inline void File_Close(File *file) {
    if (file) fclose(file);
}

static inline size_t File_Read(void *buffer, size_t size, size_t count, File *file) {
    return fread(buffer, size, count, file);
}

static inline size_t File_Write(const void *buffer, size_t size, size_t count, File *file) {
    return fwrite(buffer, size, count, file);
}

static inline int File_Seek(File *file, long offset, int origin) {
    return fseek(file, offset, origin);
}

static inline long File_Tell(File *file) {
    return ftell(file);
}

static inline int File_Eof(File *file) {
    return feof(file);
}

static inline int File_Size(File *file) {
    long pos = ftell(file);
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, pos, SEEK_SET);
    return (int)size;
}

#endif
