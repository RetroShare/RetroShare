/*******************************************************************************
 * gui/settings/rsharesettings.cpp                                             *
 *                                                                             *
 * Copyright (c) 2006-2007, crypton                                            *
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

#include <math.h>
#include <QDir>
#include <QCoreApplication>
#include <QStyleFactory>
#include <QWidget>
#include <lang/languagesupport.h>
#include <rshare.h>

#include "rsharesettings.h"
#include "gui/MainWindow.h"

#ifdef RS_JSONAPI
#include <retroshare/rsjsonapi.h>
#endif

#include <retroshare/rsnotify.h>
#include <retroshare/rspeers.h>

#ifdef Q_OS_WIN
#	include <util/retroshareWin32.h>
#endif

/* Retroshare's Settings */
#define SETTING_LANGUAGE            "LanguageCode"
#define SETTING_STYLE               "InterfaceStyle"
#define SETTING_SHEETNAME           "SheetName"
#define SETTING_PAGEBUTTONLOC       "PageButtonLocation"
#define SETTING_ACTIONBUTTONLOC     "ActionButtonLocation"
#define SETTING_TOOLBUTTONSTYLE     "ToolButtonStyle"
#define SETTING_TOOLBUTTONSIZE      "ToolButtonSize"
#define SETTING_LISTITEMICONSIZE    "ListItemIconSize"

#define SETTING_DATA_DIRECTORY      "DataDirectory"
#define SETTING_BWGRAPH_FILTER        "StatisticDialog/BWLineFilter"
#define SETTING_BWGRAPH_OPACITY       "StatisticDialog/Opacity"
#define SETTING_BWGRAPH_ALWAYS_ON_TOP "StatisticDialog/AlwaysOnTop"

#define SETTING_NEWSFEED_FLAGS		"NewsFeedFlags"
#define SETTING_CHAT_FLAGS		"ChatFlags"
#define SETTING_NOTIFY_FLAGS		"NotifyFlags"
#define SETTING_CHAT_AVATAR			"ChatAvatar"

/* Default Retroshare Settings */
#define DEFAULT_OPACITY             100

#if defined(Q_OS_WIN)
#define STARTUP_REG_KEY        "Software\\Microsoft\\Windows\\CurrentVersion\\Run"
#define RETROSHARE_REG_KEY         "RetroShare" 
#endif

/* Default bandwidth graph settings */
#define DEFAULT_BWGRAPH_FILTER          (BWGRAPH_SEND|BWGRAPH_REC)
#define DEFAULT_BWGRAPH_ALWAYS_ON_TOP   false

/* values for ToolButtonStyle */
#define SETTING_VALUE_TOOLBUTTONSTYLE_ICONONLY        1
#define SETTING_VALUE_TOOLBUTTONSTYLE_TEXTONLY        2
#define SETTING_VALUE_TOOLBUTTONSTYLE_TEXTBESIDEICON  3
#define SETTING_VALUE_TOOLBUTTONSTYLE_TEXTUNDERICON   4

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
#ifdef Q_OS_LINUX
	// use GTK as default style on linux
	setDefault(SETTING_STYLE, "GTK+");
#else
#if defined(Q_OS_MAC)
	setDefault(SETTING_STYLE, "macintosh (aqua)");
#else
	static QStringList styles = QStyleFactory::keys();
#if defined(Q_OS_WIN)
	if (styles.contains("Fusion", Qt::CaseInsensitive))
		setDefault(SETTING_STYLE, "Fusion");
	else if (styles.contains("windowsvista", Qt::CaseInsensitive))
		setDefault(SETTING_STYLE, "windowsvista");
	else
#endif
	{
		if (styles.contains("cleanlooks", Qt::CaseInsensitive))
			setDefault(SETTING_STYLE, "cleanlooks");
		else
			setDefault(SETTING_STYLE, "plastique");
	}
#endif
#endif

	setDefault(SETTING_LANGUAGE, LanguageSupport::defaultLanguageCode());
	setDefault(SETTING_SHEETNAME, ":Standard_Light");

	/* defaults here are not ideal.... but dusent matter */

	uint defChat = RS_CHAT_OPEN;
	// This is not default... RS_CHAT_FOCUS.

	uint defNotify = (RS_POPUP_CONNECT | RS_POPUP_MSG);
    uint defNewsFeed = (RS_FEED_TYPE_MSG | RS_FEED_TYPE_FILES | RS_FEED_TYPE_SECURITY | RS_FEED_TYPE_SECURITY_IP | RS_FEED_TYPE_CIRCLE | RS_FEED_TYPE_CHANNEL |RS_FEED_TYPE_FORUM | RS_FEED_TYPE_POSTED);

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
void RshareSettings::setLanguageCode(const QString& languageCode)
{
	setValue(SETTING_LANGUAGE, languageCode);
}

/** Gets the interface style key (e.g., "windows", "motif", etc.) */
QString RshareSettings::getInterfaceStyle()
{
	return value(SETTING_STYLE, "fusion").toString();
}

/** Sets the interface style key. */
void RshareSettings::setInterfaceStyle(const QString& styleKey)
{
	setValue(SETTING_STYLE, styleKey);
}

/** Gets the sheetname.*/
QString RshareSettings::getSheetName()
{
	return value(SETTING_SHEETNAME).toString();
}
/** Sets the sheetname.*/
void RshareSettings::setSheetName(const QString& sheet)
{ 
	setValue(SETTING_SHEETNAME, sheet);
}

/** Gets the page button Location.*/
bool RshareSettings::getPageButtonLoc()
{
    return value(SETTING_PAGEBUTTONLOC, true).toBool();
}
/** Sets the page button Location.*/
void RshareSettings::setPageButtonLoc(bool onToolBar)
{
	setValue(SETTING_PAGEBUTTONLOC, onToolBar);
}

/** Gets the action button Location.*/
bool RshareSettings::getActionButtonLoc()
{
	return value(SETTING_ACTIONBUTTONLOC, true).toBool();
}
/** Sets the action button Location.*/
void RshareSettings::setActionButtonLoc(bool onToolBar)
{
	setValue(SETTING_ACTIONBUTTONLOC, onToolBar);
}

/** Gets the tool button's style.*/
Qt::ToolButtonStyle RshareSettings::getToolButtonStyle()
{
	int intValue=value(SETTING_TOOLBUTTONSTYLE, SETTING_VALUE_TOOLBUTTONSTYLE_TEXTUNDERICON).toInt();
	switch (intValue)
	{
	case SETTING_VALUE_TOOLBUTTONSTYLE_ICONONLY:
		return Qt::ToolButtonIconOnly;
	case SETTING_VALUE_TOOLBUTTONSTYLE_TEXTONLY:
		return Qt::ToolButtonTextOnly;
	case SETTING_VALUE_TOOLBUTTONSTYLE_TEXTBESIDEICON:
		return Qt::ToolButtonTextBesideIcon;
	case SETTING_VALUE_TOOLBUTTONSTYLE_TEXTUNDERICON:
	default:
		return Qt::ToolButtonTextUnderIcon;
	}
}
/** Sets the tool button's style.*/
void RshareSettings::setToolButtonStyle(Qt::ToolButtonStyle style)
{
	switch (style)
	{
	case Qt::ToolButtonIconOnly:
		setValue(SETTING_TOOLBUTTONSTYLE, SETTING_VALUE_TOOLBUTTONSTYLE_ICONONLY);
		break;
	case Qt::ToolButtonTextOnly:
		setValue(SETTING_TOOLBUTTONSTYLE, SETTING_VALUE_TOOLBUTTONSTYLE_TEXTONLY);
		break;
	case Qt::ToolButtonTextBesideIcon:
		setValue(SETTING_TOOLBUTTONSTYLE, SETTING_VALUE_TOOLBUTTONSTYLE_TEXTBESIDEICON);
		break;
	case Qt::ToolButtonTextUnderIcon:
	default:
		setValue(SETTING_TOOLBUTTONSTYLE, SETTING_VALUE_TOOLBUTTONSTYLE_TEXTUNDERICON);
	}
}

int RshareSettings::computeBestIconSize(int n_sizes,int *sizes,int recommended_size)
{
    float default_size = QFontMetricsF(QWidget().font()).height()/16.0 * recommended_size ;
    float closest_ratio_dist = 10000.0f ;
    int best_default_size = sizes[0] ;
    
    for(int i=0;i<n_sizes;++i)
    {
        float ratio = default_size / sizes[i] ;
        
        if(fabsf(ratio - 1.0f) < closest_ratio_dist)
        {
            closest_ratio_dist = fabsf(ratio-1.0f) ;
            best_default_size = sizes[i] ;
        }
    }
    
    return best_default_size ;
}

/** Gets the tool button's size.*/
int RshareSettings::getToolButtonSize()
{
    static int sizes[6] = { 8,16,24,32,64,128 } ;
    
    return value(SETTING_TOOLBUTTONSIZE, computeBestIconSize(6,sizes,32)).toInt();
}

/** Sets the tool button's size.*/
void RshareSettings::setToolButtonSize(int size)
{
	switch (size)
	{
	case 8:
		setValue(SETTING_TOOLBUTTONSIZE, 8);
		break;
	case 16:
		setValue(SETTING_TOOLBUTTONSIZE, 16);
		break;
	case 24:
		setValue(SETTING_TOOLBUTTONSIZE, 24);
		break;
	case 32:
		default:
		setValue(SETTING_TOOLBUTTONSIZE, 32);
        break;
    case 64:
        setValue(SETTING_TOOLBUTTONSIZE, 64);
        break;
    case 128:
        setValue(SETTING_TOOLBUTTONSIZE, 128);
        break;
    }
}

/** Gets the list item icon's size.*/
int RshareSettings::getListItemIconSize()
{
    static int sizes[6] = { 8,16,24,32,64,128 } ;
    
    return value(SETTING_LISTITEMICONSIZE, computeBestIconSize(6,sizes,24)).toInt();
}

/** Sets the list item icon's size.*/
void RshareSettings::setListItemIconSize(int size)
{
	switch (size)
	{
	case 8:
		setValue(SETTING_LISTITEMICONSIZE, 8);
		break;
	case 16:
		setValue(SETTING_LISTITEMICONSIZE, 16);
		break;
	case 24:
	default:
		setValue(SETTING_LISTITEMICONSIZE, 24);
		break;
	case 32:
		setValue(SETTING_LISTITEMICONSIZE, 32);
            break ;
    case 64:
        setValue(SETTING_LISTITEMICONSIZE, 64);
        break ;
    case 128:
        setValue(SETTING_LISTITEMICONSIZE, 128);
        break ;
    }
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
	case RshareSettings::LASTDIR_PLUGIN:
		return "PLUGIN";
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
	return value("CloseToTray", true).toBool();
}

void RshareSettings::setCloseToTray(bool closeToTray)
{
	setValue("CloseToTray", closeToTray);
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

uint RshareSettings::getMessageFlags()
{
  return value("MessageFlags", RS_MESSAGE_CONNECT_ATTEMPT).toUInt();
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

bool RshareSettings::getChatSendMessageWithCtrlReturn()
{
	return valueFromGroup("Chat", "SendMessageWithCtrlReturn", false).toBool();
}

void RshareSettings::setChatSendMessageWithCtrlReturn(bool bValue)
{
	setValueToGroup("Chat", "SendMessageWithCtrlReturn", bValue);
}

bool RshareSettings::getChatDoNotSendIsTyping()
{
	return valueFromGroup("Chat", "DoNotSendIsTyping", false).toBool();
}

void RshareSettings::setChatDoNotSendIsTyping(bool bValue)
{
	setValueToGroup("Chat", "DoNotSendIsTyping", bValue);
}

bool RshareSettings::getChatSendAsPlainTextByDef()
{
	return valueFromGroup("Chat", "SendAsPlainTextByDef", false).toBool();
}

void RshareSettings::setChatSendAsPlainTextByDef(bool bValue)
{
	setValueToGroup("Chat", "SendAsPlainTextByDef", bValue);
}

bool RshareSettings::getShrinkChatTextEdit()
{
	return valueFromGroup("Chat", "ShrinkChatTextEdit", false).toBool();
}

void RshareSettings::setShrinkChatTextEdit(bool bValue)
{
	setValueToGroup("Chat", "ShrinkChatTextEdit", bValue);
}

bool RshareSettings::getChatSearchShowBarByDefault()
{
	return valueFromGroup("Chat", "SearchShowBarByDefault", false).toBool();
}

void RshareSettings::setChatSearchShowBarByDefault(bool bValue)
{
	setValueToGroup("Chat", "SearchShowBarByDefault", bValue);
}

void RshareSettings::setChatSearchCharToStartSearch(int iValue)
{
	setValueToGroup("Chat", "SearchCharToStartSearch", iValue);
}

int RshareSettings::getChatSearchCharToStartSearch()
{
	return valueFromGroup("Chat", "SearchCharToStartSearch", 4).toUInt();
}

void RshareSettings::setChatSearchCaseSensitively(bool bValue)
{
	setValueToGroup("Chat", "SearchCaseSensitively", bValue);
}

bool RshareSettings::getChatSearchCaseSensitively()
{
	return valueFromGroup("Chat", "SearchCaseSensitively", false).toBool();
}

void RshareSettings::setChatSearchWholeWords(bool bValue)
{
	setValueToGroup("Chat", "SearchWholeWords", bValue);
}

bool RshareSettings::getChatSearchWholeWords()
{
	return valueFromGroup("Chat", "SearchWholeWords", false).toBool();
}

void RshareSettings::setChatSearchMoveToCursor(bool bValue)
{
	setValueToGroup("Chat", "SearchMoveToCursor", bValue);
}

bool RshareSettings::getChatSearchMoveToCursor()
{
	return valueFromGroup("Chat", "SearchMoveToCursor", true).toBool();
}

void RshareSettings::setChatSearchSearchWithoutLimit(bool bValue)
{
	setValueToGroup("Chat", "SearchSearchWithoutLimit", bValue);
}

bool RshareSettings::getChatSearchSearchWithoutLimit()
{
	return valueFromGroup("Chat", "SearchSearchWithoutLimit", false).toBool();
}

void RshareSettings::setChatSearchMaxSearchLimitColor(uint uiValue)
{
	setValueToGroup("Chat", "SearchMaxSearchLimitColor", uiValue);
}

uint RshareSettings::getChatSearchMaxSearchLimitColor()
{
	return valueFromGroup("Chat", "SearchMaxSearchLimitColor", 40).toUInt();
}

void RshareSettings::setChatSearchFoundColor(QRgb rgbValue)
{
	setValueToGroup("Chat", "SearchMaxSearchFoundColor", QString::number(rgbValue));
}
QRgb RshareSettings::getChatSearchFoundColor()
{
	return valueFromGroup("Chat", "SearchMaxSearchFoundColor", QString::number(QColor(255,255,150).rgba())).toUInt();
}

bool RshareSettings::getChatLoadEmbeddedImages()
{
	return valueFromGroup("Chat", "LoadEmbeddedImages", true).toBool();
}

void RshareSettings::setChatLoadEmbeddedImages(bool value)
{
	setValueToGroup("Chat", "LoadEmbeddedImages", value);
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
	stylePath = valueFromGroup("Chat", "StylePublic", ":/qss/chat/compact/public").toString();
	// Correct changed standard path for older RetroShare versions before 31.01.2012 (can be removed later)
	if (stylePath == ":/qss/chat/public") {
		stylePath = ":/qss/chat/standard/public";
	}
	styleVariant = valueFromGroup("Chat", "StylePublicVariant", "Colored").toString();
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
int RshareSettings::getLobbyChatHistoryCount()
{
	return valueFromGroup("Chat", "LobbyChatHistoryCount", 0).toInt();
}

void RshareSettings::setLobbyChatHistoryCount(int value)
{
	setValueToGroup("Chat", "LobbyChatHistoryCount", value);
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

int RshareSettings::getDistantChatHistoryCount()
{
	return valueFromGroup("Chat", "DistantChatHistoryCount", 20).toInt();
}

void RshareSettings::setDistantChatHistoryCount(int value)
{
	setValueToGroup("Chat", "DistantChatHistoryCount", value);
}

/** Returns true if RetroShare is set to run on system boot. */
bool
RshareSettings::runRetroshareOnBoot(bool &minimized)
{
	minimized = false;

#if defined(Q_OS_WIN)
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
#if defined(Q_OS_WIN)
	if (run) {
		QString value = "\"" + QDir::toNativeSeparators(QCoreApplication::applicationFilePath()) + "\"";

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

static QString getAppPathForProtocol()
{
#if defined(Q_OS_WIN)
	return "\"" + QDir::toNativeSeparators(QCoreApplication::applicationFilePath()) + "\" -r \"%1\"";
#elif defined(Q_OS_LINUX)
	return QDir::toNativeSeparators(QCoreApplication::applicationFilePath()) + " %U";
#endif
}

/** Returns true if retroshare:// is registered as protocol */
bool RshareSettings::getRetroShareProtocol()
{
#if defined(Q_OS_WIN)
	/* Check key */
	QSettings retroshare("HKEY_CURRENT_USER\\Software\\Classes\\retroshare", QSettings::NativeFormat);
	if (retroshare.contains("Default")) {
		/* URL Protocol */
		if (retroshare.contains("URL Protocol")) {
			/* Check app path */
			QSettings command("HKEY_CURRENT_USER\\Software\\Classes\\retroshare\\shell\\open\\command", QSettings::NativeFormat);
			if (command.value("Default").toString() == getAppPathForProtocol()) {
				return true;
			}
		}
	}
#elif defined(Q_OS_LINUX)
	QFile desktop("/usr/share/applications/retroshare.desktop");
	if (!desktop.exists()) {
		desktop.setFileName("/usr/share/applications/retroshare.desktop");
	}
	if (desktop.exists()) {
		desktop.open(QIODevice::ReadOnly | QIODevice::Text);
		QTextStream in(&desktop);
		QStringList lines;
		while(!in.atEnd()) {
			lines << in.readLine();
		}
		desktop.close();
		if (lines.contains("Exec=" + getAppPathForProtocol()))
			if (lines.contains("MimeType=x-scheme-handler/retroshare;"))
				return true;
	}
#else
	/* Platforms not supported yet */
#endif
	return false;
}

/** Returns true if the user can set retroshare as protocol */
bool RshareSettings::canSetRetroShareProtocol()
{
#if defined(Q_OS_WIN)
	QSettings classRoot("HKEY_CURRENT_USER\\Software\\Classes", QSettings::NativeFormat);
	return classRoot.isWritable();
#elif defined(Q_OS_LINUX)
	return RshareSettings::getRetroShareProtocol();
#else
	return false;
#endif
}

/** Register retroshare:// as protocol */
bool RshareSettings::setRetroShareProtocol(bool value, QString &error)
{
	error = "";
#if defined(Q_OS_WIN)
	if (value) {
		QSettings retroshare("HKEY_CURRENT_USER\\Software\\Classes\\retroshare", QSettings::NativeFormat);
		retroshare.setValue("Default", "URL: RetroShare protocol");

		QSettings::Status state = retroshare.status();
		if (state == QSettings::AccessError) {
			error = tr("Registry Access Error. Maybe you need Administrator right.");
			return false;
		}
		retroshare.setValue("URL Protocol", "");

		QSettings command("HKEY_CURRENT_USER\\Software\\Classes\\retroshare\\shell\\open\\command", QSettings::NativeFormat);
		command.setValue("Default", getAppPathForProtocol());
		//state = command.status();
	} else {
		QSettings classRoot("HKEY_CURRENT_USER\\Software\\Classes", QSettings::NativeFormat);
		classRoot.remove("retroshare");

		QSettings::Status state = classRoot.status();
		if (state == QSettings::AccessError) {
			error = tr("Registry Access Error. Maybe you need Administrator right.");
			return false;
		}
	}

	return true;
#elif defined(Q_OS_LINUX)
	/* RetroShare protocol is always activated for Linux */
	Q_UNUSED(value);
	return true;
#else
	/* Platforms not supported yet */
	Q_UNUSED(value);
	return false;
#endif
}

/** Returns true if this instance have to run Local Server*/
bool RshareSettings::getUseLocalServer()
{
	return value("UseLocalServer", true).toBool();
}

/** Sets whether to run Local Server */
void RshareSettings::setUseLocalServer(bool value)
{
	if (value != getUseLocalServer()) {
		setValue("UseLocalServer", value);
		Rshare::updateLocalServer();
	}
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

/* Message */
bool RshareSettings::getMsgSetToReadOnActivate ()
{
	return valueFromGroup("Message", "SetMsgToReadOnActivate", true).toBool();
}

void RshareSettings::setMsgSetToReadOnActivate (bool value)
{
	setValueToGroup("Message", "SetMsgToReadOnActivate", value);
}

bool RshareSettings::getMsgLoadEmbeddedImages()
{
	return valueFromGroup("Message", "LoadEmbeddedImages", false).toBool();
}

void RshareSettings::setMsgLoadEmbeddedImages(bool value)
{
	setValueToGroup("Message", "LoadEmbeddedImages", value);
}

RshareSettings::enumMsgOpen RshareSettings::getMsgOpen()
{
	enumMsgOpen value = (enumMsgOpen) valueFromGroup("Message", "msgOpen", MSG_OPEN_TAB).toInt();

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

	setValueToGroup("Message", "msgOpen", value);
}

/* Forum */
bool RshareSettings::getForumMsgSetToReadOnActivate ()
{
	return valueFromGroup("Forum", "SetMsgToReadOnActivate", true).toBool();
}

void RshareSettings::setForumMsgSetToReadOnActivate(bool value)
{
	setValueToGroup("Forum", "SetMsgToReadOnActivate", value);
}

bool RshareSettings::getForumExpandNewMessages()
{
	return valueFromGroup("Forum", "ExpandNewMessages", true).toBool();
}

void RshareSettings::setForumExpandNewMessages(bool value)
{
	setValueToGroup("Forum", "ExpandNewMessages", value);
}

bool RshareSettings::getForumLoadEmbeddedImages()
{
	return valueFromGroup("Forum", "LoadEmbeddedImages", false).toBool();
}
bool RshareSettings::getForumLoadEmoticons()
{
	return valueFromGroup("Forum", "LoadEmoticons", false).toBool();
}
void RshareSettings::setForumLoadEmbeddedImages(bool value)
{
	setValueToGroup("Forum", "LoadEmbeddedImages", value);
}
void RshareSettings::setForumLoadEmoticons(bool value)
{
	setValueToGroup("Forum", "LoadEmoticons", value);
}
/* Channel */
bool RshareSettings::getChannelLoadThread()
{
	return valueFromGroup("Channel", "LoadThread", false).toBool();
}

void RshareSettings::setChannelLoadThread(bool value)
{
	setValueToGroup("Channel", "LoadThread", value);
}

/* GroupFrame settings */
static QString groupFrameSettingsTypeToString(GroupFrameSettings::Type type)
{
	switch (type) {
	case GroupFrameSettings::Nothing:
		return "";
	case GroupFrameSettings::Forum:
		return "Forum";
	case GroupFrameSettings::Channel:
		return "Channel";
	case GroupFrameSettings::Posted:
		return "Posted";
	}

	return "";
}

bool RshareSettings::getGroupFrameSettings(GroupFrameSettings::Type type, GroupFrameSettings &groupFrameSettings)
{
	QString group = groupFrameSettingsTypeToString(type);
	if (group.isEmpty()) {
		return false;
	}

	groupFrameSettings.mOpenAllInNewTab = valueFromGroup(group, "OpenAllInNewTab", false).toBool();
	groupFrameSettings.mHideTabBarWithOneTab = valueFromGroup(group, "HideTabBarWithOneTab", true).toBool();

	return true;
}

void RshareSettings::setGroupFrameSettings(GroupFrameSettings::Type type, const GroupFrameSettings &groupFrameSettings)
{
	QString group = groupFrameSettingsTypeToString(type);
	if (group.isEmpty()) {
		return;
	}

	setValueToGroup(group, "OpenAllInNewTab", groupFrameSettings.mOpenAllInNewTab);
	setValueToGroup(group, "HideTabBarWithOneTab", groupFrameSettings.mHideTabBarWithOneTab);
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

bool RshareSettings::getWebinterfaceEnabled()
{
    return valueFromGroup("Webinterface", "enabled", false).toBool();
}

void RshareSettings::setWebinterfaceEnabled(bool enabled)
{
    setValueToGroup("Webinterface", "enabled", enabled);
}

QString RshareSettings::getWebinterfaceFilesDirectory()
{
#ifdef WINDOWS_SYS
	return valueFromGroup("Webinterface","directory","./webui/").toString();
#else
    return valueFromGroup("Webinterface","directory","/usr/share/retroshare/webui/").toString();
#endif
}

void RshareSettings::setWebinterfaceFilesDirectory(const QString& s)
{
    setValueToGroup("Webinterface","directory",s);
}


bool RshareSettings::getPageAlreadyDisplayed(const QString& page_name)
{
	return valueFromGroup("PageAlreadyDisplayed",page_name,false).toBool();
}


void RshareSettings::setPageAlreadyDisplayed(const QString& page_name,bool b)
{
	return setValueToGroup("PageAlreadyDisplayed",page_name,b);
}

#ifdef RS_JSONAPI
bool RshareSettings::getJsonApiEnabled()
{
	return valueFromGroup("JsonApi", "enabled", false).toBool();
}

void RshareSettings::setJsonApiEnabled(bool enabled)
{
	setValueToGroup("JsonApi", "enabled", enabled);
}

uint16_t RshareSettings::getJsonApiPort()
{
	return static_cast<uint16_t>(
	            valueFromGroup(
	                "JsonApi", "port", RsJsonApi::DEFAULT_PORT ).toUInt() );
}

void RshareSettings::setJsonApiPort(uint16_t port)
{
	setValueToGroup("JsonApi", "port", port);
}

QString RshareSettings::getJsonApiListenAddress()
{
	return valueFromGroup("JsonApi", "listenAddress", "127.0.0.1").toString();
}

void RshareSettings::setJsonApiListenAddress(const QString& listenAddress)
{
	setValueToGroup("JsonApi", "listenAddress", listenAddress);
}

QStringList RshareSettings::getJsonApiAuthTokens()
{
	return valueFromGroup("JsonApi", "authTokens", QStringList()).toStringList();
}

void RshareSettings::setJsonApiAuthTokens(const QStringList& authTokens)
{
	setValueToGroup("JsonApi", "authTokens", authTokens);
}
#endif // RS_JSONAPI
