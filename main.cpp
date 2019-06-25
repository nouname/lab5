#include "board.h"
#include "player.h"
#include <iostream>
#include <vector>
#include <QCoreApplication>
#include <QHostAddress>
#include <QNetworkInterface>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#define DELIMETER '\n'
#define TIMEOUT 100000000000

static char character = SPACE;
static Board* board;
static int M = 3, N = 3;

using namespace std;

QString get_ip() {
    const QHostAddress &localhost = QHostAddress(QHostAddress::LocalHost);
    for (const QHostAddress &address: QNetworkInterface::allAddresses()) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol && address != localhost)
             return address.toString();
    }
    return "";
}
bool close_session() {
    QString url = "http://kappa.cs.petrsu.ru/~madrahim/tic_tac_toe/init.php?clear=1";
    QNetworkAccessManager manager;
    QNetworkReply *response = manager.get(QNetworkRequest(QUrl(url)));
    QEventLoop event;
    QObject::connect(response, SIGNAL(finished()), &event, SLOT(quit()));
    event.exec();
    QByteArray contents = response->readAll();
    return contents.isEmpty();
}

long wait(bool (*f)(), string msg) {
    cout << msg << endl;
    long timeout = TIMEOUT;
    while (!f() && timeout)
        timeout--;
    return timeout;
}

char rival_character() {
    srand(static_cast<unsigned>(time(nullptr)));
    QString url = "http://kappa.cs.petrsu.ru/~madrahim/tic_tac_toe/session";
    QNetworkAccessManager manager;
    QNetworkReply *response = manager.get(QNetworkRequest(QUrl(url)));
    QEventLoop event;
    QObject::connect(response, SIGNAL(finished()), &event, SLOT(quit()));
    event.exec();
    QByteArray contents = response->readAll();
    if (contents.isEmpty()) {
        char c[2] = {'X', 'O'};
        return c[rand() % 2];
    }
    return contents.split(SPACE)[1][0];
}

bool rival_move() {
    Board* old = new Board(M, N);
    memcpy(old, board, sizeof(Board));
    board->load();
    return old != board;
}

bool init_session(QString ip) {
    close_session();
    character = rival_character() == 'X' ? 'O' : 'X';

    cout << ip.toStdString() << endl;
    QString url = "http://kappa.cs.petrsu.ru/~madrahim/tic_tac_toe/init.php?ip=" + ip + SPACE + character + DELIMETER;
    QNetworkAccessManager manager;
    QNetworkReply *response = manager.get(QNetworkRequest(QUrl(url)));
    QEventLoop event;
    QObject::connect(response, SIGNAL(finished()), &event, SLOT(quit()));
    event.exec();
    QByteArray contents = response->readAll();
    return contents.isEmpty() && character != SPACE;
}

bool check_session() {
    QString url = "http://kappa.cs.petrsu.ru/~madrahim/tic_tac_toe/session";
    QNetworkAccessManager manager;
    QNetworkReply *response = manager.get(QNetworkRequest(QUrl(url)));
    QEventLoop event;
    QObject::connect(response, SIGNAL(finished()), &event, SLOT(quit()));
    event.exec();

    QVariant statusCode = response->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    int status = statusCode.toInt();

    if (status != 200)
        return false;

    QByteArray contents = response->readAll();
    QList<QByteArray> session = contents.split(DELIMETER);
    return session.length() == 3 && session[0] != session[1];
}

void move(Player* player) {
    int x = 0, y = 0;

    cout << "Введите координаты Х." << endl;
    cin >> x >> y;
    x--;
    y--;

    if (x < 0 || y < 0 || x >= M || y >= N || *board->get(x, y) != SPACE)
    {
        cout << "Некорректный ход, попробуйте снова." << endl;
        return move(player);
    }
    else {
        for (int i = x + 1; i < M; i++) {
            if (*board->get(i, y) == SPACE) {
                cout << "Некорректный ход, попробуйте снова." << endl;
                return move(player);
            }
        }
    }
    player->setPos(x, y);
    board->set(player);
    board->save();
}
char start() {
    board = new Board(M, N);
    do {
        Player *player = new Player(new Point(), character);
        move(player);
        if (!wait(rival_move, "Потеряна связь с игороком...")) {
            cout << "Потеряна связь с игороком...";
            return 'T';
        }
        if (board->isTerminal())
            break;

    } while (!board->isTerminal());
    return board->win('X');
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    const string s = "Ожидание закрытия игровой сессии...";
    if (!init_session(get_ip())) {
        cout << "Не удалось инициализировать игровую сессию." << endl;
        wait(close_session, s);
        return 0;
    }

    if (!wait(check_session, "Ожидание подключения игрока...")) {
        cout << "Нет подключения." << endl;
        return 0;
    }

    cout << "Вы - " << character << endl << endl;
    char done = start();
    if (done == 'T')
        return 0;
    if(done == character)
        cout << "Вы победили." << endl;
    else if (board->full())
        cout << "Ничья." << endl;
    else
        cout << "Вы проиграли." << endl;
    wait(close_session, s);
    return 0;
}
