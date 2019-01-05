/*******************************************************************************
 * gui/settings/rsettings.h                                                    *
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

#ifndef _RSETTINGS_H
#define _RSETTINGS_H

#include <QHash>
#include <QSettings>


class RSettings : public QSettings
{
  Q_OBJECT

public:
  /** Default constructor. The optional parameter <b>group</b> can be used to
   * set a prefix that will be prepended to keys specified to RSettings in
   * value() and setValue(). */
  RSettings(const QString group = QString());

    /** Default constructor. The optional parameter <b>group</b> can be used to
   * set a prefix that will be prepended to keys specified to RSettings in
   * value() and setValue(). */
  RSettings(const std::string &fileName, const QString group = QString());

  /** Resets all settings. */
  static void reset();

  /** Returns the saved value associated with <b>key</b>. If no value has been
   * set, the default value is returned.
   * \sa setDefault
   */
  virtual QVariant value(const QString &key,
                         const QVariant &defaultVal = QVariant()) const;
  /** Sets the value associated with <b>key</b> to <b>val</b>. */
  virtual void setValue(const QString &key, const QVariant &val);

  virtual QVariant valueFromGroup(const QString &group, const QString &key,
                         const QVariant &defaultVal = QVariant());
  virtual void setValueToGroup(const QString &group, const QString &key, const QVariant &val);

protected:
  bool m_bValid; // is valid - dependent on RsInit::getPreferedAccountId

  /** Sets the default setting for <b>key</b> to <b>val</b>. */
  void setDefault(const QString &key, const QVariant &val);
  /** Returns the default setting value associated with <b>key</b>. If
   * <b>key</b> has no default value, then an empty QVariant is returned. */
  QVariant defaultValue(const QString &key) const;
  /** Returns a map of all currently saved settings at the last apply()
   * point. */
  //QMap<QString, QVariant> allSettings() const;

private:
  /** Association of setting key names to default setting values. */
  QHash<QString, QVariant> _defaults; 
};

#endif

