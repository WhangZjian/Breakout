#ifndef ENDGAME_H
#define ENDGAME_H

#include "Brick.h"  // 包含 Brick 定义
#include <vector>
#include <string>

struct EndGame {
    std::string filename;
    int width, height;
    int initialLevel;
    std::vector<Brick> bricks;
    
    EndGame();
    void loadEmpty(int w, int h);
    bool loadFromFile(const std::string& filename);
    void saveToFile();
    bool islegal();
};

#endif