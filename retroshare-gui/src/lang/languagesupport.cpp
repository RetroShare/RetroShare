/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (c) 2006-2007, crypton
 * Copyright (c) 2006, Matt Edman, Justin Hipple
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

#include <QTranslator>
#include <QLocale>
#include <QLibraryInfo>
#include <rshare.h>

#include "languagesupport.h"


/** Initializes the list of available languages. */
QMap<QString, QString>
LanguageSupport::languages()
{
  static QMap<QString, QString> languages;
  if (languages.isEmpty()) {
    languages.insert("af",    "Afrikaans");
    languages.insert("bg",    "Bulgarien");
    languages.insert("cy",    "Welsh");
    languages.insert("de",    "Deutsch");
    languages.insert("da",    "Danish");
    languages.insert("en",    "English");
    languages.insert("es",    QString::fromUtf8("spanish"));
    languages.insert("fr",    QString::fromUtf8("Fran\303\247ais"));
    languages.insert("fi",    "suomi");
    languages.insert("gr",    "Greek");
    languages.insert("it",    "Italiano");
    languages.insert("ja_JP",    QString::fromUtf8("\346\227\245\346\234\254\350\252\236"));
    languages.insert("ko",    "Korean");
    languages.insert("pl",    "Polish");
    languages.insert("pt",    "Portuguese");
    languages.insert("ru",    QString::fromUtf8("\320\240\321\203\321\201\321\201\320\272\320\270\320\271"));
    languages.insert("sl",    "slovenian");
    languages.insert("sr",    "Serbian");
    languages.insert("sv",    "svenska");     
    languages.insert("tr",    QString::fromUtf8("T\303\274rk\303\247e"));
    languages.insert("zh_CN", QString::fromUtf8("\347\256\200\344\275\223\345\255\227"));
    languages.insert("zh_TW", QString::fromUtf8("\347\260\241\351\253\224\345\255\227"));
  }
  return languages;
}

/** Returns the default language code for the system locale. */
QString
LanguageSupport::defaultLanguageCode()
{
  QString language = QLocale::system().name();

  if (language != "zh_CN" && language != "zh_TW")
    language = language.mid(0, language.indexOf("_"));
  if (!isValidLanguageCode(language))
    language = "en";
  
  return language;
}

/** Returns the language code for a given language name. */
QString
LanguageSupport::languageCode(const QString &languageName)
{
  return languages().key(languageName);
}

/** Returns a list of all supported language codes. (e.g., "en"). */
QStringList
LanguageSupport::languageCodes()
{
  return languages().keys();
}

/** Returns the language name for a given language code. */
QString
LanguageSupport::languageName(const QString &languageCode)
{
  return languages().value(languageCode);
}

/** Returns a list of all supported language names (e.g., "English"). */
QStringList
LanguageSupport::languageNames()
{
  return languages().values();
}

/** Returns true if we understand the given language code. */
bool
LanguageSupport::isValidLanguageCode(const QString &languageCode)
{
  return languageCodes().contains(languageCode);
}

/** Returns true if <b>languageCode</b> requires a right-to-left layout. */
bool
LanguageSupport::isRightToLeft(const QString &languageCode)
{
  return (!languageCode.compare("ar", Qt::CaseInsensitive) 
            || !languageCode.compare("fa", Qt::CaseInsensitive)
            || !languageCode.compare("he", Qt::CaseInsensitive));
}
/** Sets the application's translator to the specified language. */
bool
LanguageSupport::translate(const QString &languageCode)
{
  if (!isValidLanguageCode(languageCode))
    return false;
  if (languageCode == "en")
    return true;

  /* Attempt to load the translations for Qt's internal widgets from their
   * installed Qt directory. */
  QTranslator *systemQtTranslator = new QTranslator(rApp);
  Q_CHECK_PTR(systemQtTranslator);

  QString qtDir = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
  if (systemQtTranslator->load(qtDir + "/qt_" + languageCode + ".qm"))
    QApplication::installTranslator(systemQtTranslator);
  else
    delete systemQtTranslator;

  /* Install a translator for RetroShare's UI widgets */
  QTranslator *retroshareTranslator = new QTranslator(rApp);
  Q_CHECK_PTR(retroshareTranslator);

  if (retroshareTranslator->load(":/lang/retroshare_" + languageCode + ".qm")) {
    QApplication::installTranslator(retroshareTranslator);
    return true;
  }
  delete retroshareTranslator;
  return false;
}

