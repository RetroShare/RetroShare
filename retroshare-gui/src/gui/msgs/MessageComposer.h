/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2007, RetroShare Team
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

#ifndef _MESSAGECOMPOSER_H
#define _MESSAGECOMPOSER_H

#include <QMainWindow>
#include <retroshare/rstypes.h>
#include "ui_MessageComposer.h"

class QAction;
class QComboBox;
class QFontComboBox;
class QTextEdit;
class QTextCharFormat;
class AttachFileItem;
class RSTreeWidgetItemCompareRole;

class MessageComposer : public QMainWindow 
{
    Q_OBJECT

public:
    enum enumType { TO, CC, BCC };
    enum enumMessageType { NORMAL, REPLY, FORWARD };

public:
    /** Default Constructor */

    MessageComposer(QWidget *parent = 0, Qt::WFlags flags = 0);
    ~MessageComposer();

    static void msgFriend(std::string id, bool group);
    static void recommendFriend(std::list <std::string> &peerids);

    static MessageComposer *newMsg(const std::string &msgId = "");
    static MessageComposer *replyMsg(const std::string &msgId, bool all);
    static MessageComposer *forwardMsg(const std::string &msgId);

    /* worker fns */
    void  insertSendList();
    void  insertFileList(const std::list<DirDetails>&);
    void  insertFileList(const std::list<FileInfo>&);
    void  addFile(const FileInfo &fileInfo);
    void  insertTitleText(const QString &title, enumMessageType type = NORMAL);
    void  insertPastedText(QString msg) ;
    void  insertForwardPastedText(QString msg);
    void  insertHtmlText(const QString &msg);
    void  insertMsgText(const QString &msg);
    void  addRecipient(enumType type, const std::string &id, bool group);
    void  Create_New_Image_Tag(const QString urlremoteorlocal);

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
    void on_contactsdockWidget_visibilityChanged(bool visible);
    void toggleContacts();

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
    void textStyle(int styleIndex);
    void textColor();
    void textAlign(QAction *a);
    void smileyWidget();
    void addSmileys();

    void currentCharFormatChanged(const QTextCharFormat &format);
    void cursorPositionChanged();

    void clipboardDataChanged();

    void fileHashingFinished(AttachFileItem* file);

    void attachFile();
    void addAttachment(std::string);
    void checkAttachmentReady();

    void fontSizeIncrease();
    void fontSizeDecrease();
    void blockQuote();
    void toggleCode();
    void addPostSplitter();

    void filterRegExpChanged();
    void clearFilter();
    void titleChanged();

    // Add to To/Cc/Bcc address fields
    void btnClickEvent();
    void recommendButtonClicked();
    void editingRecipientFinished();

    void groupsChanged(int type);
    void peerStatusChanged(const QString& peer_id, int status);

private:
    void processSettings(bool bLoad);

    void setTextColor(const QColor& col) ;
    void setupFileActions();
    void setupEditActions();
    void setupViewActions();
    void setupInsertActions();
    void setupFormatActions();

    bool load(const QString &f);
    bool maybeSave();
    //bool image_extension( QString nametomake );
    void setCurrentFileName(const QString &fileName);

    void mergeFormatOnWordOrSelection(const QTextCharFormat &format);
    void fontChanged(const QFont &f);
    void colorChanged(const QColor &c);
    void alignmentChanged(Qt::Alignment a);

    bool sendMessage_internal(bool bDraftbox);
    
    void FilterItems();
    bool FilterItem(QTreeWidgetItem *pItem, QString &sPattern);

    void calculateTitle();
    void addEmptyRecipient();

    bool getRecipientFromRow(int row, enumType &type, std::string &id, bool &group);
    void setRecipientToRow(int row, enumType type, std::string id, bool group);

    QAction *actionSave,
    *actionAlignLeft,
    *actionAlignCenter,
    *actionAlignRight,
    *actionAlignJustify,
    *actionUndo,
    *actionRedo,
    *actionCut,
    *actionCopy,
    *actionPaste;

    QAction *contactSidebarAction;

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

    /* maps of files  */
    std::list<AttachFileItem *> mAttachments;

    bool mCheckAttachment;

    RSTreeWidgetItemCompareRole *m_compareRole;
    QCompleter *m_completer;

    /** Qt Designer generated object */
    Ui::MessageComposer ui;

    std::list<FileInfo> _recList ;
};

#endif
