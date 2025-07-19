/*******************************************************************************
 * retroshare-gui/src/gui/msgs/MessageComposer.h                               *
 *                                                                             *
 * Copyright (C) 2007 by Retroshare Team     <retroshare.project@gmail.com>    *
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

#ifndef _MESSAGECOMPOSER_H
#define _MESSAGECOMPOSER_H

#include <QMainWindow>
#include <retroshare/rstypes.h>
#include <retroshare/rsmsgs.h>
#include <retroshare/rspeers.h>
#include "ui_MessageComposer.h"

#include "gui/msgs/MessageInterface.h"
#include "util/FontSizeHandler.h"

class QAction;
struct RsIdentityDetails;
class QComboBox;
class QFontComboBox;
class QTextEdit;
class QTextCharFormat;
class RSTreeWidgetItemCompareRole;
struct RsGxsChannelGroup;
struct RsGxsForumGroup;

class MessageComposer : public QMainWindow 
{
    Q_OBJECT

public:
    enum enumType { TO, CC, BCC };
    enum enumMessageType { NORMAL, REPLY, FORWARD };
    enum destinationType { PEER_TYPE_SSL, PEER_TYPE_GROUP, PEER_TYPE_GXS };

public:
    /** Default Constructor */

    MessageComposer(QWidget *parent = 0, Qt::WindowFlags flags = Qt::WindowFlags());
    ~MessageComposer();

    static void msgFriend(const RsPeerId &id);
    // send msg to all locations
    static void msgFriend(const RsPgpId &id);
    static void msgGxsIdentity(const RsGxsId& gxs_id) ;
    static void msgGroup(const std::string& group_id) ;

    static QString recommendMessage();
    static void recommendFriend(const std::set <RsPeerId> &sslIds, const RsPeerId &to = RsPeerId(), const QString &msg = "", bool autoSend = false);
    static void addConnectAttemptMsg(const RsPgpId &gpgId, const RsPeerId &sslId, const QString &sslName);
    static void sendInvite(const RsGxsId &to, bool autoSend);
#ifdef UNUSED_CODE
    static void sendChannelPublishKey(RsGxsChannelGroup &group);
    static void sendForumPublishKey(RsGxsForumGroup &group);
#endif

    static MessageComposer *newMsg(const std::string &msgId = "");
    static MessageComposer *replyMsg(const std::string &msgId, bool all);
    static MessageComposer *forwardMsg(const std::string &msgId);

    /* worker fns */
    void  setFileList(const std::list<DirDetails>&);
    void  setFileList(const std::list<FileInfo>&);
    void  addFile(const FileInfo &fileInfo);
    void  setTitleText(const QString &title, enumMessageType type = NORMAL);
    void  setQuotedMsg(const QString &msg, const QString &header);
    void  setMsgText(const QString &msg, bool asHtml = false);
    void  addRecipient(enumType type, const RsPeerId &id);
    void  addRecipient(enumType type, const RsGxsId &gxs_id) ;
    void  addRecipient(enumType type, const std::string& group_id) ;

public slots:
    /* actions to take.... */
    void  sendMessage();
    void  cancelMessage();
    void  addImage();
    
    void changeFormatType(int styleIndex );

protected:
    void closeEvent (QCloseEvent * event);
    bool eventFilter(QObject *obj, QEvent *ev);

private slots:
    /* toggle Contacts DockWidget */
    void contextMenuFileList(QPoint);
    void pasteRecommended();
    void on_contactsdockWidget_visibilityChanged(bool visible);
    void toggleContacts();
    void buildCompleter();
    void updatecontactsviewicons();
    void updateCells(int,int) ;

	void filterComboBoxChanged(int);

    void fileNew();
    void fileOpen();
    bool fileSave();
    bool fileSaveAs();
    void filePrint();
    void saveasDraft();

    //void filePrintPreview();
    void filePrintPdf();

    void textBold();
    void textUnderline();
    void textItalic();
    void textFamily(const QString &f);
    void textSize(const QString &p);
    void textStyle(QAction *a);
    void textColor();
    void textbackgroundColor();
    void textAlign(QAction *a);
    void smileyWidget();
    void addSmileys();

    void currentCharFormatChanged(const QTextCharFormat &format);
    void cursorPositionChanged();

    void clipboardDataChanged();

    void fileHashingStarted();
    void fileHashingFinished(QList<HashedFile> hashedFiles);

    void attachFile();

    void fontSizeIncrease();
    void fontSizeDecrease();
    void blockQuote();
    void toggleCode();
    void addPostSplitter();

    void titleChanged();

    // Add to To/Cc/Bcc address fields
    void addTo();
    void addCc();
    void addBcc();
    void addRecommend();
    void editingRecipientFinished();
    void contactDetails();

    void peerStatusChanged(const QString& peer_id, int status);
    void friendSelectionChanged();

    void tagAboutToShow();
    void tagSet(int tagId, bool set);
    void tagRemoveAll();
    
    void on_closeInfoFrameButton_Distant_clicked();
    void on_closeInfoFrameButton_SizeLimit_clicked();

    static QString inviteMessage();

	void checkLength();

private:
    static QString buildReplyHeader(const MessageInfo &msgInfo);
    bool buildMessage(MessageInfo& mi);

    void processSettings(bool bLoad);

    static QString getRecipientEmailAddress(const RsGxsId& id,const RsIdentityDetails& detail) ;
    static QString getRecipientEmailAddress(const RsPeerId& id,const RsPeerDetails& detail) ;

    void addContact(enumType type);
    void setTextColor(const QColor& col) ;
    void setupFileActions();
    void setupEditActions();
    void setupViewActions();
    void setupInsertActions();
    void setupFormatActions();
    void setupContactActions();

    bool load(const QString &f);
    bool maybeSave();
    //bool image_extension( QString nametomake );
    void setCurrentFileName(const QString &fileName);

    void mergeFormatOnWordOrSelection(const QTextCharFormat &format);
    void fontChanged(const QFont &f);
    void colorChanged(const QColor &c);
    void colorChanged2(const QColor &c);
    void alignmentChanged(Qt::Alignment a);

    bool sendMessage_internal(bool bDraftbox);

    void calculateTitle();
    void addEmptyRecipient();

    bool getRecipientFromRow(int row, enumType &type, destinationType& dest_type, std::string &id);
    void setRecipientToRow(int row, enumType type, destinationType dest_type,const std::string &id);

    void clearTagLabels();
    void showTagLabels();

    QAction *actionSave,
    *actionAlignLeft,
    *actionAlignCenter,
    *actionAlignRight,
    *actionAlignJustify,
    *actionUndo,
    *actionRedo,
    *actionCut,
    *actionCopy,
    *actionPaste,   
    *actionDisc,
    *actionCircle,
    *actionSquare,
    *actionDecimal,
    *actionLowerAlpha,
    *actionUpperAlpha,
    *actionLowerRoman,
    *actionUpperRoman;

    QAction *contactSidebarAction;
    QAction *mActionAddTo;
    QAction *mActionAddCC;
    QAction *mActionAddBCC;
    QAction *mActionAddRecommend;
    QAction *mActionContactDetails;

    QTreeView *channelstreeView;

    QString fileName;
    QString nametomake;

    QColor codeBackground;
    QTextCharFormat defaultCharFormat;

    QHash<QString, QString> autoLinkDictionary;
    QHash<QString, QString> autoLinkTitleDictionary;
    QHash<QString, int> autoLinkTargetDictionary;

    std::string m_msgParentId; // parent message id
    std::string m_sDraftMsgId; // existing message id
    enumMessageType m_msgType;
    std::set<uint32_t> m_tagIds;
    QList<QLabel*> tagLabels;

    // needed to send system flags with reply
    unsigned int msgFlags;

    RSTreeWidgetItemCompareRole *m_compareRole;
    QCompleter *m_completer;

	QLabel *infoLabel;
	QLabel *lengthLabel;
	QLabel *lineLabel;

	bool has_gxs;
    bool mAlreadySent; // prevents a Qt bug that calls the same action twice.

    MessageFontSizeHandler mMessageFontSizeHandler;

    /** Qt Designer generated object */
    Ui::MessageComposer ui;

    std::list<FileInfo> _recList ;
};

#endif
