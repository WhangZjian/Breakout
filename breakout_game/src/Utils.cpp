#include "Utils.h"
#include <iostream>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

void Utils::clearScreen() {
    std::cout << "\033[2J\033[1;1H";
}

void Utils::waitForKey() {
    std::cout << "Press Enter to continue...";
    std::cin.ignore();
    std::cin.get();
}

bool Utils::kbhit() {
    struct termios oldt, newt;
    int ch;
    int oldf;
    
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
    
    ch = getchar();
    
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);
    
    if (ch != EOF) {
        ungetc(ch, stdin);
        return true;
    }
    
    return false;
}

void Utils::createDirectory(const std::string& path) {
    mkdir(path.c_str(), 0755);
}