/*******************************************************************************
 * gui/settings/RsharePeerSettings.h                                           *
 *                                                                             *
 * Copyright (c) 2010, Retroshare Team <retroshare.project@gmail.com>          *
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

#ifndef _RSHAREPEERSETTINGS_H
#define _RSHAREPEERSETTINGS_H

#include <QSettings>
#include <retroshare/rstypes.h>
#include <retroshare/rsmsgs.h>

class RSStyle;

/** Handles saving and restoring RShares's settings for peers */
class RsharePeerSettings : public QSettings
{
public:
    /* create settings object */
    static void Create ();

    QString getPrivateChatColor(const ChatId& chatId);
    void    setPrivateChatColor(const ChatId& chatId, const QString &value);

    QString getPrivateChatFont(const ChatId& chatId);
    void    setPrivateChatFont(const ChatId& chatId, const QString &value);

    bool    getPrivateChatOnTop(const ChatId& chatId);
    void    setPrivateChatOnTop(const ChatId& chatId, bool value);

    void    saveWidgetInformation(const ChatId& chatId, QWidget *widget);
    void    loadWidgetInformation(const ChatId& chatId, QWidget *widget);

    bool    getShowAvatarFrame(const ChatId& chatId);
    void    setShowAvatarFrame(const ChatId& chatId, bool value);

    void    getStyle(const ChatId& chatId, const QString &name, RSStyle &style);
    void    setStyle(const ChatId& chatId, const QString &name, RSStyle &style);

protected:
    /** Default constructor. */
    RsharePeerSettings();

    bool getSettingsIdOfPeerId(const ChatId& chatId, std::string &settingsId);
    void cleanDeadIds();

    /* get value of peer */
    QVariant get(const ChatId& chatId, const QString &key, const QVariant &defaultValue = QVariant());
    /* set value of peer */
    void     set(const ChatId& chatId, const QString &key, const QVariant &value);

    /* map for fast access of the gpg id to the ssl id */
    std::map<RsPeerId, std::string> m_SslToGpg;
};

// the one and only global settings object
extern RsharePeerSettings *PeerSettings;

#endif
