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
#include <QTimer>
#include <QTextStream>
#include <QShortcut>
#include <QStyleFactory>
#include <QStyle>
#include <gui/common/vmessagebox.h>
#include <gui/common/html.h>
#include <util/stringutil.h>
#include <stdlib.h>
#include <iostream>

#include <retroshare/rsinit.h>
#include <lang/languagesupport.h>
#include "gui/settings/rsharesettings.h"

#include "rshare.h"

/* Available command-line arguments. */
#define ARG_LANGUAGE   		"lang"    		/**< Argument specifying language.    */
#define ARG_GUISTYLE   		"style"  	 	/**< Argument specfying GUI style.    */
#define ARG_GUISTYLESHEET   "stylesheet"    /**< Argument specfying GUI style.    */
#define ARG_RESET      		"reset"   		/**< Reset Rshare's saved settings.  */
#define ARG_DATADIR    		"datadir" 		/**< Directory to use for data files. */
#define ARG_LOGFILE    		"logfile"       /**< Location of our logfile.         */
#define ARG_LOGLEVEL   		"loglevel"      /**< Log verbosity.                   */


/* Static member variables */
QMap<QString, QString> Rshare::_args; /**< List of command-line arguments.  */
QString Rshare::_style;               /**< The current GUI style.           */
QString Rshare::_language;            /**< The current language.            */
QString Rshare::_stylesheet;          /**< The current GUI stylesheet.      */
Log Rshare::_log;
bool Rshare::useConfigDir;           
QString Rshare::configDir;           

/** Catches debugging messages from Qt and sends them to RetroShare's logs. If Qt
 * emits a QtFatalMsg, we will write the message to the log and then abort().
 */
void
Rshare::qt_msg_handler(QtMsgType type, const char *s)
{
  QString msg(s);
  switch (type) {
    case QtDebugMsg:
      rDebug("QtDebugMsg: %1").arg(msg);
      break;
    case QtWarningMsg:
      rNotice("QtWarningMsg: %1").arg(msg);
      break;
    case QtCriticalMsg:
      rWarn("QtCriticalMsg: %1").arg(msg);
      break;
    case QtFatalMsg:
      rError("QtFatalMsg: %1").arg(msg);
      break;
  }
  if (type == QtFatalMsg) {
    rError("Fatal Qt error. Aborting.");
    abort();
  }
}

/** Constructor. Parses the command-line arguments, resets Rshare's
 * configuration (if requested), and sets up the GUI style and language
 * translation. */
Rshare::Rshare(QStringList args, int &argc, char **argv, const QString &dir)
: QApplication(argc, argv)
{
  qInstallMsgHandler(qt_msg_handler);

#ifndef __APPLE__

  /* set default window icon */
  setWindowIcon(QIcon(":/images/rstray3.png"));

#endif

  mBlink = true;
  QTimer *timer = new QTimer(this);
  timer->setInterval(500);
  connect(timer, SIGNAL(timeout()), this, SLOT(blinkTimer()));
  timer->start();

  /* Read in all our command-line arguments. */
  parseArguments(args);

  /* Check if we're supposed to reset our config before proceeding. */
  if (_args.contains(ARG_RESET)) {
    Settings->reset();
  }
  
  /* Handle the -loglevel and -logfile options. */
  if (_args.contains(ARG_LOGFILE))
    _log.open(_args.value(ARG_LOGFILE));
  if (_args.contains(ARG_LOGLEVEL)) {
    _log.setLogLevel(Log::stringToLogLevel(
                      _args.value(ARG_LOGLEVEL)));
    if (!_args.contains(ARG_LOGFILE))
      _log.open(stdout);
  }
  if (!_args.contains(ARG_LOGLEVEL) && 
      !_args.contains(ARG_LOGFILE))
    _log.setLogLevel(Log::Off);

  /* config directory */
  useConfigDir = false;
  if (dir != "")
  {
  	setConfigDirectory(dir);
  }

  /** Initialize support for language translations. */
  //LanguageSupport::initialize();

  resetLanguageAndStyle();

  /* Switch off auto shutdown */
  setQuitOnLastWindowClosed ( false );
}

/** Destructor */
Rshare::~Rshare()
{

}

/** Enters the main event loop and waits until exit() is called. The signal
 * running() will be emitted when the event loop has started. */
int
Rshare::run()
{
  QTimer::singleShot(0, rApp, SLOT(onEventLoopStarted()));
  return rApp->exec();
}

/** Called when the application's main event loop has started. This method
 * will emit the running() signal to indicate that the application's event
 * loop is running. */
void
Rshare::onEventLoopStarted()
{
  emit running();
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
/*void
Rshare::printUsage(QString errmsg)
{
  QTextStream out(stdout);*/

  /* If there was an error message, print it out. */
  /*if (!errmsg.isEmpty()) {
    out << "** " << errmsg << " **" << endl << endl;
  }*/

  /* Now print the application usage */
  //out << "Usage: " << endl;
  //out << "\t" << qApp->arguments().at(0) << " [options]"    << endl;

  /* And available options */
  //out << endl << "Available Options:"                                   << endl;
  //out << "\t-"ARG_RESET"\t\tResets ALL stored Rshare settings."        << endl;
  //out << "\t-"ARG_DATADIR"\tSets the directory Rshare uses for data files"<< endl;
  //out << "\t-"ARG_GUISTYLE"\t\tSets Rshare's interface style."         << endl;
  //out << "\t-"ARG_GUISTYLESHEET"\t\tSets Rshare's stylesheet."         << endl;
  //out << "\t\t\t[" << QStyleFactory::keys().join("|") << "]"            << endl;
  //out << "\t-"ARG_LANGUAGE"\t\tSets Rshare's language."                << endl;
  //out << "\t\t\t[" << LanguageSupport::languageCodes().join("|") << "]" << endl;
//}

/** Displays usage information for command-line args. */
void
Rshare::showUsageMessageBox()
{
  QString usage;
  QTextStream out(&usage);

  out << "Available Options:" << endl;
  out << "<table>";
  //out << trow(tcol("-"ARG_HELP) + 
  //            tcol(tr("Displays this usage message and exits.")));
  out << trow(tcol("-"ARG_RESET) +
              tcol(tr("Resets ALL stored RetroShare settings.")));
  out << trow(tcol("-"ARG_DATADIR" &lt;dir&gt;") +
              tcol(tr("Sets the directory RetroShare uses for data files.")));
  out << trow(tcol("-"ARG_LOGFILE" &lt;file&gt;") +
              tcol(tr("Sets the name and location of RetroShare's logfile.")));
  out << trow(tcol("-"ARG_LOGLEVEL" &lt;level&gt;") +
              tcol(tr("Sets the verbosity of RetroShare's logging.") +
                   "<br>[" + Log::logLevels().join("|") +"]"));
  out << trow(tcol("-"ARG_GUISTYLE" &lt;style&gt;") +
              tcol(tr("Sets RetroShare's interface style.") +
                   "<br>[" + QStyleFactory::keys().join("|") + "]"));
  out << trow(tcol("-"ARG_GUISTYLESHEET" &lt;stylesheet&gt;") +
              tcol(tr("Sets RetroShare's interface stylesheets.")));                   
  out << trow(tcol("-"ARG_LANGUAGE" &lt;language&gt;") + 
              tcol(tr("Sets RetroShare's language.") +
                   "<br>[" + LanguageSupport::languageCodes().join("|") + "]"));
  out << "</table>";

  VMessageBox::information(0, 
    tr("RetroShare Usage Information"), usage, VMessageBox::Ok);
}

/** Returns true if the specified argument expects a value. */
bool
Rshare::argNeedsValue(QString argName)
{
  return (argName == ARG_GUISTYLE ||
		  argName == ARG_GUISTYLESHEET ||
          argName == ARG_LANGUAGE ||
          argName == ARG_DATADIR  ||        
          argName == ARG_LOGFILE  ||
          argName == ARG_LOGLEVEL);

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
  /* Check for a valid log level */
  if (_args.contains(ARG_LOGLEVEL) &&
      !Log::logLevels().contains(_args.value(ARG_LOGLEVEL))) {
    errmsg = tr("Invalid log level specified: ") + _args.value(ARG_LOGLEVEL);
    return false;
  }
  /* Check for a writable log file */
  if (_args.contains(ARG_LOGFILE) && !_log.isOpen()) {
    errmsg = tr("Unable to open log file '%1': %2")
                           .arg(_args.value(ARG_LOGFILE))
                           .arg(_log.errorString());
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
    languageCode = Settings->getLanguageCode();
  }
  /* Translate into the desired language */
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
    styleKey = Settings->getInterfaceStyle();
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
    sheet = Settings->getSheetName();
  }
  /* Apply the specified GUI stylesheet */
    _stylesheet = sheet;

    /* load the StyleSheet*/
    loadStyleSheet(_stylesheet);

    return true;
}

void Rshare::resetLanguageAndStyle()
{
    /** Translate the GUI to the appropriate language. */
    setLanguage(_args.value(ARG_LANGUAGE));

    /** Set the GUI style appropriately. */
    setStyle(_args.value(ARG_GUISTYLE));

    /** Set the GUI stylesheet appropriately. */
    setSheet(_args.value(ARG_GUISTYLESHEET));
}

void Rshare::loadStyleSheet(const QString &sheetName)
{
    QString styleSheet;

    /* load the default stylesheet */
    QFile file(":/qss/stylesheet/qss.default");
    if (file.open(QFile::ReadOnly)) {
        styleSheet = QLatin1String(file.readAll()) + "\n";
        file.close();
    }

    if (!sheetName.isEmpty()) {
        if (sheetName.left(1) == ":") {
            /* internal stylesheet */
            file.setFileName(":/qss/stylesheet/" + sheetName.mid(1) + ".qss");
        } else {
            /* external stylesheet */
            file.setFileName(QString::fromUtf8(RsInit::RsConfigDirectory().c_str()) + "/qss/" + sheetName + ".qss");
            if (!file.exists()) {
                file.setFileName(QString::fromUtf8(RsInit::getRetroshareDataDirectory().c_str()) + "/qss/" + sheetName + ".qss");
            }
        }
        if (file.open(QFile::ReadOnly)) {
            styleSheet += QLatin1String(file.readAll());
            file.close();
        }
    }
    qApp->setStyleSheet(styleSheet);
}

/** get list of available stylesheets **/
void Rshare::getAvailableStyleSheets(QMap<QString, QString> &styleSheets)
{
	QFileInfoList fileInfoList = QDir(":/qss/stylesheet/").entryInfoList(QStringList("*.qss"));
	QFileInfo fileInfo;
	foreach (fileInfo, fileInfoList) {
		if (fileInfo.isFile()) {
			QString name = fileInfo.baseName();
			styleSheets.insert(QString("%1 (%2)").arg(name, tr("built-in")), ":" + name);
		}
	}
	fileInfoList = QDir(QString::fromUtf8(RsInit::RsConfigDirectory().c_str()) + "/qss/").entryInfoList(QStringList("*.qss"));
	foreach (fileInfo, fileInfoList) {
		if (fileInfo.isFile()) {
			QString name = fileInfo.baseName();
			styleSheets.insert(name, name);
		}
	}
	fileInfoList = QDir(QString::fromUtf8(RsInit::getRetroshareDataDirectory().c_str()) + "/qss/").entryInfoList(QStringList("*.qss"));
	foreach (fileInfo, fileInfoList) {
		if (fileInfo.isFile()) {
			QString name = fileInfo.baseName();
			if (!styleSheets.contains(name)) {
				styleSheets.insert(name, name);
			}
		}
	}
}

void Rshare::refreshStyleSheet(QWidget *widget, bool processChildren)
{
	if (widget != NULL) {
		// force widget to recalculate valid style
		widget->style()->unpolish(widget);
		widget->style()->polish(widget);
		// widget might need to recalculate properties (like margins) depending on style
		QEvent event(QEvent::StyleChange);
		QApplication::sendEvent(widget, &event);
		// repaint widget
		widget->update();

		if (processChildren == true) {
			// process children recursively
			QObjectList childList =	widget->children ();
			for (QObjectList::Iterator it = childList.begin(); it != childList.end(); it++) {
				QWidget *child = qobject_cast<QWidget*>(*it);
				if (child != NULL) {
					refreshStyleSheet(child, processChildren);
				}
			}
		}
	}
}

/** Initialize plugins. */
void Rshare::initPlugins()
{
    LanguageSupport::translatePlugins(_language);
}

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

/** Writes <b>msg</b> with severity <b>level</b> to RetroShare's log. */
Log::LogMessage
Rshare::log(Log::LogLevel level, QString msg)
{
  return _log.log(level, msg);
}

/** Creates and binds a shortcut such that when <b>key</b> is pressed in
 * <b>sender</b>'s context, <b>receiver</b>'s <b>slot</b> will be called. */
void
Rshare::createShortcut(const QKeySequence &key, QWidget *sender,
                        QWidget *receiver, const char *slot)
{
  QShortcut *s = new QShortcut(key, sender);
  connect(s, SIGNAL(activated()), receiver, slot);
}

void Rshare::blinkTimer()
{
    mBlink = !mBlink;
    emit blink(mBlink);
}

bool Rshare::loadCertificate(const std::string &accountId, bool autoLogin, std::string gpgId)
{
	if (gpgId.empty()) {
		std::string gpgName, gpgEmail, sslName;
		if (!RsInit::getAccountDetails(accountId, gpgId, gpgName, gpgEmail, sslName)) {
			return false;
		}
	}
	if (!RsInit::SelectGPGAccount(gpgId)) {
		return false;
	}

	std::string lockFile;
	int retVal = RsInit::LockAndLoadCertificates(autoLogin, lockFile);
	switch (retVal) {
		case 0:	break;
		case 1:	QMessageBox::warning(	0,
										QObject::tr("Multiple instances"),
										QObject::tr("Another RetroShare using the same profile is "
										"already running on your system. Please close "
										"that instance first\n Lock file:\n") +
										QString::fromUtf8(lockFile.c_str()));
				return false;
		case 2:	QMessageBox::critical(	0,
										QObject::tr("Multiple instances"),
										QObject::tr("An unexpected error occurred when Retroshare "
										"tried to acquire the single instance lock\n Lock file:\n") +
										QString::fromUtf8(lockFile.c_str()));
				return false;
		case 3: QMessageBox::critical(	0,
										QObject::tr("Login Failure"),
										QObject::tr("Maybe password is wrong") );
				return false;
		default: std::cerr << "Rshare::loadCertificate() unexpected switch value " << retVal << std::endl;
				return false;
	}

	return true;
}
