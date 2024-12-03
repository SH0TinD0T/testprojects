#include <QCoreApplication>
#include "testserver.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    TestServer server;
    server.startServer(12345);
    return a.exec();
}
