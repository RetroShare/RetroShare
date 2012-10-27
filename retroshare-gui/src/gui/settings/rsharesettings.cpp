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
#include <QCoreApplication>
#include <QStyleFactory>
#include <QWidget>
#include <lang/languagesupport.h>
#include <rshare.h>

#include "rsharesettings.h"
#include "gui/MainWindow.h"

#include <retroshare/rsnotify.h>
#include <retroshare/rspeers.h>

#if defined(Q_WS_WIN)
#include <util/win32.h>
#endif


/* Retroshare's Settings */
#define SETTING_LANGUAGE            "LanguageCode"
#define SETTING_STYLE               "InterfaceStyle"
#define SETTING_SHEETNAME           "SheetName"

#define SETTING_DATA_DIRECTORY      "DataDirectory"
#define SETTING_BWGRAPH_FILTER        "StatisticDialog/BWLineFilter"
#define SETTING_BWGRAPH_OPACITY       "StatisticDialog/Opacity"
#define SETTING_BWGRAPH_ALWAYS_ON_TOP "StatisticDialog/AlwaysOnTop"

#define SETTING_NEWSFEED_FLAGS		"NewsFeedFlags"
#define SETTING_CHAT_FLAGS		"ChatFlags"
#define SETTING_NOTIFY_FLAGS		"NotifyFlags"
#define SETTING_TRAYNOTIFY_FLAGS	"TrayNotifyFlags"
#define SETTING_CHAT_AVATAR			"ChatAvatar"

/* Default Retroshare Settings */
#define DEFAULT_OPACITY             100


#if defined(Q_OS_WIN32)
#define STARTUP_REG_KEY        "Software\\Microsoft\\Windows\\CurrentVersion\\Run"
#define RETROSHARE_REG_KEY         "RetroShare" 
#endif

/* Default bandwidth graph settings */
#define DEFAULT_BWGRAPH_FILTER          (BWGRAPH_SEND|BWGRAPH_REC)
#define DEFAULT_BWGRAPH_ALWAYS_ON_TOP   false

// the one and only global settings object
RshareSettings *Settings = NULL;

/*static*/ void RshareSettings::Create(bool forceCreateNew)
{
    if (Settings && (forceCreateNew || Settings->m_bValid == false)) {
        // recreate with correct path
        delete (Settings);
        Settings = NULL;
    }
    if (Settings == NULL) {
        Settings = new RshareSettings ();
    }
}

/** Default Constructor */
RshareSettings::RshareSettings()
{
    m_maxTimeBeforeIdle = -1;

    initSettings();
}

void RshareSettings::initSettings()
{ 
#if defined(Q_WS_MAC)
  setDefault(SETTING_STYLE, "macintosh (aqua)");
#else
  static QStringList styles = QStyleFactory::keys();
#if defined(Q_WS_WIN)
  if (styles.contains("windowsvista", Qt::CaseInsensitive))
    setDefault(SETTING_STYLE, "windowsvista");
  else if (styles.contains("windowsxp", Qt::CaseInsensitive))
    setDefault(SETTING_STYLE, "windowsxp");
  else
#endif
  {
    if (styles.contains("cleanlooks", Qt::CaseInsensitive))
      setDefault(SETTING_STYLE, "cleanlooks");
    else
      setDefault(SETTING_STYLE, "plastique");
  }
#endif

  setDefault(SETTING_LANGUAGE, LanguageSupport::defaultLanguageCode());
  setDefault(SETTING_SHEETNAME, ":Standard");

  /* defaults here are not ideal.... but dusent matter */

  uint defChat = RS_CHAT_OPEN;
		// This is not default... RS_CHAT_FOCUS.

  uint defNotify = (RS_POPUP_CONNECT | RS_POPUP_MSG);

  uint defNewsFeed = (RS_FEED_TYPE_PEER | RS_FEED_TYPE_CHAN |
                RS_FEED_TYPE_FORUM | RS_FEED_TYPE_BLOG |
                RS_FEED_TYPE_CHAT | RS_FEED_TYPE_MSG |
                RS_FEED_TYPE_FILES | RS_FEED_TYPE_SECURITY);

  setDefault(SETTING_NEWSFEED_FLAGS, defNewsFeed);
  setDefault(SETTING_CHAT_FLAGS, defChat);
  setDefault(SETTING_NOTIFY_FLAGS, defNotify);

  setDefault("DisplayTrayGroupChat", true);
  setDefault("AddFeedsAtEnd", false);
}

/** Gets the currently preferred language code for Rshare. */
QString RshareSettings::getLanguageCode()
{
  return value(SETTING_LANGUAGE).toString();
}

/** Sets the preferred language code. */
void RshareSettings::setLanguageCode(QString languageCode)
{
  setValue(SETTING_LANGUAGE, languageCode);
}

/** Gets the interface style key (e.g., "windows", "motif", etc.) */
QString RshareSettings::getInterfaceStyle()
{
  return value(SETTING_STYLE).toString();
}

/** Sets the interface style key. */
void RshareSettings::setInterfaceStyle(QString styleKey)
{
  setValue(SETTING_STYLE, styleKey);
}

/** Gets the sheetname.*/
QString RshareSettings::getSheetName()
{
  return value(SETTING_SHEETNAME).toString();
}
/** Sets the sheetname.*/
void RshareSettings::setSheetName(QString sheet)                                  
{ 
    setValue(SETTING_SHEETNAME, sheet);
}

static QString getKeyForLastDir(RshareSettings::enumLastDir type)
{
    switch (type) {
    case RshareSettings::LASTDIR_EXTRAFILE:
        return "ExtraFile";
    case RshareSettings::LASTDIR_CERT:
        return "Certificate";
    case RshareSettings::LASTDIR_HISTORY:
        return "History";
    case RshareSettings::LASTDIR_IMAGES:
        return "Images";
    case RshareSettings::LASTDIR_MESSAGES:
        return "Messages";
    case RshareSettings::LASTDIR_BLOGS:
        return "Messages";
    case RshareSettings::LASTDIR_SOUNDS:
        return "SOUNDS";
    }
    return "";
}

QString RshareSettings::getLastDir(enumLastDir type)
{
    QString key = getKeyForLastDir(type);
    if (key.isEmpty()) {
        return "";
    }

    return valueFromGroup("LastDir", key).toString();
}

void RshareSettings::setLastDir(enumLastDir type, const QString &lastDir)
{
    QString key = getKeyForLastDir(type);
    if (key.isEmpty()) {
        return;
    }

    setValueToGroup("LastDir", key, lastDir);
}

/** Returns the bandwidth line filter. */
uint RshareSettings::getBWGraphFilter()
{
  return value(SETTING_BWGRAPH_FILTER, DEFAULT_BWGRAPH_FILTER).toUInt(); 
}

/** Saves the setting for whether or not the given line will be graphed */
void RshareSettings::setBWGraphFilter(uint line, bool status)
{
  uint filter = getBWGraphFilter();
  filter = (status ? (filter | line) : (filter & ~(line)));
  setValue(SETTING_BWGRAPH_FILTER, filter);
}

/** Get the level of opacity for the BandwidthGraph window. */
int RshareSettings::getBWGraphOpacity()
{
  return value(SETTING_BWGRAPH_OPACITY, DEFAULT_OPACITY).toInt();
}

/** Set the level of opacity for the BandwidthGraph window. */
void RshareSettings::setBWGraphOpacity(int value)
{
  setValue(SETTING_BWGRAPH_OPACITY, value);
}

/** Gets whether the bandwidth graph is always on top when displayed. */
bool RshareSettings::getBWGraphAlwaysOnTop()
{
  return value(SETTING_BWGRAPH_ALWAYS_ON_TOP,
               DEFAULT_BWGRAPH_ALWAYS_ON_TOP).toBool();
}

/** Sets whether the bandwidth graph is always on top when displayed. */
void RshareSettings::setBWGraphAlwaysOnTop(bool alwaysOnTop)
{
  setValue(SETTING_BWGRAPH_ALWAYS_ON_TOP, alwaysOnTop);
}

/** Returns true if RetroShare's main window should be visible when the
 * application starts. */
bool RshareSettings::getStartMinimized()
{
  return value("StartMinimized", false).toBool();
}

/** Sets whether to show RetroShare's main window when the application starts. */
void RshareSettings::setStartMinimized(bool startMinimized)
{
  setValue("StartMinimized", startMinimized);
}

bool RshareSettings::getCloseToTray()
{
  return value("ClosetoTray", false).toBool();
}

void RshareSettings::setCloseToTray(bool closeToTray)
{
  setValue("ClosetoTray", closeToTray);
}

/** Setting for Notify / Chat and NewsFeeds **/
uint RshareSettings::getNewsFeedFlags()
{
  return value(SETTING_NEWSFEED_FLAGS).toUInt();
}

void RshareSettings::setNewsFeedFlags(uint flags)
{
  setValue(SETTING_NEWSFEED_FLAGS, flags);
}

uint RshareSettings::getChatFlags()
{
  return value(SETTING_CHAT_FLAGS).toUInt();
}

void RshareSettings::setChatFlags(uint flags)
{
  setValue(SETTING_CHAT_FLAGS, flags);
}

uint RshareSettings::getChatLobbyFlags()
{
  return value("ChatLobbyFlags").toUInt();
}

void RshareSettings::setChatLobbyFlags(uint flags)
{
  setValue("ChatLobbyFlags", flags);
}

uint RshareSettings::getNotifyFlags()
{
  return value(SETTING_NOTIFY_FLAGS).toUInt();
}

void RshareSettings::setNotifyFlags(uint flags)
{
  setValue(SETTING_NOTIFY_FLAGS, flags);
}

uint RshareSettings::getTrayNotifyFlags()
{
  return value(SETTING_TRAYNOTIFY_FLAGS, TRAYNOTIFY_PRIVATECHAT | TRAYNOTIFY_MESSAGES | TRAYNOTIFY_CHANNELS | TRAYNOTIFY_FORUMS ).toUInt();
}

void RshareSettings::setTrayNotifyFlags(uint flags)
{
  setValue(SETTING_TRAYNOTIFY_FLAGS, flags);
}

uint RshareSettings::getTrayNotifyBlinkFlags()
{
  return value("TrayNotifyBlinkFlags", 0).toUInt();
}

void RshareSettings::setTrayNotifyBlinkFlags(uint flags)
{
  setValue("TrayNotifyBlinkFlags", flags);
}

uint RshareSettings::getMessageFlags()
{
    return value("MessageFlags").toUInt();
}

void RshareSettings::setMessageFlags(uint flags)
{
    setValue("MessageFlags", flags);
}

bool RshareSettings::getDisplayTrayChatLobby()
{
    return value("DisplayTrayChatLobby").toBool();
}

bool RshareSettings::getDisplayTrayGroupChat()
{
    return value("DisplayTrayGroupChat").toBool();
}

void RshareSettings::setDisplayTrayChatLobby(bool bValue)
{
    setValue("DisplayTrayChatLobby", bValue);
}

void RshareSettings::setDisplayTrayGroupChat(bool bValue)
{
    setValue("DisplayTrayGroupChat", bValue);
}

bool RshareSettings::getAddFeedsAtEnd()
{
    return value("AddFeedsAtEnd").toBool();
}

void RshareSettings::setAddFeedsAtEnd(bool bValue)
{
    setValue("AddFeedsAtEnd", bValue);
}

bool RshareSettings::getChatSendMessageWithCtrlReturn()
{
    return valueFromGroup("Chat", "SendMessageWithCtrlReturn", false).toBool();
}

RshareSettings::enumToasterPosition RshareSettings::getToasterPosition()
{
    return (enumToasterPosition) value("ToasterPosition", TOASTERPOS_BOTTOMRIGHT).toInt();
}

void RshareSettings::setToasterPosition(enumToasterPosition position)
{
    setValue("ToasterPosition", position);
}

QPoint RshareSettings::getToasterMargin()
{
    return value("ToasterMargin", QPoint(10, 10)).toPoint();
}

void RshareSettings::setToasterMargin(QPoint margin)
{
    setValue("ToasterMargin", margin);
}

void RshareSettings::setChatSendMessageWithCtrlReturn(bool bValue)
{
    setValueToGroup("Chat", "SendMessageWithCtrlReturn", bValue);
}

QString RshareSettings::getChatScreenFont()
{
    return valueFromGroup("Chat", "ChatScreenFont").toString();
}

void RshareSettings::setChatScreenFont(const QString &font)
{
    setValueToGroup("Chat", "ChatScreenFont", font);
}

void RshareSettings::getPublicChatStyle(QString &stylePath, QString &styleVariant)
{
    stylePath = valueFromGroup("Chat", "StylePublic", ":/qss/chat/standard/public").toString();
    // Correct changed standard path for older RetroShare versions before 31.01.2012 (can be removed later)
    if (stylePath == ":/qss/chat/public") {
        stylePath = ":/qss/chat/standard/public";
    }
    styleVariant = valueFromGroup("Chat", "StylePublicVariant", "").toString();
}

void RshareSettings::setPublicChatStyle(const QString &stylePath, const QString &styleVariant)
{
    setValueToGroup("Chat", "StylePublic", stylePath);
    setValueToGroup("Chat", "StylePublicVariant", styleVariant);
}

void RshareSettings::getPrivateChatStyle(QString &stylePath, QString &styleVariant)
{
    stylePath = valueFromGroup("Chat", "StylePrivate", ":/qss/chat/standard/private").toString();
    // Correct changed standard path for older RetroShare versions before 31.01.2012 (can be removed later)
    if (stylePath == ":/qss/chat/private") {
        stylePath = ":/qss/chat/standard/private";
    }
    styleVariant = valueFromGroup("Chat", "StylePrivateVariant", "").toString();
}

void RshareSettings::setPrivateChatStyle(const QString &stylePath, const QString &styleVariant)
{
    setValueToGroup("Chat", "StylePrivate", stylePath);
    setValueToGroup("Chat", "StylePrivateVariant", styleVariant);
}

void RshareSettings::getHistoryChatStyle(QString &stylePath, QString &styleVariant)
{
    stylePath = valueFromGroup("Chat", "StyleHistory", ":/qss/chat/standard/history").toString();
    // Correct changed standard path for older RetroShare versions before 31.01.2012 (can be removed later)
    if (stylePath == ":/qss/chat/history") {
        stylePath = ":/qss/chat/standard/history";
    }
    styleVariant = valueFromGroup("Chat", "StylePrivateVariant", "").toString();
}

void RshareSettings::setHistoryChatStyle(const QString &stylePath, const QString &styleVariant)
{
    setValueToGroup("Chat", "StyleHistory", stylePath);
    setValueToGroup("Chat", "StylePrivateVariant", styleVariant);
}

int RshareSettings::getPublicChatHistoryCount()
{
    return valueFromGroup("Chat", "PublicChatHistoryCount", 0).toInt();
}

void RshareSettings::setPublicChatHistoryCount(int value)
{
    setValueToGroup("Chat", "PublicChatHistoryCount", value);
}

int RshareSettings::getPrivateChatHistoryCount()
{
    return valueFromGroup("Chat", "PrivateChatHistoryCount", 20).toInt();
}

void RshareSettings::setPrivateChatHistoryCount(int value)
{
    setValueToGroup("Chat", "PrivateChatHistoryCount", value);
}

/** Returns true if RetroShare is set to run on system boot. */
bool
RshareSettings::runRetroshareOnBoot(bool &minimized)
{
  minimized = false;

#if defined(Q_WS_WIN)
  QString value = win32_registry_get_key_value(STARTUP_REG_KEY, RETROSHARE_REG_KEY);

  if (!value.isEmpty()) {
    /* Simple check for "-m" */
    minimized = value.contains(" -m");
    return true;
  }
  return false;
#else
  /* Platforms other than windows aren't supported yet */
  return false;
#endif
}

/** If <b>run</b> is set to true, then RetroShare will run on system boot. */
void
RshareSettings::setRunRetroshareOnBoot(bool run, bool minimized)
{
#if defined(Q_WS_WIN)
  if (run) {
    QString value = "\"" + QDir::convertSeparators(QCoreApplication::applicationFilePath()) + "\"";

    if (minimized) {
      value += " -m";
    }

    win32_registry_set_key_value(STARTUP_REG_KEY, RETROSHARE_REG_KEY, value);
  } else {
    win32_registry_remove_key(STARTUP_REG_KEY, RETROSHARE_REG_KEY);
  }
#else
  /* Platforms othe rthan windows aren't supported yet */
  Q_UNUSED(run);
  Q_UNUSED(minimized);
#endif
}

#if defined(Q_WS_WIN)
static QString getAppPathForProtocol()
{
	return "\"" + QDir::convertSeparators(QCoreApplication::applicationFilePath()) + "\" -r \"%1\"";
}
#endif

/** Returns true if retroshare:// is registered as protocol */
bool RshareSettings::getRetroShareProtocol()
{
#if defined(Q_WS_WIN)
	/* Check key */
	QSettings retroshare("HKEY_CLASSES_ROOT\\retroshare", QSettings::NativeFormat);
	if (retroshare.contains("Default")) {
		/* Check profile */
		if (retroshare.value("Profile").toString().toStdString() == rsPeers->getOwnId()) {
			/* Check app path */
			QSettings command("HKEY_CLASSES_ROOT\\retroshare\\shell\\open\\command", QSettings::NativeFormat);
			if (command.value("Default").toString() == getAppPathForProtocol()) {
				return true;
			}
		}
	}
#else
	/* Platforms other than windows aren't supported yet */
#endif
	return false;
}

/** Returns true if the user can set retroshare as protocol */
bool RshareSettings::canSetRetroShareProtocol()
{
#if defined(Q_WS_WIN)
	QSettings retroshare("HKEY_CLASSES_ROOT\\retroshare", QSettings::NativeFormat);
	return retroshare.isWritable();
#else
	return false;
#endif
}

/** Register retroshare:// as protocl */
bool RshareSettings::setRetroShareProtocol(bool value)
{
#if defined(Q_WS_WIN)
	if (value) {
		QSettings retroshare("HKEY_CLASSES_ROOT\\retroshare", QSettings::NativeFormat);
		retroshare.setValue("Default", "URL: RetroShare protocol");

		QSettings::Status state = retroshare.status();
		if (state == QSettings::AccessError) {
			return false;
		}
		retroshare.setValue("URL Protocol", "");
		retroshare.setValue("Profile", QString::fromStdString(rsPeers->getOwnId()));

		QSettings command("HKEY_CLASSES_ROOT\\retroshare\\shell\\open\\command", QSettings::NativeFormat);
		command.setValue("Default", getAppPathForProtocol());
		state = command.status();
	} else {
		QSettings retroshare("HKEY_CLASSES_ROOT", QSettings::NativeFormat);
		retroshare.remove("retroshare");

		QSettings::Status state = retroshare.status();
		if (state == QSettings::AccessError) {
			return false;
		}
	}

	return true;
#else
	/* Platforms other than windows aren't supported yet */
	Q_UNUSED(value);

	return false;
#endif
}

/** Saving Generic Widget Size / Location */

void RshareSettings::saveWidgetInformation(QWidget *widget)
{
    beginGroup("widgetInformation");
    beginGroup(widget->objectName());

    setValue("size", widget->size());
    setValue("pos", widget->pos());

    endGroup();
    endGroup();
}

void RshareSettings::saveWidgetInformation(QMainWindow *widget, QToolBar *toolBar)
{
 beginGroup("widgetInformation");
 beginGroup(widget->objectName());

 setValue("toolBarArea", widget->toolBarArea(toolBar));

 endGroup();
 endGroup();
 
 saveWidgetInformation(widget);
}

void RshareSettings::loadWidgetInformation(QWidget *widget)
{
 beginGroup("widgetInformation");
 beginGroup(widget->objectName());
 
 widget->resize(value("size", widget->size()).toSize());
 widget->move(value("pos", QPoint(200, 200)).toPoint());

 endGroup();
 endGroup();
}

void RshareSettings::loadWidgetInformation(QMainWindow *widget, QToolBar *toolBar)
{
 beginGroup("widgetInformation");
 beginGroup(widget->objectName());
 
 widget->addToolBar((Qt::ToolBarArea) value("toolBarArea", Qt::TopToolBarArea).toInt(),
                    toolBar);
 
 endGroup();
 endGroup();
 
 loadWidgetInformation(widget);
}

/* MainWindow */
int RshareSettings::getLastPageInMainWindow ()
{
    return valueFromGroup("MainWindow", "LastPage", MainWindow::Network).toInt();
}

void RshareSettings::setLastPageInMainWindow (int value)
{
    setValueToGroup("MainWindow", "LastPage", value);
}

uint RshareSettings::getStatusBarFlags()
{
    /* Default = All but disc status */
    return valueFromGroup("MainWindow", "StatusBarFlags", 0xFFFFFFFF ^ STATUSBAR_DISC).toUInt();
}

void RshareSettings::setStatusBarFlags(uint flags)
{
    setValueToGroup("MainWindow", "StatusBarFlags", flags);
}

void RshareSettings::setStatusBarFlag(uint flag, bool enable)
{
    uint flags = getStatusBarFlags();

    if (enable) {
        flags |= flag;
    } else {
        flags &= ~flag;
    }

    setStatusBarFlags(flags);
}

/* Messages */
bool RshareSettings::getMsgSetToReadOnActivate ()
{
    return valueFromGroup("MessageDialog", "SetMsgToReadOnActivate", true).toBool();
}

void RshareSettings::setMsgSetToReadOnActivate (bool bValue)
{
    setValueToGroup("MessageDialog", "SetMsgToReadOnActivate", bValue);
}

RshareSettings::enumMsgOpen RshareSettings::getMsgOpen()
{
    enumMsgOpen value = (enumMsgOpen) valueFromGroup("MessageDialog", "msgOpen", MSG_OPEN_TAB).toInt();

    switch (value) {
    case MSG_OPEN_TAB:
    case MSG_OPEN_WINDOW:
        return value;
    }

    return MSG_OPEN_TAB;
}

void RshareSettings::setMsgOpen(enumMsgOpen value)
{
    switch (value) {
    case MSG_OPEN_TAB:
    case MSG_OPEN_WINDOW:
        break;
    default:
        value = MSG_OPEN_TAB;
    }

    setValueToGroup("MessageDialog", "msgOpen", value);
}

/* Forums */
bool RshareSettings::getForumMsgSetToReadOnActivate ()
{
    return valueFromGroup("ForumDialog", "SetMsgToReadOnActivate", true).toBool();
}

void RshareSettings::setForumMsgSetToReadOnActivate (bool bValue)
{
    setValueToGroup("ForumDialog", "SetMsgToReadOnActivate", bValue);
}

bool RshareSettings::getExpandNewMessages()
{
    return valueFromGroup("ForumDialog", "ExpandNewMessages", true).toBool();
}

void RshareSettings::setExpandNewMessages (bool bValue)
{
    setValueToGroup("ForumDialog", "ExpandNewMessages", bValue);
}

/* time before idle */
uint RshareSettings::getMaxTimeBeforeIdle()
{
    if (m_maxTimeBeforeIdle == -1) {
        m_maxTimeBeforeIdle = value("maxTimeBeforeIdle", 300).toUInt();
    }

    return m_maxTimeBeforeIdle;
}

void RshareSettings::setMaxTimeBeforeIdle(uint nValue)
{
    m_maxTimeBeforeIdle = nValue;
    setValue("maxTimeBeforeIdle", nValue);
}
