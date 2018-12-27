/*******************************************************************************
 * gui/settings/rsettings.cpp                                                  *
 *                                                                             *
 * Copyright (c) 2008, crypton                                                 *
 * Copyright (c) 2008, Matt Edman, Justin Hipple                               *
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

#include <rshare.h>
#include <iostream>

#include "rsettings.h"
#include <retroshare/rsinit.h>

/** The file in which all settings will read and written. */
#define SETTINGS_FILE   (QString::fromUtf8(RsAccounts::AccountDirectory().c_str()) + "/RetroShare.conf")

/** Constructor */
RSettings::RSettings(const QString settingsGroup)
: QSettings(SETTINGS_FILE, QSettings::IniFormat)
{
    RsPeerId sPreferedId;
    m_bValid = RsAccounts::GetPreferredAccountId(sPreferedId);

    if (!settingsGroup.isEmpty())
        beginGroup(settingsGroup);
}

RSettings::RSettings(const std::string &fileName, const QString settingsGroup)
: QSettings(QString::fromStdString(fileName), QSettings::IniFormat)
{
  m_bValid = true;
  if (!settingsGroup.isEmpty())
    beginGroup(settingsGroup);
}

/** Returns the saved value associated with <b>key</b>. If no value has been
 * set, the default value is returned.
 * \sa setDefault
 */
QVariant
RSettings::value(const QString &key, const QVariant &defaultVal) const
{
    if (m_bValid == false) {
        // return only default value
        return defaultVal.isNull() ? defaultValue(key) : defaultVal;
    }
    return QSettings::value(key, defaultVal.isNull() ? defaultValue(key) : defaultVal);
}

/** Sets the value associated with <b>key</b> to <b>val</b>. */
void
RSettings::setValue(const QString &key, const QVariant &val)
{
    if (m_bValid == false) {
        std::cerr << "RSettings::setValue() Calling on invalid object, key = " << key.toStdString() << std::endl;
        return;
    }
    if (val == defaultValue(key))
        QSettings::remove(key);
    else if (val != value(key))
        QSettings::setValue(key, val);
}

QVariant RSettings::valueFromGroup(const QString &group, const QString &key, const QVariant &defaultVal)
{
    beginGroup(group);
    QVariant val = value(key, defaultVal);
    endGroup();

    return val;
}

void RSettings::setValueToGroup(const QString &group, const QString &key, const QVariant &val)
{
    beginGroup(group);
    setValue(key, val);
    endGroup();
}

/** Sets the default setting for <b>key</b> to <b>val</b>. */
void
RSettings::setDefault(const QString &key, const QVariant &val)
{
  _defaults.insert(key, val);
}

/** Returns the default setting value associated with <b>key</b>. If
 * <b>key</b> has no default value, then an empty QVariant is returned. */
QVariant
RSettings::defaultValue(const QString &key) const
{
  if (_defaults.contains(key))
    return _defaults.value(key);
  return QVariant();
}

/** Resets all settings. */
void
RSettings::reset()
{
  /* Static method, so we have to create a QSettings object. */
  QSettings settings(SETTINGS_FILE, QSettings::IniFormat);
  settings.clear();
}

/** Returns a map of all currently saved settings at the last appyl() point. */
/*QMap<QString, QVariant>
RSettings::allSettings() const
{
  QMap<QString, QVariant> settings;
  foreach (QString key, allKeys()) {
    settings.insert(key, value(key));
  }
  return settings;
}*/

