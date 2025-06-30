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

#include "gui/common/FilesDefs.h"
#include <gui/common/rshtml.h>
#include <gui/common/vmessagebox.h>
#include <gui/gxs/GxsIdDetails.h>
#include <gui/settings/rsharesettings.h>
#include <lang/languagesupport.h>
#include <util/stringutil.h>

#include <util/argstream.h>
#include <retroshare/rsinit.h>
#include <retroshare/rsversion.h>
#include <retroshare/rsplugin.h>

#include "rshare.h"

#ifdef __APPLE__
QStringList RsApplication::_links;           /**< List of links passed by arguments. */
QStringList RsApplication::_files;           /**< List of files passed by arguments. */
#endif

Log RsApplication::log_output;                     /**< Logs debugging messages to file or stdout. */
RsGUIConfigOptions RsApplication::options;
QDateTime          RsApplication::mStartupTime;
QLocalServer*      RsApplication::localServer;

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

/** Constructor. Parses the command-line arguments, resets RsApplication's
 * configuration (if requested), and sets up the GUI style and language
 * translation.
 * the const_cast below is truely horrible, but it allows to hide these unused argc/argv
 * when initing RsApplication
 */

RsApplication::RsApplication(const RsGUIConfigOptions& conf)
: QApplication(const_cast<RsGUIConfigOptions*>(&conf)->argc,const_cast<RsGUIConfigOptions*>(&conf)->argv)
{
    mStartupTime = QDateTime::currentDateTime();
    localServer = NULL;
    options = conf;

    // So we start a Local Server to listen for connections from new process
    localServer= new QLocalServer();
    QObject::connect(localServer, SIGNAL(newConnection()), this, SLOT(slotConnectionEstablished()));
    updateLocalServer();

    // clear out any old arguments (race condition?)
    QSharedMemory newArgs;
    newArgs.setKey(QString(TARGET) + "_newArgs");
    if(newArgs.attach(QSharedMemory::ReadWrite))
        newArgs.detach();

#if QT_VERSION >= QT_VERSION_CHECK (5, 0, 0)
    qInstallMessageHandler(qt_msg_handler);
#else
    qInstallMsgHandler(qt_msg_handler);
#endif

#ifndef __APPLE__

    /* set default window icon */
    setWindowIcon(FilesDefs::getIconFromQtResourcePath(":/icons/logo_128.png"));

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

    /* Check if we're supposed to reset our config before proceeding. */
    if (options.optResetParams)
    {
        RsInfo() << "Resetting Retroshare config parameters, as requested (option -R)";
        Settings->reset();
    }

    /* Handle the -loglevel and -logfile options. */
    if (options.logLevel != "Off")
    {
        if (!options.logFileName.isNull())
            log_output.open(options.logFileName);
        else
            log_output.open(stdout);

        log_output.setLogLevel(Log::stringToLogLevel(options.logLevel));
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
RsApplication::~RsApplication()
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
void RsApplication::slotConnectionEstablished()
{
	QSharedMemory newArgs;
	newArgs.setKey(QString(TARGET) + "_newArgs");

	QLocalSocket *socket = localServer->nextPendingConnection();

	if (!newArgs.attach())
	{
		/* this is not an error. It just means we were notified to check
		   newArgs, but none had been set yet.
		   TODO: implement separate ping/take messages
           std::cerr << "(EE) RsApplication::slotConnectionEstablished() Unable to attach to shared memory segment."
		   << newArgs.errorString().toStdString() << std::endl;
		   */
		socket->close();
		delete socket;
		return;
	}

	socket->close();
	delete socket;

    if(newArgs.error())
    {
        RsErr() << "Something when wrong in receiving arguments from operating system: " << newArgs.errorString().toStdString() ;
        return ;
    }
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
        std::cerr << "RsApplication::slotConnectionEstablished args:" << QString(args.takeFirst()).toStdString() << std::endl;
    }
}

QString RsApplication::retroshareVersion(bool) { return RS_HUMAN_READABLE_VERSION; }

/** Enters the main event loop and waits until exit() is called. The signal
 * running() will be emitted when the event loop has started. */
int
RsApplication::run()
{
  QTimer::singleShot(0, rApp, SLOT(onEventLoopStarted()));
  return rApp->exec();
}

QDateTime RsApplication::startupTime()
{
  return mStartupTime;
}

/** Called when the application's main event loop has started. This method
 * will emit the running() signal to indicate that the application's event
 * loop is running. */
void
RsApplication::onEventLoopStarted()
{
  emit running();
}

/** Sets the translation RetroShare will use. If one was specified on the
 * command-line, we will use that. Otherwise, we'll check to see if one was
 * saved previously. If not, we'll default to one appropriate for the system
 * locale. */
bool
RsApplication::setLanguage(QString languageCode)
{
  /* If the language code is empty, use the previously-saved setting */
  if (languageCode.isEmpty()) {
    languageCode = Settings->getLanguageCode();
  }
  /* Translate into the desired language */
  if (LanguageSupport::translate(languageCode)) {
    options.language = languageCode;
    return true;
  }
  return false;
}

/** Sets the locale RetroShare will use. If a language was specified on the
 * command-line, we will use one according to that. Otherwise, we'll check to see if a language was
 * saved previously. If not, we'll default to the system
 * locale. */
bool
RsApplication::setLocale(QString languageCode)
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
void RsApplication::customizeDateFormat()
{
  QLocale locale = QLocale(); // set to default locale
  /* get long date format without weekday */
  options.dateformat = locale.dateFormat(QLocale::LongFormat);
  options.dateformat.replace(QRegExp("^dddd,*[^ ]* *('[^']+' )*"), "");
  options.dateformat.replace(QRegExp(",* *dddd"), "");
  options.dateformat = options.dateformat.trimmed();
}

/** Get custom date format (defaultlongformat) */
QString RsApplication::customDateFormat()
{
    return options.dateformat;
}

/** Sets the GUI style RetroShare will use. If one was specified on the
 * command-line, we will use that. Otherwise, we'll check to see if one was
 * saved previously. If not, we'll default to one appropriate for the
 * operating system. */
bool
RsApplication::setStyle(QString styleKey)
{
  /* If no style was specified, use the previously-saved setting */
  if (styleKey.isEmpty()) {
    styleKey = Settings->getInterfaceStyle();
  }
  /* Apply the specified GUI style */
  if (QApplication::setStyle(styleKey))
  {
    options.guiStyle = styleKey;
    return true;
  }
  return false;
}

bool
RsApplication::setSheet(QString sheet)
{
  /* If no stylesheet was specified, use the previously-saved setting */
  if (sheet.isEmpty()) {
    sheet = Settings->getSheetName();
  }
  /* Apply the specified GUI stylesheet */
    options.guiStyleSheetFile = sheet;

    /* load the StyleSheet*/
    loadStyleSheet(options.guiStyleSheetFile);

    return true;
}

void RsApplication::resetLanguageAndStyle()
{
    /** Translate the GUI to the appropriate language. */
    setLanguage(options.language);

    /** Set the locale appropriately. */
    setLocale(options.language);

   /** Set the GUI style appropriately. */
    setStyle(options.guiStyle);

    /** Set the GUI stylesheet appropriately. */
    setSheet(options.guiStyleSheetFile);
}

// RetroShare:
//   Default:
//     :/qss/stylesheet/default.qss
//     :/qss/stylesheet/<locale>.qss
//   Internal:
//     :/qss/stylesheet/<name>.qss
//   External:
//     <ConfigDirectory|DataDirectory>/qss/<name>.qss
//   Language depended stylesheet
//     <Internal|External>_<locale>.lqss
//
// Plugin:
//   Default:
//     :/qss/stylesheet/<plugin>/<plugin>_default.qss
//     :/qss/stylesheet/<plugin>/<plugin>_<locale>.qss
//   Internal:
//     :/qss/stylesheet/<plugin>/<plugin>_<name>.qss
//   External:
//     <ConfigDirectory|DataDirectory>/qss/<plugin>/<plugin>_<name>.qss
//   Language depended stylesheet
//     <Internal|External>_<locale>.lqss

void RsApplication::loadStyleSheet(const QString &sheetName)
{
    QString locale = QLocale().name();
    QString styleSheet;

    QStringList names;
    names.push_back(""); // RetroShare

    /* Get stylesheet from plugins */
    if (rsPlugins) {
        int count = rsPlugins->nbPlugins();
        for (int iCurs = 0; iCurs < count; ++iCurs) {
            RsPlugin* plugin = rsPlugins->plugin(iCurs);
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
        QFile file(QString(":/qss/stylesheet/%1default.qss").arg(name));
        if (file.open(QFile::ReadOnly)) {
            styleSheet += QLatin1String(file.readAll()) + "\n";
            file.close();
        }

        /* load locale depended default stylesheet */
        file.setFileName(QString(":/qss/stylesheet/%1%2.qss").arg(name, locale));
        if (file.open(QFile::ReadOnly)) {
            styleSheet += QLatin1String(file.readAll()) + "\n";
            file.close();
        }

        if (!sheetName.isEmpty() && (sheetName != ":default")) {
            /* load stylesheet */
            if (sheetName.at(0) == ':') {
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

#ifdef Q_OS_WIN
    QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",QSettings::NativeFormat);
    if(/*sheetName == ":default" && */settings.value("AppsUseLightTheme")==0){
        qApp->setStyle(QStyleFactory::create("Fusion"));
        QPalette darkPalette;
        QColor darkColor = QColor(45,45,45);
        QColor disabledColor = QColor(127,127,127);
        darkPalette.setColor(QPalette::Window, darkColor);
        darkPalette.setColor(QPalette::WindowText, Qt::white);
        darkPalette.setColor(QPalette::Base, QColor(18,18,18));
        darkPalette.setColor(QPalette::AlternateBase, darkColor);
        darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
        darkPalette.setColor(QPalette::ToolTipText, Qt::white);
        darkPalette.setColor(QPalette::Text, Qt::white);
        darkPalette.setColor(QPalette::Disabled, QPalette::Text, disabledColor);
        darkPalette.setColor(QPalette::Button, darkColor);
        darkPalette.setColor(QPalette::ButtonText, Qt::white);
        darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, disabledColor);
        darkPalette.setColor(QPalette::BrightText, Qt::red);
        darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));

        darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
        darkPalette.setColor(QPalette::HighlightedText, Qt::black);
        darkPalette.setColor(QPalette::Disabled, QPalette::HighlightedText, disabledColor);

        qApp->setPalette(darkPalette);

        qApp->setStyleSheet("QToolTip { color: #ffffff; background-color: #2a82da; border: 1px solid white; }");
    } 
#endif
    qApp->setStyleSheet(styleSheet);
}

/** get list of available stylesheets **/
void RsApplication::getAvailableStyleSheets(QMap<QString, QString> &styleSheets)
{
	QFileInfoList fileInfoList = QDir(":/qss/stylesheet/").entryInfoList(QStringList("*.qss"));
	QFileInfo fileInfo;
	foreach (fileInfo, fileInfoList) {
		if (fileInfo.isFile()) {
			QString name = fileInfo.baseName();
			styleSheets.insert(QString(" %1 (%2)").arg(name, tr("built-in")), ":" + name);//Add space to name to get them up because QMap sort by Key.
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

void RsApplication::refreshStyleSheet(QWidget *widget, bool processChildren)
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
void RsApplication::initPlugins()
{
    loadStyleSheet(options.guiStyleSheetFile);
    LanguageSupport::translatePlugins(options.language);
}

/** Returns the directory RetroShare uses for its data files. */
QString RsApplication::dataDirectory()
{
    if(!options.optBaseDir.empty())
        return QString::fromUtf8(options.optBaseDir.c_str());
    else
        return defaultDataDirectory();
}

/** Returns the default location of RetroShare's data directory. */
QString
RsApplication::defaultDataDirectory()
{
#if defined(Q_OS_WIN)
  return (win32_app_data_folder() + "\\RetroShare");
#else
  return (QDir::homePath() + "/.retroshare");
#endif
}

/** Creates RsApplication's data directory, if it doesn't already exist. */
bool
RsApplication::createDataDirectory(QString *errmsg)
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

/** Set RsApplication's data directory - externally */
bool RsApplication::setConfigDirectory(const QString& dir)
{
  options.optBaseDir = std::string(dir.toUtf8());
  return true;
}

/** Writes <b>msg</b> with severity <b>level</b> to RetroShare's log. */
Log::LogMessage RsApplication::log(Log::LogLevel level, QString msg)
{
  return log_output.log(level, msg);
}

/** Creates and binds a shortcut such that when <b>key</b> is pressed in
 * <b>sender</b>'s context, <b>receiver</b>'s <b>slot</b> will be called. */
void RsApplication::createShortcut(const QKeySequence &key, QWidget *sender, QWidget *receiver, const char *slot)
{
  QShortcut *s = new QShortcut(key, sender);
  connect(s, SIGNAL(activated()), receiver, slot);
}

#ifdef __APPLE__
bool RsApplication::event(QEvent *event)
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

void RsApplication::blinkTimer()
{
    mBlink = !mBlink;
    emit blink(mBlink);

    if (mBlink) {
        /* Tick every second (create an own timer when needed) */
        emit secondTick();
    }
}

bool RsApplication::loadCertificate(const RsPeerId &accountId, bool autoLogin)
{
	if (!RsAccounts::SelectAccount(accountId))
	{
		return false;
	}

	std::string lockFile;
    RsInit::LoadCertificateStatus retVal = RsInit::LockAndLoadCertificates(autoLogin, lockFile);

    switch (retVal)
    {
    case RsInit::OK:
        break;
    case RsInit::ERR_ALREADY_RUNNING:	QMessageBox::warning(	nullptr,
                                                            QObject::tr("Multiple instances"),
                                                            QObject::tr("Another RetroShare using the same profile is "
                                                                        "already running on your system. Please close "
                                                                        "that instance first\n Lock file:\n") +
                                                            QString::fromUtf8(lockFile.c_str()));
        return false;
    case RsInit::ERR_CANT_ACQUIRE_LOCK:	QMessageBox::critical(	nullptr,
                                                            QObject::tr("Multiple instances"),
                                                            QObject::tr("An unexpected error occurred when Retroshare "
                                                                        "tried to acquire the single instance lock\n Lock file:\n") +
                                                            QString::fromUtf8(lockFile.c_str()));
        return false;
    case RsInit::ERR_CERT_CRYPTO_IS_TOO_WEAK: QMessageBox::critical(	nullptr,
                                                            QObject::tr("Old certificate"),
                                                            QObject::tr("This node uses old certificate settings that are considered too "
                                                                        "weak by your current OpenSSL library version. You need to create a new node "
                                                                        "possibly using the same profile."));
        return false;
    case RsInit::ERR_CANNOT_CONFIGURE_TOR: QMessageBox::critical(	nullptr,
                                                            QObject::tr("Tor error"),
                                                            QObject::tr("Cannot run/configure Tor. Make sure it is installed on your system."));
        return false;
        //		case 3: QMessageBox::critical(	0,
        //										QObject::tr("Login Failure"),
        //										QObject::tr("Maybe password is wrong") );
        return false;
    default: std::cerr << "RsApplication::loadCertificate() unexpected switch value " << retVal << std::endl;
        return false;
    }

	return true;
}

bool RsApplication::updateLocalServer()
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
