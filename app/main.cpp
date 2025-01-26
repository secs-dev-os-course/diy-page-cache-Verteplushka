#include <iostream>
#include <windows.h>
#include "cache.h"

int main() {
    Cache cache(4096, 10); // Блок 4KB, максимум 10 блоков в кэше

    try {
        HANDLE fd = cache.openFile("../data/text.txt");
        std::cout << "OpenFile returned: " << fd << std::endl;
        char buffer[128] = {0};

        ssize_t readSize = cache.readFile(fd, buffer, sizeof(buffer));
        std::cout << "ReadFile returned size: " << readSize << ", fd: " << fd << ", buffer: " << buffer << std::endl;

        char buffer2[128] = {0};
        ssize_t readSize2 = cache.readFile(fd, buffer2, sizeof(buffer2));
        std::cout << "ReadFile returned size: " << readSize2 << ", fd: " << fd << ", buffer: " << buffer2 << std::endl;

        const char* data = "Hello, Viktor!";
        ssize_t writeSize = cache.writeFile(fd, data, strlen(data));
        std::cout << "WriteFile returned size: " << writeSize << ", fd: " << fd << ", buffer: " << data << std::endl;

        const char* data2 = "Hello, Petua!";
        ssize_t writeSize2 = cache.writeFile(fd, data2, strlen(data2));
        std::cout << "WriteFile returned size: " << writeSize2 << ", fd: " << fd << ", buffer: " << data2 << std::endl;

        int closedFile = cache.closeFile(fd);

        std::cout << "CloseFile returned: " << closedFile << std::endl;
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
    }

    return 0;
}
