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


#ifndef _RSHARESETTINGS_H
#define _RSHARESETTINGS_H

#include <QHash>
#include <QSettings>

#include <gui/linetypes.h>
#include "rsettings.h"

/* Defines for get/setTrayNotifyFlags */
#define TRAYNOTIFY_PRIVATECHAT          0x00000001
#define TRAYNOTIFY_MESSAGES             0x00000002
#define TRAYNOTIFY_CHANNELS             0x00000004
#define TRAYNOTIFY_FORUMS               0x00000008
#define TRAYNOTIFY_TRANSFERS            0x00000010

#define TRAYNOTIFY_PRIVATECHAT_COMBINED 0x00000020
#define TRAYNOTIFY_MESSAGES_COMBINED    0x00000040
#define TRAYNOTIFY_CHANNELS_COMBINED    0x00000080
#define TRAYNOTIFY_FORUMS_COMBINED      0x00000100
#define TRAYNOTIFY_TRANSFERS_COMBINED   0x00000200

#define TRAYNOTIFY_BLINK_PRIVATECHAT    0x00000001
#define TRAYNOTIFY_BLINK_MESSAGES       0x00000002
#define TRAYNOTIFY_BLINK_CHANNELS       0x00000004
#define TRAYNOTIFY_BLINK_FORUMS         0x00000008
#define TRAYNOTIFY_BLINK_TRANSFERS      0x00000010

#define RS_CHATLOBBY_BLINK              0x00000001

#define STATUSBAR_DISC  0x00000001

//Forward declaration.
class QWidget;
class QTableWidget;
class QToolBar;
class QMainWindow;

/** Handles saving and restoring RShares's settings
 *
 * NOTE: Qt 4.1 documentation states that constructing a QSettings object is
 * "very fast", so we shouldn't need to create a global instance of this
 * class.
 */
class RshareSettings : public RSettings
{
public:
  enum enumLastDir
  {
    LASTDIR_EXTRAFILE,
    LASTDIR_CERT,
    LASTDIR_HISTORY,
    LASTDIR_IMAGES,
    LASTDIR_MESSAGES,
    LASTDIR_BLOGS,
    LASTDIR_SOUNDS
  };

  enum enumToasterPosition
  {
    TOASTERPOS_TOPLEFT,
    TOASTERPOS_TOPRIGHT,
    TOASTERPOS_BOTTOMLEFT,
    TOASTERPOS_BOTTOMRIGHT
  };

  enum enumMsgOpen
  {
    MSG_OPEN_TAB,
    MSG_OPEN_WINDOW
  };

public:
  /* create settings object */
  static void Create(bool forceCreateNew = false);

  /** Gets the currently preferred language code for RShare. */
  QString getLanguageCode();
  /** Saves the preferred language code. */
  void setLanguageCode(QString languageCode);
 
  /** Gets the interface style key (e.g., "windows", "motif", etc.) */
  QString getInterfaceStyle();
  /** Sets the interface style key. */
  void setInterfaceStyle(QString styleKey);
  
  /** Sets the stylesheet */
  void setSheetName(QString sheet);
  /** Gets the stylesheet */
  QString getSheetName();
  
  /** Returns true if RetroShare's main window should be visible when the
  * application starts. */
  bool getStartMinimized();
  /** Sets whether to show main window when the application starts. */
  void setStartMinimized(bool startMinimized);

  bool getCloseToTray();
  void setCloseToTray(bool closeToTray);

  /** Returns true if RetroShare should start on system boot. */
  bool runRetroshareOnBoot(bool &minimized);

  /** Set whether to run RetroShare on system boot. */
  void setRunRetroshareOnBoot(bool run, bool minimized);

  /** Returns true if the user can set retroshare as protocol */
  bool canSetRetroShareProtocol();
  /** Returns true if retroshare:// is registered as protocol */
  bool getRetroShareProtocol();
  /** Register retroshare:// as protocl */
  bool setRetroShareProtocol(bool value);

  /* Get the destination log file. */
  QString getLogFile();
  /** Set the destination log file. */
  void setLogFile(QString file);

  QString getLastDir(enumLastDir type);
  void setLastDir(enumLastDir type, const QString &lastDir);

  /* Get the bandwidth graph line filter. */
  uint getBWGraphFilter();
  /** Set the bandwidth graph line filter. */
  void setBWGraphFilter(uint line, bool status);

  /** Set the bandwidth graph opacity setting. */
  int getBWGraphOpacity();
  /** Set the bandwidth graph opacity settings. */
  void setBWGraphOpacity(int value);

  /** Gets whether the bandwidth graph is always on top. */
  bool getBWGraphAlwaysOnTop();
  /** Sets whether the bandwidth graph is always on top. */
  void setBWGraphAlwaysOnTop(bool alwaysOnTop);
  
  uint getNewsFeedFlags();
  void setNewsFeedFlags(uint flags);

  uint getChatFlags();
  void setChatFlags(uint flags);

  uint getChatLobbyFlags();
  void setChatLobbyFlags(uint flags);

  uint getNotifyFlags();
  void setNotifyFlags(uint flags);

  uint getTrayNotifyFlags();
  void setTrayNotifyFlags(uint flags);

  uint getTrayNotifyBlinkFlags();
  void setTrayNotifyBlinkFlags(uint flags);

  uint getMessageFlags();
  void setMessageFlags(uint flags);

  bool getDisplayTrayChatLobby();
  void setDisplayTrayChatLobby(bool bValue);
  bool getDisplayTrayGroupChat();
  void setDisplayTrayGroupChat(bool bValue);

  bool getAddFeedsAtEnd();
  void setAddFeedsAtEnd(bool bValue);

  bool getChatSendMessageWithCtrlReturn();
  void setChatSendMessageWithCtrlReturn(bool bValue);

  enumToasterPosition getToasterPosition();
  void setToasterPosition(enumToasterPosition position);

  QPoint getToasterMargin();
  void   setToasterMargin(QPoint margin);

  /* chat font */
  QString getChatScreenFont();
  void    setChatScreenFont(const QString &font);

  /* chat styles */
  void getPublicChatStyle(QString &stylePath, QString &styleVariant);
  void setPublicChatStyle(const QString &stylePath, const QString &styleVariant);

  void getPrivateChatStyle(QString &stylePath, QString &styleVariant);
  void setPrivateChatStyle(const QString &stylePath, const QString &styleVariant);

  void getHistoryChatStyle(QString &stylePath, QString &styleVariant);
  void setHistoryChatStyle(const QString &stylePath, const QString &styleVariant);

  /* Chat */
  int  getPublicChatHistoryCount();
  void setPublicChatHistoryCount(int value);

  int  getPrivateChatHistoryCount();
  void setPrivateChatHistoryCount(int value);

  int  getLobbyChatHistoryCount();
  void setLobbyChatHistoryCount(int value);

  //! Save placement, state and size information of a window.
  void saveWidgetInformation(QWidget *widget);

  //! Load placement, state and size information of a window.
  void loadWidgetInformation(QWidget *widget);

  //! Method overload. Save window and toolbar information.
  void saveWidgetInformation(QMainWindow *widget, QToolBar *toolBar);

  //! Method overload. Restore window and toolbar information.
  void loadWidgetInformation(QMainWindow *widget, QToolBar *toolBar);

  /* MainWindow */
  int  getLastPageInMainWindow ();
  void setLastPageInMainWindow (int value);
  uint getStatusBarFlags();
  void setStatusBarFlags(uint flags);
  void setStatusBarFlag(uint flag, bool enable);

  /* Messages */
  bool getMsgSetToReadOnActivate();
  void setMsgSetToReadOnActivate(bool value);
  bool getMsgLoadEmbeddedImages();
  void setMsgLoadEmbeddedImages(bool value);

  enumMsgOpen getMsgOpen();
  void setMsgOpen(enumMsgOpen value);

  /* Forums */
  bool getForumMsgSetToReadOnActivate();
  void setForumMsgSetToReadOnActivate(bool value);
  bool getForumExpandNewMessages();
  void setForumExpandNewMessages(bool value);
  bool getForumOpenAllInNewTab();
  void setForumOpenAllInNewTab(bool value);
  bool getForumLoadEmbeddedImages();
  void setForumLoadEmbeddedImages(bool value);

  /* time before idle */
  uint getMaxTimeBeforeIdle();
  void setMaxTimeBeforeIdle(uint value);

protected:
  /** Default constructor. */
  RshareSettings();

  void initSettings();

  /* member for fast access */
  int m_maxTimeBeforeIdle;
};

// the one and only global settings object
extern RshareSettings *Settings;

#endif

