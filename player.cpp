#include "player.h"
#include <algorithm>

Player::Player(Point *point, char player)
{
    this->player = player;
    this->point = point;
}

Point *Player::getPos()
{
    return point;
}

void Player::setPos(int x, int y)
{
    Point *point = new Point(x, y);
    this->point = point;
}

char Player::getChar()
{
    return player;
}
