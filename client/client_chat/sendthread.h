#ifndef SENDTHREAD_H
#define SENDTHREAD_H

#include <QThread>
#include <QMutex>
#include <QQueue>
#include <sys/socket.h>
#include <QWaitCondition>
#include <QDebug>
#include <QString>

class SendThread : public QThread
{
    Q_OBJECT
public:
    explicit SendThread(int socketDescriptor, QObject *parent = nullptr)
        : QThread(parent), m_socket(socketDescriptor), m_running(true) {}

    void stop() {
        QMutexLocker locker(&m_mutex);
        m_running = false;
        m_condition.wakeOne(); // Wake up thread to exit
    }

    void sendMessage(const QString &message) {
        QMutexLocker locker(&m_mutex);
        QString m_new = message;
        qDebug()<< m_new << " " << m_new.size();
        if (!m_messageQueue.isEmpty()) {
            m_messageQueue.dequeue();  // Удаляем старое сообщение, если есть
        }
        //std::cout<<message<<std::endl;
        //qDebug() << message;
        m_new.resize(1024, '\0');
        qDebug()<< m_new << " " << m_new.size();

        m_messageQueue.enqueue(m_new);
        m_condition.wakeOne(); // Wake up thread to process new message
    }

signals:
    void sendError(const QString &error);

protected:
    void run() override {
        while(true) {
            QMutexLocker locker(&m_mutex);
            while(m_messageQueue.isEmpty() && m_running) {
                m_condition.wait(&m_mutex);
            }

            if(!m_running) break;

            QString message = m_messageQueue.dequeue();
            locker.unlock();

            QByteArray data = message.toUtf8();
            ssize_t bytesSent = ::send(m_socket, data.constData(), data.size(), 0);

            if(bytesSent <= 0) {
                emit sendError("Failed to send message");
                break;
            }
        }
    }

private:
    int m_socket;
    bool m_running;
    QMutex m_mutex;
    QWaitCondition m_condition;
    QQueue<QString> m_messageQueue;
};

#endif // SENDTHREAD_H
