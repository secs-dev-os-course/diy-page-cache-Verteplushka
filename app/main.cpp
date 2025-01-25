#include <iostream>
#include "cache.h"

int main() {
    Cache cache(4096, 10); // Блок 4KB, максимум 10 блоков в кэше

    try {
        int fd = cache.openFile("../data/text.txt");
        std::cout << "OpenFile returned: " << fd << std::endl;
        char buffer[128] = {0};

        ssize_t readSize = cache.readFile(fd, buffer, sizeof(buffer));
        std::cout << "ReadFile returned size: " << readSize << ", fd: " << fd << ", buffer: " << buffer << std::endl;

        const char* data = "Hello, world!";
        ssize_t writeSize = cache.writeFile(fd, data, strlen(data));
        std::cout << "WriteFile returned size: " << writeSize << ", fd: " << fd << ", buffer: " << data << std::endl;

        cache.syncFile(fd);
        int closedFile = cache.closeFile(fd);
        std::cout << "CloseFile returned: " << closedFile << std::endl;
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
    }

    return 0;
}
