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

#include <QColor>
#include <QFont>
#include <QDateTime>
#include <QWidget>

#include <string>
#include <list>

#include <retroshare/rsinit.h>
#include <retroshare/rspeers.h>
#include <retroshare/rsmsgs.h>

#include "RsharePeerSettings.h"
#include "rsharesettings.h"
#include "gui/style/RSStyle.h"

/** The file in which all settings of he peers will read and written. */
#define SETTINGS_FILE   (QString::fromUtf8(RsAccounts::ConfigDirectory().c_str()) + "/RSPeers.conf")

/* clean dead id's after these days */
#define DAYS_TO_CLEAN   7

/* Group for general data */
#define GROUP_GENERAL   "Default"

/* the one and only global settings object */
RsharePeerSettings *PeerSettings = NULL;

/*static*/ void RsharePeerSettings::Create ()
{
    if (PeerSettings == NULL) {
        PeerSettings = new RsharePeerSettings();
    }
}

/** Default Constructor */
RsharePeerSettings::RsharePeerSettings()
    : QSettings(SETTINGS_FILE, QSettings::IniFormat)
{
    cleanDeadIds();
}

void RsharePeerSettings::cleanDeadIds()
{
    beginGroup(GROUP_GENERAL);
    QDateTime lastClean = value("lastClean").toDateTime();
    endGroup();

    QDateTime currentDate = QDateTime::currentDateTime();

    if (lastClean.addDays(DAYS_TO_CLEAN) < currentDate) {
        /* clean */
        QStringList groups = childGroups();
        for (QStringList::iterator group = groups.begin(); group != groups.end(); group++) {
            if (*group == GROUP_GENERAL) {
                continue;
            }

            ChatLobbyId lid;
            if (rsMsgs->isLobbyId((*group).toStdString(), lid)) {
                continue;
            }
            if (rsPeers->isGPGAccepted((*group).toStdString()) == false) {
                remove(*group);
            }
        }

        beginGroup(GROUP_GENERAL);
        setValue("lastClean", currentDate);
        endGroup();
    }
}

bool RsharePeerSettings::getSettingsIdOfPeerId(const std::string &peerId, std::string &settingsId)
{
    ChatLobbyId lid;
    if (rsMsgs->isLobbyId(peerId, lid)) {
        settingsId = peerId;
        return true;
    }

    std::map<std::string, std::string>::iterator it = m_SslToGpg.find(peerId);
    if (it != m_SslToGpg.end()) {
        settingsId = it->second;
        return true;
    }

    RsPeerDetails details;
    if (rsPeers->getPeerDetails(peerId, details) == false) {
        return false;
    }

    settingsId = details.gpg_id;
    m_SslToGpg[peerId] = settingsId;

    return true;
}

/* get value of peer */
QVariant RsharePeerSettings::get(const std::string &peerId, const QString &key, const QVariant &defaultValue)
{
    QVariant result;

    std::string settingsId;
    if (getSettingsIdOfPeerId(peerId, settingsId) == false) {
        /* settings id not found */
        return result;
    }

    beginGroup(QString::fromStdString(settingsId));
    result = value(key, defaultValue);
    endGroup();

    return result;
}

/* set value of peer */
void RsharePeerSettings::set(const std::string &peerId, const QString &key, const QVariant &value)
{
    std::string settingsId;
    if (getSettingsIdOfPeerId(peerId, settingsId) == false) {
        /* settings id not found */
        return;
    }

    beginGroup(QString::fromStdString(settingsId));
    if (value.isNull()) {
        remove(key);
    } else {
        setValue(key, value);
    }
    endGroup();
}

QString RsharePeerSettings::getPrivateChatColor(const std::string &peerId)
{
    return get(peerId, "PrivateChatColor", QColor(Qt::black).name()).toString();
}

void RsharePeerSettings::setPrivateChatColor(const std::string &peerId, const QString &value)
{
    set(peerId, "PrivateChatColor", value);
}

QString RsharePeerSettings::getPrivateChatFont(const std::string &peerId)
{
    return get(peerId, "PrivateChatFont", Settings->getChatScreenFont()).toString();
}

void RsharePeerSettings::setPrivateChatFont(const std::string &peerId, const QString &value)
{
    if (Settings->getChatScreenFont() == value) {
        set(peerId, "PrivateChatFont", QVariant());
    } else {
        set(peerId, "PrivateChatFont", value);
    }
}

bool RsharePeerSettings::getPrivateChatOnTop(const std::string &peerId)
{
    return get(peerId, "PrivateChatOnTop", false).toBool();
}

void RsharePeerSettings::setPrivateChatOnTop(const std::string &peerId, bool value)
{
    set(peerId, "PrivateChatOnTop", value);
}

void RsharePeerSettings::saveWidgetInformation(const std::string &peerId, QWidget *widget)
{
    std::string settingsId;
    if (getSettingsIdOfPeerId(peerId, settingsId) == false) {
        /* settings id not found */
        return;
    }

    beginGroup(QString::fromStdString(settingsId));
    beginGroup("widgetInformation");
    beginGroup(widget->objectName());

    setValue("size", widget->size());
    setValue("pos", widget->pos());

    endGroup();
    endGroup();
    endGroup();
}

void RsharePeerSettings::loadWidgetInformation(const std::string &peerId, QWidget *widget)
{
    std::string settingsId;
    if (getSettingsIdOfPeerId(peerId, settingsId) == false) {
        /* settings id not found */
        return;
    }

    beginGroup(QString::fromStdString(settingsId));
    beginGroup("widgetInformation");
    beginGroup(widget->objectName());

    widget->resize(value("size", widget->size()).toSize());
    widget->move(value("pos", QPoint(200, 200)).toPoint());

    endGroup();
    endGroup();
    endGroup();
}

bool RsharePeerSettings::getShowAvatarFrame(const std::string &peerId)
{
    return get(peerId, "ShowAvatarFrame", true).toBool();
}

void RsharePeerSettings::setShowAvatarFrame(const std::string &peerId, bool value)
{
    return set(peerId, "ShowAvatarFrame", value);
}

bool RsharePeerSettings::getShowParticipantsFrame(const std::string &peerId)
{
    return get(peerId, "ShowParticipantsFrame", true).toBool();
}

void RsharePeerSettings::setShowParticipantsFrame(const std::string &peerId, bool value)
{
    return set(peerId, "ShowParticipantsFrame", value);
}

void RsharePeerSettings::getStyle(const std::string &peerId, const QString &name, RSStyle &style)
{
    std::string settingsId;
    if (getSettingsIdOfPeerId(peerId, settingsId) == false) {
        /* settings id not found */
        return;
    }

    beginGroup(QString::fromStdString(settingsId));
    beginGroup("style");
    beginGroup(name);

    style.readSetting(*this);

    endGroup();
    endGroup();
    endGroup();
}

void RsharePeerSettings::setStyle(const std::string &peerId, const QString &name, RSStyle &style)
{
    std::string settingsId;
    if (getSettingsIdOfPeerId(peerId, settingsId) == false) {
        /* settings id not found */
        return;
    }

    beginGroup(QString::fromStdString(settingsId));
    beginGroup("style");
    beginGroup(name);

    style.writeSetting(*this);

    endGroup();
    endGroup();
    endGroup();
}
