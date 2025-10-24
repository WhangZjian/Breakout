#ifndef BRICK_H
#define BRICK_H

enum class BrickType {
    EMPTY = ' ',
    NORMAL = '@',
    DURABLE = '#',
    INDESTRUCTIBLE = '*'
};

struct Brick {
    int x, y;
    BrickType type;
    int durability;
    
    Brick(int x, int y, BrickType type);
};

#endif