#include "ema_search_string.hpp"
#include <iostream>
#include <fstream>
#include <chrono>
#include <vector>
#include <cstring>

constexpr std::size_t CHUNK_SIZE = 16 * 1024;

void ema_search_str(int repetitions, std::string filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error opening file!" << std::endl;
        return;
    }

    const std::string target = "search_string_5000";
    std::size_t target_size = target.size();
    int found_count = 0;

    auto start_time = std::chrono::high_resolution_clock::now();

    for (int r = 0; r < repetitions; ++r) {
        file.clear();
        file.seekg(0, std::ios::beg);

        std::vector<char> buffer(CHUNK_SIZE + target_size - 1);

        std::string leftover;
        while (!file.eof()) {
            file.read(buffer.data() + leftover.size(), CHUNK_SIZE);
            std::streamsize bytes_read = file.gcount();

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
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end_time - start_time;

    std::cout << "EMA Search: Found " << found_count << " occurrences in " << repetitions
              << " repetitions, taking " << elapsed.count() << " seconds." << std::endl;
}
