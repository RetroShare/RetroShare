/*******************************************************************************
 * gui/chat/ChatStyle.h                                                        *
 *                                                                             *
 * LibResAPI: API for local socket server                                      *
 *                                                                             *
 * Copyright (C) 2006, Thunder                                                 *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#ifndef _CHATSTYLE_H
#define _CHATSTYLE_H

#include <QColor>
#include <QString>
#include <QDateTime>
#include <QHash>
#include <QMetaType>
#include <QDir>

/* Flags for ChatStyle::formatMessage */
#define CHAT_FORMATMSG_SYSTEM          1

#define FORMATMSG_COUNT  6

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
        FORMATMSG_SYSTEM = 5,
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

    QString formatMessage(enumFormatMessage type, const QString &name, const QDateTime &timestamp, const QString &message, unsigned int flag = 0, const QColor &backgroundColor = Qt::white);

    static bool getAvailableStyles(enumStyleType styleType, QList<ChatStyleInfo> &styles);
    static bool getAvailableVariants(const QString &stylePath, QStringList &variants);

private slots:
    void styleChanged(int styleType);

private:
    enumStyleType m_styleType;
    QDir m_styleDir;
    QString m_styleVariant;

    QString m_style[FORMATMSG_COUNT];
};

#endif // _CHATSTYLE_H
