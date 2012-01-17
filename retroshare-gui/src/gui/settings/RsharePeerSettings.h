/****************************************************************
 * This file is distributed under the following license:
 *
 *  Copyright (C) 2006 - 2010, RetroShare Team
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

#ifndef _RSHAREPEERSETTINGS_H
#define _RSHAREPEERSETTINGS_H

#include <QSettings>

class RSStyle;

/** Handles saving and restoring RShares's settings for peers */
class RsharePeerSettings : public QSettings
{
public:
    /* create settings object */
    static void Create ();

    QString getPrivateChatColor(const std::string &peerId);
    void    setPrivateChatColor(const std::string &peerId, const QString &value);

    QString getPrivateChatFont(const std::string &peerId);
    void    setPrivateChatFont(const std::string &peerId, const QString &value);

    bool    getPrivateChatOnTop(const std::string &peerId);
    void    setPrivateChatOnTop(const std::string &peerId, bool value);

    void    saveWidgetInformation(const std::string &peerId, QWidget *widget);
    void    loadWidgetInformation(const std::string &peerId, QWidget *widget);

    bool    getShowAvatarFrame(const std::string &peerId);
    void    setShowAvatarFrame(const std::string &peerId, bool value);

    bool    getShowParticipantsFrame(const std::string &peerId);
    void    setShowParticipantsFrame(const std::string &peerId, bool value);

    void    getStyle(const std::string &peerId, const QString &name, RSStyle &style);
    void    setStyle(const std::string &peerId, const QString &name, RSStyle &style);

protected:
    /** Default constructor. */
    RsharePeerSettings();

    bool getSettingsIdOfPeerId(const std::string &peerId, std::string &settingsId);
    void cleanDeadIds();

    /* get value of peer */
    QVariant get(const std::string &peerId, const QString &key, const QVariant &defaultValue = QVariant());
    /* set value of peer */
    void     set(const std::string &peerId, const QString &key, const QVariant &value);

    /* map for fast access of the gpg id to the ssl id */
    std::map<std::string, std::string> m_SslToGpg;
};

// the one and only global settings object
extern RsharePeerSettings *PeerSettings;

#endif
