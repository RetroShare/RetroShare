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

#ifndef _HISTORY_KEEPER_H_
#define _HISTORY_KEEPER_H_

#include <QObject>
#include <QDebug>
#include <QString>
#include <QStringList>

#include "IMHistoryItem.h"

//! An engine for instant messaging history management

//!     This class holds history for instant messages. It stores all messages
//! in xml file. Something like
//! <?xml version="1.0" encoding="UTF-8"?>
//! <!DOCTYPE history_file>
//! <history_file format_version="1.0">
//!   <message dt="10000" sender="ALL" receiver="THIS">manual message</message>
//!   ...
//!   other messages in <message..> ... </message> tags
//!   ...
//! </history_file>
//! 
//! The class loads all messages from the file after creation, and saves them
//! at destruction. This means, the more history user has, the more memory
//! will be used. Maybe it's not good, but it isn't so large, I think 
class IMHistoryKeeper//: public QObject
{
//    Q_OBJECT
public:
    IMHistoryKeeper();
    
    IMHistoryKeeper(QString historyFileName);

    //! A destructor

    //! Warning: history messages will be saved to the file here. This means,
    //! a IMHistoryKeeper object must be deleted properly.
    virtual ~IMHistoryKeeper();
    
    //! last error description
    QString errorMessage();

    //! Select messages from history

    //! Fills given list with html-decorated messages (see formStringList(..))
    //! Takes no more then messageText messages from history, where messages
    //! were sent from fromID to toID, or from toID to fromID. Also, if
    //! fromID id ""(empty string), all messages, sent to toID, will be
    //! extracted
    int getMessages(QStringList& messagesList,
                    const QString fromID, const QString toID,
                    const int messagesCount = 5);

    //! Adds new message to the history

    //! Adds new message to the history, but the message will be saved to
    //! file only after destroing the object
    void addMessage(const QString fromID, const QString toID,
                    const QString messageText);
            
protected:
    int loadHistoryFile(QString fileName);
    void formStringList(QList<IMHistoryItem>& itemList, QStringList& strList);

    QList<IMHistoryItem> hitems;
    QString hfName ; //! history file name
    QString lastErrorMessage;

};

#endif //  _HISTORY_KEEPER_H_
