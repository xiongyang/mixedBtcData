#include "btcutil.h"

#include <time.h>
#include <iostream>
#include <sstream>
#include <chrono>

#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>


#include "boost/iostreams/filtering_stream.hpp"
#include "boost/iostreams/filter/gzip.hpp"
#include "boost/iostreams/device/array.hpp"
#include "boost/iostreams/copy.hpp"



const char* getSymbolBack(const char  s)
{
    switch (s)
    {
    case symbol_USDT : return "USDT";
    case symbol_BTC : return "BTC";
    case symbol_BCH : return "BCH";
    case symbol_ETH : return "ETH";
    case symbol_LTC : return "LTC";
    case symbol_XRP : return "XRP";
    case symbol_DASH : return "DASH";
    case symbol_ETC : return "ETC";
    case symbol_EOS : return "EOS";
    case symbol_ZEC : return "ZEC";
    case symbol_OMG : return "OMG";
    default:
        return "XXX";
    }
}


QDebug operator<<(QDebug o, const LogicTrade& t)
{
    o << "Trade:"
      << "[" << getSymbolBack(t.symbol1);
    o << ":" << t.symbol1_delta << "] "
      << "[" << getSymbolBack(t.symbol2);
    o << ":" << t.symbol2_delta << "]"
      << " [avgPrice:" <<   -1 * (t.symbol2_delta / t.symbol1_delta) << "]";

    return o;
}

std::ostream& operator<<( std::ostream& o, const LogicTrade& t)
{
    o << "Trade:"
      << "[" << getSymbolBack(t.symbol1);
    o << ":" << t.symbol1_delta << "] "
      << "[" << getSymbolBack(t.symbol2);
    o << ":" << t.symbol2_delta << "] "
      << " [avgPrice:" <<   -1 * (t.symbol2_delta / t.symbol1_delta) << "]";
    return o;
}



const std::string currentDateTime()
{
    time_t     now = time(0);
    struct tm  tstruct;
    char       buf[80];
    tstruct = *gmtime(&now);
    // Visit http://en.cppreference.com/w/cpp/chrono/c/strftime
    // for more information about date/time format

    //    2018-01-06T14%3A52%3A59
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H%%3A%M%%3A%S", &tstruct);

    return buf;
}

const std::string makeTime(time_t t)
{
    struct tm  tstruct;
    char       buf[80];
    tstruct = *gmtime(&t);
    // Visit http://en.cppreference.com/w/cpp/chrono/c/strftime
    // for more information about date/time format

    //    2018-01-06T14%3A52%3A59
    strftime(buf, sizeof(buf), "%Y%m%dT%H%M%S", &tstruct);
    return buf;
}

const std::string currentDateTimeOrg()
{
    time_t     now = time(0);
    return makeTime(now);
}

std::string convertTime(uint64_t ts)
{
    time_t t = ts / 1000;
    return makeTime(t);
}

QJsonObject getJson(QByteArray&  reply)
{
    QJsonParseError jsonError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(reply, &jsonError);
    if (jsonError.error != QJsonParseError::NoError)
    {
        qDebug() << "Error Parse Reply from " << reply
                 << " Error " << jsonError.errorString();
        return QJsonObject();
    }
    else
    {
        return std::move(jsonDoc.object());
    }
}


std::pair<char,char> splitSymbol(QString& s)
{
    std::pair<char,char> ret;
    char& firstSymbol = ret.first;
    char& secondSymbol = ret.second;

    do {
        if(s.startsWith("btc")) { firstSymbol = symbol_BTC; break;}
        if(s.startsWith("bch")) { firstSymbol = symbol_BCH;break;}
        if(s.startsWith("eth")) { firstSymbol = symbol_ETH;break;}
        if(s.startsWith("ltc")) { firstSymbol = symbol_LTC;break;}
        if(s.startsWith("xrp")) { firstSymbol = symbol_XRP;break;}
        if(s.startsWith("dash")){  firstSymbol = symbol_DASH;break;}
        if(s.startsWith("etc")) { firstSymbol = symbol_ETC;break;}
        if(s.startsWith("eos")) { firstSymbol = symbol_EOS;break;}
        if(s.startsWith("zec")) { firstSymbol = symbol_ZEC;break;}
        if(s.startsWith("omg")) { firstSymbol = symbol_OMG;break;}
    } while(false);

    if(s.endsWith("usdt")) secondSymbol = symbol_USDT;
    if(s.endsWith("btc")) secondSymbol = symbol_BTC;
    if(s.endsWith("eth")) secondSymbol = symbol_ETH;

    return ret;
}

bool isLiveOrder (QString& status )
{
    if(status == "pre-submitted "
            || status == "submitting"
            || status == "submitted"
            || status == "partial-filled"
            )
        return true;
    else
        return false;
}


bool gzipDecompress(const QByteArray & in , QByteArray& out) {
    std::stringstream ss_decomp;
    boost::iostreams::filtering_istream stream;
    stream.push(boost::iostreams::gzip_decompressor());
    //in.push(ss_comp);     //从字符流中解压
    stream.push(boost::iostreams::array_source(in.constData(), in.count()));     //从文件中解压
    boost::iostreams::copy(stream, ss_decomp);
    out.append(ss_decomp.str().c_str() , (ss_decomp.str().size()));
    return true;
}

unsigned long long getCurrentTimeMsec()
{
    return  std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

uint32_t getMarketTickType(const QStringRef s)
{
    typeUnion ret;
    char& firstSymbol = ret.types.symbol1;
    char& secondSymbol = ret.types.symbol2;
    ret.types.dataType = Type_MarketTick;
    ret.types.exchange = exchange_huobi;

    if(s.startsWith("btc")) firstSymbol = symbol_BTC;
    if(s.startsWith("bch")) firstSymbol = symbol_BCH;
    if(s.startsWith("eth")) firstSymbol = symbol_ETH;
    if(s.startsWith("ltc")) firstSymbol = symbol_LTC;
    if(s.startsWith("xrp")) firstSymbol = symbol_XRP;
    if(s.startsWith("dash")) firstSymbol = symbol_DASH;
    if(s.startsWith("etc")) firstSymbol = symbol_ETC;
    if(s.startsWith("eos")) firstSymbol = symbol_EOS;
    if(s.startsWith("zec")) firstSymbol = symbol_ZEC;
    if(s.startsWith("omg")) firstSymbol = symbol_OMG;

    if(s.endsWith("usdt")) secondSymbol = symbol_USDT;
    if(s.endsWith("btc")) secondSymbol = symbol_BTC;
    if(s.endsWith("eth")) secondSymbol = symbol_ETH;

    return ret.type;
}

uint32_t getDepthTickType(const QStringRef s, uint8_t exch )
{
    typeUnion ret;
    ret.type = getMarketTickType(s);
    ret.types.dataType = Type_DepthTick;
    ret.types.exchange = exch;
    return ret.type;
}
char getSymbolChar(const QString& symbol)
{
    if(symbol == "usdt") return symbol_USDT;
    if(symbol == "btc") return symbol_BTC;
    if(symbol == "bch") return symbol_BCH;
    if(symbol == "eth") return symbol_ETH;
    if(symbol == "ltc") return symbol_LTC;
    if(symbol == "xrp") return symbol_XRP;
    if(symbol == "dash") return symbol_DASH;
    if(symbol == "etc") return symbol_ETC;
    if(symbol == "eos") return symbol_EOS;
    if(symbol == "zec") return symbol_ZEC;
    if(symbol == "omg") return symbol_OMG;

    return symbol_INVALID;
}

uint16_t makeSymbol(char c1, char c2)
{
    return c1 + c2 * 256;
}

uint16_t getSymbolId(const QString& channel)
{
    QStringList l = channel.split("_");
    return makeSymbol( getSymbolChar( l.at(3)) ,  getSymbolChar( l.at(4)) );
}

uint32_t getOkExDepthTickType(uint16_t id)
{
    typeUnion ret;
    ret.type = id;
    ret.types.dataType = Type_DepthTick;
    ret.types.exchange = exchange_okex;
    return ret.type;
}
