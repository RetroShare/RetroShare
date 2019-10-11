/*******************************************************************************
 * retroshare-gui/src/: main.cpp                                               *
 *                                                                             *
 * libretroshare: retroshare core library                                      *
 *                                                                             *
 * Copyright 2006-2007 by Crypton <retroshare@lunamutt.com>                    *
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

#include <QBuffer>
#include <QDateTime>
#include <QDir>
#include <QFileOpenEvent>
#include <QLocale>
#include <QLocalSocket>
#include <QRegExp>
#include <QSharedMemory>
#include <QShortcut>
#include <QString>
#include <QStyle>
#include <QStyleFactory>
#include <QTextStream>
#include <QTimer>
#ifdef __APPLE__
#include <QUrl>
#endif

#include <iostream>
#include <stdlib.h>

#include <gui/common/rshtml.h>
#include <gui/common/vmessagebox.h>
#include <gui/gxs/GxsIdDetails.h>
#include <gui/settings/rsharesettings.h>
#include <lang/languagesupport.h>
#include <util/stringutil.h>

#include <retroshare/rsinit.h>
#include <retroshare/rsversion.h>
#include <retroshare/rsplugin.h>

#include "rshare.h"

/* Available command-line arguments. */
#define ARG_RESET           "reset"         /**< Reset Rshare's saved settings.   */
#define ARG_DATADIR         "datadir"       /**< Directory to use for data files. */
#define ARG_LOGFILE         "logfile"       /**< Location of our logfile.         */
#define ARG_LOGLEVEL        "loglevel"      /**< Log verbosity.                   */
#define ARG_GUISTYLE        "style"         /**< Argument specfying GUI style.    */
#define ARG_GUISTYLESHEET   "stylesheet"    /**< Argument specfying GUI style.    */
#define ARG_LANGUAGE        "lang"          /**< Argument specifying language.    */
#define ARG_OPMODE_S        "o"             /**< OpMode (Full, NoTurtle, Gaming, Minimal) */
#define ARG_OPMODE_L        "opmode"        /**< OpMode (Full, NoTurtle, Gaming, Minimal) */
#define ARG_RSLINK_S        "r"             /**< Open RsLink with protocol retroshare:// */
#define ARG_RSLINK_L        "link"          /**< Open RsLink with protocol retroshare:// */
#define ARG_RSFILE_S        "f"             /**< Open RsFile with or without arg.  */
#define ARG_RSFILE_L        "rsfile"        /**< Open RsFile with or without arg.  */
//Other defined for server in /libretroshare/src/rsserver/rsinit.cc:351

// The arguments here can be send to a running instance.
// If the command line contains arguments not listed here, we have to start a new instance.
// For example, the user wants to start a second instance using --base-dir arg of libretroshare.
static const char* const forwardableArgs[] = {
    ARG_OPMODE_S,
    ARG_OPMODE_L,
    ARG_RSLINK_S,
    ARG_RSLINK_L,
    ARG_RSFILE_S,
    ARG_RSFILE_L,
    NULL,
};

/* Static member variables */
QMap<QString, QString> Rshare::_args; /**< List of command-line arguments.  */
Log Rshare::_log;                     /**< Logs debugging messages to file or stdout. */
QString Rshare::_style;               /**< The current GUI style.           */
QString Rshare::_stylesheet;          /**< The current GUI stylesheet.      */
QString Rshare::_language;            /**< The current language.            */
QString Rshare::_dateformat;          /**< The format of dates in feed items etc. */
QString Rshare::_opmode;              /**< The operating mode passed by args. */
QStringList Rshare::_links;           /**< List of links passed by arguments. */
QStringList Rshare::_files;           /**< List of files passed by arguments. */
QDateTime Rshare::mStartupTime;
bool Rshare::useConfigDir;
QString Rshare::configDir;
QLocalServer* Rshare::localServer;

/** Catches debugging messages from Qt and sends them to RetroShare's logs. If Qt
 * emits a QtFatalMsg, we will write the message to the log and then abort().
 */
#if QT_VERSION >= QT_VERSION_CHECK (5, 0, 0)
void qt_msg_handler(QtMsgType type, const QMessageLogContext &, const QString &msg)
#else
void qt_msg_handler(QtMsgType type, const char *msg)
#endif
{
  switch (type) {
    case QtDebugMsg:
      rDebug(QString("QtDebugMsg: %1").arg(msg));
      break;
    case QtWarningMsg:
      rNotice(QString("QtWarningMsg: %1").arg(msg));
      break;
    case QtCriticalMsg:
      rWarn(QString("QtCriticalMsg: %1").arg(msg));
      break;
    case QtFatalMsg:
      rError(QString("QtFatalMsg: %1").arg(msg));
      break;
#if QT_VERSION >= QT_VERSION_CHECK (5, 5, 0)
    case QtInfoMsg:
      break;
#endif
  }
  if (type == QtFatalMsg) {
    rError("Fatal Qt error. Aborting.");
    abort();
  }
}

static bool notifyRunningInstance()
{
    // Connect to the Local Server of the main process to notify it
    // that a new process had been started
    QLocalSocket localSocket;
    localSocket.connectToServer(QString(TARGET));

    std::cerr << "Rshare::Rshare waitForConnected to other instance." << std::endl;
    if( localSocket.waitForConnected(100) )
    {
      std::cerr << "Rshare::Rshare Connection etablished. Waiting for disconnection." << std::endl;
      localSocket.waitForDisconnected(1000);
      return true;
    }
    else
    {
        std::cerr << "Rshare::Rshare failed to connect to other instance." << std::endl;
        return false;
    }
}

/** Constructor. Parses the command-line arguments, resets Rshare's
 * configuration (if requested), and sets up the GUI style and language
 * translation. */
Rshare::Rshare(QStringList args, int &argc, char **argv, const QString &dir)
: QApplication(argc, argv)
{
  mStartupTime = QDateTime::currentDateTime();
  localServer = NULL;

  //Initialize connection to LocalServer to know if other process runs.
  {
    QString serverName = QString(TARGET);

    // check if another instance is running
    bool haveRunningInstance = notifyRunningInstance();

    bool sendArgsToRunningInstance = haveRunningInstance;
    if(args.empty())
        sendArgsToRunningInstance = false;
    // if we find non-forwardable args, start a new instance
    for(int i = 0; i < args.size(); ++i)
    {
        const char* const* argit = forwardableArgs;
        bool found = false;
        while(*argit && i < args.size())
        {
            if(args.value(i) == "-"+QString(*argit) || args.value(i) == "--"+QString(*argit))
            {
                found = true;
                if(argNeedsValue(*argit))
                    i++;
            }
            argit++;
        }
        if(!found)
            sendArgsToRunningInstance = false;
    }

    if (sendArgsToRunningInstance) {
      // load into shared memory
      QBuffer buffer;
      buffer.open(QBuffer::ReadWrite);
      QDataStream out(&buffer);
      out << args;
      int size = buffer.size();

      QSharedMemory newArgs;
      newArgs.setKey(serverName + "_newArgs");
      if (newArgs.isAttached()) newArgs.detach();

      if (!newArgs.create(size)) {
        std::cerr << "(EE) Rshare::Rshare Unable to create shared memory segment of size:"
                  << size << " error:" << newArgs.errorString().toStdString() << "." << std::endl;
#ifdef Q_OS_UNIX
        std::cerr << "Look with `ipcs -m` for nattch==0 segment. And remove it with `ipcrm -m 'shmid'`." << std::endl;
        //No need for windows, as it removes shared segment directly even when crash.
#endif
        newArgs.detach();
        ::exit(EXIT_FAILURE);
      }
      newArgs.lock();
      char *to = (char*)newArgs.data();
      const char *from = buffer.data().data();
      memcpy(to, from, qMin(newArgs.size(), size));
      newArgs.unlock();

      std::cerr << "Rshare::Rshare waitForConnected to other instance." << std::endl;
      if(notifyRunningInstance())
      {
        newArgs.detach();
        std::cerr << "Rshare::Rshare Arguments was sended." << std::endl
                  << " To disable it, in Options - General - Misc," << std::endl
                  << " uncheck \"Use Local Server to get new Arguments\"." << std::endl;
        ::exit(EXIT_SUCCESS); // Terminate the program using STDLib's exit function
      }
      else
          std::cerr << "Rshare::Rshare failed to connect to other instance." << std::endl;
      newArgs.detach();
    }

    if(!haveRunningInstance)
    {
        // No main process exists
        // Or started without arguments
        // So we start a Local Server to listen for connections from new process
        localServer= new QLocalServer();
        QObject::connect(localServer, SIGNAL(newConnection()), this, SLOT(slotConnectionEstablished()));
        updateLocalServer();
		// clear out any old arguments (race condition?)
		QSharedMemory newArgs;
		newArgs.setKey(QString(TARGET) + "_newArgs");
		if(newArgs.attach(QSharedMemory::ReadWrite))
			newArgs.detach();
    }
  }

#if QT_VERSION >= QT_VERSION_CHECK (5, 0, 0)
  qInstallMessageHandler(qt_msg_handler);
#else
  qInstallMsgHandler(qt_msg_handler);
#endif

#ifndef __APPLE__

  /* set default window icon */
  setWindowIcon(QIcon(":/icons/logo_128.png"));

#endif

  mBlink = true;
  QTimer *timer = new QTimer(this);
  timer->setInterval(500);
  connect(timer, SIGNAL(timeout()), this, SLOT(blinkTimer()));
  timer->start();

  timer = new QTimer(this);
  timer->setInterval(60000);
  connect(timer, SIGNAL(timeout()), this, SIGNAL(minuteTick()));
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

	/* Initialize GxsIdDetails */
	GxsIdDetails::initialize();
}

/** Destructor */
Rshare::~Rshare()
{
	/* Cleanup GxsIdDetails */
	GxsIdDetails::cleanup();
	if (localServer)
	{
		localServer->close();
		delete localServer;
	}
}

/**
 * @brief Executed when new instance connect command is sent to LocalServer
 */
void Rshare::slotConnectionEstablished()
{
	QSharedMemory newArgs;
	newArgs.setKey(QString(TARGET) + "_newArgs");

	QLocalSocket *socket = localServer->nextPendingConnection();

	if (!newArgs.attach())
	{
		/* this is not an error. It just means we were notified to check
		   newArgs, but none had been set yet.
		   TODO: implement separate ping/take messages
		   std::cerr << "(EE) Rshare::slotConnectionEstablished() Unable to attach to shared memory segment."
		   << newArgs.errorString().toStdString() << std::endl;
		   */
		socket->close();
		delete socket;
		return;
	}

	socket->close();
	delete socket;

	QBuffer buffer;
	QDataStream in(&buffer);
	QStringList args;

	newArgs.lock();
	buffer.setData((char*)newArgs.constData(), newArgs.size());
	buffer.open(QBuffer::ReadOnly);
	in >> args;
	newArgs.unlock();
	newArgs.detach();

	emit newArgsReceived(args);
	while (!args.empty())
	{
		std::cerr << "Rshare::slotConnectionEstablished args:" << QString(args.takeFirst()).toStdString() << std::endl;
	}
}

QString Rshare::retroshareVersion(bool) { return RS_HUMAN_READABLE_VERSION; }

/** Enters the main event loop and waits until exit() is called. The signal
 * running() will be emitted when the event loop has started. */
int
Rshare::run()
{
  QTimer::singleShot(0, rApp, SLOT(onEventLoopStarted()));
  return rApp->exec();
}

QDateTime Rshare::startupTime()
{
  return mStartupTime;
}

/** Called when the application's main event loop has started. This method
 * will emit the running() signal to indicate that the application's event
 * loop is running. */
void
Rshare::onEventLoopStarted()
{
  emit running();
}

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
  out << trow(tcol("-" ARG_RESET) +
              tcol(tr("Resets ALL stored RetroShare settings.")));
  out << trow(tcol("-" ARG_DATADIR" &lt;dir&gt;") +
              tcol(tr("Sets the directory RetroShare uses for data files.")));
  out << trow(tcol("-" ARG_LOGFILE" &lt;" + tr("filename") + "&gt;") +
              tcol(tr("Sets the name and location of RetroShare's logfile.")));
  out << trow(tcol("-" ARG_LOGLEVEL" &lt;" + tr("level") + "&gt;") +
              tcol(tr("Sets the verbosity of RetroShare's logging.") +
                   "<br>[" + Log::logLevels().join("|") +"]"));
  out << trow(tcol("-" ARG_GUISTYLE" &lt;" + tr("style") +"&gt;") +
              tcol(tr("Sets RetroShare's interface style.") +
                   "<br>[" + QStyleFactory::keys().join("|") + "]"));
  out << trow(tcol("-" ARG_GUISTYLESHEET" &lt;" + tr("stylesheet") + "&gt;") +
              tcol(tr("Sets RetroShare's interface stylesheets.")));
  out << trow(tcol("-" ARG_LANGUAGE" &lt;" + tr("language") + "&gt;") +
              tcol(tr("Sets RetroShare's language.") +
                   "<br>[" + LanguageSupport::languageCodes().join("|") + "]"));
  out << trow(tcol("--" ARG_OPMODE_L" &lt;" + tr("opmode") + "&gt;") +
              tcol(tr("Sets RetroShare's operating mode.") +
                   "<br>[full|noturtle|gaming|minimal]"));
  out << trow(tcol("-" ARG_RSLINK_L" &lt;" + tr("RsLinkURL") + "&gt;") +
              tcol(tr("Open RsLink with protocol retroshare://")));
  out << trow(tcol("-" ARG_RSFILE_L" &lt;" + tr("filename") + "&gt;") +
              tcol(tr("Open RsFile with or without arg.")));
  out << "</table>";

  VMessageBox::information(0, 
    tr("RetroShare GUI Usage Information"), usage, VMessageBox::Ok);
}

/** Returns true if the specified argument expects a value. */
bool
Rshare::argNeedsValue(QString argName)
{
	return (
	        argName == ARG_DATADIR  ||
	        argName == ARG_LOGFILE  ||
          argName == ARG_LOGLEVEL ||
	        argName == ARG_GUISTYLE ||
	        argName == ARG_GUISTYLESHEET ||
	        argName == ARG_LANGUAGE ||
	        argName == ARG_OPMODE_S ||
	        argName == ARG_OPMODE_L ||
	        argName == ARG_RSLINK_S ||
	        argName == ARG_RSLINK_L ||
	        argName == ARG_RSFILE_S ||
	        argName == ARG_RSFILE_L   );
}

/** Parses the list of command-line arguments for their argument names and
 * values. */
void
Rshare::parseArguments(QStringList args, bool firstRun)
{
	QString arg, value;

	/* Loop through all command-line args/values and put them in a map */
	for (int i = 0; i < args.size(); ++i) {
		/* Get the argument name and set a blank value */
		arg = args.at(i);//.toLower(); Need Upper case for file name.
		if (arg.toLower() == "empty") continue;
		value = "";

		/* Check if it starts with a - or -- */
		if (arg.startsWith("-")) {
			arg = arg.mid((arg.startsWith("--") ? 2 : 1));
			/* Check if it takes a value and there is one on the command-line */
			if (i < args.size()-1 && argNeedsValue(arg.toLower())) {
				value = args.at(++i);
			}
		} else {
			/* Check if links or files without arg */
			if (arg.toLower().startsWith("retroshare://")) {
				value = arg;
				arg = ARG_RSLINK_L;
			} else {
				if (QFile(arg).exists()) {
					value = arg;
					arg = ARG_RSFILE_L;
				}
			}
		}

		/* handle opmode that could be change while running.*/
		QString omValue = QString(value).prepend(";").append(";").toLower();
		QString omValues = QString(";full;noturtle;gaming;minimal;");
		if ((arg == ARG_OPMODE_S || arg == ARG_OPMODE_L ) &&
		    omValues.contains(omValue)) {
			_opmode = value;
		}

		/* Don't send theses argument to _args map to allow multiple. */
		if (arg == ARG_RSLINK_S || arg == ARG_RSLINK_L) {
			_links.append(value);
		} else if (arg == ARG_RSFILE_S || arg == ARG_RSFILE_L) {
			_files.append(value);
		} else if (firstRun) {
			/* Place this arg/value in the map only first time*/
			_args.insert(arg, value);
		}
	}
}

/** Verifies that all specified arguments were valid. */
bool
Rshare::validateArguments(QString &errmsg)
{
  /* Check for a writable log file */
  if (_args.contains(ARG_LOGFILE) && !_log.isOpen()) {
    errmsg = tr("Unable to open log file '%1': %2")
                           .arg(_args.value(ARG_LOGFILE))
                           .arg(_log.errorString());
    return false;
  }
  /* Check for a valid log level */
  if (_args.contains(ARG_LOGLEVEL) &&
      !Log::logLevels().contains(_args.value(ARG_LOGLEVEL))) {
    errmsg = tr("Invalid log level specified:")+" " + _args.value(ARG_LOGLEVEL);
    return false;
  }
  /* Check for a valid GUI style */
  if (_args.contains(ARG_GUISTYLE) &&
      !QStyleFactory::keys().contains(_args.value(ARG_GUISTYLE),
                                      Qt::CaseInsensitive)) {
    errmsg = tr("Invalid GUI style specified:")+" " + _args.value(ARG_GUISTYLE);
    return false;
  }
  /* Check for a language that Retroshare recognizes. */
  if (_args.contains(ARG_LANGUAGE) &&
      !LanguageSupport::isValidLanguageCode(_args.value(ARG_LANGUAGE))) {
    errmsg = tr("Invalid language code specified:")+" " + _args.value(ARG_LANGUAGE);
    return false;
  }
  /* Check for an opmode that Retroshare recognizes. */
  if (_args.contains(ARG_OPMODE_S) &&
      !QString(";full;noturtle;gaming;minimal;").contains(QString(_args.value(ARG_OPMODE_S)).prepend(";").append(";").toLower())) {
    errmsg = tr("Invalid operating mode specified:")+" " + _args.value(ARG_OPMODE_S);
    return false;
  }
  /* Check for an opmode that Retroshare recognizes. */
  if (_args.contains(ARG_OPMODE_L) &&
      !QString(";full;noturtle;gaming;minimal;").contains(QString(_args.value(ARG_OPMODE_L)).prepend(";").append(";").toLower())) {
    errmsg = tr("Invalid operating mode specified:")+" " + _args.value(ARG_OPMODE_L);
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

/** Sets the locale RetroShare will use. If a language was specified on the
 * command-line, we will use one according to that. Otherwise, we'll check to see if a language was
 * saved previously. If not, we'll default to the system
 * locale. */
bool
Rshare::setLocale(QString languageCode)
{
  bool retVal = false;
  /* If the language code is empty, use the previously-saved setting */
  if (languageCode.isEmpty()) {
    languageCode = Settings->getLanguageCode();
  }
  /* Set desired locale as default locale */
  if (LanguageSupport::localize(languageCode)) {
	retVal=true;
  }
  customizeDateFormat();
  return retVal;
}

/** customize date format for feeds etc. */
void Rshare::customizeDateFormat()
{
  QLocale locale = QLocale(); // set to default locale
  /* get long date format without weekday */
  _dateformat = locale.dateFormat(QLocale::LongFormat);
  _dateformat.replace(QRegExp("^dddd,*[^ ]* *('[^']+' )*"), "");
  _dateformat.replace(QRegExp(",* *dddd"), "");
  _dateformat = _dateformat.trimmed();
}

/** Get custom date format (defaultlongformat) */
QString Rshare::customDateFormat()
{
    return _dateformat;
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

    /** Set the locale appropriately. */
    setLocale(_args.value(ARG_LANGUAGE));

   /** Set the GUI style appropriately. */
    setStyle(_args.value(ARG_GUISTYLE));

    /** Set the GUI stylesheet appropriately. */
    setSheet(_args.value(ARG_GUISTYLESHEET));
}

// RetroShare:
//   Default:
//     :/qss/stylesheet/qss.default
//     :/qss/stylesheet/qss.<locale>
//   Internal:
//     :/qss/stylesheet/<name>.qss
//   External:
//     <ConfigDirectory|DataDirectory>/qss/<name>.qss
//   Language depended stylesheet
//     <Internal|External>_<locale>.lqss
//
// Plugin:
//   Default:
//     :/qss/stylesheet/<plugin>/<plugin>_qss.default
//     :/qss/stylesheet/<plugin>/<plugin>_qss.<locale>
//   Internal:
//     :/qss/stylesheet/<plugin>/<plugin>_<name>.qss
//   External:
//     <ConfigDirectory|DataDirectory>/qss/<plugin>/<plugin>_<name>.qss
//   Language depended stylesheet
//     <Internal|External>_<locale>.lqss

void Rshare::loadStyleSheet(const QString &sheetName)
{
    QString locale = QLocale().name();
    QString styleSheet;

    QStringList names;
    names.push_back(""); // RetroShare

    /* Get stylesheet from plugins */
    if (rsPlugins) {
        int count = rsPlugins->nbPlugins();
        for (int i = 0; i < count; ++i) {
            RsPlugin* plugin = rsPlugins->plugin(i);
            if (plugin) {
                QString pluginStyleSheetName = QString::fromUtf8(plugin->qt_stylesheet().c_str());
                if (!pluginStyleSheetName.isEmpty()) {
                    names.push_back(QString("%1/%1_").arg(pluginStyleSheetName));
                }
            }
        }
    }

    foreach (QString name, names) {
        /* load the default stylesheet */
        QFile file(QString(":/qss/stylesheet/%1qss.default").arg(name));
        if (file.open(QFile::ReadOnly)) {
            styleSheet += QLatin1String(file.readAll()) + "\n";
            file.close();
        }

        /* load locale depended default stylesheet */
        file.setFileName(QString(":/qss/stylesheet/%1qss.%2").arg(name, locale));
        if (file.open(QFile::ReadOnly)) {
            styleSheet += QLatin1String(file.readAll()) + "\n";
            file.close();
        }

        if (!sheetName.isEmpty()) {
            /* load stylesheet */
            if (sheetName.left(1) == ":") {
                /* internal stylesheet */
                file.setFileName(QString(":/qss/stylesheet/%1%2.qss").arg(name, sheetName.mid(1)));
            } else {
                /* external stylesheet */
                file.setFileName(QString("%1/qss/%2%3.qss").arg(QString::fromUtf8(RsAccounts::ConfigDirectory().c_str()), name, sheetName));
                if (!file.exists()) {
                    file.setFileName(QString("%1/qss/%2%3.qss").arg(QString::fromUtf8(RsAccounts::systemDataDirectory().c_str()), name, sheetName));
                }
            }
            if (file.open(QFile::ReadOnly)) {
                styleSheet += QLatin1String(file.readAll()) + "\n";
                file.close();

                /* load language depended stylesheet */
                QFileInfo fileInfo(file.fileName());
                file.setFileName(fileInfo.path() + "/" + fileInfo.baseName() + "_" + locale + ".lqss");
                if (file.open(QFile::ReadOnly)) {
                    styleSheet += QLatin1String(file.readAll()) + "\n";
                    file.close();
                }

                /* replace %THISPATH% by file path so url can get relative files */
                styleSheet = styleSheet.replace("url(%THISPATH%",QString("url(%1").arg(fileInfo.absolutePath()));
            }
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
	fileInfoList = QDir(QString::fromUtf8(RsAccounts::ConfigDirectory().c_str()) + "/qss/").entryInfoList(QStringList("*.qss"));
	foreach (fileInfo, fileInfoList) {
		if (fileInfo.isFile()) {
			QString name = fileInfo.baseName();
			styleSheets.insert(name, name);
		}
	}
	fileInfoList = QDir(QString::fromUtf8(RsAccounts::systemDataDirectory().c_str()) + "/qss/").entryInfoList(QStringList("*.qss"));
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
			for (QObjectList::Iterator it = childList.begin(); it != childList.end(); ++it) {
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
    loadStyleSheet(_stylesheet);
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
#if defined(Q_OS_WIN)
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
                 tr("Could not create data directory: %1").arg(path));
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

#ifdef __APPLE__
bool Rshare::event(QEvent *event)
{
  switch (event->type()) {
    case QEvent::FileOpen:{
      QFileOpenEvent* fileOE = static_cast<QFileOpenEvent *>(event);
      QStringList args;
      if (! fileOE->file().isEmpty()) {
        _files.append(fileOE->file());
        emit newArgsReceived(QStringList());
        return true;
      } else if (! fileOE->url().isEmpty()) {
        _links.append(fileOE->url().toString());
        emit newArgsReceived(QStringList());
        return true;
      }
    }
    default:
    return QApplication::event(event);
  }
}
#endif

void Rshare::blinkTimer()
{
    mBlink = !mBlink;
    emit blink(mBlink);

    if (mBlink) {
        /* Tick every second (create an own timer when needed) */
        emit secondTick();
    }
}

bool Rshare::loadCertificate(const RsPeerId &accountId, bool autoLogin)
{
	if (!RsAccounts::SelectAccount(accountId))
	{
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
        case 3:
//		case 3: QMessageBox::critical(	0,
//										QObject::tr("Login Failure"),
//										QObject::tr("Maybe password is wrong") );
				return false;
		default: std::cerr << "Rshare::loadCertificate() unexpected switch value " << retVal << std::endl;
				return false;
	}

	return true;
}

bool Rshare::updateLocalServer()
{
	if (localServer) {
		QString serverName = QString(TARGET);
		if (Settings->getUseLocalServer()) {
			localServer->removeServer(serverName);
			localServer->listen(serverName);
			return true;
		}
		localServer->close();
	}
	return false;
}
