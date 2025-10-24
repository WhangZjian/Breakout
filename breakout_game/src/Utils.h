#ifndef UTILS_H
#define UTILS_H

#include <string>

class Utils {
public:
    static void clearScreen();
    static void waitForKey();
    static bool kbhit();
    static void createDirectory(const std::string& path);
};

#endif