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

class Cache {
public:
    Cache(size_t blockSize, size_t maxBlocks);
    ~Cache();

    int openFile(const std::string& path);
    int closeFile(int fd);
    ssize_t readFile(int fd, void* buf, size_t count);
    ssize_t writeFile(int fd, const void* buf, size_t count);
    off_t seekFile(int fd, off_t offset, int whence);
    int syncFile(int fd);

private:
    struct CacheBlock {
        off_t offset;
        std::vector<char> data;
        bool dirty;
        int accessFrequency;
    };

    struct FileDescriptor {
        int fd;
        std::string path;
        off_t filePos;
    };

    size_t blockSize;
    size_t maxBlocks;
    std::map<int, std::map<off_t, CacheBlock>> cache;
    std::map<int, FileDescriptor> openFiles;
    std::mutex cacheMutex;

    CacheBlock* getOrCreateBlock(int fd, off_t offset);
    void flushBlock(int fd, CacheBlock& block);
    void evictBlock(int fd);
};

#endif
