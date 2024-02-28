/*******************************************************************************
 * rshare.h                                                                    *
 *                                                                             *
 * Copyright (c) 2006  Crypton                 <retroshare.project@gmail.com>  *
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

#ifndef _RSHARE_H
#define _RSHARE_H

#if defined(Q_OS_WIN)
#include <windows.h>
#include "util/retroshareWin32.h"
#endif

#include <QApplication>
#include <QKeySequence>
#include <QLocalServer>
#include <QMap>
#include <QString>

#include "util/log.h"

#include "retroshare/rstypes.h"
#include "retroshare/rsinit.h"

/** Pointer to this RetroShare application instance. */
#define rApp  (static_cast<RsApplication *>(qApp))

#define rDebug(fmt)   (rApp->log(Log::Debug, (fmt)))
#define rInfo(fmt)    (rApp->log(Log::Info, (fmt)))
#define rNotice(fmt)  (rApp->log(Log::Notice, (fmt)))
#define rWarn(fmt)    (rApp->log(Log::Warn, (fmt)))
#define rError(fmt)   (rApp->log(Log::Error, (fmt)))

struct RsGUIConfigOptions: public RsConfigOptions
{
    RsGUIConfigOptions()
        : optResetParams(false), logLevel("Off"), argc(0)
    {}

    bool optResetParams;		// reset all GUI parameters

    QString dateformat;         // The format for dates in feed items etc.
    QString language;           // The current language.

    QString logFileName;		// output filename for log
    QString logLevel;			// severity threshold for log output

    QString guiStyle;           // CSS Style for the GUI
    QString guiStyleSheetFile;  // CSS Style for the GUI

    int argc ;					// stores argc parameter. Only used by the creator of QApplication
    char *argv[1] ;				// stores argv parameter. Only used by the creator of QApplication
};

class RsApplication : public QApplication
{
  Q_OBJECT

public:
  /** Constructor. */
  RsApplication(const RsGUIConfigOptions& conf);
  /** Destructor. */
  ~RsApplication();

  /** Return the version info */
  static QString retroshareVersion(bool=true);

#ifdef TO_REMOVE
  /** Return the map of command-line arguments and values. */
  static QMap<QString, QString> arguments() { return _args; }
  /** Parse the list of command-line arguments. */
  static void parseArguments(QStringList args, bool firstRun = true);
  /** Validates that all arguments were well-formed. */
  bool validateArguments(QString &errmsg);
  /** Prints usage information to the given text stream. */
  //void printUsage(QString errmsg = QString());
  /** Displays usage information for command-line args. */
  static void showUsageMessageBox();
  /** Returns true if the user wants to see usage information. */
  static bool showUsage();
#endif

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
  /** Load certificate */
  static bool loadCertificate(const RsPeerId &accountId, bool autoLogin);
  /** Start or Stop Local server to get new arguments depends setting */
  static bool updateLocalServer();

  /**
   * Update Language, Style and StyleSheet.
   * First args are cheked for a style then the settings.
   */
  static void resetLanguageAndStyle();

  /** Initialize plugins. */
  static void initPlugins();

  /** Returns the current GUI style. */
  static QString style() { return options.guiStyle; }
  /** Returns the current GUI stylesheet. */
  static QString stylesheet() { return options.guiStyleSheetFile; }
  /** Returns the current language. */
  static QString language() { return options.language; }

  /** Sets/Returns the operating mode. */
  static void setOpMode(const QString& op ) { options.opModeStr = op.toStdString(); }
  static QString opmode() { return QString::fromStdString(options.opModeStr); }

#ifdef __APPLE__
  /** Returns links passed by arguments. */
  static QStringList* links() { return &_links; }
  /** Returns files passed by arguments. */
  static QStringList* files()  {return &_files; }
#endif
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
  static bool setConfigDirectory(const QString &dir);
  
  /** Enters the main event loop and waits until exit() is called. The signal
  * running() will be emitted when the event loop has started. */
  static int run();
  
  /** Creates and binds a shortcut such that when <b>key</b> is pressed in
  * <b>sender</b>'s context, <b>receiver</b>'s <b>slot</b> will be called. */
  static void createShortcut(const QKeySequence &key, QWidget *sender,
                             QWidget *receiver, const char *slot);

#ifdef __APPLE__
  /**To process event from Mac system */
  bool event(QEvent *);
#endif

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
  /** Global timer every minute */
  void minuteTick();
  /** Emitted when other process is connected */
  void newArgsReceived(QStringList args);

private slots:
  /** Called when the application's main event loop has started. This method
   * will emit the running() signal to indicate that the application's event
   * loop is running. */
  void onEventLoopStarted();
  void blinkTimer();
  /**
   * @brief Called when accept new connection from new instance.
   */
  void slotConnectionEstablished();

private:
  /** customize the date format (defaultlongformat) */
  static void customizeDateFormat();

#ifdef TO_REMOVE
  /** Returns true if the specified arguments wants a value. */
  static bool argNeedsValue(const QString &argName);
#endif

#ifdef __APPLE__
  static QStringList _links;           /**< List of links passed by arguments. */
  static QStringList _files;           /**< List of files passed by arguments. */
#endif
  static QDateTime mStartupTime;       // startup time
  bool mBlink;
  static QLocalServer* localServer;
  static RsGUIConfigOptions options;
  static Log log_output;
};

#endif
