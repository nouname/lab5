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
static int M = 0, N = 0;

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
    return session.length() == 3 && session[0].split(SPACE)[0] != session[1].split(SPACE)[0];
}

bool rival_move() {
    QByteArray contents = response("move");
    return contents[0] != character && !contents.isEmpty();
}

bool init_session(QString ip) {
    character = rival_character() == 'X' ? 'O' : 'X';

    cout << ip.toStdString() << endl;
    QByteArray contents = response("init.php?ip=" + ip + SPACE + character + DELIMETER);
    return contents.isEmpty() && character != SPACE;
}

void move(Player* player) {
    int x = 0, y = 0;
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
    board->display();
    response("move.php?move=" + QString(character));
}

bool set_size() {
    if (character != 'X')
        return false;
    while (M < 1 || N < 1 || M > 10 || N > 10) {
        cout << "Введите M, N: ";
        cin >> M >> N;
        if (M < 1 || N < 1)
            cout << "Значения должны превышать число 0. Повторите ввод.\n";
        else if (M > 10 || N > 10)
            cout << "Значения не должны превышать число 10. Повторите ввод.\n";
    }
    return response("resize.php?size=" + QString::number(M) + DELIMETER + QString::number(N)).isEmpty();
}

bool get_size() {
    if (response("size") == nullptr)
        return false;
    QList<QByteArray> contents = response("size").split(DELIMETER);
    if (contents.isEmpty())
        return false;
    M = contents[0].toInt();
    N = contents[1].toInt();
    return true;
}

char start() {

    Player *player = new Player(new Point(), character);
    if (character == 'X') {
        set_size();
        board = new Board(M, N);
        move(player);
    }
    else {
        if (!wait(get_size, "Противник устанавливает размер игрового поля..."))
             cout << "Потеряна связь с противником." << endl;
        board = new Board(M, N);
        board->display();
    }
    do {
        board->load();
        if (board->isTerminal()) {
            board->display();
            break;
        }
        if (!wait(rival_move, "Ожидание хода противника...")) {
            cout << "Потеряна связь с противником." << endl;
            return 'T';
        }
        board->load();
        if (board->isTerminal()) {
            board->display();
            break;
        }
        player = new Player(new Point(), character);
        move(player);

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
    if (done == character)
        cout << "Вы победили." << endl;
    else if (done == SPACE)
        cout << "Вы проиграли." << endl;
    else
        cout << "Ничья." << endl;
    wait(close_session, s);
    return 0;
}
