#include "Brick.h"

Brick::Brick(int x, int y, BrickType type) : x(x), y(y), type(type) {
    durability = (type == BrickType::DURABLE) ? 3 : 1;
}