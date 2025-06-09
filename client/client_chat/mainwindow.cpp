#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QDebug"
#include <QSql>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlQueryModel>
#include <QSqlRecord>
#include "ui_logindialog.h"
#include "qmessagebox.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    QWidget::setFixedSize(800, 600);
    ui->setupUi(this);
    db = QSqlDatabase::addDatabase("QPSQL");
    db_connect(db);
}

MainWindow::~MainWindow()
{
    db.close();
    delete ui;
}

void MainWindow::db_connect(QSqlDatabase &db)
{
    QString dbName = "application";
    QString hostName = "localhost";
    int port = 5432;
    QString dbuserName = "postgres";
    QString password = "root";
    db.setDatabaseName(dbName);
    db.setHostName    (hostName);
    db.setPort        (port   );
    db.setUserName    (dbuserName);
    db.setPassword    (password);
}

void MainWindow::on_enter_button_clicked()
{
    if(OpenDialog.exec()==QDialog::Accepted){
        qDebug("accepted");

        if(db.open()){
            qDebug("db opened success");
            QString userName;
            QString userPassword;
            OpenDialog.getConnectionParameters(userName, userPassword);

            //авторизация

            QSqlQuery query(db);
            //QString current_query = "SELECT COUNT(*) FROM users WHERE name = \'" + userName + "\' AND password = \'" + userPassword + "\';";
            query.prepare("SELECT COUNT(*) FROM users WHERE name = ? AND password = ?");
            query.addBindValue(userName);
            query.addBindValue(userPassword);
            if(!query.exec()){
                qDebug() << "Query error! " << query.lastError();
            }
            else {
                query.first();
                int key = query.value(0).toInt();
                if(!key) {
                    qDebug()<<"authorization failed";
                    QMessageBox::warning(this, "Авторизация", "Неверное имя пользователя или пароль.\nЗарегистрироваться?");
                    this->on_enter_button_clicked();
                }
                else{
                    qDebug()<<"authorization success";
                    QMessageBox::information(this, "Авторизация", "Успешная авторизация!");
                    Chat.setDB(db, userName);
                    Chat.show(); //открываем чат
                    this->close();
                }
            }
        }
        else {
            qDebug()<< "error opening db: " << db.lastError().text();
            QMessageBox::warning(this, "Ошибка", "Не удалось получить доступ к базе данных!");
            this->close();
        }
    }
    else{
        OpenDialog.close();
    }
}

void MainWindow::on_register_button_clicked()
{
    if(RegDialog.exec()==QDialog::Accepted){
        qDebug() << "reg accepted";
        if(db.open()){
            qDebug("db is opened");
            QString userName;
            QString userPassword;
            RegDialog.getConnectionParameters(userName, userPassword);

            if (userName == "" || userName == " "){
                QMessageBox::warning(this, "Пустое имя", "Поле \'Имя\' не должно быть пустым");
                RegDialog.close();
                return;
                //this->on_register_button_clicked();
            }

            if (userPassword == "" || userPassword == " "){
                QMessageBox::warning(this, "Пустой пароль", "Поле \'Пароль\' не должно быть пустым");
                RegDialog.close();
                return;
                //this->on_register_button_clicked();
            }


            //регистрация

            QSqlQuery query;
            query.prepare("INSERT INTO users (name, password) VALUES (?, ?)");
            query.addBindValue(userName);
            query.addBindValue(userPassword);
            if(!query.exec()){
                qDebug() << "Query error: " << query.lastError();
                QMessageBox::warning(this, "Ошибка", "Пользователь с таким именем уже существует!");
                RegDialog.close();
            }
            else{
                QMessageBox::information(this, "Успех", "Регистрация прошла успешно!");
                RegDialog.close();
            }

        }
        else{
            qDebug()<< "error opening db: " << db.lastError().text();
            QMessageBox::warning(this, "Ошибка", "Не удалось получить доступ к базе данных!");
            this->close();
        }
    }
    else{
        RegDialog.close();
    }
}
