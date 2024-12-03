#include "dialog.h"
#include "./ui_dialog.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardItem>
#include <QDebug>

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog),
    socket(new QTcpSocket(this)),
    model(new QStandardItemModel(this))
{
    ui->setupUi(this);

    ui->treeView->setModel(model);

    connect(socket, &QTcpSocket::connected, this, &Dialog::onConnected);
    connect(socket, &QTcpSocket::readyRead, this, &Dialog::onReadyRead);
    connect(socket, &QTcpSocket::errorOccurred, this, &Dialog::onError);

    updateConnectionStatus("Не подключено");
}

Dialog::~Dialog()
{
    delete ui;
}

void Dialog::on_pushButton_clicked()
{
    socket->connectToHost("127.0.0.1", 12345);
    updateConnectionStatus("Подключение...");
}

void Dialog::onConnected()
{
    updateConnectionStatus("Подключено");
    socket->write("GET_DATA");
}

void Dialog::onReadyRead()
{
    QByteArray response = socket->readAll();
    updateConnectionStatus("Данные получены");
    qDebug() << "Получены данные от сервера: " << response;

    // Парсим JSON-данные
    QJsonDocument doc = QJsonDocument::fromJson(response);
    if (doc.isObject()) {
        QJsonObject jsonObj = doc.object();
        parseJsonToTreeView(jsonObj);
    }
}

void Dialog::onError(QAbstractSocket::SocketError socketError)
{
    // Обработка ошибок при подключении
    updateConnectionStatus("Ошибка: " + socket->errorString());
}

void Dialog::updateConnectionStatus(const QString &status)
{
    // Обновление состояния подключения в label
    ui->label->setText(status);
}

// Функция для парсинга данных и заполнения QTreeView
void Dialog::parseJsonToTreeView(const QJsonObject &jsonObj)
{
    model->clear();

    if (jsonObj.contains("blocks")) {
        QJsonArray blocksArray = jsonObj["blocks"].toArray();

        for (int i = 0; i < blocksArray.size(); ++i) {
            QJsonObject blockObj = blocksArray[i].toObject();
            QString blockLabel = QString("Block ID: %1").arg(blockObj["block_id"].toInt());

            QStandardItem *blockItem = new QStandardItem(blockLabel);
            model->appendRow(blockItem);

            if (blockObj.contains("data_json")) {
                QJsonObject dataJson = blockObj["data_json"].toObject();

                if (dataJson.contains("description")) {
                    QString description = dataJson["description"].toString();
                    QStandardItem *descItem = new QStandardItem("Description: " + description);
                    blockItem->appendRow(descItem);
                }

                if (dataJson.contains("ip")) {
                    QString ip = dataJson["ip"].toString();
                    QStandardItem *ipItem = new QStandardItem("IP: " + ip);
                    blockItem->appendRow(ipItem);
                }

                if (dataJson.contains("label")) {
                    QString label = dataJson["label"].toString();
                    QStandardItem *labelItem = new QStandardItem("Label: " + label);
                    blockItem->appendRow(labelItem);
                }

                if (dataJson.contains("mtc") && dataJson.contains("mtr")) {
                    int mtc = dataJson["mtc"].toInt();
                    int mtr = dataJson["mtr"].toInt();
                    QStandardItem *mtcItem = new QStandardItem(QString("MTC: %1, MTR: %2").arg(mtc).arg(mtr));
                    blockItem->appendRow(mtcItem);
                }

                if (dataJson.contains("blocks")) {
                    QJsonArray innerBlocks = dataJson["blocks"].toArray();
                    for (int j = 0; j < innerBlocks.size(); ++j) {
                        QJsonObject innerBlockObj = innerBlocks[j].toObject();
                        QString innerBlockLabel = innerBlockObj["name"].toString();

                        QStandardItem *innerBlockItem = new QStandardItem("Block Name: " + innerBlockLabel);
                        blockItem->appendRow(innerBlockItem);

                        if (innerBlockObj.contains("algorithms")) {
                            QString algorithms = innerBlockObj["algorithms"].toString();
                            QStandardItem *algorithmsItem = new QStandardItem("Algorithms: " + algorithms);
                            innerBlockItem->appendRow(algorithmsItem);
                        }

                        if (innerBlockObj.contains("boards")) {
                            QJsonArray boardsArray = innerBlockObj["boards"].toArray();
                            for (int k = 0; k < boardsArray.size(); ++k) {
                                QJsonObject boardObj = boardsArray[k].toObject();
                                QString boardLabel = boardObj["name"].toString();

                                QStandardItem *boardItem = new QStandardItem("Board Name: " + boardLabel);
                                innerBlockItem->appendRow(boardItem);

                                if (boardObj.contains("num")) {
                                    int num = boardObj["num"].toInt();
                                    QStandardItem *numItem = new QStandardItem("Board Number: " + QString::number(num));
                                    boardItem->appendRow(numItem);
                                }

                                if (boardObj.contains("port_count")) {
                                    int portCount = boardObj["port_count"].toInt();
                                    QStandardItem *portCountItem = new QStandardItem("Port Count: " + QString::number(portCount));
                                    boardItem->appendRow(portCountItem);
                                }

                                if (boardObj.contains("ports")) {
                                    QJsonArray portsArray = boardObj["ports"].toArray();
                                    for (int l = 0; l < portsArray.size(); ++l) {
                                        QJsonObject portObj = portsArray[l].toObject();
                                        QString portLabel = QString("Port %1").arg(portObj["num"].toInt());

                                        QStandardItem *portItem = new QStandardItem(portLabel);
                                        boardItem->appendRow(portItem);

                                        if (portObj.contains("id")) {
                                            int portId = portObj["id"].toInt();
                                            QStandardItem *portIdItem = new QStandardItem("Port ID: " + QString::number(portId));
                                            portItem->appendRow(portIdItem);
                                        }

                                        if (portObj.contains("signal")) {
                                            int signal = portObj["signal"].toInt();
                                            QStandardItem *signalItem = new QStandardItem("Signal: " + QString::number(signal));
                                            portItem->appendRow(signalItem);
                                        }

                                        if (portObj.contains("media")) {
                                            int media = portObj["media"].toInt();
                                            QStandardItem *mediaItem = new QStandardItem("Media: " + QString::number(media));
                                            portItem->appendRow(mediaItem);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

