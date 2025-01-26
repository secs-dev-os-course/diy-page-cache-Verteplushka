#ifndef CACHE_H
#define CACHE_H

#ifdef _WIN32
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif

#include <map>
#include <vector>
#include <mutex>
#include <string>
#include <windows.h>

class Cache {
public:
    Cache(size_t blockSize, size_t maxBlocks);
    ~Cache();

    HANDLE openFile(const std::string& path);
    int closeFile(HANDLE fd);
    ssize_t readFile(HANDLE fd, void* buf, size_t count);
    ssize_t writeFile(HANDLE fd, const void* buf, size_t count);
    off_t seekFile(HANDLE fd, off_t offset, int whence);
    int syncFile(HANDLE fd);

private:
    struct CacheBlock {
        off_t offset;
        std::vector<char> data;
        size_t dataSize = 0;
        bool dirty;
        int accessFrequency;
    };

    struct FileDescriptor {
        HANDLE fd;
        std::string path;
        off_t filePos;
    };

    size_t blockSize;
    size_t maxBlocks;
    std::map<HANDLE, std::map<off_t, CacheBlock>> cache;
    std::map<HANDLE, FileDescriptor> openFiles;
    std::mutex cacheMutex;

    CacheBlock* getOrCreateBlock(HANDLE fd, off_t offset);
    void flushBlock(HANDLE fd, CacheBlock& block);
    void evictBlock(HANDLE fd);
};

#endif
