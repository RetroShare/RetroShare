#ifndef _HISTORY_KEEPER_H_
#define _HISTORY_KEEPER_H_

#include <QObject>
#include <QDebug>
#include <QString>
#include <QStringList>

//#include "IMHistoryReader.h"
#include "IMHistoryItem.h"

class IMHistoryKeeper//: public QObject
{
//    Q_OBJECT
public:
    IMHistoryKeeper();
    IMHistoryKeeper(QString historyFileName);
    virtual ~IMHistoryKeeper();
    
    //int setHistoryFileName(QString historyFileName) ;
    QString errorMessage();
    int getMessages(QStringList& messagesList,
                    const QString fromID, const QString toID,
                    const int messagesCount = 5);

    void addMessage(const QString fromID, const QString toID,
                    const QString messageText);
            
protected:
    int loadHistoryFile(QString fileName);
    void formStringList(QList<IMHistoryItem>& itemList, QStringList& strList);

    QList<IMHistoryItem> hitems;
    QString hfName ; //! history file name
    QString lastErrorMessage;

};

#endif //  _HISTORY_KEEPER_H_
