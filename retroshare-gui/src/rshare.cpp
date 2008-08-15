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



#include <QDir>
#include <QTextStream>
#include <QStyleFactory>
#include <util/string.h>
#include <lang/languagesupport.h>
#include "gui/Preferences/rsharesettings.h"

#include "rshare.h"

/* Available command-line arguments. */
#define ARG_LANGUAGE   		"lang"    		/**< Argument specifying language.    */
#define ARG_GUISTYLE   		"style"  	 	/**< Argument specfying GUI style.    */
#define ARG_GUISTYLESHEET   "stylesheet"   /**< Argument specfying GUI style.    */
#define ARG_RESET      		"reset"   		/**< Reset Rshare's saved settings.  */
#define ARG_DATADIR    		"datadir" 		/**< Directory to use for data files. */


/* Static member variables */
QMap<QString, QString> Rshare::_args; /**< List of command-line arguments.  */
QString Rshare::_style;               /**< The current GUI style.           */
QString Rshare::_language;            /**< The current language.            */
QString Rshare::_stylesheet;          /**< The current GUI style.           */
bool Rshare::useConfigDir;           
QString Rshare::configDir;           



/** Constructor. Parses the command-line arguments, resets Rshare's
 * configuration (if requested), and sets up the GUI style and language
 * translation. */
Rshare::Rshare(QStringList args, int &argc, char **argv, QString dir)
: QApplication(argc, argv)
{
  /* Read in all our command-line arguments. */
  parseArguments(args);

  /* Check if we're supposed to reset our config before proceeding. */
  if (_args.contains(ARG_RESET)) {
    RshareSettings settings;
    settings.reset();
  }

  /* config directory */
  useConfigDir = false;
  if (dir != "")
  {
  	setConfigDirectory(dir);
  }

  /** Initialize support for language translations. */
  LanguageSupport::initialize();

  /** Translate the GUI to the appropriate language. */
  setLanguage(_args.value(ARG_LANGUAGE));

  /** Set the GUI style appropriately. */
  setStyle(_args.value(ARG_GUISTYLE));
  
  /** Set the GUI stylesheet appropriately. */
  setSheet(_args.value(ARG_GUISTYLESHEET));

  /* Switch off auto shutdown */
  setQuitOnLastWindowClosed ( false );
}

/** Destructor */
Rshare::~Rshare()
{

}

#if defined(Q_OS_WIN)
/** On Windows, we need to catch the WM_QUERYENDSESSION message
 * so we know that it is time to shutdown. */
bool
Rshare::winEventFilter(MSG *msg, long *result)
{
  if (msg->message == WM_QUERYENDSESSION) {
    emit shutdown();
  }
  return QApplication::winEventFilter(msg, result);
}
#endif

/** Display usage information regarding command-line arguments. */
void
Rshare::printUsage(QString errmsg)
{
  QTextStream out(stdout);

  /* If there was an error message, print it out. */
  if (!errmsg.isEmpty()) {
    out << "** " << errmsg << " **" << endl << endl;
  }

  /* Now print the application usage */
  out << "Usage: " << endl;
  out << "\t" << qApp->arguments().at(0) << " [options]"    << endl;

  /* And available options */
  out << endl << "Available Options:"                                   << endl;
  out << "\t-"ARG_RESET"\t\tResets ALL stored Rshare settings."        << endl;
  out << "\t-"ARG_DATADIR"\tSets the directory Rshare uses for data files"<< endl;
  out << "\t-"ARG_GUISTYLE"\t\tSets Rshare's interface style."         << endl;
  out << "\t-"ARG_GUISTYLESHEET"\t\tSets Rshare's stylesheet."         << endl;
  out << "\t\t\t[" << QStyleFactory::keys().join("|") << "]"            << endl;
  out << "\t-"ARG_LANGUAGE"\t\tSets Rshare's language."                << endl;
  out << "\t\t\t[" << LanguageSupport::languageCodes().join("|") << "]" << endl;
}

/** Returns true if the specified argument expects a value. */
bool
Rshare::argNeedsValue(QString argName)
{
  return (argName == ARG_GUISTYLE ||
		  argName == ARG_GUISTYLESHEET ||
          argName == ARG_LANGUAGE ||
          argName == ARG_DATADIR);

}

/** Parses the list of command-line arguments for their argument names and
 * values. */
void
Rshare::parseArguments(QStringList args)
{
  QString arg, value;

  /* Loop through all command-line args/values and put them in a map */
  for (int i = 0; i < args.size(); i++) {
    /* Get the argument name and set a blank value */
    arg   = args.at(i).toLower();
    value = "";

    /* Check if it starts with a - or -- */
    if (arg.startsWith("-")) {
      arg = arg.mid((arg.startsWith("--") ? 2 : 1));
    }
    /* Check if it takes a value and there is one on the command-line */
    if (i < args.size()-1 && argNeedsValue(arg)) {
      value = args.at(++i);
    }
    /* Place this arg/value in the map */
    _args.insert(arg, value);
  }
}

/** Verifies that all specified arguments were valid. */
bool
Rshare::validateArguments(QString &errmsg)
{
  /* If they want help, just return false now 
  if (_args.contains(ARG_HELP)) {
    return false;
  }*/
  /* Check for a language that Retroshare recognizes. */
  if (_args.contains(ARG_LANGUAGE) &&
      !LanguageSupport::isValidLanguageCode(_args.value(ARG_LANGUAGE))) {
    errmsg = tr("Invalid language code specified: ") + _args.value(ARG_LANGUAGE);
    return false;
  }
  /* Check for a valid GUI style */
  if (_args.contains(ARG_GUISTYLE) &&
      !QStyleFactory::keys().contains(_args.value(ARG_GUISTYLE),
                                      Qt::CaseInsensitive)) {
    errmsg = tr("Invalid GUI style specified: ") + _args.value(ARG_GUISTYLE);
    return false;
  }
  return true;
}

/** Sets the translation RetroShare will use. If one was specified on the
 * command-line, we will use that. Otherwise, we'll check to see if one was
 * saved previously. If not, we'll default to one appropriate for the system
 * locale. */
bool
Rshare::setLanguage(QString languageCode)
{
  /* If the language code is empty, use the previously-saved setting */
  if (languageCode.isEmpty()) {
    RshareSettings settings;
    languageCode = settings.getLanguageCode();
  }
  /* Translate into the desired langauge */
  if (LanguageSupport::translate(languageCode)) {
    _language = languageCode;
    return true;
  }
  return false;
}

/** Sets the GUI style RetroShare will use. If one was specified on the
 * command-line, we will use that. Otherwise, we'll check to see if one was
 * saved previously. If not, we'll default to one appropriate for the
 * operating system. */
bool
Rshare::setStyle(QString styleKey)
{
  /* If no style was specified, use the previously-saved setting */
  if (styleKey.isEmpty()) {
    RshareSettings settings;
    styleKey = settings.getInterfaceStyle();
  }
  /* Apply the specified GUI style */
  if (QApplication::setStyle(styleKey)) {
    _style = styleKey;
    return true;
  }
  return false;
}

bool
Rshare::setSheet(QString sheet)
{
  /* If no stylesheet was specified, use the previously-saved setting */
  if (sheet.isEmpty()) {
    RshareSettings settings;
    sheet = settings.getSheetName();
  }
  /* Apply the specified GUI stylesheet */
  /*if (QApplication::setSheet(sheet)) {*/
    _stylesheet = sheet;
    return true;
  /*}
  return false;*/
 
}


/** Displays the help page associated with the specified topic. If no topic is
 * specified, then the default help page will be displayed. */
/**void
Rshare::help(QString topic)
{
  _help->show(topic);
}*/

/** Returns the directory RetroShare uses for its data files. */
QString
Rshare::dataDirectory()
{
  if (useConfigDir)
  {
    return configDir;
  }
  else if (_args.contains(ARG_DATADIR)) {
    return _args.value(ARG_DATADIR);
  }
  return defaultDataDirectory();
}

/** Returns the default location of RetroShare's data directory. */
QString
Rshare::defaultDataDirectory()
{
#if defined(Q_OS_WIN32)
  return (win32_app_data_folder() + "\\RetroShare");
#else
  return (QDir::homePath() + "/.RetroShare");
#endif
}

/** Creates Rshare's data directory, if it doesn't already exist. */
bool
Rshare::createDataDirectory(QString *errmsg)
{
  QDir datadir(dataDirectory());
  if (!datadir.exists()) {
    QString path = datadir.absolutePath();
    if (!datadir.mkpath(path)) {
      return err(errmsg, 
                 QString("Could not create data directory: %1").arg(path));
    }
  }
  return true;
}


/** Set Rshare's data directory - externally */
bool Rshare::setConfigDirectory(QString dir)
{
  useConfigDir = true;
  configDir = dir;
  return true;
}



