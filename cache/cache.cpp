#include "cache.h"
#include <fstream>
#include <stdexcept>
#include <iostream>
#include <windows.h>
#include <algorithm>

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

HANDLE Cache::openFile(const std::string& path) {
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
    openFiles[fileHandle] = {fileHandle, path, 0};
    return fileHandle;
}

int Cache::closeFile(HANDLE fd) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    auto it = openFiles.find(fd);
    if (it == openFiles.end()) return -1;

    for (auto& [offset, block] : cache[fd]) {
        if (block.dirty) {
            flushBlock(fd, block);
        }
    }

    CloseHandle(fd);
    openFiles.erase(it);
    cache.erase(fd);
    return 0;
}

ssize_t Cache::readFile(HANDLE fd, void* buf, size_t count) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    if (openFiles.find(fd) == openFiles.end()) return -1;

    char* buffer = static_cast<char*>(buf);
    size_t bytesRead = 0;

    while (bytesRead < count) {
        off_t offset = openFiles[fd].filePos / blockSize * blockSize;
        CacheBlock* block = getOrCreateBlock(fd, offset);
        if (!block) break; 

        block->accessFrequency++;

        size_t blockOffset = openFiles[fd].filePos % blockSize;
        size_t availableInBlock = block->dataSize - blockOffset;

        if(availableInBlock <= 0){
            break;
        }

        size_t bytesToCopy = (count - bytesRead < availableInBlock) ? (count - bytesRead) : availableInBlock;

        if (blockOffset + bytesToCopy > block->dataSize) {
            bytesToCopy = block->dataSize - blockOffset;
        }

        memcpy(buffer + bytesRead, block->data.data() + blockOffset, bytesToCopy);
        openFiles[fd].filePos += bytesToCopy;
        bytesRead += bytesToCopy;

        if (bytesRead >= count) {
            break;
        }

        if (bytesToCopy < availableInBlock) {
            break;
        }
    }

    return bytesRead;
}

ssize_t Cache::writeFile(HANDLE fd, const void* buf, size_t count) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    if (openFiles.find(fd) == openFiles.end()) return -1;

    const char* buffer = static_cast<const char*>(buf);
    size_t bytesWritten = 0;

    while (bytesWritten < count) {
        off_t offset = openFiles[fd].filePos / blockSize * blockSize;
        CacheBlock* block = getOrCreateBlock(fd, offset);

        block->accessFrequency++;

        size_t blockOffset = openFiles[fd].filePos % blockSize;
        size_t bytesToCopy = (count - bytesWritten) < (blockSize - blockOffset) ? (count - bytesWritten) : (blockSize - blockOffset);

        memcpy(block->data.data() + blockOffset, buffer + bytesWritten, bytesToCopy);
        block->dirty = true;

        openFiles[fd].filePos += bytesToCopy;
        if(block->dataSize < openFiles[fd].filePos){
            block->dataSize = openFiles[fd].filePos;
        }
        bytesWritten += bytesToCopy;
    }

    return bytesWritten;
}

off_t Cache::seekFile(HANDLE fd, off_t offset, int whence) {
    std::lock_guard<std::mutex> lock(cacheMutex);

    if (openFiles.find(fd) == openFiles.end()) return -1;

    FileDescriptor& fileDesc = openFiles[fd];
    LARGE_INTEGER move, newPointer;

    move.QuadPart = offset;

    if (whence == SEEK_SET) {
        fileDesc.filePos = offset;
    } else if (whence == SEEK_CUR) {
        fileDesc.filePos += offset;
    } else if (whence == SEEK_END) {
        LARGE_INTEGER fileSize;
        if (!GetFileSizeEx(fd, &fileSize)) {
            return -1;
        }
        fileDesc.filePos = fileSize.QuadPart + offset;
    } else {
        return -1;
    }

    if (fileDesc.filePos < 0) {
        fileDesc.filePos = 0;
        return -1;
    }

    // move.QuadPart = fileDesc.filePos;
    // if (!SetFilePointerEx(fd, move, &newPointer, FILE_BEGIN)) {
    //     return -1;
    // }

    return fileDesc.filePos;
}


int Cache::syncFile(HANDLE fd) {
    std::lock_guard<std::mutex> lock(cacheMutex);
    if (openFiles.find(fd) == openFiles.end()) return -1;

    for (auto& [offset, block] : cache[fd]) {
        if (block.dirty) {
            flushBlock(fd, block);
        }
    }
    return 0;
}

Cache::CacheBlock* Cache::getOrCreateBlock(HANDLE fd, off_t offset) {
    auto& fileCache = cache[fd];
    if (fileCache.find(offset) == fileCache.end()) {
        if (fileCache.size() >= maxBlocks) {
            evictBlock(fd);
        }

        CacheBlock newBlock{offset, std::vector<char>(blockSize), false, 0};
        DWORD bytesRead = 0;
        SetFilePointer(fd, offset, nullptr, FILE_BEGIN);
        ReadFile(fd, newBlock.data.data(), blockSize, &bytesRead, nullptr);

        newBlock.dataSize = bytesRead;
        fileCache[offset] = std::move(newBlock);
    }

    return &fileCache[offset];
}

void Cache::flushBlock(HANDLE fd, CacheBlock& block) {
    SetFilePointer(fd, block.offset, nullptr, FILE_BEGIN);
    DWORD bytesWritten;
    // size_t blockOffset = openFiles[fd].filePos % blockSize;
    size_t blockOffset = block.dataSize;
    WriteFile(fd, block.data.data(), blockOffset, &bytesWritten, nullptr);
    block.dirty = false;
}

void Cache::evictBlock(HANDLE fd) {
    auto& fileCache = cache[fd];

    if (!fileCache.empty()) {
        auto lfuBlockIt = std::min_element(fileCache.begin(), fileCache.end(),
            [](const std::pair<off_t, CacheBlock>& a, const std::pair<off_t, CacheBlock>& b) {
                return a.second.accessFrequency < b.second.accessFrequency;
            });

        if (lfuBlockIt->second.dirty) {
            flushBlock(fd, lfuBlockIt->second);
        }

        fileCache.erase(lfuBlockIt);
    }
}
