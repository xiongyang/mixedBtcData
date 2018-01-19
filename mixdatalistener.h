#ifndef MIXDATALISTENER_H
#define MIXDATALISTENER_H

#include <fstream>

#include <QObject>
#include <QJsonObject>
#include <QVector>
#include <QStringRef>
#include <QByteArray>
#include <QMap>

QT_BEGIN_NAMESPACE
class QFile;
class QLabel;
class QLineEdit;
class QPushButton;
class QSslError;
class QNetworkReply;
class QCheckBox;
class QWebSocket;
class QTcpServer;
QT_END_NAMESPACE

class MixDataListener : public QObject
{
    Q_OBJECT
public:
    explicit MixDataListener(QObject *parent = nullptr);

signals:

public slots:
   void onWsBtcConnected();
   void onWsBtcDisconnected();
   void WsBtcBinaryMessageReceived(const QByteArray &message);

private:
     void resetFile();
     void doConnectToHuoBi();
     void checkConnect();

     void doSubscribeFormatHuoBi(std::vector<std::string> usdtmarkets, const std::string& prefix, const std::string& tail);
     void doSubscribeHuoBi(const char* subscribeTopic);
     void doSubsribceHuoBi();
     std::string genSubscribeID();
     void processHeartbeat(const QJsonObject& obj);
     void processMarketTick(QVector<QStringRef> parts, QJsonObject jsonObj);
     void processDepthTick(QJsonObject jsonObj, QVector<QStringRef> parts);


private slots:

    void onBiAnConnect(const QString& symbol);
    void onBiAnDisconnect(const QString& symbol);
    void processBiAnData(const QString& symbol, const QString& data);

private:
    void subscribeBiAn(const QString& symbol);
    void subscribeAllBiAn();

private:


private:
     QWebSocket * _wsBTC;
     bool         isHuoBiConnected{false};
     int          subscribeID{1};

     std::ofstream currentFile;
     int64_t       currentFileSize{0};
     static constexpr int64_t maxFileSize = 3.9 * 1024 * 1024 * 1024;

     //bi an
     QMap<QString, QWebSocket*>  subscribed;
     std::map<std::string, bool> biAnStatus;
};

#endif // MIXDATALISTENER_H
