// myserver.h

#ifndef MYSERVER_H
#define MYSERVER_H

#include <QTcpServer>
#include "mythread.h"

class MyServer : public QTcpServer
{
    Q_OBJECT
public:
    explicit MyServer(QObject *parent = 0);
    void startServer();
    QString port;
signals:

public slots:

protected:
    void incomingConnection(qintptr socketDescriptor);

};

#endif // MYSERVER_H
