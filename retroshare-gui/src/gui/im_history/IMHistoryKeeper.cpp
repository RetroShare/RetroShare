/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2009 The RetroShare Team, Oleksiy Bilyanskyy
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/

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

    formStringList(ril, messagesList) ;

    return 0; // successful end
}

//=============================================================================

void
IMHistoryKeeper::formStringList(QList<IMHistoryItem>& itemList,
                                QStringList& strList)
{
    strList.clear();

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

