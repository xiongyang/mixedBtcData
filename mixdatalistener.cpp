#include "mixdatalistener.h"
#include "btcutil.h"
#include "logger.h"

#include <string>
#include <chrono>


#include <QWebSocket>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>


MixDataListener::MixDataListener(QObject *parent) : QObject(parent)
{
    _sockHuobi = new QWebSocket();
    connect(_sockHuobi, &QWebSocket::connected, this, &MixDataListener::onWsBtcConnected);
    connect(_sockHuobi, &QWebSocket::disconnected, this, &MixDataListener::onWsBtcDisconnected);
    connect(_sockHuobi, &QWebSocket::binaryMessageReceived, this, &MixDataListener::WsBtcBinaryMessageReceived);


    sockOkex = new QWebSocket();
    connect(sockOkex, &QWebSocket::connected, this, &MixDataListener::onOkexConnect);
    connect(sockOkex, &QWebSocket::disconnected, this, &MixDataListener::onOkexDisconnect);
    connect(sockOkex, &QWebSocket::textMessageReceived, this, &MixDataListener::onReceiveOkexMessage);

    sockBit = new QWebSocket();
    connect(sockBit, &QWebSocket::connected, this, &MixDataListener::onBitConnect);
    connect(sockBit, &QWebSocket::disconnected, this, &MixDataListener::onBitDisconnect);
    connect(sockBit, &QWebSocket::textMessageReceived, this, &MixDataListener::onBitReceiveMessage);

    QTimer* t = new QTimer();
    connect(t, &QTimer::timeout, this,  &MixDataListener::printCount);
    t->start(60000);


    resetFile();


    QTimer* checkHBConnectTimer = new QTimer(this);
    connect(checkHBConnectTimer, &QTimer::timeout, this, &MixDataListener::checkConnect);
    checkHBConnectTimer->start(30000);

    //for first connect
     QTimer::singleShot(1000, this, &MixDataListener::checkConnect);

    QTimer::singleShot(2000, this, &MixDataListener::subscribeAllBiAn);

    QTimer* okex_heartbeat_timer = new QTimer(this);
    connect(okex_heartbeat_timer, &QTimer::timeout, this, &MixDataListener::sendOkexHeartbeat);
    okex_heartbeat_timer->start(30000);
}

void MixDataListener::onWsBtcConnected()
{
    isHuoBiConnected = true;
    doSubsribceHuoBi();
    Logger << "onWsBtcConnected";

}

void MixDataListener::onWsBtcDisconnected()
{
    isHuoBiConnected = false;
    QTimer::singleShot(2000, this, &MixDataListener::doConnectToHuoBi);
    Logger << "onWsBtcDisconnected";
}

void MixDataListener::WsBtcBinaryMessageReceived(const QByteArray &message)
{
    static int recvCount = 0;

    if(++recvCount % 10240 == 0)  Logger << "onWsBtcBinaryMessageReceived" <<  "Count:" <<  recvCount ;


    QByteArray baUnpack;
    //取回来的内容要解压
    bool bResult = gzipDecompress(message, baUnpack);

    if (!bResult)
     {
        Logger << "decompress fail";
        return;
     }
    QJsonParseError jsonError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(baUnpack, &jsonError);
    if (jsonError.error != QJsonParseError::NoError)
    {
           Logger << "Parse to Json Failed";
           return;
    }

    if(!jsonDoc.isObject())
    {
        Logger << " Is Not Object ";
        return;
    }

    QJsonObject jsonObj = jsonDoc.object();
    if (jsonObj.contains("ping")) {
        processHeartbeat(jsonObj);
    }
    else if (jsonObj.contains("ch")) {
        //订阅的数据包
        QString symbol = jsonObj.value("ch").toString();
        QVector<QStringRef> parts = symbol.splitRef(".");

        if(parts.size() == 3)
        {
            // qDebug() << jsonObj;
             processMarketTick(parts, jsonObj);
        }
        else if(parts.size() == 4 && parts[2] == "depth")
        {
            processDepthTick(jsonObj, parts);
        }
        else
        {
            qDebug() << jsonObj;
        }

        return;
    }
}

void MixDataListener::onOkexConnect()
{
    isOkexConnected = true;

    Logger << "okExConnected";
    doSubscribeOkex();
}

void MixDataListener::onOkexDisconnect()
{
    Logger << "onOkexDisconnect";
    isOkexConnected = false;
    QTimer::singleShot(2000, this, &MixDataListener::doConnectToOkex);
}

void MixDataListener::onReceiveOkexMessage(const QString &message)
{
    QJsonParseError jsonError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(message.toLatin1(), &jsonError);

    if(jsonError.error != QJsonParseError::NoError) return;

    if(!jsonDoc.isArray()) return;

    QJsonValueRef jsonObjRef = jsonDoc.array()[0];
    if(!jsonObjRef.isObject())
    {
        Logger << "Receive Invalid JsonValue " <<message.toStdString();
        return;
    }
    QJsonObject jsonObj = jsonObjRef.toObject();
    if(!jsonObj.contains("channel")) return;
    QString channel = jsonObj["channel"].toString();
    if(channel.contains("depth_20"))
    {
        processOkexDepths(jsonObj, channel);
    }
    else
    {
        count++;
        qDebug() << message;
    }
}

void MixDataListener::onBitConnect()
{
    qDebug() << "BitFinex Connected";

    subscribeBitFinex();
}

void MixDataListener::onBitDisconnect()
{
    qDebug() << "BitFinex Disconnect";
}

void MixDataListener::onBitReceiveMessage(const QString& msg)
{
    qDebug() << "BitFinex " << msg;
    count ++;
}

void MixDataListener::printCount()
{
    qDebug() << "============== LastMin Count ==========  " << count;
    count =0;
}

void MixDataListener::resetFile()
{
    if(currentFile)
    {
        currentFile.close();
    }
    currentFileSize = 0;

    std::string fileName = currentDateTimeOrg() + ".bin";
    currentFile.open(fileName, std::ios_base::out | std::ios_base::binary);
    qDebug() << "OutputFile is " << QString(fileName.c_str());
    if(!currentFile)
    {
        qDebug() << "OutputFile  " << QString(fileName.c_str()) << "  Failed ";
    }
}

void MixDataListener::doSubsribceHuoBi()
{
    std::vector<std::string>   usdtmarkets{"btc", "bch","eth", "ltc", "xrp", "dash", "etc", "eos", "zec", "omg"};
    std::vector<std::string>   btcmarkets{"bch", "eth", "ltc", "xrp", "dash", "etc", "eos", "zec", "omg"};
    std::vector<std::string>   ethmarkets{ "eos", "omg"};

    doSubscribeFormatHuoBi(usdtmarkets, "market.", "usdt.detail");
    doSubscribeFormatHuoBi(btcmarkets,"market.", "btc.detail");
    doSubscribeFormatHuoBi(ethmarkets,"market.", "eth.detail");

    doSubscribeFormatHuoBi(usdtmarkets,"market.", "usdt.depth.step0");
    doSubscribeFormatHuoBi(btcmarkets,"market.", "btc.depth.step0");
    doSubscribeFormatHuoBi(ethmarkets,"market.", "eth.depth.step0");
}

void MixDataListener::doSubsribeOkEx(const std::string &symbol1, const std::string &symbol2)
{
    QJsonObject json;
    std::string symbol = symbol1 + "_" + symbol2;
    QString channel =  QString("ok_sub_spot_") + symbol.c_str() +  "_depth_20";
    json.insert("event", "addChannel");
    json.insert("channel", channel);
    json.insert("binary", "0");
    QJsonDocument jsonDoc;
    jsonDoc.setObject(json);
    QByteArray ba = jsonDoc.toJson(QJsonDocument::Compact);
    QString jsonStr(ba);
    qDebug() <<  "DoSubscribe " << jsonStr;
    sockOkex->sendTextMessage(jsonStr);
}

void MixDataListener::doSubscribeFormatOkex(std::vector<std::string> firstSymbols, const std::string &secondSymbol)
{

    for(auto& each : firstSymbols)
    {
           doSubsribeOkEx(each, secondSymbol);
    }
}

void MixDataListener::doSubscribeOkex()
{
    std::vector<std::string>   usdtmarkets{"btc", "bch","eth", "ltc", "xrp", "dash", "etc", "eos", "zec", "omg"};
    std::vector<std::string>   btcmarkets{"bch", "eth", "ltc", "xrp", "dash", "etc", "eos", "zec", "omg"};
    std::vector<std::string>   ethmarkets{ "eos", "omg"};

    doSubscribeFormatOkex(usdtmarkets, "usdt");
    doSubscribeFormatOkex(btcmarkets,"btc");
    doSubscribeFormatOkex(ethmarkets,"eth");


    {
        QJsonObject json;
        std::string symbol = "btc_usdt";
        QString channel =  QString("ok_sub_spot_") + symbol.c_str() +  "_deals";
        json.insert("event", "addChannel");
        json.insert("channel", channel);
        json.insert("binary", "0");
        QJsonDocument jsonDoc;
        jsonDoc.setObject(json);
        QByteArray ba = jsonDoc.toJson(QJsonDocument::Compact);
        QString jsonStr(ba);
        qDebug() <<  "DoSubscribe " << jsonStr;
        sockOkex->sendTextMessage(jsonStr);
    }

}

void MixDataListener::sendOkexHeartbeat()
{
    QJsonObject json;
    json.insert("event", "ping");
    QJsonDocument jsonDoc;
    jsonDoc.setObject(json);
    QByteArray ba = jsonDoc.toJson(QJsonDocument::Compact);
    QString jsonStr(ba);
  //  qDebug() <<  "sendHeartBeat " << jsonStr;
    sockOkex->sendTextMessage(jsonStr);
}

void MixDataListener::processOkexDepths(const QJsonObject &obj, QString channel)
{
      uint16_t symbolid =getSymbolId(channel);
      uint32_t type = getOkExDepthTickType(symbolid);

    //  qDebug() << obj;


      DepthTick   tick;
      tick.ts =  getCurrentTimeMsec();


      const QJsonObject& tickObject = obj;
      QJsonObject dataObj = obj["data"].toObject();

      QJsonArray bids = dataObj["bids"].toArray();
      QJsonArray asks = dataObj["asks"].toArray();



                            //  qDebug() << ask;
      tick.askDepthsSize = asks.size();
      tick.bidDepthsSize = bids.size();


      uint32_t totalSize = sizeof(DepthTick) + sizeof(Depth) *  (tick.askDepthsSize  + tick.bidDepthsSize);
      TickHeader   header{type, totalSize};
      currentFile.write((char*)&header, sizeof(TickHeader));
      currentFile.write((char*)&tick, sizeof(DepthTick));


      for(int i = 0;  i != asks.size(); ++i)
      {
          QJsonArray ask =  asks[i].toArray();
          Depth d;
          d.price =   ask[0].toString().toDouble();
          d.vol =  ask[1].toString().toDouble();
        //  qDebug() << "ask" <<  "-->[A:" << d.price << ":"<< d.vol << "]";
          currentFile.write((char*)&d, sizeof(Depth));
      }

      for(int i = 0;  i != bids.size(); ++i)
      {
          QJsonArray bid =  bids[i].toArray();
          Depth d;
          d.price =   bid[0].toDouble();
          d.vol =  bid[1].toDouble();
         // qDebug() << "bid" <<  "-->[B:" << d.price << ":"<< d.vol << "]";
          currentFile.write((char*)&d, sizeof(Depth));
      }

      currentFileSize +=  (sizeof(TickHeader) + totalSize);

      if(currentFileSize > maxFileSize)
      {
          resetFile();
      }

}

std::string MixDataListener::genSubscribeID()
{
    std::string id_str = "id" + std::to_string(subscribeID++);

    return id_str;
}

void MixDataListener::processHeartbeat(const QJsonObject &jsonObj)
{
    QJsonObject jsonPong;
    jsonPong.insert("pong", jsonObj.value("ping"));
    QJsonDocument docPong(jsonPong);
    _sockHuobi->sendTextMessage(QString(docPong.toJson(QJsonDocument::Compact)));
}

void MixDataListener::processMarketTick(QVector<QStringRef> parts, QJsonObject jsonObj)
{
    uint32_t type = getMarketTickType(parts[1]);

    MarketTick   tick;
    QJsonObject tickObject = jsonObj["tick"].toObject();

    tick.amount =  tickObject["amount"].toDouble();
    tick.close = tickObject["close"].toDouble();
    // tick.count = tickObject["count"].toInt();
    tick.high  = tickObject["high"].toDouble();
    tick.low = tickObject["low"].toDouble();
    tick.open = tickObject["open"].toDouble();
    tick.index = uint64_t(tickObject["id"].toDouble());
   // tick.ts = uint64_t(tickObject["ts"].toDouble());
    tick.ts = getCurrentTimeMsec();
    tick.vol = tickObject["vol"].toDouble();

    TickHeader   header{type, sizeof(MarketTick)};
    currentFile.write((char*)&header, sizeof(TickHeader));
    currentFile.write((char*)&tick, sizeof(MarketTick));
    currentFileSize +=  (sizeof(TickHeader) + sizeof(MarketTick));

    if(currentFileSize > maxFileSize)
    {
        resetFile();
    }
}

void MixDataListener::processDepthTick(const QJsonObject& jsonObj, QVector<QStringRef> parts)
{
    uint32_t type = getDepthTickType(parts[1]);

    DepthTick   tick;
    QJsonObject tickObject = jsonObj["tick"].toObject();
    tick.ts = getCurrentTimeMsec();
    tick.index = uint64_t(tickObject["version"].toDouble());

    QJsonArray bids = tickObject["bids"].toArray();
    QJsonArray asks = tickObject["asks"].toArray();
    tick.askDepthsSize = asks.size();
    tick.bidDepthsSize = bids.size();

    uint32_t totalSize = sizeof(DepthTick) + sizeof(Depth) *  (tick.askDepthsSize  + tick.bidDepthsSize);
    TickHeader   header{type, totalSize};
    currentFile.write((char*)&header, sizeof(TickHeader));
    currentFile.write((char*)&tick, sizeof(DepthTick));


    for(int i = 0;  i != asks.size(); ++i)
    {
        QJsonArray ask =  asks[i].toArray();
        Depth d;
        d.price =   ask[0].toDouble();
        d.vol =  ask[1].toDouble();
      //  qDebug() << ask <<  "-->[A:" << d.price << ":"<< d.vol << "]";
        currentFile.write((char*)&d, sizeof(Depth));
    }

    for(int i = 0;  i != bids.size(); ++i)
    {
        QJsonArray bid =  bids[i].toArray();
        Depth d;
        d.price =   bid[0].toDouble();
        d.vol =  bid[1].toDouble();
     //   qDebug() <<bid <<  "-->[B:" << d.price << ":"<< d.vol << "]";
        currentFile.write((char*)&d, sizeof(Depth));
    }

    currentFileSize +=  (sizeof(TickHeader) + totalSize);

    if(currentFileSize > maxFileSize)
    {
        resetFile();
    }
}

void MixDataListener::processBiAnData(const QString &symbol, const QString &message)
{
    QJsonParseError jsonError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(message.toLatin1(), &jsonError);
    if (jsonError.error != QJsonParseError::NoError)
    {
         Logger << "BiAn onTextMessageReceived Parse Error " << message.toStdString() << std::endl;
         return;
    }
    if (jsonDoc.isObject())
    {
        uint32_t type = getDepthTickType(symbol.midRef(0),exchange_bian );
        DepthTick   tick;
        tick.ts = getCurrentTimeMsec();
        tick.index = 0;
        tick.askDepthsSize = 20;
        tick.bidDepthsSize = 20;

        uint32_t totalSize = sizeof(DepthTick) + sizeof(Depth) *  (tick.askDepthsSize  + tick.bidDepthsSize);
        TickHeader   header{type, totalSize};
        currentFile.write((char*)&header, sizeof(TickHeader));
        currentFile.write((char*)&tick, sizeof(DepthTick));

        QJsonObject jsonObj = jsonDoc.object();
        QJsonArray asks = jsonObj["asks"].toArray();
        QJsonArray bids = jsonObj["bids"].toArray();


        for(int i = 0;  i != asks.size(); ++i)
        {
            Depth d;
            auto askdepth = asks[i].toArray();
            d.price = askdepth[0].toString().toDouble();
            d.vol = askdepth[1].toString().toDouble();
            currentFile.write((char*)&d, sizeof(Depth));
        }

        for(int i = 0;  i != bids.size(); ++i)
        {
            Depth d;
            auto biddepth = bids[i].toArray();
            d.price = biddepth[0].toString().toDouble();
            d.vol = biddepth[1].toString().toDouble();
            currentFile.write((char*)&d, sizeof(Depth));
        }

        currentFileSize +=  (sizeof(TickHeader) + totalSize);

        if(currentFileSize > maxFileSize)
        {
            resetFile();
        }
    }
}


void MixDataListener::onBiAnConnect(const QString &symbol)
{
    Logger << "BiAnonConnect " << symbol.toStdString();
    biAnStatus[symbol.toStdString()] = true;
}

void MixDataListener::onBiAnDisconnect(const QString &symbol)
{
    Logger << "BiAn symbol socket disconnected  resubscribe "  << symbol.toStdString();
    biAnStatus[symbol.toStdString()] = false;
    subscribeBiAn(symbol);
}

void MixDataListener::subscribeBiAn(const QString &symbol)
{
    Logger << "BiAn subscribe " << symbol.toStdString();
    QWebSocket* s = new QWebSocket();
    connect(s, &QWebSocket::connected, [=](){onBiAnConnect(symbol);});
    connect(s, &QWebSocket::disconnected, [=](){ onBiAnDisconnect(symbol);  });
    connect(s, &QWebSocket::textMessageReceived,  [=](const QString& data) {processBiAnData(symbol, data); } );

    QString urlStr = "wss://stream.binance.com:9443/ws/" + symbol + "@depth20";
    QUrl url(urlStr);
    s->open(url);

    auto iter = subscribed.find(symbol);
    if(iter != subscribed.end())
    {
        (*iter)->close();
        subscribed.erase(iter);
        (*iter)->deleteLater();
    }

    subscribed.insert(symbol, s);
}

void MixDataListener::subscribeAllBiAn()
{
    qDebug() << "subscribeALL Bi An";
    Logger <<  "subscribeALL Bi An";
    std::vector<std::string>   usdtmarkets{"btc", "bch","eth", "ltc", "xrp", "dash", "etc", "eos", "zec", "omg"};
    std::vector<std::string>   btcmarkets{"bch", "eth", "ltc", "xrp", "dash", "etc", "eos", "zec", "omg"};
    std::vector<std::string>   ethmarkets{ "eos", "omg"};

    for(auto& each : usdtmarkets)
    {
        std::string market_symbol = each + "usdt";
        subscribeBiAn(market_symbol.c_str());
    }

    for(auto& each : btcmarkets)
    {
        std::string market_symbol = each + "btc";
        subscribeBiAn(market_symbol.c_str());
    }

    for(auto& each : ethmarkets)
    {
        std::string market_symbol = each + "eth";
        subscribeBiAn(market_symbol.c_str());
    }
}
void MixDataListener::doSubscribeHuoBi(const char* subscribeTopic)
{

    QJsonObject json;
    json.insert("sub", subscribeTopic);
    json.insert("id",  genSubscribeID().c_str());
    QJsonDocument jsonDoc;
    jsonDoc.setObject(json);
    QByteArray ba = jsonDoc.toJson(QJsonDocument::Compact);
    QString jsonStr(ba);
    qDebug() <<  "DoSubscribe " << jsonStr;
    Logger <<  "DoSubscribe " << jsonStr.toStdString();
    _sockHuobi->sendTextMessage(jsonStr);
}


void MixDataListener::doSubscribeFormatHuoBi(std::vector<std::string> usdtmarkets, const std::string& prefix, const std::string& tail)
{
    for(auto& each : usdtmarkets)
    {
        std::string market_topic =prefix + each + tail;
        doSubscribeHuoBi(market_topic.c_str());
    }
}

void MixDataListener::doConnectToHuoBi()
{
    Logger << "ConnectToHuoBi ";
    if(isHuoBiConnected)
    {
        Logger << "ConnectToHuoBi is connected so ignore the connect request";
        return;
    }

    _sockHuobi->close();
    QTimer::singleShot(100, [=](){
        qDebug() << " doConnectToHuoBi" ;
        Logger << " doConnectToHuoBi ";
        QUrl url("wss://api.huobi.pro/ws");
        _sockHuobi->open(url);

    });
}

void MixDataListener::doConnectToOkex()
{
    Logger << "ConnectToOkex ";
    if(isOkexConnected)
    {
        Logger << "ConnectToOkex is connected so ignore the connect request";
        return;
    }

    sockOkex->close();
    QTimer::singleShot(100, [=](){
        qDebug() << " doConnectToOkex" ;
        Logger << " doConnectToOkex ";
        QUrl url("wss://real.okex.com:10441/websocket");
        sockOkex->open(url);
    });
}

void MixDataListener::doConnectToBit()
{
    Logger << "doConnectToBit out ";
    if(isBitFinexConnected)
    {
        Logger << "ConnectToBit is connected so ignore the connect request";
        return;
    }

    sockBit->close();
    QTimer::singleShot(100, [=](){
        qDebug() << " doConnectToBit inner" ;
        Logger << " doConnectToBit inner";
        QUrl url("wss://api.bitfinex.com/ws/2");
        sockBit->open(url);
    });
}

void MixDataListener::checkConnect()
{
    if(!isHuoBiConnected)
        QTimer::singleShot(2000, this, &MixDataListener::doConnectToHuoBi);

    if(!isOkexConnected)
         QTimer::singleShot(3000, this, &MixDataListener::doConnectToOkex);

    if(!isOkexConnected)
        QTimer::singleShot(3500, this, &MixDataListener::doConnectToBit);

    for(auto& each : biAnStatus)
    {
        if(each.second == false)
        {
            std::string s = each.first;
            QTimer::singleShot(5000, [=](){
                QString symbol(s.c_str());
                subscribeBiAn(symbol); }
            );
        }
    }
}

void MixDataListener::subscribeBitFinex()
{

//    {
//        QJsonObject json;
//        json.insert("event", "subscribe");
//        json.insert("channel",  "trades");
//        json.insert("symbol", "tBTCUSD");
//        QJsonDocument jsonDoc;
//        jsonDoc.setObject(json);
//        QByteArray ba = jsonDoc.toJson(QJsonDocument::Compact);
//        QString jsonStr(ba);
//        qDebug() <<  "subscribeBitFinex " << jsonStr;
//        Logger <<  "subscribeBitFinex " << jsonStr.toStdString();
//        sockBit->sendTextMessage(jsonStr);
//    }


//    {
//        QJsonObject json;
//        json.insert("event", "subscribe");
//        json.insert("channel",  "book");
//        json.insert("symbol", "tBTCUSD");
//        QJsonDocument jsonDoc;
//        jsonDoc.setObject(json);
//        QByteArray ba = jsonDoc.toJson(QJsonDocument::Compact);
//        QString jsonStr(ba);
//        qDebug() <<  "subscribeBitFinex " << jsonStr;
//        Logger <<  "subscribeBitFinex " << jsonStr.toStdString();
//        sockBit->sendTextMessage(jsonStr);
//    }


}
