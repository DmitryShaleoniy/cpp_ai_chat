#ifndef RECEIVERTHREAD_H
#define RECEIVERTHREAD_H

#include <QThread>
#include <QMutex>
#include <sys/socket.h>

class ReceiverThread : public QThread
{
    Q_OBJECT
public:
    explicit ReceiverThread(int socketDescriptor, QObject *parent = nullptr)
        : QThread(parent), m_socket(socketDescriptor), m_running(true) {}

    void stop() {
        QMutexLocker locker(&m_mutex);
        m_running = false;
    }

signals:
    void newMessage(const QString &message);
    void connectionClosed();

protected:
    void run() override {
        char buffer[1024];
        while(true) {
            {
                QMutexLocker locker(&m_mutex);
                if(!m_running) break;
            }

            ssize_t bytesReceived = recv(m_socket, buffer, sizeof(buffer), 0);

            if (bytesReceived <= 0) {
                emit connectionClosed();
                break;
            }

            buffer[bytesReceived] = '\0';
            emit newMessage(QString::fromUtf8(buffer, bytesReceived));
        }
    }

private:
    int m_socket;
    bool m_running;
    QMutex m_mutex;
};

#endif // RECEIVERTHREAD_H
