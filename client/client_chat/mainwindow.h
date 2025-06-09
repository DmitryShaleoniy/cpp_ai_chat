#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "logindialog.h"
#include "registerdialog.h"
#include "chat_widget.h"
#include <QSqlDatabase>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_enter_button_clicked();

    void on_register_button_clicked();

private:
    Ui::MainWindow *ui;
    LogInDialog OpenDialog;
    registerdialog RegDialog;
    chat_widget Chat;
    QSqlDatabase db;

    void db_connect(QSqlDatabase& db);
};
#endif // MAINWINDOW_H
