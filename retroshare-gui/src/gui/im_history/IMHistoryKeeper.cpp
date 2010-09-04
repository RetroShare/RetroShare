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
};

//=============================================================================

IMHistoryKeeper::~IMHistoryKeeper()
{
    saveHistory();
}

//=============================================================================

void IMHistoryKeeper::init(QString historyFileName)
{
    hfName = historyFileName;
    loadHistoryFile();
}

//=============================================================================

void IMHistoryKeeper::addMessage(bool incoming, std::string &id, const QString &name, const QDateTime &sendTime, const QString &messageText)
{
    IMHistoryItem item(incoming, id, name, sendTime, messageText);

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
    if (!hreader.read(hitems, hfName)) {
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

void IMHistoryKeeper::clear()
{
    hitems.clear();
    historyChanged = true;

    emit historyClear();
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
    }
}
