#ifndef GAME_H
#define GAME_H

#include "Config.h"
#include "EndGame.h"
#include "Brick.h"  // 包含 Brick 定义
#include <vector>
#include <string>
#include <chrono>
#include <thread>

// 移除 BrickType 和 Brick 的定义，它们现在在 Brick.h 中

struct Ball {
    double x, y;
    double dx, dy;
    bool attached;
    
    Ball() : x(0), y(0), dx(0), dy(0), attached(true) {}
};

class Game {
private:
    // Game state
    int width, height;
    int paddleX, paddleWidth;
    Ball ball;
    std::vector<std::vector<Brick>> bricks;
    int score;
    int lives;
    int level;
    bool gameRunning;
    bool paused;
    
    // Configuration
    Config config;
    EndGame endgame;
    
    // Timing
    std::chrono::steady_clock::time_point lastUpdate;
    
public:
    Game();
    void run();
    
private:
    void initializeGame();
    void mainMenu();
    void startGame();
    void gameLoop();
    void drawGame();
    void processInput();
    void updateGame();
    void handleCollisions();
    void loadLevel();
    void nextLevel();
    void gameOver();
    void showPauseMenu();
    
    // Configuration functions
    void createConfig();
    void loadConfig();
    
    // Endgame functions
    void createEndGame();
    void loadEndGame();
    void saveEndGameFromPause();
};

#endif