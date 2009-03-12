#include "IMHistoryWriter.h"

#include <QFile>

#include <QDebug>

#include <QDateTime>

//=============================================================================

IMHistoryWriter::IMHistoryWriter()
                :errMess("No error")
{
  // nothing to do here
}

//=============================================================================

bool
IMHistoryWriter::write(QList<IMHistoryItem>& itemList,
                            const QString fileName  )
{
    qDebug() << "  IMHistoryWriter::write is here" ;

    errMess = "No error";

//==== check for file and open it
    QFile fl(fileName);
    if (fl.open(QIODevice::WriteOnly | QIODevice::Truncate));
    else
    {
        errMess = QString("error opening file %1 (code %2)")
                         .arg(fileName).arg( fl.error() );
        return false ;
    }

    //==== set the file, and check it once more
    setDevice(&fl);

    writeStartDocument();
    writeDTD("<!DOCTYPE history_file>");
    writeStartElement("history_file");
    writeAttribute("format_version", "1.0");

    foreach(IMHistoryItem item, itemList)
    {
        writeStartElement("message");
        writeAttribute( "dt", QString::number(item.time().toTime_t()) ) ;
        writeAttribute( "sender", item.sender() );
        writeAttribute( "receiver", item.receiver() ) ;
        writeCharacters(  item.text()); 
        writeEndElement();
    }

    writeEndDocument() ;

    fl.close();

    qDebug() << "  IMHistoryWriter::write done" ;

    return true;
}