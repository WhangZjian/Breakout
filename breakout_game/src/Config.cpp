#include "Config.h"
#include "Utils.h"
#include <fstream>
#include <iostream>

Config::Config() : filename("default"), ballSpeed(5), randomSeed(-1), initialLevel(1) {}

void Config::loadDefault() {
    filename = "default";
    ballSpeed = 5;
    randomSeed = -1;
    initialLevel = 1;
}

bool Config::loadFromFile(const std::string& fname) {
    std::string path = "config/" + fname + ".config";
    std::ifstream file(path);
    
    if (!file.is_open()) {
        return false;
    }
    
    filename = fname;
    file >> ballSpeed;
    file >> randomSeed;
    file >> initialLevel;
    
    file.close();
    return true;
}

void Config::saveToFile() {
    Utils::createDirectory("config");
    std::string path = "config/" + filename + ".config";
    std::ofstream file(path);
    
    if (file.is_open()) {
        file << ballSpeed << std::endl;
        file << randomSeed << std::endl;
        file << initialLevel << std::endl;
        file.close();
    }
}