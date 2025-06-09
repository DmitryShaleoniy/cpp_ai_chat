#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>

namespace Ui {
class LogInDialog;
}

class LogInDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LogInDialog(QWidget *parent = nullptr);
    void getConnectionParameters(QString& name, QString& password);
    ~LogInDialog();

private:
    Ui::LogInDialog *ui;
};

#endif // LOGINDIALOG_H
