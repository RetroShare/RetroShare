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

#include "languagesupport.h"

/** Static list of supported languages and codes. */
QMap<QString, QString> LanguageSupport::_languages;

/** Initializes the list of available languages. */
void
LanguageSupport::initialize()
{
  _languages.clear();
  _languages.insert("en",    "English");
  _languages.insert("de",    "Deutsch");
  _languages.insert("af",    "Afrikaans");
  _languages.insert("cn_simp",    "Chinese-Simple");
  _languages.insert("cn_trad",    "Chinese-Trad");
  _languages.insert("dk",    "danish");
  _languages.insert("fr",    
    QString::fromUtf8("fran\303\247ais"));
  _languages.insert("gr",    "Greek");
  _languages.insert("it",    "Italiano");
  _languages.insert("jp",    "Japanese");
  _languages.insert("kr",    "Korean");
  _languages.insert("pl",    "Polish");
  _languages.insert("pt",    "Portuguese");
  _languages.insert("ru",
    QString::fromUtf8("\320\240\321\203\321\201\321\201\320\272\320\270\320\271"));
  _languages.insert("es",    
    QString::fromUtf8("spanish"));
  _languages.insert("sl",    "slovenian");
  _languages.insert("sr",    "Serbian");
  _languages.insert("se",    "Swedish");     
  _languages.insert("tr",    "Turkish");
  _languages.insert("fi",    "suomi");
  _languages.insert("zh_CN", 
      QString::fromUtf8("\347\256\200\344\275\223\345\255\227"));
  _languages.insert("zh_TW", 
      QString::fromUtf8("\347\260\241\351\253\224\345\255\227"));

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
LanguageSupport::languageCode(QString languageName)
{
  return _languages.key(languageName);
}

/** Returns a list of all supported language codes. (e.g., "en"). */
QStringList
LanguageSupport::languageCodes()
{
  return _languages.keys();
}

/** Returns the language name for a given language code. */
QString
LanguageSupport::languageName(QString languageCode)
{
  return _languages.value(languageCode);
}

/** Returns a list of all supported language names (e.g., "English"). */
QStringList
LanguageSupport::languageNames()
{
  return _languages.values();
}

/** Returns a list of all supported language codes and names. */
QMap<QString, QString>
LanguageSupport::languages()
{
  return _languages;
}

/** Returns true if we understand the given language code. */
bool
LanguageSupport::isValidLanguageCode(QString code)
{
  return languageCodes().contains(code.toLower());
}

/** Sets the application's translator to the specified language. */
bool
LanguageSupport::translate(QString langCode)
{
  if (isValidLanguageCode(langCode)) {
    QTranslator *translator = new QTranslator();
    if (translator->load(QString(":/lang/") + langCode.toLower())) {
      QApplication::installTranslator(translator);
      return true;
    }
    delete translator;
  }
  return false;
}

