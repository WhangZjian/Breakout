#ifndef CONFIG_H
#define CONFIG_H

#include <string>

struct Config {
    std::string filename;
    double ballSpeed;
    int randomSeed;
    int initialLevel;
    
    Config();
    void loadDefault();
    bool loadFromFile(const std::string& filename);
    void saveToFile();
};

#endif