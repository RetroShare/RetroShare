#include "IMHistoryKeeper.h"

#include <QFile>
#include <QIODevice>

#include <QtAlgorithms> //for qSort

#include <QXmlStreamReader>

#include "IMHistoryReader.h"
#include "IMHistoryWriter.h"

//#include <iostream>


//=============================================================================

IMHistoryKeeper::IMHistoryKeeper()
{
    hfName = "";
};

//=============================================================================

IMHistoryKeeper::IMHistoryKeeper(QString historyFileName)
{
    hfName = historyFileName ;
    loadHistoryFile( historyFileName );
    //setHistoryFileName( historyFileName );
    //IMHistoryWriter wri;
    //wri.write(hitems, hfName);
}

//=============================================================================

IMHistoryKeeper::~IMHistoryKeeper()
{
   //=== we have to save all messages
    qSort( hitems.begin(), hitems.end() ) ; // not nesessary, but just in case...
                                          // it will not take a long time over
                                        //ordered array
                                            
    IMHistoryWriter wri;
    wri.write(hitems, hfName);
}

//=============================================================================

void
IMHistoryKeeper::addMessage(const QString fromID, const QString toID,
                            const QString messageText)
{
    IMHistoryItem item(fromID, toID, messageText,
                        QDateTime::currentDateTime());

    hitems.append( item );

    //std::cerr << "IMHistoryKeeper::addMessage "
    //          << messageText.toStdString() << "\n";

    //std::cerr << "IMHistoryKeeper::addMessage count is" << hitems.count();
}
//=============================================================================

int
IMHistoryKeeper::loadHistoryFile(QString fileName)
{
    qDebug() << "  IMHistoryKeeper::loadHistoryFile is here";
    
    QFile fl(fileName);
    if ( !fl.exists() )
    {
       lastErrorMessage = QString("history file not found (%1)").arg(fileName) ;
       return 1;
    }

    IMHistoryReader hreader;    
    if( !hreader.read( hitems, fileName ) )
    {
        lastErrorMessage = hreader.errorMessage();
        return 1;
    }

    qSort( hitems.begin(), hitems.end() )          ;

    qDebug() << "  IMHistoryKeeper::loadHistoryFile finished";
    return 0;
}

//=============================================================================

QString
IMHistoryKeeper::errorMessage()
{
    return lastErrorMessage;
    lastErrorMessage = "No error" ;
}

//=============================================================================

int
IMHistoryKeeper::getMessages(QStringList& messagesList,
                             const QString fromID, const QString toID,
                             const int messagesCount )
{
    int messFound = 0;
    QList<IMHistoryItem> ril;//result item list

    QListIterator<IMHistoryItem> hii(hitems);
    hii.toBack();
    while (hii.hasPrevious() && (messFound<messagesCount))
    {
        IMHistoryItem hitem = hii.previous();
        if ( ( (fromID.isEmpty())&&( hitem.receiver()==toID) ) ||
             ( (hitem.sender()==fromID)&&( hitem.receiver()==toID) ) ||
             ( (hitem.receiver()== fromID)&&(hitem.sender()==toID) ) )
        {
            ril << hitem ;
            messFound++;
            if (messFound>=messagesCount)
                break;
        }
    }
/*
    QList<IMHistoryItem>::iterator hii ; // history items iterator
    for (hii = hitems.begin(); hii != hitems.end(); hii++)
    {
        //IMHistoryItem* hitem = *hii;
        if ( ( (hii->sender()==fromID)&&( hii->receiver()==toID) ) ||
             ( (hii->receiver()== fromID)&&(hii->sender()==toID) ) )
        {
            ril << *hii ;
            messFound++;
            if (messFound>=messagesCount)
                break;
        }
    }
*/
    
    formStringList(ril, messagesList) ;

    return 0; // successful end
}

//=============================================================================

void
IMHistoryKeeper::formStringList(QList<IMHistoryItem>& itemList,
                                QStringList& strList)
{
    strList.clear();
/*
    QList<IMHistoryItem>::const_iterator hii=itemList.constBegin();
    for(hii; hii!= itemList.constEnd(); hii++)
    {
        strList.append( hii->text() );
    }
    */
    QListIterator<IMHistoryItem> hii(itemList);
    hii.toBack();
    while (hii.hasPrevious() )
    {
        IMHistoryItem hitem = hii.previous();

        QString tline;

        tline = QString("<strong><u>%1</u> %2 : </strong>"
                        "<span style=\"color:#008800\">%3</span>")
                       .arg(hitem.time().toString( Qt::TextDate ) )
                       .arg(hitem.sender())
                       .arg(hitem.text()) ;
        
        strList.append( tline );
    }
}
        
//=============================================================================

