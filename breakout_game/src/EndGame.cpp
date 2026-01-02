#include "EndGame.h"
#include "Utils.h"
#include <fstream>
#include <sstream>

EndGame::EndGame() : filename("empty"), width(9), height(18), initialLevel(1) {}

void EndGame::loadEmpty(int w, int h) {
    filename = "empty";
    width = w;
    height = h;
    initialLevel = 1;
    bricks.clear();
}

bool EndGame::loadFromFile(const std::string& fname) {
    std::string path = "endgames/" + fname + ".end";
    std::ifstream file(path);
    
    if (!file.is_open()) {
        return false;
    }
    
    filename = fname;
    bricks.clear();
    
    file >> width >> height;
    file >> initialLevel;
    
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        
        std::istringstream iss(line);
        char command;
        iss >> command;
        
        if (command == 'P') {
            int x, y;
            char typeChar;
            iss >> x >> y >> typeChar;
            
            BrickType type;
            switch (typeChar) {
                case '@': type = BrickType::NORMAL; break;
                case '#': type = BrickType::DURABLE; break;
                case '*': type = BrickType::INDESTRUCTIBLE; break;
                default: continue;
            }
            
            bricks.emplace_back(x, y, type);
        }
    }
    
    file.close();
    return true;
}

void EndGame::saveToFile() {
    Utils::createDirectory("endgames");
    std::string path = "endgames/" + filename + ".end";
    std::ofstream file(path);
    
    if (file.is_open()) {
        file << width << " " << height << std::endl;
        file << initialLevel << std::endl;
        
        for (const auto& brick : bricks) {
            file << "P " << brick.x << " " << brick.y << " " 
                 << static_cast<char>(brick.type) << std::endl;
        }
        
        file.close();
    }
}