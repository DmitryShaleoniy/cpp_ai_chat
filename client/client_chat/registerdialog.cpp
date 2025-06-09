#include "registerdialog.h"
#include "ui_registerdialog.h"

registerdialog::registerdialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::registerdialog)
{
    QWidget::setFixedSize(365, 112);
    ui->setupUi(this);
}

void registerdialog::getConnectionParameters(QString &name, QString &password)
{
    name = ui->nameLine->text();
    password = ui->passwordLine->text();
}

registerdialog::~registerdialog()
{
    delete ui;
}
