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

bool IMHistoryReader::read(QList<IMHistoryItem>& resultList, const QString fileName, int &lasthiid)
{
    errMess = "No error";

    resultList.clear();

    //==== check for file and open it
    QFile fl(fileName);
    if (fl.exists()) {
        fl.open(QIODevice::ReadOnly);
    } else {
        errMess = QString("file not found (%1)").arg(fileName);
        return false ;
    }

    //==== set the file, and check it once more
    setDevice(&fl);

    if (atEnd()) {
        errMess = "end of document reached before anything happened";
        return false;
    }

    //==== now, read the first element (it should be document element)
    while (!atEnd()) {
        readNext();	
        if (isStartElement()) {
            if (name() == "history_file" && attributes().value("format_version") == "1.0") {
                readHistory(resultList, lasthiid);
                break;
            } else {
                errMess = "The file is not a history file with format version 1.0";
                return false ;
            }
        }
    }

    if (error()) {
        errMess = errorString();
//    } else {
//        QList<IMHistoryItem>::const_iterator hii;//history items iterator
//        for (hii = result.constBegin(); hii != result.constEnd(); ++hii) {
//            resultList << *hii;
//        }
    }

    return !error();
}

//=============================================================================

QString IMHistoryReader::errorMessage()
{
    QString result = errMess;
    errMess = "No error" ;
    return result;
}

//=============================================================================

void IMHistoryReader::readUnknownElement()
{
    Q_ASSERT(isStartElement());

    qDebug()<< "  " << "unknown node " << name().toString();

    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
            break;
        }

        if (isStartElement()) {
            readUnknownElement();
        }
    }
}

//=============================================================================

void IMHistoryReader::readHistory(QList<IMHistoryItem> &historyItems, int &lasthiid)
{
    Q_ASSERT(isStartElement());

    // qDebug()<< "  " << "node with message " << name() ;

    historyItems.clear();
    lasthiid = 0;

    bool recalculate = false;

    while (!atEnd()) {
        readNext();
        if (isEndElement()) {
	    break;
        }

        if (isStartElement()) {
            if ( name() == "message") {
                IMHistoryItem item;
                readMessage(item);
                if (item.hiid == 0) {
                    recalculate = true;
                } else {
                    if (item.hiid > lasthiid) {
                        lasthiid = item.hiid;
                    }
                }
                historyItems.append(item);
            } else {
                readUnknownElement();	
            }
        }
    }

    if (recalculate) {
        // calculate hiid
        QList<IMHistoryItem>::iterator item = historyItems.begin();
        for (item = historyItems.begin(); item != historyItems.end(); item++) {
            if (item->hiid == 0) {
                item->hiid = ++lasthiid;
            }
        }
    }
}

//=============================================================================

void IMHistoryReader::readMessage(IMHistoryItem &historyItem)
{
//   Q_ASSERT(isStartElement() );
   
    if (isStartElement() && (name() == "message")) {
        //=== process attributes

        historyItem.hiid = attributes().value("hiid").toString().toInt();
        historyItem.incoming = (attributes().value("incoming").toString().toInt() == 1);
        historyItem.id = attributes().value("id").toString().toStdString();
        historyItem.name = attributes().value("name").toString();

        int ti = attributes().value("sendTime").toString().toInt();
        historyItem.sendTime = QDateTime::fromTime_t(ti);

        ti = attributes().value("recvTime").toString().toInt();
        if (ti) {
            historyItem.recvTime = QDateTime::fromTime_t(ti);
        } else {
            historyItem.recvTime = historyItem.sendTime;
        }

        //=== after processing attributes, read the message text
        QString tstr = readElementText();

        //=== remove '\0' chars from the string. Is it a QXmlStuff bug,
        //    if they appear?
        for (int i = 0; i< tstr.length(); i++) {
            if (tstr.at(i) == '\n') {
                tstr.remove(i, 1);
            }
        }

        historyItem.messageText = tstr;

        //qDebug() << QString(" readMessage: %1, %2, %3, %4" )
        //            .arg(rez.text()).arg(rez.sender())
        //            .arg(rez.receiver()).arg(ti) ;
    }
}

