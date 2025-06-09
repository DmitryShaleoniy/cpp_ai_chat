#include "chat_widget.h"
#include "ui_chat_widget.h"
#include <QScrollBar>

chat_widget::chat_widget(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::chat_widget)
    , receiverThread(nullptr)
    , senderThread(nullptr)
    , connect_soc(-1)
{
    //QWidget::setFixedSize(915, 621);

    ui->setupUi(this);
    ui->messagesWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

    //update_users();

    //создаём сокет

    //connect_soc = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);


}

void chat_widget::handleNewMessage(const QString &message)
{
    // QListWidgetItem* item = new QListWidgetItem();
    // item->setText(message);
    // item->setTextAlignment(Qt::AlignLeft);
    // item->setFont(QFont("Arial", 14));

    // ui->messagesWidget->setSpacing(5);
    // ui->messagesWidget->addItem(item);
    // ui->messagesWidget->scrollToBottom();
    if(message=="u"){
        QString dialog_name;
        //current_dialog = dialog_name;
        dialog_name = current_dialog;

        ui->messagesWidget->clear();
        QSqlQuery query(db);

        query.prepare("UPDATE users SET in_dialog = ? WHERE name = ?");
        query.addBindValue(dialog_name);
        query.addBindValue(current_user);
        query.exec();

        query.prepare("SELECT sender, type, content FROM messages WHERE (sender = ? AND reciever = ?) OR (sender = ? AND reciever = ?)");
        query.addBindValue(current_user);
        query.addBindValue(dialog_name);
        query.addBindValue(dialog_name);
        query.addBindValue(current_user);
        if(!query.exec()){
            qDebug() << "Query error! " << query.lastError();
        }
        else {
            QString current_sender;
            QString current_type;
            QString current_text;
            while(query.next()){
                current_sender = query.value(0).toString();
                current_type = query.value(1).toString();
                current_text = query.value(2).toString();
                if (current_type == "text") {
                    QListWidgetItem* item = new QListWidgetItem();
                    item->setText(current_text);
                    item->setTextAlignment(current_sender == current_user ? Qt::AlignRight : Qt::AlignLeft);
                    if (current_sender == current_user) {
                        QColor color("#1D334A");
                        color.setAlpha(150);
                        item->setBackground(color);
                    }
                    //item->setBackground(current_sender == current_user ? QColor("#002137") : QColor(1));
                    item->setFont(QFont("Arial", 14));
                    ui->messagesWidget->setSpacing(5);
                    ui->messagesWidget->addItem(item);

                    //ui->messagesWidget->addItem(item);
                    //delete item;
                }
                else if (current_type == "img") {
                    QSqlQuery img_query(db);
                    img_query.prepare("SELECT image_data FROM images WHERE NAME = ?");
                    img_query.addBindValue(current_text);
                    if(!img_query.exec()){
                        qDebug() << "Query error! " << img_query.lastError();
                    }
                    if(img_query.next()){
                        QByteArray image_data = img_query.value(0).toByteArray();
                        QPixmap pixmap;
                        pixmap.loadFromData(image_data);
                        QListWidgetItem* item= new QListWidgetItem();
                        QLabel* label = new QLabel();
                        //label->setPixmap(pixmap.scaled(200,200,Qt::KeepAspectRatio));
                        //item->setSizeHint(label->sizeHint());

                        // Применяем скругленные углы
                        QPixmap rounded(pixmap.size());
                        rounded.fill(Qt::transparent);

                        QPainter painter(&rounded);
                        painter.setRenderHint(QPainter::Antialiasing);
                        QPainterPath path;
                        path.addRoundedRect(0, 0, pixmap.width(), pixmap.height(), 100, 100);
                        painter.setClipPath(path);
                        painter.drawPixmap(0, 0, pixmap);

                        label->setPixmap(rounded.scaled(300,300,Qt::KeepAspectRatio));
                        item->setSizeHint(label->sizeHint());

                        // Добавляем тень
                        QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect();
                        shadow->setBlurRadius(10);
                        shadow->setColor(QColor(0, 0, 0, 70));
                        shadow->setOffset(3, 3);
                        label->setGraphicsEffect(shadow);

                        // Создаем контейнер для выравнивания
                        QWidget* container = new QWidget();
                        QHBoxLayout* layout = new QHBoxLayout(container);
                        layout->setContentsMargins(0, 0, 0, 0);

                        if(current_sender == current_user) {
                            // Выравниваем справа для текущего пользователя
                            layout->addStretch();
                            layout->addWidget(label);
                            item->setTextAlignment(Qt::AlignRight);
                        } else {
                            // Выравниваем слева для других отправителей
                            layout->addWidget(label);
                            layout->addStretch();
                            item->setTextAlignment(Qt::AlignLeft);
                        }

                        if (current_sender == current_user) {
                            QColor color("#1D334A");
                            color.setAlpha(120);
                            item->setBackground(color);
                        }

                        ui->messagesWidget->setSpacing(5);
                        ui->messagesWidget->addItem(item);
                        ui->messagesWidget->setItemWidget(item, container);
                    }
                }
            }
            ui->messagesWidget->scrollToBottom();
        }
    }
}

void chat_widget::handleConnectionClosed()
{
    QMessageBox::warning(this, "Connection closed", "The server has disconnected");
    closeConnection();
}

void chat_widget::closeConnection()
{
    if(receiverThread) {
        receiverThread->stop();
        receiverThread->wait();
        delete receiverThread;
        receiverThread = nullptr;
    }

    if(senderThread) {
        senderThread->stop();
        senderThread->wait();
        delete senderThread;
        senderThread = nullptr;
    }

    if(connect_soc != -1) {
        ::close(connect_soc);
        connect_soc = -1;
    }
}

void chat_widget::receiveMessages()
{
    char buffer[1024];
    while(true){
        ssize_t bytesReceived = recv(connect_soc, buffer, sizeof(buffer), 0);
        if (bytesReceived <= 0) {
            std::cerr << "Server disconnected." << std::endl;
            break;
        }
        buffer[bytesReceived] = '\0';
        QString message(buffer);
        QListWidgetItem* item = new QListWidgetItem();
        item->setText(message); //сервер отправит нам напрямую, только если юзер находится в диалоге с отправителем
        item->setTextAlignment(Qt::AlignLeft);

        //item->setBackground(current_sender == current_user ? QColor("#002137") : QColor(1));
        item->setFont(QFont("Arial", 14));
        std::mutex m;
        m.lock();
        ui->messagesWidget->setSpacing(5);
        ui->messagesWidget->insertItem(0, item);
        m.unlock();
        //std::cout << "Server: " << buffer << std::endl;
    }
}

void chat_widget::setDB(QSqlDatabase &datbase, QString username)
{
    current_user = username;
    db = datbase;
    if(db.isOpen()){
        qDebug() << "db success";

        QSqlQuery query(db);

        query.prepare("UPDATE users SET in_dialog = null WHERE name = ?");
        query.addBindValue(current_user);
        if(!query.exec()){
            qDebug() << "last query error: " << query.lastError();
        }

        model.setQuery("SELECT name FROM users WHERE name <> \'" + current_user + "\'", db);
        ui->usersView->setModel(&model);
        //QHeaderView *header = ui->usersView->horizontalHeader();
        ui->usersView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    }
    else {
        qDebug() << "chat db opening error: " << db.lastError();
        return;
    }

    if((connect_soc = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP))<0){
        perror("failed to create client socket!");
        exit(EXIT_FAILURE);
    }
    else {
        std::cout<<"socket created"<<std::endl;
    }

    sockaddr_in client; //сюда получаем параметры

    client.sin_family = AF_INET; //семейство адресов, которые будем принимать (IPv4)
    client.sin_addr.s_addr = inet_addr(ip);
    client.sin_port = htons(1234); //здесь задали порт

    std::cout<< "connecting to server..." << std::endl;
    if(::connect(connect_soc, (sockaddr*)&client, sizeof(client))<0){
        std::cout<<"epic fail"<<std::endl;
    }

    // std::thread recieve_thread([this]() { this->receiveMessages(); });
    // receive_thread.detach();

    // Создаем поток для приема сообщений
    receiverThread = new ReceiverThread(connect_soc);

    // Подключаем сигналы
    connect(receiverThread, &ReceiverThread::newMessage,
            this, &chat_widget::handleNewMessage);
    connect(receiverThread, &ReceiverThread::connectionClosed,
            this, &chat_widget::handleConnectionClosed);

    // Запускаем поток
    receiverThread->start();

    senderThread = new SendThread(connect_soc);

    // Подключаем сигналы
    connect(senderThread, &SendThread::sendError,
            this, &chat_widget::handleSendError);

    // Запускаем поток
    senderThread->start();

    senderThread->sendMessage(username);
}

void chat_widget::handleSendError(const QString &error)
{
    QMessageBox::warning(this, "Send Error", error);
    closeConnection();
}

void chat_widget::update_users()
{
    model.setQuery("SELECT name FROM users WHERE name <> \'" + current_user + "\'", db);
    ui->usersView->setModel(&model);
}

void chat_widget::closeEvent(QCloseEvent *event) {
    QSqlQuery query(db);

    query.prepare("UPDATE users SET in_dialog = null WHERE name = ?");
    query.addBindValue(current_user);
    if(!query.exec()){
        qDebug() << "last query error: " << query.lastError();
    }

    query.prepare("UPDATE users SET id = null WHERE name = ?");
    query.addBindValue(current_user);
    if(!query.exec()){
        qDebug() << "last query error: " << query.lastError();
    }
    //close(connect_soc);
    event->accept();
}

chat_widget::~chat_widget()
{
    delete ui;
}

void chat_widget::on_usersView_doubleClicked(const QModelIndex &index) //тут подгружаем историю чата
{

    QString dialog_name = index.data().toString();
    current_dialog = dialog_name;


    if (dialog_name.isEmpty()){
        return;
    }
    ui->messagesWidget->clear();
    QSqlQuery query(db);

    query.prepare("UPDATE users SET in_dialog = ? WHERE name = ?");
    query.addBindValue(dialog_name);
    query.addBindValue(current_user);
    query.exec();

    query.prepare("SELECT sender, type, content FROM messages WHERE (sender = ? AND reciever = ?) OR (sender = ? AND reciever = ?)");
    query.addBindValue(current_user);
    query.addBindValue(dialog_name);
    query.addBindValue(dialog_name);
    query.addBindValue(current_user);
    if(!query.exec()){
        qDebug() << "Query error! " << query.lastError();
    }
    else {
        QString current_sender;
        QString current_type;
        QString current_text;
        while(query.next()){
            current_sender = query.value(0).toString();
            current_type = query.value(1).toString();
            current_text = query.value(2).toString();
            if (current_type == "text") {
                QListWidgetItem* item = new QListWidgetItem();
                item->setText(current_text);
                item->setTextAlignment(current_sender == current_user ? Qt::AlignRight : Qt::AlignLeft);
                if (current_sender == current_user) {
                    QColor color("#1D334A");
                    color.setAlpha(150);
                    item->setBackground(color);
                }
                //item->setBackground(current_sender == current_user ? QColor("#002137") : QColor(1));
                item->setFont(QFont("Arial", 14));
                ui->messagesWidget->setSpacing(5);
                ui->messagesWidget->addItem(item);

                //ui->messagesWidget->addItem(item);
                //delete item;
            }
            else if (current_type == "img") {
                QSqlQuery img_query(db);
                img_query.prepare("SELECT image_data FROM images WHERE NAME = ?");
                img_query.addBindValue(current_text);
                if(!img_query.exec()){
                    qDebug() << "Query error! " << img_query.lastError();
                }
                if(img_query.next()){
                    QByteArray image_data = img_query.value(0).toByteArray();
                    QPixmap pixmap;
                    pixmap.loadFromData(image_data);
                    QListWidgetItem* item= new QListWidgetItem();
                    QLabel* label = new QLabel();
                    //label->setPixmap(pixmap.scaled(200,200,Qt::KeepAspectRatio));
                    //item->setSizeHint(label->sizeHint());

                    // Применяем скругленные углы
                    QPixmap rounded(pixmap.size());
                    rounded.fill(Qt::transparent);

                    QPainter painter(&rounded);
                    painter.setRenderHint(QPainter::Antialiasing);
                    QPainterPath path;
                    path.addRoundedRect(0, 0, pixmap.width(), pixmap.height(), 100, 100);
                    painter.setClipPath(path);
                    painter.drawPixmap(0, 0, pixmap);

                    label->setPixmap(rounded.scaled(300,300,Qt::KeepAspectRatio));
                    item->setSizeHint(label->sizeHint());

                    // Добавляем тень
                    QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect();
                    shadow->setBlurRadius(10);
                    shadow->setColor(QColor(0, 0, 0, 70));
                    shadow->setOffset(3, 3);
                    label->setGraphicsEffect(shadow);

                    // Создаем контейнер для выравнивания
                    QWidget* container = new QWidget();
                    QHBoxLayout* layout = new QHBoxLayout(container);
                    layout->setContentsMargins(0, 0, 0, 0);

                    if(current_sender == current_user) {
                        // Выравниваем справа для текущего пользователя
                        layout->addStretch();
                        layout->addWidget(label);
                        item->setTextAlignment(Qt::AlignRight);
                    } else {
                        // Выравниваем слева для других отправителей
                        layout->addWidget(label);
                        layout->addStretch();
                        item->setTextAlignment(Qt::AlignLeft);
                    }

                    if (current_sender == current_user) {
                        QColor color("#1D334A");
                        color.setAlpha(120);
                        item->setBackground(color);
                    }

                    ui->messagesWidget->setSpacing(5);
                    ui->messagesWidget->addItem(item);
                    ui->messagesWidget->setItemWidget(item, container);
                }
            }
        }
        ui->messagesWidget->scrollToBottom();
    }
}



void chat_widget::on_sendButton_clicked() //отправка сообщения
{
    QString content = ui->messageEdit->text();
    ui->messageEdit->clear();
    if (content[0] != '*'){
        QListWidgetItem* item = new QListWidgetItem();
        item->setText(content);
        item->setTextAlignment(Qt::AlignRight);

        QColor color("#1D334A");
        color.setAlpha(150);
        item->setBackground(color);

        //item->setBackground(current_sender == current_user ? QColor("#002137") : QColor(1));
        item->setFont(QFont("Arial", 14));
        ui->messagesWidget->setSpacing(5);
        ui->messagesWidget->addItem(item); //добавили от своего имени
        ui->messagesWidget->scrollToBottom();

        //senderThread->sendMessage(content);

        //QString currnt_dialog;

        QSqlQuery query(db);

        query.prepare("INSERT INTO messages (sender, reciever, type, content) VALUES (?, ?, \'text\', ?)");
        query.addBindValue(current_user);
        query.addBindValue(current_dialog);
        //query.addBindValue('text');
        query.addBindValue(content);

        if(!query.exec()){
            qDebug() << "Query error! " << query.lastError();
            QMessageBox::warning(this, "Внимение!", "Сообщение не доставлено, проблемы с подключением");
        }

        // query.prepare("SELECT in_dialog FROM users WHERE name = ?");
        // query.addBindValue(current_user);
        // if(!query.exec()){
        //     qDebug() << "last query error: " << query.lastError();
        // }

        query.prepare("SELECT in_dialog FROM users WHERE name = ?");
        query.addBindValue(current_dialog);
        if(!query.exec()){
            qDebug() << "Query error! " << query.lastError();
            QMessageBox::warning(this, "Внимение", "Сообщение не доставлено, проблемы с подключением");
        }
        else {
            if(query.next()){
                QString dialog_dialog = query.value(0).toString();
                if(dialog_dialog == current_user){
                    senderThread->sendMessage(current_dialog);//отправили серваку имя юзера, которому нужно обновить базу данных
                }
            }
        }
    }
    else { //значит у нас тут промпт... пу пу пу...
        std::cout<<"starting getting image"<<std::endl;
        QListWidgetItem* item = new QListWidgetItem();
        item->setText("Генерирую картинку...");
        item->setTextAlignment(Qt::AlignRight);

        QColor color("#1D334A");
        color.setAlpha(150);
        item->setBackground(color);

        //item->setBackground(current_sender == current_user ? QColor("#002137") : QColor(1));
        item->setFont(QFont("Arial", 14));
        ui->messagesWidget->setSpacing(5);
        ui->messagesWidget->addItem(item); //добавили от своего имени
        ui->messagesWidget->scrollToBottom();

        QString sending = content + "&&" + current_user;
        std::cout<<"sending image"<<std::endl;
        senderThread->sendMessage(sending);
    }
}

