#include "ema_search_string_cache.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <number of repetitions>" << std::endl;
        return 1;
    }

    int repetitions = std::stoi(argv[1]);
    ema_search_str_cache(repetitions, "../data/little_data.txt");

    return 0;
}
