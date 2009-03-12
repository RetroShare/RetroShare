#ifndef __IM_History_writer__
#define __IM_History_writer__

#include <QXmlStreamWriter>

#include <QString>
//#include <QStringList>

#include "IMHistoryItem.h"

class IMHistoryWriter : public QXmlStreamWriter
{
public:
    IMHistoryWriter();

    bool write(QList<IMHistoryItem>& itemList,
               const QString fileName  );

    QString errorMessage();

private:


    QString errMess;
} ;



#endif