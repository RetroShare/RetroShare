/*******************************************************************************
 * lang/languagesupport.cpp                                                    *
 *                                                                             *
 * Copyright (c) 2006-2007, crypton    <retroshare.project@gmail.com>          *
 * Copyright (c) 2006, Matt Edman, Justin Hipple                               *
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

#ifndef _LANGUAGESUPPORT_H
#define _LANGUAGESUPPORT_H

#include <QApplication>
#include <QStringList>
#include <QMap>

class LanguageSupport
{
public:
  /** Returns the default language code for the system locale. */
  static QString defaultLanguageCode();
  /** Returns the language code for a given language name. */
  static QString languageCode(const QString &languageName);
  /** Returns a list of all supported language codes (e.g., "en"). */
  static QStringList languageCodes();
  /** Returns the language name for a given language code. */
  static QString languageName(const QString &languageCode);
  /** Returns a list of all supported language names (e.g., "English"). */
  static QStringList languageNames();
  /** Returns a list of all supported language codes and names. */
  static QMap<QString, QString> languages();
  /** Returns a list of all supported language codes and locales. */
  static QMap<QString, QLocale> locales();
  /** Returns true if we understand the given language code. */
  static bool isValidLanguageCode(const QString &languageCode);
  /** Returns true if <b>languageCode</b> requires a right-to-left layout. */
  static bool isRightToLeft(const QString &languageCode);
  /** Sets the application's translator to the specified language. */
  static bool translate(const QString &languageCode);
  /** Sets the application's translator to the specified language for the plugins. */
  static bool translatePlugins(const QString &languageCode);
  /** Sets the application's locale according to the specified language. */
  static bool localize(const QString &languageCode);
};

#endif
