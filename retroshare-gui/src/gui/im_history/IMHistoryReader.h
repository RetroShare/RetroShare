#ifndef __IM_History_reader__
#define __IM_History_reader__

#include <QXmlStreamReader>

#include <QString>
#include <QStringList>

#include "IMHistoryItem.h"

class IMHistoryReader : public QXmlStreamReader
{
public:
    IMHistoryReader();

    bool read(QList<IMHistoryItem>& resultList,
              const QString fileName  );
    
    QString errorMessage();

private:
    void readUnknownElement();
    QList<IMHistoryItem> readHistory();
    IMHistoryItem readMessage();

    QString errMess;
} ;


#endif

