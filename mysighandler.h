#ifndef MYSIGHANDLER_H
#define MYSIGHANDLER_H

#include <QObject>



class MySigHandler : public QObject
{
    Q_OBJECT
public:
    explicit MySigHandler(QObject *parent = nullptr);

signals:
    void willStopApp();


public slots:
};

#endif // MYSIGHANDLER_H
