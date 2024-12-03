#include "testserver.h"
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QXmlStreamReader>
#include <QDebug>
#include <QTimer>

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

const QString TestServer::XML_FOLDER_PATH = "C:/Users/Gregory/Documents/TestProjects/xml";


TestServer::TestServer(QObject *parent)
    : QObject(parent),
    server(new QTcpServer(this)),
    clientSocket(nullptr),
    timer(new QTimer(this))
{
    connect(server, &QTcpServer::newConnection, this, &TestServer::onNewConnection);
    initializeDatabase();

    connect(timer, &QTimer::timeout, this, &TestServer::onTimeout);
    timer->start(60000);
}

TestServer::~TestServer()
{
    if (clientSocket) clientSocket->disconnectFromHost();
    db.close();
}

void TestServer::startServer(quint16 port)
{
    if (server->listen(QHostAddress::Any, port)) {
        qDebug() << "Сервер запущен на порте" << port;
        readXmlFilesAndPopulateDatabase(XML_FOLDER_PATH);
    }
    else qDebug() << "Не удалось запустить сервер";
}

void TestServer::onNewConnection()
{
    clientSocket = server->nextPendingConnection();
    connect(clientSocket, &QTcpSocket::readyRead, this, &TestServer::onReadyRead);
    connect(clientSocket, &QTcpSocket::disconnected, this, &TestServer::onDisconnected);

    qDebug() << "Клиент подключился";
}

void TestServer::onReadyRead()
{
    QByteArray data = clientSocket->readAll();

    if (data == "GET_DATA") {
        clientSocket->write(getJsonDataFromDatabase().toUtf8());
        clientSocket->flush();
        qDebug() << "Данные отправлены клиенту.";
    } else qDebug() << "Неизвестный запрос от клиента:" << data;
}

void TestServer::onDisconnected()
{
    qDebug() << "Клиент отключился";
    clientSocket->deleteLater();
    clientSocket = nullptr;
}

void TestServer::initializeDatabase()
{
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("server_data.db");

    if (!db.open()) {
        qDebug() << "Не удалось открыть базу данных:" << db.lastError().text();
        return;
    }

    QSqlQuery query;

    query.exec("CREATE TABLE IF NOT EXISTS blocks ("
               "block_id INTEGER PRIMARY KEY, "
               "data_json TEXT)");

    if (query.lastError().isValid()) qDebug() << "Ошибка создания таблицы:" << query.lastError().text();
}

void TestServer::readXmlFilesAndPopulateDatabase(const QString &folderPath)
{
    QDir directory(folderPath);
    QStringList xmlFiles = directory.entryList(QStringList() << "*.xml", QDir::Files);

    for (const QString &fileName : xmlFiles) {
        QFile file(directory.filePath(fileName));
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qDebug() << "Не удалось открыть файл:" << fileName;
            continue;
        }

        QXmlStreamReader xml(&file);
        parseXmlAndStoreData(xml);
        file.close();
    }
}

void TestServer::parseXmlAndStoreData(QXmlStreamReader &xml)
{
    QJsonObject rootObject;
    QJsonArray blocksArray;

    while (!xml.atEnd() && !xml.hasError()) {
        xml.readNext();

        if (xml.isStartElement()) {
            if (xml.name() == "block") {
                QJsonObject blockObject;
                blockObject["block_id"] = xml.attributes().value("id").toInt();
                rootObject["block_id"] = blockObject["block_id"];
                blockObject["name"] = xml.attributes().value("Name").toString();
                blockObject["ip"] = xml.attributes().value("IP").toString();
                blockObject["board_count"] = xml.attributes().value("BoardCount").toInt();
                blockObject["mtr"] = xml.attributes().value("MtR").toInt();
                blockObject["mtc"] = xml.attributes().value("MtC").toInt();
                blockObject["description"] = xml.attributes().value("Description").toString();
                blockObject["label"] = xml.attributes().value("Label").toString();

                QJsonArray boardsArray;

                while (!(xml.isEndElement() && xml.name() == "block")) {
                    xml.readNext();

                    if (xml.isStartElement() && xml.name() == "board") {
                        QJsonObject boardObject;
                        boardObject["id"] = xml.attributes().value("id").toInt();
                        boardObject["num"] = xml.attributes().value("Num").toInt();
                        boardObject["name"] = xml.attributes().value("Name").toString();
                        boardObject["port_count"] = xml.attributes().value("PortCount").toInt();
                        boardObject["int_links"] = xml.attributes().value("IntLinks").toString();
                        boardObject["algorithms"] = xml.attributes().value("Algoritms").toString();

                        QJsonArray portsArray;

                        while (!(xml.isEndElement() && xml.name() == "board")) {
                            xml.readNext();

                            if (xml.isStartElement() && xml.name() == "port") {
                                QJsonObject portObject;
                                portObject["id"] = xml.attributes().value("id").toInt();
                                portObject["num"] = xml.attributes().value("Num").toInt();
                                portObject["media"] = xml.attributes().value("Media").toInt();
                                portObject["signal"] = xml.attributes().value("Signal").toInt();
                                portsArray.append(portObject);
                            }
                        }

                        boardObject["ports"] = portsArray;
                        boardsArray.append(boardObject);
                    }
                }

                blockObject["boards"] = boardsArray;
                blocksArray.append(blockObject);
            }
        }
    }

    if (xml.hasError()) qDebug() << "Ошибка парсинга XML:" << xml.errorString();

    rootObject["blocks"] = blocksArray;

    QJsonDocument doc(rootObject);
    QString jsonString = doc.toJson(QJsonDocument::Indented);

    saveJsonToDatabase(rootObject["block_id"].toInt(), jsonString);
}

void TestServer::saveJsonToDatabase(int blockId, const QString &jsonString)
{
    QSqlQuery query;
    query.prepare("INSERT OR REPLACE INTO blocks (block_id, data_json) VALUES (:block_id, :data_json)");
    query.bindValue(":block_id", blockId);
    query.bindValue(":data_json", jsonString);

    if (!query.exec()) qDebug() << "Ошибка вставки JSON в базу данных:" << query.lastError().text();
    else qDebug() << "JSON успешно вставлен для блока с ID:" << blockId;
}

QString TestServer::getJsonDataFromDatabase()
{
    QSqlQuery query;
    query.exec("SELECT block_id, data_json FROM blocks");

    if (query.lastError().isValid()) {
        qDebug() << "Ошибка выполнения запроса:" << query.lastError().text();
        return "{\"error\": \"Ошибка при извлечении данных.\"}";
    }

    QJsonArray blocksArray;

    while (query.next()) {
        int blockId = query.value("block_id").toInt();
        QString jsonString = query.value("data_json").toString();

        QJsonObject blockObject;
        blockObject["block_id"] = blockId;
        blockObject["data_json"] = QJsonDocument::fromJson(jsonString.toUtf8()).object();

        blocksArray.append(blockObject);
    }

    QJsonObject responseObject;
    responseObject["blocks"] = blocksArray;

    QJsonDocument responseDoc(responseObject);
    QString responseString = responseDoc.toJson(QJsonDocument::Indented);

    return responseString;
}

void TestServer::onTimeout()
{
    qDebug() << "Проверка изменений в XML файлах...";

    QDir directory(XML_FOLDER_PATH);
    QStringList xmlFiles = directory.entryList(QStringList() << "*.xml", QDir::Files);

    for (const QString &fileName : xmlFiles) {
        QFileInfo fileInfo(directory.filePath(fileName));
        QDateTime lastModified = fileInfo.lastModified();

        if (fileModificationTimes.contains(fileName)) {
            if (fileModificationTimes[fileName] != lastModified) {
                qDebug() << "Изменения в файле:" << fileName;

                QFile file(directory.filePath(fileName));
                if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                    QXmlStreamReader xml(&file);
                    parseXmlAndStoreData(xml);
                    file.close();
                }

                fileModificationTimes[fileName] = lastModified;
            }
        } else fileModificationTimes[fileName] = lastModified;
    }
}
