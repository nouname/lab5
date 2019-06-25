#ifndef BOARD_H
#define BOARD_H

#include "player.h"
#include <vector>

#define SPACE ' '

class Board
{
public:
    Board(int M = 3, int N = 3);
    bool full();
    char win(char player);
    bool isTerminal();
    void set(Player* player);
    Player *get(int x, int y);
    bool save();
    bool load();

private:
    int M = 3;
    int N = 3;
    Player ***matrix;
    void init();
    void display();
    const Player *none = new Player(new Point(0, 0), SPACE);
};

#endif // BOARD_H
