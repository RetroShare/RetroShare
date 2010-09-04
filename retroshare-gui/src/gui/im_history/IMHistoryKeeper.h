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

#include "IMHistoryItem.h"

class QTimer;

//! An engine for instant messaging history management

//! This class holds history for instant messages. It stores all messages
//! in xml file. Something like
//! <?xml version="1.0" encoding="UTF-8"?>
//! <!DOCTYPE history_file>
//! <history_file format_version="1.0">
//!   <message sendTime="10000" id="id" name="Name">manual message</message>
//!   ...
//!   other messages in <message..> ... </message> tags
//!   ...
//! </history_file>
//! 
//! The class loads all messages from the file after creation, and saves them
//! at destruction. This means, the more history user has, the more memory
//! will be used. Maybe it's not good, but it isn't so large, I think 

class IMHistoryKeeper : public QObject
{
    Q_OBJECT

public:
    IMHistoryKeeper();
    
    //! A destructor

    //! Warning: history messages will be saved to the file here. This means,
    //! a IMHistoryKeeper object must be deleted properly.
    virtual ~IMHistoryKeeper();

    //! last error description
    QString errorMessage();

    //! initialize history keeper
    void init(QString historyFileName);

    //! Select messages from history

    //! Fills given list with items
    bool getMessages(QList<IMHistoryItem> &historyItems, const int messagesCount);

    //! Adds new message to the history

    //! Adds new message to the history, but the message will be saved to
    //! file only after destroing the object
    void addMessage(bool incoming, std::string &id, const QString &name, const QDateTime &sendTime, const QString &messageText);

    //! Clear the history
    void clear();

private:
    bool loadHistoryFile();

    QList<IMHistoryItem> hitems;
    QString hfName ; //! history file name
    bool historyChanged;
    QString lastErrorMessage;
    QTimer *saveTimer;

private slots:
    void saveHistory();

signals:
    void historyAdd(IMHistoryItem item) const;
    void historyClear() const;
};

#endif //  _HISTORY_KEEPER_H_
