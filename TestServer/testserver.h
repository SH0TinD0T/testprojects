#ifndef TESTSERVER_H
#define TESTSERVER_H

#include <QString>
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QXmlStreamReader>
#include <QString>
#include <QTimer>
#include <QMap>


class TestServer : public QObject
{
    Q_OBJECT

public:
    explicit TestServer(QObject *parent = nullptr);
    ~TestServer();
    void startServer(quint16 port);

    static const QString XML_FOLDER_PATH;

private slots:
    void onNewConnection();
    void onReadyRead();
    void onDisconnected();
    void onTimeout();

private:
    void initializeDatabase();
    void readXmlFilesAndPopulateDatabase(const QString &folderPath);
    void parseXmlAndStoreData(QXmlStreamReader &xml);
    void saveJsonToDatabase(int blockId, const QString &jsonString);
    QString getJsonDataFromDatabase();

    QTcpServer *server;
    QTcpSocket *clientSocket;
    QSqlDatabase db;
    QTimer *timer;
    QMap<QString, QDateTime> fileModificationTimes;
};

#endif // TESTSERVER_H
