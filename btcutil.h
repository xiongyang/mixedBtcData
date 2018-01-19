#ifndef BTCUTIL_H
#define BTCUTIL_H

#include <string>
#include <iostream>

#include <QByteArray>
#include <QJsonObject>
#include <QString>

#include "btcdatastruct.h"


const char* getSymbolBack(const char s);

const std::string currentDateTime();
const std::string makeTime(time_t t);
const std::string currentDateTimeOrg();

QJsonObject getJson(QByteArray&  reply);
std::ostream& operator<<( std::ostream& o, const LogicTrade& t);
QDebug operator<<(QDebug o, const LogicTrade& t);
std::pair<char,char> splitSymbol(QString& s);
bool isLiveOrder (QString& status );
bool gzipDecompress(const QByteArray & in , QByteArray& out);
unsigned long long getCurrentTimeMsec();

uint32_t getMarketTickType(const QStringRef s);
uint32_t getDepthTickType(const QStringRef s, uint8_t exch = exchange_huobi);

// 2-->100 3-->1000
inline double myround(double s, double p)
{
  return round(s * p + 0.00000000000000001)  / p;
}
constexpr double  minAmount = 0.00000000000000001;
#endif // BTCUTIL_H
