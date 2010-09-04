/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006, Thunder
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


#ifndef _CHATSTYLE_H
#define _CHATSTYLE_H

#include <QString>
#include <QDateTime>
#include <QHash>

#include "HandleRichText.h"

/* Flags for ChatStyle::formatMessage */
#define CHAT_FORMATMSG_EMBED_SMILEYS    1
#define CHAT_FORMATMSG_EMBED_LINKS      2

/* Flags for ChatStyle::formatText */
#define CHAT_FORMATTEXT_EMBED_SMILEYS  1
#define CHAT_FORMATTEXT_EMBED_LINKS    2

class ChatStyle
{
public:
    enum enumFormatMessage
    {
        FORMATMSG_INCOMING,
        FORMATMSG_OUTGOING,
        FORMATMSG_HINCOMING,
        FORMATMSG_HOUTGOING
    };

    QHash<QString, QString> smileys;

public:
    /* Default constructor */
    ChatStyle();
    /* Default destructor */
    ~ChatStyle();

    void setStylePath(QString path);
    void loadEmoticons();

    QString formatMessage(enumFormatMessage type, QString &name, QDateTime &timestamp, QString &message, unsigned int flag);
    QString formatText(QString &message, unsigned int flag);

    void showSmileyWidget(QWidget *parent, QWidget *button, const char *slotAddMethod);

private:
    QString stylePath;

    /** store default information for embedding HTML */
    RsChat::EmbedInHtmlAhref defEmbedAhref;
    RsChat::EmbedInHtmlImg defEmbedImg;
};

#endif // _CHATSTYLE_H
