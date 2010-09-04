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
#include <lang/languagesupport.h>
#include <rshare.h>

#include "rsharesettings.h"

#include <retroshare/rsnotify.h>

#include <QWidget>
#include <QMainWindow>

#if defined(Q_WS_WIN)
#include <util/win32.h>
#endif


/* Retroshare's Settings */
#define SETTING_LANGUAGE            "LanguageCode"
#define SETTING_STYLE               "InterfaceStyle"
#define SETTING_SHEETNAME           "SheetName"

#define SETTING_DATA_DIRECTORY      "DataDirectory"
#define SETTING_SHOW_MAINWINDOW_AT_START  "ShowMainWindowAtStart"
#define SETTING_BWGRAPH_FILTER        "StatisticDialog/BWLineFilter"
#define SETTING_BWGRAPH_OPACITY       "StatisticDialog/Opacity"
#define SETTING_BWGRAPH_ALWAYS_ON_TOP "StatisticDialog/AlwaysOnTop"

#define SETTING_NEWSFEED_FLAGS		"NewsFeedFlags"
#define SETTING_CHAT_FLAGS		"ChatFlags"
#define SETTING_NOTIFY_FLAGS		"NotifyFlags"
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

/*static*/ void RshareSettings::Create ()
{
    if (Settings && Settings->m_bValid == false) {
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
  setDefault(SETTING_SHEETNAME, true); 
  setDefault(SETTING_SHOW_MAINWINDOW_AT_START, true);

  /* defaults here are not ideal.... but dusent matter */

  uint defChat = (RS_CHAT_OPEN_NEW |
		RS_CHAT_REOPEN );
		// This is not default... RS_CHAT_FOCUS.

  uint defNotify = (RS_POPUP_CONNECT | RS_POPUP_MSG |
                	RS_POPUP_CHAT | RS_POPUP_CALL);

  uint defNewsFeed = (RS_FEED_TYPE_PEER | RS_FEED_TYPE_CHAN |
                RS_FEED_TYPE_FORUM | RS_FEED_TYPE_BLOG |
                RS_FEED_TYPE_CHAT | RS_FEED_TYPE_MSG |
                RS_FEED_TYPE_FILES);

  setDefault(SETTING_NEWSFEED_FLAGS, defNewsFeed);
  setDefault(SETTING_CHAT_FLAGS, defChat);
  setDefault(SETTING_NOTIFY_FLAGS, defNotify);

  setDefault("DisplayTrayGroupChat", true);
  setDefault("AddFeedsAtEnd", false);
}

/** Gets/sets the currently saved chat avatar. */
QImage RshareSettings::getChatAvatar() const
{
  return value(SETTING_CHAT_AVATAR).value<QImage>();
}
void RshareSettings::setChatAvatar(const QImage& I)
{
  setValue(SETTING_CHAT_AVATAR,I) ;
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
bool
RshareSettings::showMainWindowAtStart()
{
  return value(SETTING_SHOW_MAINWINDOW_AT_START).toBool();
}

/** Sets whether to show RetroShare's main window when the application starts. */
void
RshareSettings::setShowMainWindowAtStart(bool show)
{
  setValue(SETTING_SHOW_MAINWINDOW_AT_START, show);
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

uint RshareSettings::getNotifyFlags()
{
  return value(SETTING_NOTIFY_FLAGS).toUInt();
}

void RshareSettings::setNotifyFlags(uint flags)
{
  setValue(SETTING_NOTIFY_FLAGS, flags);
}

bool RshareSettings::getDisplayTrayGroupChat()
{
    return value("DisplayTrayGroupChat").toBool();
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

void RshareSettings::setChatSendMessageWithCtrlReturn(bool bValue)
{
    setValueToGroup("Chat", "SendMessageWithCtrlReturn", bValue);
}

/** Returns true if RetroShare is set to run on system boot. */
bool
RshareSettings::runRetroshareOnBoot()
{
#if defined(Q_WS_WIN)
  if (!win32_registry_get_key_value(STARTUP_REG_KEY, RETROSHARE_REG_KEY).isEmpty()) {
    return true;
  } else {
    return false;
  }
#else
  /* Platforms other than windows aren't supported yet */
  return false;
#endif
}

/** If <b>run</b> is set to true, then RetroShare will run on system boot. */
void
RshareSettings::setRunRetroshareOnBoot(bool run)
{
#if defined(Q_WS_WIN)
  if (run) {
    win32_registry_set_key_value(STARTUP_REG_KEY, RETROSHARE_REG_KEY,
        QString("\"" +
                QDir::convertSeparators(QCoreApplication::applicationFilePath())) +
                "\"");
  } else {
    win32_registry_remove_key(STARTUP_REG_KEY, RETROSHARE_REG_KEY);
  }
#else
  /* Platforms othe rthan windows aren't supported yet */
  Q_UNUSED(run);
  return;
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

/* Messages */
bool RshareSettings::getMsgSetToReadOnActivate ()
{
    return valueFromGroup("MessageDialog", "SetMsgToReadOnActivate", true).toBool();
}

void RshareSettings::setMsgSetToReadOnActivate (bool bValue)
{
    setValueToGroup("MessageDialog", "SetMsgToReadOnActivate", bValue);
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
        m_maxTimeBeforeIdle = valueFromGroup("General", "maxTimeBeforeIdle", 30).toUInt();
    }

    return m_maxTimeBeforeIdle;
}

void RshareSettings::setMaxTimeBeforeIdle(uint nValue)
{
    m_maxTimeBeforeIdle = nValue;
    setValueToGroup("General", "maxTimeBeforeIdle", nValue);
}
