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

Game::Game() : width(9), height(18), paddleWidth(3), score(0), lives(3), level(1), 
               gameRunning(false), paused(false) {
    paddleX = width / 2;
    srand(time(nullptr));
}

void Game::run() {
    initializeGame();
    mainMenu();
}

void Game::initializeGame() {
    config.loadDefault();
    endgame.loadEmpty(width, height);
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
    level = config.initialLevel;
    
    // Initialize ball
    ball.x = width / 2.0;
    ball.y = height - 2;
    ball.dx = 0;
    ball.dy = 0;
    ball.attached = true;
    
    // Initialize paddle
    paddleX = width / 2 - paddleWidth / 2;
    
    // Load level
    loadLevel();
    
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
            double speed = config.ballSpeed;
            int delay = static_cast<int>(1000.0 / speed);
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
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
                ball.dx = (rand() % 3 - 1) * 0.5; // -0.5, 0, or 0.5
                ball.dy = -1.0;
            }
            break;
        case 'p':
            paused = true;
            showPauseMenu();
            break;
        case 'r':
            loadLevel();
            break;
    }
}

void Game::updateGame() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdate);
    
    if (elapsed.count() < 50) return; // Update every 50ms
    
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
    for (const auto& row : bricks) {
        for (const auto& brick : row) {
            if (brick.type != BrickType::EMPTY && brick.type != BrickType::INDESTRUCTIBLE) {
                levelComplete = false;
                break;
            }
        }
        if (!levelComplete) break;
    }
    
    if (levelComplete) {
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
    
    // Paddle collision
    if (ball.y >= height - 2 && ball.dy > 0) {
        if (ball.x >= paddleX && ball.x <= paddleX + paddleWidth) {
            double hitPos = (ball.x - paddleX) / paddleWidth;
            double dx = (hitPos - 0.5) * 2.0; // -1 to 1
            ball.dx = dx * 1.5;
            ball.dy = -abs(ball.dy);
            ball.y = height - 2;
        }
    }
    
    // Brick collisions
    for (auto& row : bricks) {
        for (auto& brick : row) {
            if (brick.type == BrickType::EMPTY) continue;
            
            if (ball.x >= brick.x && ball.x <= brick.x + 1 &&
                ball.y >= brick.y && ball.y <= brick.y + 1) {
                
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
}

void Game::drawGame() {
    Utils::clearScreen();
    
    // Draw score and status
    std::cout << "Score: " << score << " | Lives: " << lives << " | Level: " << level << std::endl;
    std::cout << std::string(width * 2 + 2, '-') << std::endl;
    
    // Draw game area
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
            for (const auto& row : bricks) {
                for (const auto& brick : row) {
                    if (brick.x == x && brick.y == y && brick.type != BrickType::EMPTY) {
                        std::cout << static_cast<char>(brick.type) << static_cast<char>(brick.type);
                        brickFound = true;
                        break;
                    }
                }
                if (brickFound) break;
            }
            if (brickFound) continue;
            
            // Check for paddle
            if (y == height - 1 && x >= paddleX && x < paddleX + paddleWidth) {
                std::cout << "--";
                continue;
            }
            
            // Empty space
            std::cout << "  ";
        }
        std::cout << "|" << std::endl;
    }
    
    std::cout << std::string(width * 2 + 2, '-') << std::endl;
    
    if (paused) {
        std::cout << "PAUSED - Press 'p' to continue, 's' to save, 'r' to restart" << std::endl;
    } else {
        std::cout << "Controls: a-left, d-right, space-launch, p-pause, r-restart" << std::endl;
    }
}

void Game::loadLevel() {
    bricks.clear();
    
    // Create some sample bricks for the level
    for (int y = 0; y < 4; y++) {
        std::vector<Brick> row;
        for (int x = 0; x < width; x++) {
            BrickType type;
            if (y == 0 && x % 3 == 0) {
                type = BrickType::INDESTRUCTIBLE;
            } else if (y == 1 && x % 2 == 0) {
                type = BrickType::DURABLE;
            } else {
                type = BrickType::NORMAL;
            }
            row.emplace_back(x, y, type);
        }
        bricks.push_back(row);
    }
    
    // Reset ball position
    ball.attached = true;
    ball.x = paddleX + paddleWidth / 2.0;
    ball.y = height - 2;
}

void Game::nextLevel() {
    level++;
    if (level > 3) { // Simple level cap
        std::cout << "Congratulations! You beat all levels!" << std::endl;
        Utils::waitForKey();
        gameRunning = false;
        return;
    }
    
    std::cout << "Level " << level << " complete! Loading next level..." << std::endl;
    Utils::waitForKey();
    loadLevel();
}

void Game::gameOver() {
    drawGame();
    std::cout << "GAME OVER! Final Score: " << score << std::endl;
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
    std::string filename;
    std::cout << "Enter endgame name (q to cancel): ";
    std::cin >> filename;
    
    if (filename == "q") return;
    
    EndGame newEndGame;
    newEndGame.filename = filename;
    
    std::cout << "Enter width (8-20): ";
    std::cin >> newEndGame.width;
    std::cout << "Enter height (8-20): ";
    std::cin >> newEndGame.height;
    std::cout << "Enter initial level: ";
    std::cin >> newEndGame.initialLevel;
    
    // Simple brick placement - in a full implementation, this would be interactive
    newEndGame.bricks.emplace_back(2, 2, BrickType::NORMAL);
    newEndGame.bricks.emplace_back(4, 2, BrickType::DURABLE);
    newEndGame.bricks.emplace_back(6, 2, BrickType::INDESTRUCTIBLE);
    
    newEndGame.saveToFile();
    endgame = newEndGame;
    
    // Update game dimensions
    width = newEndGame.width;
    height = newEndGame.height;
    
    std::cout << "End game created successfully!" << std::endl;
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
        width = endgame.width;
        height = endgame.height;
        level = endgame.initialLevel;
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
    newEndGame.width = width;
    newEndGame.height = height;
    newEndGame.initialLevel = level;
    
    // Copy current bricks
    for (const auto& row : bricks) {
        for (const auto& brick : row) {
            if (brick.type != BrickType::EMPTY) {
                newEndGame.bricks.push_back(brick);
            }
        }
    }
    
    newEndGame.saveToFile();
    std::cout << "End game saved successfully!" << std::endl;
    Utils::waitForKey();
}