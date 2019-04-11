/*******************************************************************************
 * gui/settings/RsharePeerSettings.cpp                                         *
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

#include <QColor>
#include <QFont>
#include <QDateTime>
#include <QWidget>

#include <string>
#include <list>

#include <retroshare/rsinit.h>
#include <retroshare/rspeers.h>

#include "RsharePeerSettings.h"
#include "rsharesettings.h"
#include "gui/style/RSStyle.h"

/** The file in which all settings of he peers will read and written. */
#define SETTINGS_FILE   (QString::fromUtf8(RsAccounts::AccountDirectory().c_str()) + "/RSPeers.conf")

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
        for (QStringList::iterator group = groups.begin(); group != groups.end(); ++group) {
            if (*group == GROUP_GENERAL) {
                continue;
            }

            // TODO: implement cleanup for chatlobbies and distant chat

            ChatId chatId((*group).toStdString());
            // remove if not a chat id and pgp id was removed from friendslist
            if(chatId.isNotSet() && rsPeers->isGPGAccepted(RsPgpId((*group).toStdString())) == false) {
                remove(*group);
            }
        }

        beginGroup(GROUP_GENERAL);
        setValue("lastClean", currentDate);
        endGroup();
    }
}

bool RsharePeerSettings::getSettingsIdOfPeerId(const ChatId& chatId, std::string &settingsId)
{
    if(chatId.isPeerId())
    {
        RsPeerId peerId = chatId.toPeerId();
        // for ssl id, get pgp id
        // check if pgp id is cached
        std::map<RsPeerId, std::string>::iterator it = m_SslToGpg.find(peerId);
        if (it != m_SslToGpg.end()) {
            settingsId = it->second;
            return true;
        }
        // if not fetch and store it
        RsPeerDetails details;
        if (rsPeers->getPeerDetails(peerId, details) == false) {
            return false;
        }
        settingsId = details.gpg_id.toStdString();
        m_SslToGpg[peerId] = settingsId ;
        return true;
    }
    if(chatId.isDistantChatId() || chatId.isLobbyId() || chatId.isBroadcast())
    {
        settingsId = chatId.toStdString();
        return true;
    }
    return false;
}

/* get value of peer */
QVariant RsharePeerSettings::get(const ChatId& chatId, const QString &key, const QVariant &defaultValue)
{
    QVariant result;

    std::string settingsId;
    if (getSettingsIdOfPeerId(chatId, settingsId) == false) {
        /* settings id not found */
        return result;
    }

    beginGroup(QString::fromStdString(settingsId));
    result = value(key, defaultValue);
    endGroup();

    return result;
}

/* set value of peer */
void RsharePeerSettings::set(const ChatId& chatId, const QString &key, const QVariant &value)
{
    std::string settingsId;
    if (getSettingsIdOfPeerId(chatId, settingsId) == false) {
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

QString RsharePeerSettings::getPrivateChatColor(const ChatId& chatId)
{
    return get(chatId, "PrivateChatColor", QColor(Qt::black).name()).toString();
}

void RsharePeerSettings::setPrivateChatColor(const ChatId& chatId, const QString &value)
{
    set(chatId, "PrivateChatColor", value);
}

QString RsharePeerSettings::getPrivateChatFont(const ChatId& chatId)
{
    return get(chatId, "PrivateChatFont", Settings->getChatScreenFont()).toString();
}

void RsharePeerSettings::setPrivateChatFont(const ChatId& chatId, const QString &value)
{
    if (Settings->getChatScreenFont() == value) {
        set(chatId, "PrivateChatFont", QVariant());
    } else {
        set(chatId, "PrivateChatFont", value);
    }
}

bool RsharePeerSettings::getPrivateChatOnTop(const ChatId& chatId)
{
    return get(chatId, "PrivateChatOnTop", false).toBool();
}

void RsharePeerSettings::setPrivateChatOnTop(const ChatId& chatId, bool value)
{
    set(chatId, "PrivateChatOnTop", value);
}

void RsharePeerSettings::saveWidgetInformation(const ChatId& chatId, QWidget *widget)
{
    std::string settingsId;
    if (getSettingsIdOfPeerId(chatId, settingsId) == false) {
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

void RsharePeerSettings::loadWidgetInformation(const ChatId& chatId, QWidget *widget)
{
    std::string settingsId;
    if (getSettingsIdOfPeerId(chatId, settingsId) == false) {
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

bool RsharePeerSettings::getShowAvatarFrame(const ChatId& chatId)
{
    return get(chatId, "ShowAvatarFrame", true).toBool();
}

void RsharePeerSettings::setShowAvatarFrame(const ChatId& chatId, bool value)
{
    return set(chatId, "ShowAvatarFrame", value);
}

void RsharePeerSettings::getStyle(const ChatId& chatId, const QString &name, RSStyle &style)
{
    std::string settingsId;
    if (getSettingsIdOfPeerId(chatId, settingsId) == false) {
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

void RsharePeerSettings::setStyle(const ChatId& chatId, const QString &name, RSStyle &style)
{
    std::string settingsId;
    if (getSettingsIdOfPeerId(chatId, settingsId) == false) {
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
