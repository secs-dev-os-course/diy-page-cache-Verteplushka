#include "cache.h"
#include <fstream>
#include <stdexcept>
#include <iostream>
#include <windows.h>

Cache::Cache(size_t blockSize, size_t maxBlocks)
    : blockSize(blockSize), maxBlocks(maxBlocks) {}

Cache::~Cache() {
    for (auto& [fd, fileCache] : cache) {
        for (auto& [offset, block] : fileCache) {
            if (block.dirty) {
                flushBlock(fd, block);
            }
        }
    }
}

int Cache::openFile(const std::string& path) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    HANDLE fileHandle = CreateFileA(
        path.c_str(), 
        GENERIC_READ | GENERIC_WRITE, 
        0, 
        nullptr, 
        OPEN_EXISTING, 
        FILE_ATTRIBUTE_NORMAL, 
        nullptr
    );
    if (fileHandle == INVALID_HANDLE_VALUE) {
        throw std::runtime_error("Failed to open file: " + path);
    }
    int fd = reinterpret_cast<int>(fileHandle);  // Use intptr_t here
    openFiles[fd] = {fd, path, 0};
    return fd;
}

int Cache::closeFile(int fd) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    auto it = openFiles.find(fd);
    if (it == openFiles.end()) return -1;

    for (auto& [offset, block] : cache[fd]) {
        if (block.dirty) {
            flushBlock(fd, block);
        }
    }

    CloseHandle(reinterpret_cast<HANDLE>(fd));  // Reinterpret as HANDLE
    openFiles.erase(it);
    cache.erase(fd);
    return 0;
}

ssize_t Cache::readFile(int fd, void* buf, size_t count) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    if (openFiles.find(fd) == openFiles.end()) return -1;

    char* buffer = static_cast<char*>(buf);
    size_t bytesRead = 0;

    while (bytesRead < count) {
        off_t offset = openFiles[fd].filePos / blockSize * blockSize;
        CacheBlock* block = getOrCreateBlock(fd, offset);

        size_t blockOffset = openFiles[fd].filePos % blockSize;
        size_t bytesToCopy;
        if((count - bytesRead) > (blockSize - blockOffset)){
            bytesToCopy = blockSize - blockOffset;
        } 
        else{
            bytesToCopy = count - bytesRead;
        }
        // size_t bytesToCopy = std::min(count - bytesRead, blockSize - blockOffset);

        memcpy(buffer + bytesRead, block->data.data() + blockOffset, bytesToCopy);
        openFiles[fd].filePos += bytesToCopy;
        bytesRead += bytesToCopy;
    }

    return bytesRead;
}

ssize_t Cache::writeFile(int fd, const void* buf, size_t count) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    if (openFiles.find(fd) == openFiles.end()) return -1;

    const char* buffer = static_cast<const char*>(buf);
    size_t bytesWritten = 0;

    while (bytesWritten < count) {
        off_t offset = openFiles[fd].filePos / blockSize * blockSize;
        CacheBlock* block = getOrCreateBlock(fd, offset);

        size_t blockOffset = openFiles[fd].filePos % blockSize;
        size_t bytesToCopy;
        if((count - bytesWritten) > (blockSize - blockOffset)){
            bytesToCopy = blockSize - blockOffset;
        } 
        else{
            bytesToCopy = count - bytesWritten;
        }
        // size_t bytesToCopy = std::min(count - bytesWritten, blockSize - blockOffset);

        memcpy(block->data.data() + blockOffset, buffer + bytesWritten, bytesToCopy);
        block->dirty = true;
        openFiles[fd].filePos += bytesToCopy;
        bytesWritten += bytesToCopy;
    }

    return bytesWritten;
}

off_t Cache::seekFile(int fd, off_t offset, int whence) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    if (openFiles.find(fd) == openFiles.end()) return -1;

    if (whence == SEEK_SET) {
        openFiles[fd].filePos = offset;
    } else if (whence == SEEK_CUR) {
        openFiles[fd].filePos += offset;
    } else if (whence == SEEK_END) {
        throw std::runtime_error("SEEK_END is not supported.");
    } else {
        return -1;
    }
    return openFiles[fd].filePos;
}

int Cache::syncFile(int fd) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    if (openFiles.find(fd) == openFiles.end()) return -1;

    for (auto& [offset, block] : cache[fd]) {
        if (block.dirty) {
            flushBlock(fd, block);
        }
    }
    return 0;
}

Cache::CacheBlock* Cache::getOrCreateBlock(int fd, off_t offset) {
    auto& fileCache = cache[fd];
    if (fileCache.find(offset) == fileCache.end()) {
        if (fileCache.size() >= maxBlocks) {
            evictBlock(fd);
        }

        CacheBlock newBlock{offset, std::vector<char>(blockSize), false};
        DWORD bytesRead;
        SetFilePointer(reinterpret_cast<HANDLE>(fd), offset, nullptr, FILE_BEGIN);
        ReadFile(reinterpret_cast<HANDLE>(fd), newBlock.data.data(), blockSize, &bytesRead, nullptr);
        fileCache[offset] = std::move(newBlock);
    }

    return &fileCache[offset];
}

void Cache::flushBlock(int fd, CacheBlock& block) {
    SetFilePointer(reinterpret_cast<HANDLE>(fd), block.offset, nullptr, FILE_BEGIN);
    DWORD bytesWritten;
    WriteFile(reinterpret_cast<HANDLE>(fd), block.data.data(), blockSize, &bytesWritten, nullptr);
    block.dirty = false;
}

void Cache::evictBlock(int fd) {
    auto& fileCache = cache[fd];
    if (!fileCache.empty()) {
        auto it = fileCache.begin();
        if (it->second.dirty) {
            flushBlock(fd, it->second);
        }
        fileCache.erase(it);
    }
}