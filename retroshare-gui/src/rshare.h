/****************************************************************
 *  Retroshare QT Gui is distributed under the following license:
 *
 *  Copyright (C) 2006,  crypton
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



#ifndef _RSHARE_H
#define _RSHARE_H

#if defined(Q_OS_WIN)
#include <windows.h>
#include <util/win32.h>
#endif

#include <QApplication>
#include <QMap>
#include <QString>


/** Rshare's version string */
#define RSHARE_VERSION    "0.7"


class Rshare : public QApplication
{
  Q_OBJECT

public:
  /** Constructor. */
  Rshare(QStringList args, int &argc, char **argv, QString dir);
  /** Destructor. */
  ~Rshare();

  /** Return the map of command-line arguments and values. */
  static QMap<QString, QString> arguments() { return _args; }
  /** Validates that all arguments were well-formed. */
  bool validateArguments(QString &errmsg);
  /** Prints usage information to the given text stream. */
  void printUsage(QString errmsg = QString());

  /** Sets the current language. */
  static bool setLanguage(QString languageCode = QString());
  /** Sets the current GUI style. */
  static bool setStyle(QString styleKey = QString());
  /** Shows the specified help topic, or the default if empty. */
  //static void help(QString topic = QString());

  /** Returns the current language. */
  static QString language() { return _language; }
  /** Returns the current GUI style. */
  static QString style() { return _style; }
  /** Returns Rshare's application version. */
  static QString version() { return RSHARE_VERSION; }

  /** Returns Rshare's main TorControl object. */
//  static TorControl* torControl() { return _torControl; }
  
  /** Returns the location Rshare uses for its data files. */
  static QString dataDirectory();
  /** Returns the default location of Rshare's data directory. */
  static QString defaultDataDirectory();
  /** Creates Rshare's data directory, if it doesn't already exist. */
  static bool createDataDirectory(QString *errmsg);
  
  /** Creates Rshare's data directory, if it doesn't already exist. */
  static bool setConfigDirectory(QString dir);
  
signals:
  /** Signals that the application needs to shutdown now. */
  void shutdown();

protected:
#if defined(Q_OS_WIN)
  /** Filters Windows events, looking for events of interest */
  bool winEventFilter(MSG *msg, long *result);
#endif

private:
  /** Parse the list of command-line arguments. */
  void parseArguments(QStringList args);
  /** Returns true if the specified arguments wants a value. */
  bool argNeedsValue(QString argName);

  static QMap<QString, QString> _args; /**< List of command-line arguments.  */
  static QString _style;               /**< The current GUI style.           */
  static QString _language;            /**< The current language.            */

  static bool    useConfigDir;
  static QString configDir;

};

#endif

