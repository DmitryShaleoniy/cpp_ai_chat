#ifndef CHAT_WIDGET_H
#define CHAT_WIDGET_H

#include <QMainWindow>
#include "QSqlDatabase"
#include <QDebug>
#include <QString>
#include <QSqlError>
#include <QStringListModel>
#include <QSqlQueryModel>
#include <QSqlQuery>
#include <QListWidget>
#include <QPixmap>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QGraphicsDropShadowEffect>

#include <sys/socket.h> //для сокетов
#include <netinet/in.h> //для связи с интернетом (тут определены заголовки IPv4 и IPv6)
#include <unistd.h> //для связи с POSIX операционными системами
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <iostream>
#include <arpa/inet.h>
#include <thread>
#include <mutex>
#include <QCloseEvent>

#include<QThread>

#include "receiverthread.h"
#include "sendthread.h"

#include <QMessageBox>

namespace Ui {
class chat_widget;
}

class chat_widget : public QMainWindow
{
    Q_OBJECT

public:
    explicit chat_widget(QWidget *parent = nullptr);
    void setDB(QSqlDatabase& datbase, QString username);
    ~chat_widget();

private slots:
    void on_usersView_doubleClicked(const QModelIndex &index);

    void handleNewMessage(const QString &message);

    void handleSendError(const QString &error);
    void handleConnectionClosed();

    void on_sendButton_clicked();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void receiveMessages();
    Ui::chat_widget *ui;
    QString current_user;
    QString current_dialog;
    QSqlDatabase db;
    QSqlQueryModel model;
    ReceiverThread* receiverThread;
    SendThread* senderThread;
    void closeConnection();

    //chat_widget(QCloseEvent *event);

    char ip[15] = "127.0.0.1";
    u_short port = 1234;

    int connect_soc;

    void update_users();
};

#endif // CHAT_WIDGET_H
