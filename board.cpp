#include "board.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QEventLoop>
#include <QString>
#include <iostream>
#include <QFile>

using namespace std;

Board::Board(int M, int N)
{
    this->M = M;
    this->N = N;
    init();
    save();
}

void Board::init()
{
    matrix = new Player **[M];
    for (int i = 0; i < M; i++) {
        matrix[i] = new Player *[N];
        for (int j = 0; j < N; j++)
            matrix[i][j] = new Player(new Point(), SPACE);
    }
}

bool Board::full() {
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++)
            if (*matrix[i][j] == SPACE)
                return false;
    }
    return true;
}

void Board::display() {
    load();
    cout << string(static_cast<unsigned>(N * 2 + 1), '-');
    for(int i = 0; i < M; i++)
    {
        cout << '\n';
        for (int j = 0; j < N; j++)
            cout << "|" << matrix[i][j]->getChar();

        if(i < M - 1) {
            cout << "|\n";
            for (int j = 0; j < N; j++)
                cout << "|-";
            cout << '|';
        }
    }
    cout << "|\n";
    cout << string(static_cast<unsigned>(N * 2 + 1), '-');
    cout << '\n';
}

char Board::win(char player) {
    int i, j, count;
    for(i = 0; i < M; i++) {
        count = 0;
        for(j = 0; j < N; j++) {
            if (*matrix[i][j] == player)
                count++;
        }
        if (count >= N)
            return player;
    }
    for(i = 0; i < N; i++) {
        count = 0;
        for(j = 0; j < M; j++) {
            if (*matrix[j][i] == player)
                count++;
        }
        if (count >= M)
            return player;
    }
    if (M == N) {
        count = 0;
        for (i = 0; i < M; i++)
            if (*matrix[i][i] == player)
                count++;
        if (count >= M)
            return player;

        count = 0;
        for (i = 0; i < M; i++)
            if (*matrix[M - i - 1][M - i - 1] == player)
                count++;
        if (count >= M)
            return player;
    }
    return SPACE;
}

bool Board::isTerminal() {
    load();
    save();
    return full() || win('X') != SPACE || win('O') != SPACE;
}

void Board::set(Player *player) {
    matrix[player->getPos()->x][player->getPos()->y] = player;
    save();
}

Player* Board::get(int x, int y) {
    return matrix[x][y];
}

bool Board::save() {
    QString s = "";
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++)
            s += matrix[i][j]->getChar();

    }
    QString url = "http://kappa.cs.petrsu.ru/~madrahim/tic_tac_toe/save.php?board=" + s;
    QNetworkAccessManager manager;
    QNetworkReply *response = manager.get(QNetworkRequest(QUrl(url)));
    QEventLoop event;
    QObject::connect(response, SIGNAL(finished()), &event, SLOT(quit()));
    event.exec();
    QByteArray contents = response->readAll();
    return contents.isEmpty();

}
void Board::load() {
    QString url = "http://kappa.cs.petrsu.ru/~madrahim/tic_tac_toe/board";
    QNetworkAccessManager manager;
    QNetworkReply *response = manager.get(QNetworkRequest(QUrl(url)));
    QEventLoop event;
    QObject::connect(response, SIGNAL(finished()), &event, SLOT(quit()));
    event.exec();

    QVariant statusCode = response->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    int status = statusCode.toInt();

    if (status != 200)
        return;

    QByteArray contents = response->readAll();
    for (int i = 0; i < contents.length(); i++) {
        int column = i % N;
        *matrix[(i - column) / N][column] = contents[i];
    }
}

bool Board::equal(Board *other)
{
    if (M != other->M || N != other->N)
        return false;
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
            if (matrix[i][j] != other->matrix[i][j])
                return false;
        }
    }
    return true;
}

