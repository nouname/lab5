#include "board.h"
#include "player.h"
#include <iostream>
#include <vector>
#include <signal.h>
#include <sstream>
#include <QCoreApplication>
#include <QHostAddress>
#include <QNetworkInterface>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#define DELIMETER '\n'
#define TIMEOUT 100000000000

static char character = SPACE;
static Board *board;
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

QByteArray response(QString file) {
    QString url = "http://kappa.cs.petrsu.ru/~madrahim/tic_tac_toe/" + file;
    QNetworkAccessManager manager;
    QNetworkReply *reply= manager.get(QNetworkRequest(QUrl(url)));
    QEventLoop event;
    QObject::connect(reply, SIGNAL(finished()), &event, SLOT(quit()));
    event.exec();

    QVariant statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute);
    int status = statusCode.toInt();

    if (status != 200)
        return "";

    return reply->readAll();
}

bool close_session() {
    QByteArray contents = response("init.php?clear=1");
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
    QByteArray contents = response("session");
    if (contents.isEmpty()) {
        char c[2] = {'X', 'O'};
        return c[rand() % 2];
    }
    return contents.split(SPACE)[1][0];
}

bool check_session() {
    QByteArray contents = response("session");
    QList<QByteArray> session = contents.split(DELIMETER);
    return session.length() == 3 && session[0] != session[1];
}

bool rival_move() {
    QByteArray contents = response("move");
    return contents[0] != character || contents.isEmpty();
}

bool init_session(QString ip) {
    if (check_session())
        close_session();
    character = rival_character() == 'X' ? 'O' : 'X';

    cout << ip.toStdString() << endl;
    QByteArray contents = response("init.php?ip=" + ip + SPACE + character + DELIMETER);
    return contents.isEmpty() && character != SPACE;
}

void move(Player* player) {
    int x = 0, y = 0;
    board->load();
    board->display();
    cout << "Введите координаты " << character << ": " << endl;
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
    response("move.php?move=" + QString(character));
    board->save();
    board->display();
}

char start() {
    board = new Board(M, N);
    Player *player = new Player(new Point(), character);
    if (character == 'X')
        move(player);
    else
        board->display();
    do {
        if (!wait(rival_move, "Ожидание хода противника...")) {
            cout << "Потеряна связь с противником." << endl;
            return 'T';
        }
        if (board->isTerminal())
            break;
        player = new Player(new Point(), character);
        move(player);
        board->load();

    } while (!board->isTerminal());
    return board->win(character);
}

void terminate(int s) {
    stringstream ss;
    ss << endl << "Получен сигнал " << s << ".\nОжидание закрытия игровой сессии...";
    wait(close_session, ss.str());
    exit(1);
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    struct sigaction handler;
    handler.sa_handler = terminate;
    sigemptyset(&handler.sa_mask);
    handler.sa_flags = 0;
    sigaction(SIGINT, &handler, nullptr);

    const string s = "Ожидание закрытия игровой сессии...";
    if (!init_session(get_ip())) {
        cout << "Не удалось инициализировать игровую сессию." << endl;
        wait(close_session, s);
        return 0;
    }

    if (!wait(check_session, "Ожидание соединения игроков...")) {
        cout << "Нет соединения." << endl;
        return 0;
    }

    cout << "Вы - " << character << endl;
    char done = start();
    if (done == 'T') {
        wait(close_session, s);
        return 0;
    }
    if(done == character)
        cout << "Вы победили." << endl;
    else if (board->full())
        cout << "Ничья." << endl;
    else
        cout << "Вы проиграли." << endl;
    wait(close_session, s);
    return 0;
}
