/****************************************************************
 *
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2011, RetroShare Team
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

#ifndef CHATWIDGET_H
#define CHATWIDGET_H

#include <QWidget>
#include <QCompleter>
#include <QTextCursor>
#include <QTextCharFormat>
#include <QToolButton>
#include "gui/common/HashBox.h"
#include "gui/common/RsButtonOnText.h"
#include "ChatStyle.h"
#include "gui/style/RSStyle.h"
#include "ChatLobbyUserNotify.h"

#include <retroshare/rsmsgs.h>
#include <retroshare/rsfiles.h>

class QAction;
class QTextEdit;
class QPushButton;
class ChatWidget;
class QMenu;

namespace Ui {
class ChatWidget;
}

// a Container for the logic behind buttons in a PopupChatDialog
// Plugins can implement this interface to provide their own buttons
class ChatWidgetHolder
{
public:
	ChatWidgetHolder(ChatWidget *chatWidget) : mChatWidget(chatWidget) {}
	virtual ~ChatWidgetHolder() {}

	// status comes from notifyPeerStatusChanged
	// see rststaus.h for possible values
	virtual void updateStatus(int /*status*/) {}

protected:
	ChatWidget *mChatWidget;
};

class ChatWidget : public QWidget
{
	Q_OBJECT

public:
	enum MsgType { MSGTYPE_NORMAL, MSGTYPE_HISTORY, MSGTYPE_OFFLINE, MSGTYPE_SYSTEM };
	enum ChatType { CHATTYPE_UNKNOWN, CHATTYPE_PRIVATE, CHATTYPE_LOBBY, CHATTYPE_DISTANT };

	explicit ChatWidget(QWidget *parent = 0);
	~ChatWidget();

    void init(const ChatId &chat_id, const QString &title);
    ChatId getChatId();
    ChatType chatType();

    // allow/disallow sendng of messages
    void blockSending(QString msg);
    void unblockSending();

	bool hasNewMessages() { return newMessages; }
	bool isTyping() { return typing; }

	void focusDialog();
	QToolButton* getNotifyButton();
	void setNotify(ChatLobbyUserNotify* clun);
	void scrollToAnchor(QString anchor);
	void addToParent(QWidget *newParent);
	void removeFromParent(QWidget *oldParent);

	void setWelcomeMessage(QString &text);
	void addChatMsg(bool incoming, const QString &name, const QDateTime &sendTime, const QDateTime &recvTime, const QString &message, MsgType chatType);
	void addChatMsg(bool incoming, const QString &name, const RsGxsId gxsId, const QDateTime &sendTime, const QDateTime &recvTime, const QString &message, MsgType chatType);
    void updateStatusString(const QString &statusMask, const QString &statusString, bool permanent = false);

	void addToolsAction(QAction *action);

	QString getTitle() { return title; }
	int getPeerStatus() { return peerStatus; }
	void setName(const QString &name);

	bool setStyle();
	const RSStyle *getStyle() { return &style; }

	// Adds one widget in the chat bar. Used to add e.g. new buttons. The widget should be
	// small enough in size.
	void addChatBarWidget(QWidget *w) ;
	void addTitleBarWidget(QWidget *w);
	void hideChatText(bool hidden);
	RSButtonOnText* getNewButtonOnTextBrowser();
	RSButtonOnText* getNewButtonOnTextBrowser(QString text);

	// Adds a new horizonal widget in the layout of the chat window.
	void addChatHorizontalWidget(QWidget *w) ;

	bool isActive();
	void setDefaultExtraFileFlags(TransferRequestFlags f) ;
	void pasteText(const QString&);

	const QList<ChatWidgetHolder*> &chatWidgetHolderList() { return mChatWidgetHolder; }

public slots:
	void updateStatus(const QString &peer_id, int status);

private slots:
	//void pasteCreateMsgLink() ;
	void clearChatHistory();
	void deleteChatHistory();
	void messageHistory();
	void resetStatusBar() ;

signals:
	void infoChanged(ChatWidget*);
	void newMessage(ChatWidget*);
	void statusChanged(int);

protected:
	bool eventFilter(QObject *obj, QEvent *event);
	virtual void showEvent(QShowEvent *event);
	virtual void resizeEvent(QResizeEvent *event);
	void updateTitle();

private slots:
	void contextMenuTextBrowser(QPoint);
	void contextMenuSearchButton(QPoint);
	void chatCharFormatChanged();

	void fileHashingFinished(QList<HashedFile> hashedFiles);

	void smileyWidget();
	void addSmiley();

	void addExtraFile();
	void addExtraPicture();
	void on_closeInfoFrameButton_clicked();
	void on_searchButton_clicked(bool bValue);
	void on_searchBefore_clicked();
	void on_searchAfter_clicked();
	void toogle_FindCaseSensitively();
	void toogle_FindWholeWords();
	void toogle_MoveToCursor();
	void toogle_SeachWithoutLimit();

	void on_notifyButton_clicked();

	void on_markButton_clicked(bool bValue);

	void chooseColor();
	void chooseFont();
	void resetFont();
	void resetFonts();
	void setFont();

	void updateLenOfChatTextEdit();
	void sendChat();

	void updatePeersCustomStateString(const QString& peer_id, const QString& status_string) ;

	bool fileSave();
	bool fileSaveAs();

	void quote();
	void dropPlacemark();
	void saveImage();

private:
	bool findText(const QString& qsStringToFind);
	bool findText(const QString& qsStringToFind, bool bBackWard, bool bForceMove);
	void removeFoundText();
	void updateStatusTyping();
	void setCurrentFileName(const QString &fileName);

	void colorChanged();
	void setColorAndFont(bool both);
	void processSettings(bool load);

	uint32_t maxMessageSize();

	void completeNickname(bool reverse);
    QAbstractItemModel *modelFromPeers();

    ChatId chatId;
	QString title;
	QString name;
	QString completionWord;
	int completionPosition;

	QColor currentColor;
	QFont  currentFont;

	QString fileName;

	bool newMessages;
	bool typing;
	int peerStatus;

    bool sendingBlocked;

	time_t lastStatusSendTime;

	ChatStyle chatStyle;
	RSStyle style;

	bool firstShow;
	bool inChatCharFormatChanged;
	bool firstSearch;
	QPalette qpSave_leSearch;
	std::map<QTextCursor,QTextCharFormat> smFoundCursor;
	int iCharToStartSearch;
	bool bFindCaseSensitively;
	bool bFindWholeWords;
	bool bMoveToCursor;
	bool bSearchWithoutLimit;
	uint uiMaxSearchLimitColor;
	QColor cFoundColor;
	QString qsLastsearchText;
	QTextCursor qtcCurrent;

	QTextCursor qtcMark;

	int lastUpdateCursorPos;
	int lastUpdateCursorEnd;

	TransferRequestFlags mDefaultExtraFileFlags ; // flags for extra files shared in this chat. Will be 0 by default, but might be ANONYMOUS for chat lobbies.
	QDate lastMsgDate ;

    QCompleter *completer;

	QList<ChatWidgetHolder*> mChatWidgetHolder;
	ChatLobbyUserNotify* notify;

	Ui::ChatWidget *ui;
};

#endif // CHATWIDGET_H
