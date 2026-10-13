#ifndef _WSYS_FILE_H_
#define _WSYS_FILE_H_

#include <stdio.h>
#include <stdint.h>
#include <string.h>

// File class - wraps FILE* with C++ interface
class File
{
public:
    enum {
        START = SEEK_SET,
        CURRENT = SEEK_CUR,
        END = SEEK_END
    };

    enum {
        READ = 0,
        WRITE = 1,
        BINARY = 0x8000
    };

    File() : m_fp(NULL) { m_name[0] = 0; }
    ~File() { close(); }

    int open(const char *path, int mode = READ) {
        strncpy(m_name, path, sizeof(m_name)-1);
        const char *m = (mode & WRITE) ? ((mode & BINARY) ? "wb" : "w")
                                       : ((mode & BINARY) ? "rb" : "r");
        m_fp = fopen(path, m);
        return m_fp ? 0 : -1;
    }

    void close() {
        if (m_fp) { fclose(m_fp); m_fp = NULL; }
    }

    int read(void *buf, int bytes) {
        return m_fp ? (int)fread(buf, 1, bytes, m_fp) : 0;
    }

    int write(const void *buf, int bytes) {
        return m_fp ? (int)fwrite(buf, 1, bytes, m_fp) : 0;
    }

    long seek(long offset, int origin = START) {
        return m_fp ? fseek(m_fp, offset, origin) : -1;
    }

    long tell() { return m_fp ? ftell(m_fp) : -1; }
    long position() { return tell(); }
    long size() {
        if (!m_fp) return 0;
        long pos = ftell(m_fp);
        fseek(m_fp, 0, SEEK_END);
        long sz = ftell(m_fp);
        fseek(m_fp, pos, SEEK_SET);
        return sz;
    }
    int eof() { return m_fp ? feof(m_fp) : 1; }
    const char *getName() const { return m_name; }
    bool isOpen() const { return m_fp != NULL; }

private:
    FILE *m_fp;
    char m_name[256];
};

#endif
