#ifndef BTCDATASTRUCT_H
#define BTCDATASTRUCT_H

#include <QString>

const char Type_DepthTick = 'D';
const char Type_TradeTick = 'T';
const char Type_MarketTick = 'M';

static const char symbol_USDT = 0;
static const char symbol_BTC = 1;
static const char symbol_BCH = 2;
static const char symbol_ETH = 3;
static const char symbol_LTC = 4;
static const char symbol_XRP = 5;
static const char symbol_DASH = 6;
static const char symbol_ETC = 7;
static const char symbol_EOS = 8;
static const char symbol_ZEC = 9;
static const char symbol_OMG = 10;

constexpr uint8_t exchange_huobi = 0;
constexpr uint8_t exchange_bian = 1;


struct typeStruct
{
    char symbol1;
    char symbol2;
    char dataType;
    uint8_t exchange;
};

union typeUnion {
    uint32_t type;
    typeStruct types;
} ;



struct TickHeader
{
    typeUnion t;

    int size;
};


struct Depth
{
    double price;
    double vol;
};

struct DepthTick
{
    uint64_t ts;
    uint64_t index;
    uint16_t askDepthsSize;
    uint16_t bidDepthsSize;
    // appending the depths Array
};

struct TradeTick
{
    uint64_t ts;
    uint64_t index;
    double amount;
    double price;
    char direction;  // 'B' = Buy  'S' = Sell
};

struct MarketTick
{
    uint64_t ts;
    uint64_t index;
    // int count;

    double amount;
    double vol;
    double open;
    double high;
    double low;
    double close;
};


struct OrderStatus
{
    int orderID;
    double amount {0};
    double filled_amount {0};
    double filled_cash {0};
    double filled_fee {0};
    QString symbol;
    QString status;
    bool isBuy{0};
};

struct LogicTrade
{
    double symbol1_delta;
    double symbol2_delta;
    char symbol1;
    char symbol2;
};

struct SubscribeData
{
    QString symbolPair;
    char marketDataType;
};


#endif // BTCDATASTRUCT_H
