#include "logindialog.h"
#include "ui_logindialog.h"

LogInDialog::LogInDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::LogInDialog)
{
    QWidget::setFixedSize(365, 112);
    ui->setupUi(this);
}

void LogInDialog::getConnectionParameters(QString& name, QString& password)
{
    name = ui->nameLine->text();
    password = ui->passwordLine->text();
}

LogInDialog::~LogInDialog()
{
    delete ui;
}
