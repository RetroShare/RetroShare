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
#include <QMetaType>

#include "HandleRichText.h"

/* Flags for ChatStyle::formatMessage */
#define CHAT_FORMATMSG_EMBED_SMILEYS    1
#define CHAT_FORMATMSG_EMBED_LINKS      2

/* Flags for ChatStyle::formatText */
#define CHAT_FORMATTEXT_EMBED_SMILEYS  1
#define CHAT_FORMATTEXT_EMBED_LINKS    2

#define FORMATMSG_COUNT  5

class ChatStyleInfo
{
public:
    ChatStyleInfo() {}

public:
    QString stylePath;
    QString styleName;
    QString styleDescription;
    QString authorName;
    QString authorEmail;
};

Q_DECLARE_METATYPE(ChatStyleInfo)

class ChatStyle : public QObject
{
    Q_OBJECT

public:
    enum enumFormatMessage
    {
        FORMATMSG_INCOMING = 0,
        FORMATMSG_OUTGOING = 1,
        FORMATMSG_HINCOMING = 2,
        FORMATMSG_HOUTGOING = 3,
        FORMATMSG_OOUTGOING = 4,
        // FORMATMSG_COUNT
    };

    enum enumStyleType
    {
        TYPE_UNKNOWN,
        TYPE_PUBLIC,
        TYPE_PRIVATE,
        TYPE_HISTORY
    };

public:
    /* Default constructor */
    ChatStyle();
    /* Default destructor */
    ~ChatStyle();

    bool setStylePath(const QString &stylePath, const QString &styleVariant);
    bool setStyleFromSettings(enumStyleType styleType);

    QString formatMessage(enumFormatMessage type, const QString &name, const QDateTime &timestamp, const QString &message, unsigned int flag);
    QString formatText(const QString &message, unsigned int flag);

    static bool getAvailableStyles(enumStyleType styleType, QList<ChatStyleInfo> &styles);
    static bool getAvailableVariants(const QString &stylePath, QStringList &variants);

private slots:
    void styleChanged(int styleType);

private:
    enumStyleType m_styleType;
    QDir m_styleDir;
    QString m_styleVariant;

    QString m_style[FORMATMSG_COUNT];

    /** store default information for embedding HTML */
    RsChat::EmbedInHtmlAhref defEmbedAhref;
};

#endif // _CHATSTYLE_H
