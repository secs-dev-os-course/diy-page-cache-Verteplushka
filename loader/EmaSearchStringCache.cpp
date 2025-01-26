#include "EmaSearchStringCache.hpp"
#include "../cache/cache.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <cstring>


constexpr std::size_t CHUNK_SIZE = 16 * 1024;

void ema_search_str_cache(int repetitions, const std::string filename) {
    Cache cache(1024, 128);
    HANDLE fd = 0;

    try {
        fd = cache.openFile(filename);
    } catch (const std::exception& ex) {
        std::cerr << "Error opening file: " << ex.what() << std::endl;
        return;
    }

    const std::string target = "search_string_5000";
    std::size_t target_size = target.size();
    int found_count = 0;

    auto start_time = std::chrono::high_resolution_clock::now();

    for (int r = 0; r < repetitions; ++r) {
        std::vector<char> buffer(CHUNK_SIZE + target_size - 1);
        std::string leftover;

        while (true) {
            ssize_t bytes_read = cache.readFile(fd, buffer.data() + leftover.size(), CHUNK_SIZE);

            if (bytes_read <= 0) break;

            std::memcpy(buffer.data(), leftover.data(), leftover.size());
            std::size_t total_size = leftover.size() + bytes_read;

            for (std::size_t i = 0; i + target_size <= total_size; ++i) {
                if (std::memcmp(buffer.data() + i, target.data(), target_size) == 0) {
                    ++found_count;
                }
            }

            if (total_size >= target_size) {
                leftover.assign(buffer.data() + total_size - target_size + 1, target_size - 1);
            } else {
                leftover.assign(buffer.data(), total_size);
            }
        }

        cache.seekFile(fd, 0, SEEK_SET);
    }

    cache.closeFile(fd);

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end_time - start_time;

    std::cout << "EMA Search: Found " << found_count << " occurrences in " << repetitions
              << " repetitions, taking " << elapsed.count() << " seconds." << std::endl;
}
