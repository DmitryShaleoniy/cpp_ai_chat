#ifndef REGISTERDIALOG_H
#define REGISTERDIALOG_H

#include <QDialog>

namespace Ui {
class registerdialog;
}

class registerdialog : public QDialog
{
    Q_OBJECT

public:
    explicit registerdialog(QWidget *parent = nullptr);
    void getConnectionParameters(QString& name, QString& password);
    ~registerdialog();

private:
    Ui::registerdialog *ui;
};

#endif // REGISTERDIALOG_H
