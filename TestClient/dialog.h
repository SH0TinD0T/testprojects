#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QTcpSocket>
#include <QStandardItemModel>
#include <QJsonObject>

namespace Ui {
class Dialog;
}

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = nullptr);
    ~Dialog();

private slots:
    void on_pushButton_clicked();
    void onConnected();
    void onReadyRead();
    void onError(QAbstractSocket::SocketError socketError);

private:
    void updateConnectionStatus(const QString &status);
    void parseJsonToTreeView(const QJsonObject &jsonObj);

private:
    Ui::Dialog *ui;
    QTcpSocket *socket;
    QStandardItemModel *model;
};

#endif // DIALOG_H
