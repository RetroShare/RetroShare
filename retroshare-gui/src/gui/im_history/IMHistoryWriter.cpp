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

bool IMHistoryWriter::write(QList<IMHistoryItem>& itemList, const QString fileName)
{
    qDebug() << "  IMHistoryWriter::write is here" ;

    errMess = "No error";

    if (itemList.size() == 0) {
        return remove(fileName);
    }

    //==== check for file and open it
    QFile fl(fileName);
    if (fl.open(QIODevice::WriteOnly | QIODevice::Truncate) == false) {
        errMess = QString("error opening file %1 (code %2)")
                  .arg(fileName).arg( fl.error() );
        return false;
    }

    //==== set the file, and check it once more
    setDevice(&fl);

    writeStartDocument();
    writeDTD("<!DOCTYPE history_file>");
    writeStartElement("history_file");
    writeAttribute("format_version", "1.0");

    foreach(IMHistoryItem item, itemList) {
        writeStartElement("message");
        writeAttribute("hiid", QString::number(item.hiid));
        writeAttribute("incoming", QString::number(item.incoming ? 1 : 0));
        writeAttribute("id", QString::fromStdString(item.id));
        writeAttribute("name", item.name);
        writeAttribute("sendTime", QString::number(item.sendTime.toTime_t()));
        writeAttribute("recvTime", QString::number(item.recvTime.toTime_t()));
        writeCDATA(item.messageText);
        writeEndElement();
    }

    writeEndDocument() ;

    fl.close();

    qDebug() << "  IMHistoryWriter::write done" ;

    return true;
}

bool IMHistoryWriter::remove(const QString &fileName)
{
    if (!QFile::remove(fileName)) {
        qDebug() << "  IMHistoryWriter::remove Failed to remove history file";
        return false;
    }

    return true;
}
