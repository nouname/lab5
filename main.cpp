#include "board.h"
#include "player.h"
#include <iostream>
#include <vector>
#include <QCoreApplication>
#include <QHostAddress>
#include <QNetworkInterface>
#include <qnetworkaccessmanager.h>
#include <QNetworkReply>

#define DELIMETER '+'

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

bool init_session(QString ip) {
    cout << ip.toStdString() << endl;
    QString url = "http://kappa.cs.petrsu.ru/~madrahim/tic_tac_toe/init.php?ip=" + ip + DELIMETER;
    QNetworkAccessManager manager;
    QNetworkReply *response = manager.get(QNetworkRequest(QUrl(url)));
    QEventLoop event;
    QObject::connect(response, SIGNAL(finished()), &event, SLOT(quit()));
    event.exec();
    QByteArray contents = response->readAll();
    return !contents.isEmpty();
}

bool close_session() {
    QString url = "http://kappa.cs.petrsu.ru/~madrahim/tic_tac_toe/init.php?ip=";
    QNetworkAccessManager manager;
    QNetworkReply *response = manager.get(QNetworkRequest(QUrl(url)));
    QEventLoop event;
    QObject::connect(response, SIGNAL(finished()), &event, SLOT(quit()));
    event.exec();
    QByteArray contents = response->readAll();
    return !contents.isEmpty();
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
}
char start() {
    board = new Board(M, N);
    do {
        Player *player = new Player(new Point(), 'X');
        move(player);
        if (board->isTerminal())
            break;
        player = new Player(new Point(), 'O');
        move(player);

    } while (!board->isTerminal());
    return board->win('X');
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    if (!init_session(get_ip())) {
        cout << "Не удалось инициализировать игровую сессию." << endl;
        cout << "Ожидание закрытия игровой сессии..." << endl;
        while (!close_session());
        return 0;
    }
    long timeout = 60000000000;
    cout << "Ожидание подключения игрока..." << endl;
    while (!check_session() && timeout)
        timeout--;
    if (!timeout) {
        cout << "Нет подключения." << endl;
        cout << "Ожидание закрытия игровой сессии..." << endl;
        while (!close_session());
        return 0;
    }
    cout << "Вы - X" << endl << endl;
    char done = start();
    if(done == 'X')
        cout << "X победил." << endl;
    else if (board->full())
        cout << "Ничья." << endl;
    else
        cout << "O победил." << endl;
    cout << "Ожидание закрытия игровой сессии..." << endl;
    while (!close_session());
    return 0;
}
