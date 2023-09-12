/*******************************************************************************
 * gui/settings/rsharesettings.h                                               *
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

#ifndef _RSHARESETTINGS_H
#define _RSHARESETTINGS_H

#include <stdint.h>
#include <QHash>
#include <QRgb>
#include <QSettings>

#include <stdint.h>
#include <gui/linetypes.h>
#include "rsettings.h"

#define RS_CHATLOBBY_BLINK              0x00000001

#define STATUSBAR_DISC  0x00000001

//Forward declaration.
class QWidget;
class QToolBar;
class QMainWindow;

class GroupFrameSettings
{
public:
	enum Type { Nothing, Forum, Channel, Posted };

public:
	GroupFrameSettings()
	{
		mOpenAllInNewTab = false;
		mHideTabBarWithOneTab = true;
	}

public:
	bool mOpenAllInNewTab;
	bool mHideTabBarWithOneTab;
};

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
        LASTDIR_SOUNDS,
        LASTDIR_PLUGIN
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
	void setLanguageCode(const QString& languageCode);

	/** Gets the interface style key (e.g., "windows", "motif", etc.) */
	QString getInterfaceStyle();
	/** Sets the interface style key. */
	void setInterfaceStyle(const QString& styleKey);

	/** Sets the stylesheet */
	void setSheetName(const QString& sheet);
	/** Gets the stylesheet */
	QString getSheetName();

	/** Gets the page button Location.*/
	bool getPageButtonLoc();
	/** Sets the page button Location.*/
	void setPageButtonLoc(bool onToolBar);

	/** Gets the action button Location.*/
	bool getActionButtonLoc();
	/** Sets the action button Location.*/
	void setActionButtonLoc(bool onToolBar);

	/** Gets the tool button's style.*/
	Qt::ToolButtonStyle getToolButtonStyle();
	/** Sets the tool button's style.*/
	void setToolButtonStyle(Qt::ToolButtonStyle style);

	/** Gets the tool button's size.*/
	int getToolButtonSize();
	/** Sets the tool button's size.*/
	void setToolButtonSize(int size);

	/** Gets the list item icon's size.*/
	int getListItemIconSize();
	/** Sets the list item icon's size.*/
	void setListItemIconSize(int size);

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
	/** Register retroshare:// as protocol */
	bool setRetroShareProtocol(bool value, QString &error);

	/** Returns true if this instance have to run Local Server*/
	bool getUseLocalServer();
	/** Sets whether to run Local Server */
	void setUseLocalServer(bool value);

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

	void setHiddenServiceKey() ;

	uint getNewsFeedFlags();
	void setNewsFeedFlags(uint flags);

	uint getChatFlags();
	void setChatFlags(uint flags);

	uint getChatLobbyFlags();
	void setChatLobbyFlags(uint flags);

	uint getNotifyFlags();
	void setNotifyFlags(uint flags);

	uint getMessageFlags();
	void setMessageFlags(uint flags);

	bool getDisplayTrayChatLobby();
	void setDisplayTrayChatLobby(bool bValue);
	bool getDisplayTrayGroupChat();
	void setDisplayTrayGroupChat(bool bValue);

	bool getChatSendMessageWithCtrlReturn();
	void setChatSendMessageWithCtrlReturn(bool bValue);

	bool getChatDoNotSendIsTyping();
	void setChatDoNotSendIsTyping(bool bValue);

	bool getChatSendAsPlainTextByDef();
	void setChatSendAsPlainTextByDef(bool bValue);

	bool getShrinkChatTextEdit();
	void setShrinkChatTextEdit(bool bValue);

	bool getChatSearchShowBarByDefault();
	void setChatSearchShowBarByDefault(bool bValue);

	void setChatSearchCharToStartSearch(int iValue);
	int getChatSearchCharToStartSearch();

	void setChatSearchCaseSensitively(bool bValue);
	bool getChatSearchCaseSensitively();

	void setChatSearchWholeWords(bool bValue);
	bool getChatSearchWholeWords();

	void setChatSearchMoveToCursor(bool bValue);
	bool getChatSearchMoveToCursor();

	void setChatSearchSearchWithoutLimit(bool bValue);
	bool getChatSearchSearchWithoutLimit();

	void setChatSearchMaxSearchLimitColor(uint uiValue);
	uint getChatSearchMaxSearchLimitColor();

	void setChatSearchFoundColor(QRgb rgbValue);
	QRgb getChatSearchFoundColor();

	bool getChatLoadEmbeddedImages();
	void setChatLoadEmbeddedImages(bool value);

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
	
	int  getDistantChatHistoryCount();
	void setDistantChatHistoryCount(int value);

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

	/* Message */
	bool getMsgSetToReadOnActivate();
	void setMsgSetToReadOnActivate(bool value);
	bool getMsgLoadEmbeddedImages();
	void setMsgLoadEmbeddedImages(bool value);

	enumMsgOpen getMsgOpen();
	void setMsgOpen(enumMsgOpen value);

	/* Forum */
	bool getForumMsgSetToReadOnActivate();
	void setForumMsgSetToReadOnActivate(bool value);
	bool getForumExpandNewMessages();
	void setForumExpandNewMessages(bool value);
	bool getForumLoadEmbeddedImages();
	void setForumLoadEmbeddedImages(bool value);
	bool getForumLoadEmoticons();
	void setForumLoadEmoticons(bool value);

	/* Channel */
	bool getChannelLoadThread();
	void setChannelLoadThread(bool value);

	/* GroupFrame settings */
	bool getGroupFrameSettings(GroupFrameSettings::Type type, GroupFrameSettings &groupFrameSettings);
	void setGroupFrameSettings(GroupFrameSettings::Type type, const GroupFrameSettings &groupFrameSettings);

	/* time before idle */
	uint getMaxTimeBeforeIdle();
	void setMaxTimeBeforeIdle(uint value);

    // webinterface
    bool getWebinterfaceEnabled();
    void setWebinterfaceEnabled(bool enabled);

	QString getWebinterfaceFilesDirectory();
	void setWebinterfaceFilesDirectory(const QString& dirname);

    // proxy function that computes the best icon size among sizes passed as array, to match the recommended size on screen.
    int computeBestIconSize(int n_sizes, int *sizes, int recommended_size);

    bool getPageAlreadyDisplayed(const QString& page_code) ;
    void setPageAlreadyDisplayed(const QString& page_code,bool b) ;

#ifdef RS_JSONAPI
	bool getJsonApiEnabled();
	void setJsonApiEnabled(bool enabled);

	uint16_t getJsonApiPort();
	void setJsonApiPort(uint16_t port);

	QString getJsonApiListenAddress();
	void setJsonApiListenAddress(const QString& listenAddress);

	QStringList getJsonApiAuthTokens();
	void setJsonApiAuthTokens(const QStringList& authTokens);
#endif // ifdef RS_JSONAPI

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
