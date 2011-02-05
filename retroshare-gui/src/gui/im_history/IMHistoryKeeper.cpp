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
#include <iostream>

#include <QFile>
#include <QIODevice>
#include <QTimer>

#include <QtAlgorithms> //for qSort

#include <QXmlStreamReader>

#include "IMHistoryReader.h"
#include "IMHistoryWriter.h"


//=============================================================================

IMHistoryKeeper::IMHistoryKeeper()
{
    historyChanged = false;

    // save histroy every 10 seconds (when changed)
    saveTimer = new QTimer(this);
    saveTimer->connect(saveTimer, SIGNAL(timeout()), this, SLOT(saveHistory()));
    saveTimer->setInterval(10000);
    saveTimer->start();

    lasthiid = 0;
};

//=============================================================================

IMHistoryKeeper::~IMHistoryKeeper()
{
    saveHistory();
}

//=============================================================================

void IMHistoryKeeper::init(QString historyFileName)
{
    lasthiid = 0;

    hfName = historyFileName;
    loadHistoryFile();
}

//=============================================================================

void IMHistoryKeeper::addMessage(bool incoming, const std::string &id, const QString &name, const QDateTime &sendTime, const QDateTime &recvTime, const QString &messageText)
{
    IMHistoryItem item(++lasthiid, incoming, id, name, sendTime, recvTime, messageText);

    hitems.append(item);

    historyChanged = true;

    emit historyAdd(item);

    //std::cerr << "IMHistoryKeeper::addMessage "
    //          << messageText.toStdString() << "\n";

    //std::cerr << "IMHistoryKeeper::addMessage count is" << hitems.count();
}

//=============================================================================

bool IMHistoryKeeper::loadHistoryFile()
{
    qDebug() << "  IMHistoryKeeper::loadHistoryFile is here";

    if (hfName.isEmpty()) {
        lastErrorMessage = "history file not set";
        return false;
    }

    QFile fl(hfName);
    if (!fl.exists()) {
       lastErrorMessage = QString("history file not found (%1)").arg(hfName) ;
       return false;
    }

    IMHistoryReader hreader;
    if (!hreader.read(hitems, hfName, lasthiid)) {
        lastErrorMessage = hreader.errorMessage();
        return false;
    }

    qSort(hitems.begin(), hitems.end());

    qDebug() << "  IMHistoryKeeper::loadHistoryFile finished";

    historyChanged = false;

    return true;
}

//=============================================================================

QString IMHistoryKeeper::errorMessage()
{
    QString errorMessage = lastErrorMessage;
    lastErrorMessage.clear();
    return errorMessage;
}

//=============================================================================

bool IMHistoryKeeper::getMessages(QList<IMHistoryItem> &historyItems, const int messagesCount)
{
    int messFound = 0;

    historyItems.clear();

    QListIterator<IMHistoryItem> hii(hitems);
    hii.toBack();
    while (hii.hasPrevious()) {
        IMHistoryItem hitem = hii.previous();

        historyItems.insert(historyItems.begin(), hitem);
        messFound++;
        if (messagesCount && messFound >= messagesCount) {
            break;
        }
    }

    return true; // successful end
}

//=============================================================================

bool IMHistoryKeeper::getMessage(int hiid, IMHistoryItem &item)
{
    QList<IMHistoryItem>::iterator it;
    for (it = hitems.begin(); it != hitems.end(); it++) {
        if (it->hiid == hiid) {
            item = *it;
            return true;
        }
    }

    return false;
}

//=============================================================================

void IMHistoryKeeper::clear()
{
    hitems.clear();
    historyChanged = true;

    lasthiid = 0;

    emit historyClear();
}

//=============================================================================

/*static*/ bool IMHistoryKeeper::compareItem(const IMHistoryItem &item, bool incoming, const std::string &id, const QDateTime &sendTime, const QString &messageText)
{
    // "\n" is not saved in xml
    QString copyMessage1(messageText);
    copyMessage1.replace("\n", "");

    QString copyMessage2(item.messageText);
    copyMessage2.replace("\n", "");

    if (item.incoming == incoming && item.id == id && item.sendTime == sendTime && copyMessage1 == copyMessage2) {
        return true;
    }

    return false;
}

//=============================================================================

bool IMHistoryKeeper::findMessage(bool incoming, const std::string &id, const QDateTime &sendTime, const QString &messageText, int &hiid)
{
    hiid = 0;

    QList<IMHistoryItem>::const_iterator it;
    for (it = hitems.begin(); it != hitems.end(); it++) {
        if (compareItem(*it, incoming, id, sendTime, messageText)) {
            hiid = it->hiid;
            return true;
        }
    }

    return false;
}

//=============================================================================

void IMHistoryKeeper::removeMessage(int hiid)
{
    QList<IMHistoryItem>::iterator it;
    for (it = hitems.begin(); it != hitems.end(); it++) {
        if (it->hiid == hiid) {
            emit historyRemove(*it);
            hitems.erase(it);
            historyChanged = true;
            break;
        }
    }
}

//=============================================================================

void IMHistoryKeeper::removeMessages(QList<int> &hiids)
{
    bool changed = false;

    QList<IMHistoryItem>::iterator it = hitems.begin();
    while (it != hitems.end()) {
        if (qFind(hiids, it->hiid) != hiids.end()) {
            emit historyRemove(*it);

            it = hitems.erase(it);

            changed = true;

            continue;
        }
        it++;
    }

    if (changed) {
        historyChanged = true;
    }
}

void IMHistoryKeeper::clearHistory(){

    IMHistoryWriter wri;
    if(!wri.remove(hfName))
        std::cerr << "\nFailed to remove history file" << std::endl;

}

//=============================================================================

void IMHistoryKeeper::saveHistory()
{
    if (historyChanged && hfName.isEmpty() == false) {
        //=== we have to save all messages
        qSort( hitems.begin(), hitems.end() ) ; // not nesessary, but just in case...
                                                // it will not take a long time over ordered array

        IMHistoryWriter wri;
        wri.write(hitems, hfName);

        historyChanged = false;
    }
}
