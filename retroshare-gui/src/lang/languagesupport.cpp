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
#include <retroshare/rsplugin.h>

#include "languagesupport.h"

static QMap<RsPlugin*, QTranslator*> translatorPlugins;

/** Initializes the list of available languages. */
QMap<QString, QString>
LanguageSupport::languages()
{
  static QMap<QString, QString> languages;
  if (languages.isEmpty()) {
    //languages.insert("af",    "Afrikaans");
    //languages.insert("bg",    "Bulgarien");
    //languages.insert("cy",    "Welsh");
    languages.insert("cs",    "Czech");
    languages.insert("de",    "Deutsch");
    languages.insert("da",    "Danish");
    languages.insert("en",    "English");
    languages.insert("es",    QString::fromUtf8("spanish"));
    languages.insert("fr",    QString::fromUtf8("Fran\303\247ais"));
    languages.insert("fi",    "suomi");
    //languages.insert("gr",    "Greek");
    //languages.insert("it",    "Italiano");
    languages.insert("ja_JP",    QString::fromUtf8("\346\227\245\346\234\254\350\252\236"));
    languages.insert("ko",    "Korean");
    languages.insert("pl",    "Polska");
    //languages.insert("pt",    "Portuguese");
    languages.insert("ru",    QString::fromUtf8("\320\240\321\203\321\201\321\201\320\272\320\270\320\271"));
    //languages.insert("sl",    "slovenian");
    //languages.insert("sr",    "Serbian");
    languages.insert("sv",    "svenska");     
    languages.insert("tr",    QString::fromUtf8("T\303\274rk\303\247e"));
    languages.insert("zh_CN", QString::fromUtf8("\347\256\200\344\275\223\345\255\227"));
    //languages.insert("zh_TW", QString::fromUtf8("\347\260\241\351\253\224\345\255\227"));
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

static void removePluginTranslation()
{
    QMap<RsPlugin*, QTranslator*>::iterator it;
    for (it = translatorPlugins.begin(); it != translatorPlugins.end(); ++it) {
      if (it.value()) {
        QApplication::removeTranslator(it.value());
        delete(it.value());
      }
    }
    translatorPlugins.clear();
}

/** Sets the application's translator to the specified language. */
bool
LanguageSupport::translate(const QString &languageCode)
{
  if (!isValidLanguageCode(languageCode))
    return false;

  static QTranslator *retroshareTranslator = NULL;
  if (retroshareTranslator) {
    // remove the previous translator, is needed, when switching to en
    QApplication::removeTranslator(retroshareTranslator);
    delete(retroshareTranslator);
    retroshareTranslator = NULL;

    removePluginTranslation();
  }

  if (languageCode == "en")
    return true;

  /* Attempt to load the translations for Qt's internal widgets from their
   * installed Qt directory. */
  QString qtTranslation = QLibraryInfo::location(QLibraryInfo::TranslationsPath) + "/qt_" + languageCode + ".qm";
  QTranslator *systemQtTranslator = new QTranslator(rApp);
  Q_CHECK_PTR(systemQtTranslator);

  if (QFile::exists(qtTranslation) && systemQtTranslator->load(qtTranslation))
    QApplication::installTranslator(systemQtTranslator);
  else {
    /* Attempt to load the translations for Qt's internal widgets from the translations directory in the exe dir. */
    qtTranslation = QCoreApplication::applicationDirPath() + "/translations/qt_" + languageCode + ".qm";
    if (QFile::exists(qtTranslation) && systemQtTranslator->load(qtTranslation))
      QApplication::installTranslator(systemQtTranslator);
    else
      delete systemQtTranslator;
  }

  /* Install a translator for RetroShare's UI widgets */
  retroshareTranslator = new QTranslator();
  Q_CHECK_PTR(retroshareTranslator);

  bool result = true;

  if (retroshareTranslator->load(":/lang/retroshare_" + languageCode + ".qm")) {
    QApplication::installTranslator(retroshareTranslator);
  } else {
    delete retroshareTranslator;
    retroshareTranslator = NULL;
    result = false;
  }

  result = translatePlugins(languageCode) && result;

  return result;
}

/** Sets the application's translator to the specified language for the plugins. */
bool LanguageSupport::translatePlugins(const QString &languageCode)
{
	removePluginTranslation();

	if (rsPlugins == NULL) {
		return true;
	}

	int count = rsPlugins->nbPlugins();
	for (int i = 0; i < count; ++i) {
		RsPlugin* plugin = rsPlugins->plugin(i);
		if (plugin) {
			QTranslator* translator = plugin->qt_translator(rApp, languageCode);
			if (translator) {
				QApplication::installTranslator(translator);
				translatorPlugins[plugin] = translator;
			}
		}
	}
	return true;
}
