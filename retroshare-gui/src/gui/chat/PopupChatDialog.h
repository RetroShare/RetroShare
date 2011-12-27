/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006, crypton
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


#ifndef _POPUPCHATDIALOG_H
#define _POPUPCHATDIALOG_H

#include "ui_PopupChatDialog.h"

class QAction;
class QTextEdit;
class QTextCharFormat;
class AttachFileItem;
class ChatInfo;

#include <retroshare/rsmsgs.h>
#include "ChatStyle.h"
#include "gui/style/RSStyle.h"

class PopupChatDialog : public QWidget
{
  Q_OBJECT

public:
    enum enumChatType { TYPE_NORMAL, TYPE_HISTORY, TYPE_OFFLINE };

public:
    static PopupChatDialog *getExistingInstance(const std::string &id);
    static PopupChatDialog *getPrivateChat(const std::string &id, uint chatflags);
    static void cleanupChat();
    static void chatFriend(const std::string &id);
    static void closeChat(const std::string &id);
    static void privateChatChanged(int list, int type);

    void updateStatusString(const QString& peer_id, const QString& statusString);
    std::string getPeerId() { return dialogId; }
    QString getTitle() { return dialogName; }
    bool hasNewMessages() { return newMessages; }
    bool isTyping() { return typing; }
    int getPeerStatus() { return peerStatus; }
    void focusDialog();
    void activate();
    bool setStyle();
    const RSStyle &getStyle();
    virtual void updateStatus(const QString &peer_id, int status);

public slots:
    void updateStatus_slot(const QString &peer_id, int status);

protected:
    /** Default constructor */
    PopupChatDialog(const std::string &id, const QString &name, QWidget *parent = 0, Qt::WFlags flags = 0);
    /** Default destructor */
    ~PopupChatDialog();

    virtual void resizeEvent(QResizeEvent *event);
    virtual void dragEnterEvent(QDragEnterEvent *event);
    virtual void dropEvent(QDropEvent *event);

    bool eventFilter(QObject *obj, QEvent *ev);

    void insertChatMsgs();
    void addChatMsg(bool incoming, const QString &name, const QDateTime &sendTime, const QDateTime &recvTime, const QString &message, enumChatType chatType);

private slots:
    void pasteLink() ;
    void contextMenu(QPoint) ;

    void fileHashingFinished(AttachFileItem* file);

    void smileyWidget();
    void addSmiley();

    void resetStatusBar() ;
    void updateStatusTyping() ;

    void on_actionMessageHistory_triggered();
    void addExtraFile();
    void addExtraPicture();
    void showAvatarFrame(bool show);
    void on_closeInfoFrameButton_clicked();

    void setColor();
    void getFont();
    void setFont();

    void sendChat();

    void updatePeersCustomStateString(const QString& peer_id, const QString& status_string) ;

    void on_actionClear_Chat_History_triggered();
    void on_actionDelete_Chat_History_triggered();

    bool fileSave();
    bool fileSaveAs();
    void clearOfflineMessages();

private:
    void setCurrentFileName(const QString &fileName);

    void colorChanged(const QColor &c);
    void fontChanged(const QFont &font);
    void addAttachment(std::string,int flag);
    void processSettings(bool bLoad);

    void onPrivateChatChanged(int list, int type);

    QAction *actionTextBold;
    QAction *actionTextUnderline;
    QAction *actionTextItalic;

    std::string dialogId;
    QString dialogName;
    unsigned int lastChatTime;
    std::string  lastChatName;

    time_t last_status_send_time ;
    QColor mCurrentColor;
    QFont  mCurrentFont;

    std::list<ChatInfo> savedOfflineChat;
    QString wholeChat;
    QString fileName;

    bool newMessages;
    bool typing;
    int peerStatus;
    ChatStyle chatStyle;
    bool manualDelete;

    RSStyle style;

protected:
	 virtual bool sendPrivateChat(const std::wstring& msg) ;	// can be derived to send chat to e.g. a chat lobby

    /** Qt Designer generated object */
    Ui::PopupChatDialog ui;
};

#endif
