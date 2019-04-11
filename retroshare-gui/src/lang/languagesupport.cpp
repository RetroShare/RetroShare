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

#include <QTranslator>
#include <QLocale>
#include <QLibraryInfo>
#include <rshare.h>
#include <retroshare/rsplugin.h>
#include <retroshare/rsinit.h>

#include "languagesupport.h"

static QMap<RsPlugin*, QTranslator*> translatorPlugins;

#define EXTERNAL_TRANSLATION_DIR QString::fromUtf8(RsAccounts::systemDataDirectory().c_str())

/** Initializes the list of available languages. */
QMap<QString, QString>
LanguageSupport::languages()
{
  static QMap<QString, QString> languages;
  if (languages.isEmpty()) {
    //languages.insert("af",    "Afrikaans");
    //languages.insert("bg",    "Bulgarien");
    //languages.insert("cy",    "Welsh");
    languages.insert("ca_ES",    QString::fromUtf8("Catal\303\240"));
    languages.insert("cs",    QString::fromUtf8("\304\214esky"));
    languages.insert("de",    "Deutsch");
    languages.insert("da",    "Dansk");
    languages.insert("nl",    "Nederlands");
    languages.insert("en",    "English");
    languages.insert("es",    QString::fromUtf8("Espa\303\261ol"));
    languages.insert("fr",    QString::fromUtf8("Fran\303\247ais"));
    languages.insert("fi",    "Suomi");
    languages.insert("el",    QString::fromUtf8("\316\225\316\273\316\273\316\267\316\275\316\271\316\272\316\254"));
    languages.insert("hu",    "Magyar");
    languages.insert("it",    "Italiano");
    languages.insert("ja_JP",    QString::fromUtf8("\346\227\245\346\234\254\350\252\236"));
    languages.insert("ko",    QString::fromUtf8("\355\225\234\352\265\255\354\226\264"));
    languages.insert("pl",    "Polski");
    //languages.insert("pt",    "Portuguese");
    languages.insert("ru",    QString::fromUtf8("\320\240\321\203\321\201\321\201\320\272\320\270\320\271"));
    //languages.insert("sl",    "slovenian");
    //languages.insert("sr",    "Serbian");
    languages.insert("sv",    "Svenska");
    languages.insert("tr",    QString::fromUtf8("T\303\274rk\303\247e"));
    languages.insert("zh_CN", QString::fromUtf8("\347\256\200\344\275\223\345\255\227"));
    //languages.insert("zh_TW", QString::fromUtf8("\347\260\241\351\253\224\345\255\227"));
  }
  return languages;
}
QMap<QString, QLocale>
LanguageSupport::locales()
{
  static QMap<QString, QLocale> locales;
  if (locales.isEmpty()) {
    //locales.insert("af", QLocale(QLocale::Afrikaans, QLocale::SouthAfrica));
    //locales.insert("bg", QLocale(QLocale::Bulgarian, QLocale::Bulgaria),);
    //locales.insert("cy", QLocale(QLocale::Welsh, QLocale::UnitedKingdom));
    locales.insert("ca", QLocale(QLocale::Catalan, QLocale::Spain));
    locales.insert("cs", QLocale(QLocale::Czech, QLocale::CzechRepublic));
    locales.insert("de", QLocale(QLocale::German, QLocale::Germany));
    locales.insert("da", QLocale(QLocale::Danish, QLocale::Denmark));
    locales.insert("nl", QLocale(QLocale::Dutch, QLocale::Netherlands));
    locales.insert("en", QLocale(QLocale::English, QLocale::UnitedStates));
    locales.insert("es", QLocale(QLocale::Spanish, QLocale::Spain));
    locales.insert("fr", QLocale(QLocale::French, QLocale::France));
    locales.insert("fi", QLocale(QLocale::Finnish, QLocale::Finland));
    locales.insert("el", QLocale(QLocale::Greek, QLocale::Greece));
    locales.insert("hu", QLocale(QLocale::Hungarian, QLocale::Hungary));
    locales.insert("it", QLocale(QLocale::Italian, QLocale::Italy));
    locales.insert("ja", QLocale(QLocale::Japanese, QLocale::Japan));
    locales.insert("ko", QLocale(QLocale::Korean, QLocale::RepublicOfKorea));
    locales.insert("pl", QLocale(QLocale::Polish, QLocale::Poland));
    //locales.insert("pt", QLocale(QLocale::Portuguese, QLocale::Brazil));
    locales.insert("ru", QLocale(QLocale::Russian, QLocale::RussianFederation));
    //locales.insert("sl", QLocale(QLocale::Slovenian, QLocale::Slovenia));
    //locales.insert("sr", QLocale(QLocale::Serbian, QLocale::SerbiaAndMontenegro));
    locales.insert("sv", QLocale(QLocale::Swedish, QLocale::Sweden));     
    locales.insert("tr", QLocale(QLocale::Turkish, QLocale::Turkey));
    locales.insert("zh", QLocale(QLocale::Chinese, QLocale::China));
  }
  return locales;
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

  /* Attempt to load the translations for Qt's internal widgets from their installed Qt directory. */
  QTranslator *systemQtTranslator = new QTranslator(rApp);
  Q_CHECK_PTR(systemQtTranslator);

  if (systemQtTranslator->load("qt_" + languageCode + ".qm", QLibraryInfo::location(QLibraryInfo::TranslationsPath))) {
    QApplication::installTranslator(systemQtTranslator);
  } else {
    /* Attempt to load the translations for Qt's internal widgets from the translations directory in the exe dir. */
    if (systemQtTranslator->load("qt_" + languageCode + ".qm", QCoreApplication::applicationDirPath() + "/translations")) {
      QApplication::installTranslator(systemQtTranslator);
    } else {
      /* Attempt to load the translations for Qt's internal widgets from the translations directory in the data dir. */
      if (systemQtTranslator->load("qt_" + languageCode + ".qm", EXTERNAL_TRANSLATION_DIR + "/translations")) {
        QApplication::installTranslator(systemQtTranslator);
      } else {
        delete systemQtTranslator;
      }
    }
  }

  /* Install a translator for RetroShare's UI widgets */
  retroshareTranslator = new QTranslator();
  Q_CHECK_PTR(retroshareTranslator);

  bool result = true;

  if (retroshareTranslator->load("retroshare_" + languageCode + ".qm", EXTERNAL_TRANSLATION_DIR + "/translations")) {
    QApplication::installTranslator(retroshareTranslator);
  } else if (retroshareTranslator->load(":/lang/retroshare_" + languageCode + ".qm")) {
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

	QString externalDir = EXTERNAL_TRANSLATION_DIR + "/translations";

	int count = rsPlugins->nbPlugins();
	for (int i = 0; i < count; ++i) {
		RsPlugin* plugin = rsPlugins->plugin(i);
		if (plugin) {
			QTranslator* translator = plugin->qt_translator(rApp, languageCode, externalDir);
			if (translator) {
				QApplication::installTranslator(translator);
				translatorPlugins[plugin] = translator;
			}
		}
	}
	return true;
}
/** Sets the application's locale according to the specified language. */
bool LanguageSupport::localize(const QString &languageCode)
{
  if (!isValidLanguageCode(languageCode))
    return false;
  QLocale::setDefault(locales().key(languageCode));
  return true;
}
