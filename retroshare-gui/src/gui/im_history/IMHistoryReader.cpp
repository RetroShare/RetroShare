#include "IMHistoryReader.h"

#include <QFile>

#include <QDebug>

//=============================================================================

IMHistoryReader::IMHistoryReader()
                :errMess("No error")
{
  // nothing to do here
}

//=============================================================================

bool
IMHistoryReader::read(QList<IMHistoryItem>& resultList,
                      const QString fileName)
{
    errMess = "No error";

    QList<IMHistoryItem> result;

    //==== check for file and open it
    QFile fl(fileName);
    if (fl.exists())
        fl.open(QIODevice::ReadOnly);
    else
    {
        errMess = QString("file not found (%1)").arg(fileName);
            return false ;
    }

    //==== set the file, and check it once more
    setDevice(&fl);

    if ( atEnd() )
    {
        errMess = "end of document reache before anything happened";
        return false;
    }

    //==== now, read the first element (it should be document element)
    while (!atEnd()) 
    {
        readNext();	
        if ( isStartElement() ) 
	    { 
            if (name() == "history_file" && 
                attributes().value("format_version") == "1.0")
	        {
                result = readHistory();
                break;
	        }
	        else
            {
               errMess="The file is not a history file with format version 1.0";
	            return false ;
            }
        }
    }
   
    if ( error() )
        errMess = errorString();
    else
    {
       resultList.clear();
      
       QList<IMHistoryItem>::const_iterator hii;//history items iterator
       for (hii = result.constBegin(); hii != result.constEnd(); ++hii)
	        resultList << *hii ;
    }

   return !error();
}

//=============================================================================

QString
IMHistoryReader::errorMessage()
{
   QString result = errMess;
   errMess = "No error" ;
   return result;
}

//=============================================================================

void
IMHistoryReader::readUnknownElement()
{
   Q_ASSERT(isStartElement());
   
   qDebug()<< "  " << "unknown node " << name().toString(); 
        
   while (!atEnd()) 
   {
      readNext();
      if (isEndElement())
          break;
      
      if (isStartElement())
          readUnknownElement();
   }
}

//=============================================================================

QList<IMHistoryItem>
IMHistoryReader::readHistory()
{
    Q_ASSERT(isStartElement());
   
  // qDebug()<< "  " << "node with message " << name() ;

    QList<IMHistoryItem> rez;
      
    while (!atEnd()) 
    {
        readNext();      
        if (isEndElement())
	    break;
      
        if (isStartElement())
        {
            if ( name() == "message" )
            {
                IMHistoryItem item = readMessage();
                rez.append(item);
            }
            else     
                readUnknownElement();	
        }
    }

    return rez;
}

//=============================================================================
//#include <QXmlAttributes>
IMHistoryItem
IMHistoryReader::readMessage()
{
//   Q_ASSERT(isStartElement() );
   
   IMHistoryItem rez;// = new IMHistoryItem();

   if ( isStartElement() && (name() == "message"))
   {
      //=== process attributes 
       int ti = attributes().value("dt").toString().toInt() ;
       rez.setTime( QDateTime::fromTime_t( ti ) );
       rez.setSender( attributes().value("sender").toString() ) ;
       rez.setReceiver( attributes().value("receiver").toString() );
      //=== after processing attributes, read the message text
       QString tstr = readElementText();

      //=== remove '\0' chars from the string. Is it a QXmlStuff bug, 
      //    if they appear?
       for(int i =0; i< tstr.length(); i++)
       {
           if (tstr.at(i) == '\n')
               tstr.remove(i,1);
       }
         
       rez.setText( tstr );

       //qDebug() << QString(" readMessage: %1, %2, %3, %4" )
       //            .arg(rez.text()).arg(rez.sender())
       //            .arg(rez.receiver()).arg(ti) ;
   }

   return rez;
}

//=============================================================================
//=============================================================================
//=============================================================================
