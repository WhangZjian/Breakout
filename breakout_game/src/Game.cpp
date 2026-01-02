#include "Game.h"
#include "Utils.h"
#include "Brick.h"
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <ctime>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <cmath>
#include <set>

Game::Game() : width(19), height(18), paddleWidth(7), score(0), lives(3), level(1), 
               gameRunning(false), paused(false), EndGameCreating(false) {
    paddleX = width / 2;
    srand(time(nullptr));
}

void Game::run() {
    initializeGame();
    mainMenu();
}

void Game::initializeGame() {
    config.loadDefault();
    loadLevel("Level_1");
}

void Game::mainMenu() {
    while (true) {
        Utils::clearScreen();
        std::cout << "=== BREAKOUT GAME ===" << std::endl;
        std::cout << "g - Start Game" << std::endl;
        std::cout << "n - Create End Game" << std::endl;
        std::cout << "m - Load End Game" << std::endl;
        std::cout << "i - Create Config" << std::endl;
        std::cout << "u - Load Config" << std::endl;
        std::cout << "q - Quit" << std::endl;
        std::cout << "=====================" << std::endl;
        
        char choice;
        std::cin >> choice;
        
        switch (choice) {
            case 'g':
                startGame();
                break;
            case 'n':
                createEndGame();
                break;
            case 'm':
                loadEndGame();
                break;
            case 'i':
                createConfig();
                break;
            case 'u':
                loadConfig();
                break;
            case 'q':
                return;
            default:
                std::cout << "Invalid choice!" << std::endl;
                Utils::waitForKey();
        }
    }
}

void Game::startGame() {
    gameRunning = true;
    paused = false;
    score = 0;
    lives = 3;
    fps = 50;
    level = config.initialLevel;
    
    // Initialize ball
    ball.x = width / 2.0;
    ball.y = height - 2;
    ball.dx = 0;
    ball.dy = 0;
    ball.attached = true;
    ball.speed = config.ballSpeed / fps * 5.0;
    
    // Initialize paddle
    paddleX = width / 2 - paddleWidth / 2;
    
    gameLoop();
}

void Game::gameLoop() {
    lastUpdate = std::chrono::steady_clock::now();
    
    while (gameRunning) {
        if (!paused) {
            processInput();
            updateGame();
            drawGame();
            
            // Control game speed based on ball speed
            // double speed = config.ballSpeed;
            // int delay = 50; //static_cast<int>(1000.0 / speed);
            std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(1000.0 / fps)));
        } else {
            processInput();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
}

void Game::processInput() {
    if (!Utils::kbhit()) return;
    
    char ch = std::cin.get();
    
    if (paused) {
        switch (ch) {
            case 'p':
                paused = false;
                break;
            case 's':
                saveEndGameFromPause();
                break;
            case 'r':
                return;
        }
        return;
    }
    
    switch (ch) {
        case 'a':
            if (paddleX > 0) paddleX--;
            break;
        case 'd':
            if (paddleX < width - paddleWidth) paddleX++;
            break;
        case ' ':
            if (ball.attached) {
                ball.attached = false;
                ball.dx = (rand() % 3 - 1) * 0.5 * ball.speed; // -0.5, 0, or 0.5
                ball.dy = -ball.cacdy();
            }
            break;
        case 'p':
            paused = true;
            showPauseMenu();
            break;
        case 'r':
            loadLevel("Level_" + static_cast<char>('0'+level));
            break;
    }
}

void Game::updateGame() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdate);
    
//    if (elapsed.count() < 50) return; // Update every 50ms
    
    lastUpdate = now;
    
    if (ball.attached) {
        ball.x = paddleX + paddleWidth / 2.0;
        return;
    }
    
    // Move ball
    ball.x += ball.dx;
    ball.y += ball.dy;
    
    handleCollisions();
    
    // Check if level complete
    bool levelComplete = true;
    for (const auto& brick : bricks) {
        if (brick.type != BrickType::EMPTY && brick.type != BrickType::INDESTRUCTIBLE) {
            levelComplete = false;
            break;
        }
        if (!levelComplete) break;
    }
    
    if (levelComplete) {
        drawGame();
        nextLevel();
    }
}

void Game::handleCollisions() {
    // Wall collisions
    if (ball.x <= 0 || ball.x >= width - 1) {
        ball.dx = -ball.dx;
        ball.x = (ball.x <= 0) ? 0 : width - 1;
    }
    
    if (ball.y <= 0) {
        ball.dy = -ball.dy;
        ball.y = 0;
    }
    
    // Paddle collision
    if (ball.y >= height - 2 && ball.dy > 0) {
        if (ball.x >= paddleX && ball.x <= paddleX + paddleWidth) {
            double hitPos = (ball.x - paddleX) / paddleWidth;
            // double dx = (hitPos - 0.5) * 2.0; // -1 to 1
            ball.dx = (hitPos - 0.5) * ball.speed * 1.5;
            ball.dy = -ball.cacdy();
            ball.y = height - 2;
        }
    }
    
    // Bottom collision (lose life)
    if (ball.y >= height) {
        lives--;
        if (lives <= 0) {
            gameOver();
            return;
        }
        ball.attached = true;
        ball.x = paddleX + paddleWidth / 2.0;
        ball.y = height - 2;
        return;
    }
    
    // Brick collisions
    for (auto& brick : bricks) {
        if (brick.type == BrickType::EMPTY) continue;
        
        if (ball.x >= brick.x  && ball.x <= brick.x + 1.3 &&
            ball.y >= brick.y  && ball.y <= brick.y + 1.3) {
            
            // Handle different brick types
            if (brick.type == BrickType::NORMAL) {
                brick.type = BrickType::EMPTY;
                score += 10;
            } else if (brick.type == BrickType::DURABLE) {
                brick.durability--;
                if (brick.durability <= 0) {
                    brick.type = BrickType::EMPTY;
                }
                score += 5;
            }
            // INDESTRUCTIBLE bricks don't break
            
            // Bounce ball
            double brickCenterX = brick.x + 0.5;
            double brickCenterY = brick.y + 0.5;
            double dx = ball.x - brickCenterX;
            double dy = ball.y - brickCenterY;
            
            if (abs(dx) > abs(dy)) {
                ball.dx = -ball.dx;
            } else {
                ball.dy = -ball.dy;
            }
            
            return; // Only handle one collision per frame
        }
    }
}

void Game::drawGame() {
    Utils::clearScreen();
    
    // Draw score and status
    if (!EndGameCreating) {
        std::cout << "Score: " << score << " | Lives: " << lives << " | Level: " << level << std::endl;   
    }

    // Draw game area    
    std::cout << std::string(width * 2 + 2, '-') << std::endl;
    for (int y = 0; y < height; y++) {
        std::cout << "|";
        for (int x = 0; x < width; x++) {
            // Check for ball
            if (!ball.attached && static_cast<int>(ball.x) == x && static_cast<int>(ball.y) == y) {
                std::cout << "()";
                continue;
            }
            
            // Check for bricks
            bool brickFound = false;
            for (const auto& brick : bricks) {
                if (brick.x == x && brick.y == y && brick.type != BrickType::EMPTY) {
                    std::cout << static_cast<char>(brick.type) << static_cast<char>(brick.type);
                    brickFound = true;
                    break;
                }
                if (brickFound) break;
            }
            if (brickFound) continue;
            
            // Check for paddle
            if (EndGameCreating==false && y == height - 1 && x >= paddleX && x < paddleX + paddleWidth) {
                std::cout << "--";
                continue;
            }
            
            // Empty space
            std::cout << "  ";
        }
        std::cout << "|" << std::endl;
    }
    
    std::cout << std::string(width * 2 + 2, '-') << std::endl;
    if (EndGameCreating) {
        std::cout << "P x y type -- place a type brick at (x,y)" << std::endl;
        std::cout << "D x y -- delete the birck at (x,y)" << std::endl;
        std::cout << "f -- finish placing" << std::endl;
        std::cout << "q -- quit placing" << std::endl;
    }
    else if (paused) {
        std::cout << "PAUSED - Press 'p' to continue, 's' to save, 'r' to restart" << std::endl;
    } 
    else {
        std::cout << "Controls: a-left, d-right, space-launch, p-pause, r-restart" << std::endl;
    }
}

void Game::updateLevel() {
    width = endgame.width*2+1;
    height = endgame.height;
    level = endgame.initialLevel;
    bricks = endgame.bricks;
}

bool Game::loadLevel(const std::string& Level_Name) {
    bricks.clear();

    if (endgame.loadFromFile(Level_Name)) {
        //std::cout << bricks.size() << std::endl;
        //exit(0);
        updateLevel();
        ball.attached = true;
        ball.x = paddleX + paddleWidth / 2.0;
        ball.y = height - 2;
        return 1;
    } else {
        std::cout << "Failed to load " << Level_Name << "!" << std::endl;
        return 0;
    }
}

void Game::nextLevel() {
    std::cout << "Level " << level << " complete! Loading next level..." << std::endl;
    level++;
    if (level > 3) { // Simple level cap
        std::cout << "Congratulations! You beat all levels!" << std::endl;
        Utils::waitForKey();
        gameRunning = false;
        return;
    }
    
    Utils::waitForKey();
    loadLevel("Level_" + static_cast<char>('0'+level));
}

void Game::gameOver() {
    drawGame();
    std::cout << "GAME OVER! Final Score: " << score << std::endl;
    loadLevel("Level_1");
    Utils::waitForKey();
    gameRunning = false;
}

void Game::showPauseMenu() {
    drawGame();
    std::cout << "PAUSE MENU" << std::endl;
    std::cout << "p - Continue" << std::endl;
    std::cout << "s - Save End Game" << std::endl;
    std::cout << "r - Restart Level" << std::endl;
}

void Game::createConfig() {
    std::string filename;
    std::cout << "Enter config name (q to cancel): ";
    std::cin >> filename;
    
    if (filename == "q") return;
    
    Config newConfig;
    newConfig.filename = filename;
    
    std::cout << "Enter ball speed (1-10): ";
    std::cin >> newConfig.ballSpeed;
    std::cout << "Enter random seed (-1 for random): ";
    std::cin >> newConfig.randomSeed;
    std::cout << "Enter initial level: ";
    std::cin >> newConfig.initialLevel;
    
    newConfig.saveToFile();
    config = newConfig;
}

void Game::loadConfig() {
    std::string filename;
    std::cout << "Current config: " << config.filename << std::endl;
    std::cout << "Enter config name to load (q to cancel): ";
    std::cin >> filename;
    
    if (filename == "q") return;
    
    if (config.loadFromFile(filename)) {
        std::cout << "Config loaded successfully!" << std::endl;
    } else {
        std::cout << "Failed to load config!" << std::endl;
    }
    Utils::waitForKey();
}

void Game::createEndGame() {
    EndGame oldendgame = endgame;
    EndGameCreating = true;

    std::string filename;
    std::cout << "Enter endgame name (q to cancel): ";
    std::cin >> filename;
    
    if (filename == "q") return;
    
    endgame.filename = filename;
    endgame.bricks.clear();
    
    std::cout << "Enter width (8-20): ";
    std::cin >> endgame.width;
    std::cout << "Enter height (8-20): ";
    std::cin >> endgame.height;
    std::cout << "Enter initial level: ";
    std::cin >> endgame.initialLevel;
    
    // Simple brick placement - in a full implementation, this would be interactive
    updateLevel();
    drawGame();

    std::set<std::pair<int,int>> findbrick; 

    while (true) {
        char command;
        std::cin >> command;
        
        if (command == 'P') {
            int x, y;
            char typeChar;
            std::cin >> x >> y >> typeChar;
            x = 2 * x + 1;

            if (x>=width || x<0 || y>=height-3 || y<0){
                drawGame();
                std::cout << "Invaild coordinate!" << std::endl;
                continue;
            }
            if (findbrick.count({x, y})){
                drawGame();
                std::cout << "Brick exists!" << std::endl;
                continue;
            }
            findbrick.insert({x, y});
            
            BrickType type;
            switch (typeChar) {
                case '@': type = BrickType::NORMAL; break;
                case '#': type = BrickType::DURABLE; break;
                case '*': type = BrickType::INDESTRUCTIBLE; break;
                default: continue;
            }
            
            endgame.bricks.emplace_back(x, y, type);
            updateLevel();
            drawGame();
        }
        else if (command=='D'){
            int x, y;
            bool isDeleted = false;
            std::cin >> x >> y;
            x = 2 * x + 1;

            for(auto it = endgame.bricks.begin(); it != endgame.bricks.end(); it++){
                if ((*it).x==x && (*it).y==y) {
                    auto itnext = it + 1;
                    endgame.bricks.erase(it, itnext);
                    updateLevel();
                    drawGame();
                    std::cout << "Brick deleted!" << std::endl;
                    isDeleted = true;
                    break;
                }
            }

            if(!isDeleted) {
                drawGame();
                std::cout << "Brick not found!" << std::endl;
            }
        }
        else if (command=='f') {
            std::cout << "End game created successfully!" << std::endl;
            endgame.saveToFile();
            break;
        }
        else if (command == 'q') {
            break;
        }
        else {
            drawGame();
            std::cout << "Invaild command!" << std::endl;
        }
    }
    
    // 复原设置
    endgame=oldendgame;
    updateLevel();
    EndGameCreating = false;
    
    Utils::waitForKey();
}

void Game::loadEndGame() {
    std::string filename;
    std::cout << "Current endgame: " << endgame.filename << std::endl;
    std::cout << "Enter endgame name to load (q to cancel): ";
    std::cin >> filename;
    
    if (filename == "q") return;
    
    if (endgame.loadFromFile(filename)) {
        std::cout << "End game loaded successfully!" << std::endl;
        updateLevel();
    } else {
        std::cout << "Failed to load end game!" << std::endl;
    }
    Utils::waitForKey();
}

void Game::saveEndGameFromPause() {
    std::string filename;
    std::cout << "Enter filename to save endgame: ";
    std::cin >> filename;
    
    EndGame newEndGame;
    newEndGame.filename = filename;
    newEndGame.width = width / 2;
    newEndGame.height = height;
    newEndGame.initialLevel = level;
    
    // Copy current bricks
    for (const auto& brick : bricks) {
        if (brick.type != BrickType::EMPTY) {
            newEndGame.bricks.push_back(brick);
        }
    }
    
    newEndGame.saveToFile();
    std::cout << "End game saved successfully!" << std::endl;
    Utils::waitForKey();
}