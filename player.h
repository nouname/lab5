#ifndef X_H
#define X_H

#include "point.h"

class Player
{
public:
    friend bool operator != (Player left, char right) {
        return left.player != right;
    }

    friend bool operator == (Player left, char right) {
        return left.player == right;
    }

    Player &operator = (char player) {
        this->player = player;
        return *this;
    }

    Player &operator ! () {
        this->player = this->player == 'X' ? 'O' : 'X';
        return *this;
    }

    Player(Point *point, char player = 'X');
    Point *getPos();
    char getChar();
    void setPos(int x, int y);

private:
    char player;
    Point *point = nullptr;
};

#endif // X_H
