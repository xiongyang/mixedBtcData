#include <iostream>
#include <signal.h>

#include <QCoreApplication>
#include <QtWebSockets/QtWebSockets>
#include <QTimer>
#include <QObject>

//#include "btcorder.h"
#include "logger.h"

#include "mysighandler.h"


MySigHandler h;

void closeApp(int sig)
{
     Logger << "Recvie Signal <<  " << sig << " will stop program after 1000 ms";
     emit h.willStopApp();
    // QTimer::singleShot(1000, g_app, &QCoreApplication::quit);
}

void handleSig(int sig)
{
    switch(sig)
    {
    case SIGTERM:
    case SIGINT:
        closeApp(sig);
     return;
    }
}



int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    initLogger();
    qDebug() << "MainWindow  " << QSslSocket::supportsSsl();

    Logger << "MainWindow  " << QSslSocket::supportsSsl();

    signal(SIGTERM, handleSig);
    signal(SIGINT, handleSig);
    QObject::connect(&h, &MySigHandler::willStopApp, &a, &QCoreApplication::quit);

    if(argc ==2)
    {
        Logger << "BtcOrder Start with " << argv[1];
       // BtcOrder inst( std::stoi(argv[1]));
        qDebug() << "after cons BtcOrder ";
        return  a.exec();
    }
    else
    {
     //   BtcOrder inst;
        qDebug() << "after cons BtcOrder ";
        return  a.exec();
    }

}
