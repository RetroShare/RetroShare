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



#ifndef _RSHARE_H
#define _RSHARE_H

#if defined(Q_OS_WIN)
#include <windows.h>
#include <util/win32.h>
#endif

#include <QApplication>
#include <QMap>
#include <QString>
#include <QKeySequence>

#include "util/log.h"

/** Rshare's version string */
#define RSHARE_VERSION    "0.7"

/** Pointer to this RetroShare application instance. */
#define rApp  ((Rshare *)qApp)

#define rDebug(fmt)   (rApp->log(Log::Debug, (fmt)))
#define rInfo(fmt)    (rApp->log(Log::Info, (fmt)))
#define rNotice(fmt)  (rApp->log(Log::Notice, (fmt)))
#define rWarn(fmt)    (rApp->log(Log::Warn, (fmt)))
#define rError(fmt)   (rApp->log(Log::Error, (fmt)))


class Rshare : public QApplication
{
  Q_OBJECT

public:
  /** Constructor. */
  Rshare(QStringList args, int &argc, char **argv, const QString &dir);
  /** Destructor. */
  ~Rshare();

  /** Return the map of command-line arguments and values. */
  static QMap<QString, QString> arguments() { return _args; }
  /** Validates that all arguments were well-formed. */
  bool validateArguments(QString &errmsg);
  /** Prints usage information to the given text stream. */
  //void printUsage(QString errmsg = QString());
  /** Displays usage information for command-line args. */
  static void showUsageMessageBox();
  /** Returns true if the user wants to see usage information. */
  static bool showUsage();

  /** Sets the current language. */
  static bool setLanguage(QString languageCode = QString());
  /** Sets the current locale. */
  static bool setLocale(QString languageCode = QString());
  /** Get custom date format (defaultlongformat) */
  static QString customDateFormat();

  /** Sets the current GUI style. */
  static bool setStyle(QString styleKey = QString());
  /** Sets the current GUI stylesheet. */
  static bool setSheet(QString sheet = QString());
  /** Loads stylesheet from external file **/
  static void loadStyleSheet(const QString &sheetName);
  /** get list of available stylesheets **/
  static void getAvailableStyleSheets(QMap<QString, QString> &styleSheets);
  /** Recalculates matching stylesheet for widget **/
  static void refreshStyleSheet(QWidget *widget, bool processChildren);

  static bool loadCertificate(const std::string &accountId, bool autoLogin);

  /**
   * Update Language, Style and StyleSheet.
   * First args are cheked for a style then the settings.
   */
  static void resetLanguageAndStyle();

  /** Initialize plugins. */
  static void initPlugins();

  /** Returns the current language. */
  static QString language() { return _language; }
  /** Returns the current GUI style. */
  static QString style() { return _style; }
  /** Returns the current GUI stylesheet. */
  static QString stylesheet() { return _stylesheet; }
  /** Returns Rshare's application version. */
  static QString version() { return RSHARE_VERSION; }
  /** Returns Rshare's application startup time. */
  static QDateTime startupTime();

  /** Returns the location Rshare uses for its data files. */
  static QString dataDirectory();
  /** Returns the default location of Rshare's data directory. */
  static QString defaultDataDirectory();
  /** Creates Rshare's data directory, if it doesn't already exist. */
  static bool createDataDirectory(QString *errmsg);
  
  /** Writes <b>msg</b> with severity <b>level</b> to RetroShare's log. */
  static Log::LogMessage log(Log::LogLevel level, QString msg);
  
  /** Creates Rshare's data directory, if it doesn't already exist. */
  static bool setConfigDirectory(QString dir);
  
  /** Enters the main event loop and waits until exit() is called. The signal
  * running() will be emitted when the event loop has started. */
  static int run();
  
  /** Creates and binds a shortcut such that when <b>key</b> is pressed in
  * <b>sender</b>'s context, <b>receiver</b>'s <b>slot</b> will be called. */
  static void createShortcut(const QKeySequence &key, QWidget *sender,
                             QWidget *receiver, const char *slot);
  
signals:
  /** Emitted when the application is running and the main event loop has
   * started. */ 
  void running();
  /** Signals that the application needs to shutdown now. */
  void shutdown();
  /** Global blink timer */
  void blink(bool on);
  /** Global timer every second */
  void secondTick();

protected:
#if defined(Q_OS_WIN)
  /** Filters Windows events, looking for events of interest */
  bool winEventFilter(MSG *msg, long *result);
#endif

private slots:
  /** Called when the application's main event loop has started. This method
   * will emit the running() signal to indicate that the application's event
   * loop is running. */
  void onEventLoopStarted();
  void blinkTimer();

private:
  /** customize the date format (defaultlongformat) */
  static void customizeDateFormat();
  /** Catches debugging messages from Qt and sends them to 
   * RetroShare's logs. */
  static void qt_msg_handler(QtMsgType type, const char *msg);

  /** Parse the list of command-line arguments. */
  void parseArguments(QStringList args);
  /** Returns true if the specified arguments wants a value. */
  bool argNeedsValue(QString argName);

  static QMap<QString, QString> _args; /**< List of command-line arguments.  */
  static QString _style;               /**< The current GUI style.           */
  static QString _stylesheet;          /**< The current GUI stylesheet.      */
  static QString _language;            /**< The current language.            */
  static QString _dateformat;          /**< The format for dates in feed items etc. */
  static Log _log; 					/**< Logs debugging messages to file or stdout. */
  static QDateTime mStartupTime;       // startup time

  static bool    useConfigDir;
  static QString configDir;
  bool mBlink;
};

#endif
