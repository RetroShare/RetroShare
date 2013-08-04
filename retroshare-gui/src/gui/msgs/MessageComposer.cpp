/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006,2007 RetroShare Team
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

#include <QMessageBox>
#include <QCloseEvent>
#include <QClipboard>
#include <QTextCodec>
#include <QPrintDialog>
#include <QPrinter>
#include <QTextList>
#include <QColorDialog>
#include <QTextDocumentFragment>
#include <QTimer>
#include <QCompleter>
#include <QItemDelegate>
#include <QDateTime>
#include <QFileInfo>
#include <QTextStream>

#include <algorithm>

#include "MessageComposer.h"

#include <retroshare/rsiface.h>
#include <retroshare/rspeers.h>
#include <retroshare/rsmsgs.h>
#include <retroshare/rsstatus.h>
#include <retroshare/rsfiles.h>

#include "gui/notifyqt.h"
#include "gui/common/RSTreeWidgetItem.h"
#include "gui/common/GroupDefs.h"
#include "gui/common/StatusDefs.h"
#include "gui/common/PeerDefs.h"
#include "gui/RetroShareLink.h"
#include "gui/settings/rsharesettings.h"
#include "gui/common/Emoticons.h"
#include "textformat.h"
#include "util/misc.h"
#include "util/DateTime.h"
#include "TagsMenu.h"
#include "gui/common/TagDefs.h"
#include "gui/connect/ConfCertDialog.h"
#include "util/HandleRichText.h"

#define IMAGE_GROUP16    ":/images/user/group16.png"
#define IMAGE_FRIENDINFO ":/images/peerdetails_16x16.png"

#define COLUMN_CONTACT_NAME   0
#define COLUMN_CONTACT_DATA   0

#define ROLE_CONTACT_ID       Qt::UserRole
#define ROLE_CONTACT_SORT     Qt::UserRole + 1

#define COLOR_CONNECT Qt::blue

#define TYPE_GROUP  0
#define TYPE_SSL    1

#define COLUMN_RECIPIENT_TYPE  0
#define COLUMN_RECIPIENT_ICON  1
#define COLUMN_RECIPIENT_NAME  2
#define COLUMN_RECIPIENT_COUNT 3
#define COLUMN_RECIPIENT_DATA COLUMN_RECIPIENT_ICON // the column with a QTableWidgetItem

#define ROLE_RECIPIENT_ID     Qt::UserRole
#define ROLE_RECIPIENT_GROUP  Qt::UserRole + 1

#define COLUMN_FILE_CHECKED 0
#define COLUMN_FILE_NAME    0
#define COLUMN_FILE_SIZE    1
#define COLUMN_FILE_HASH    2
#define COLUMN_FILE_COUNT   3

#define STYLE_NORMAL "QLineEdit#%1 { border : none; }"
#define STYLE_FAIL   "QLineEdit#%1 { border : none; color : red; }"

class MessageItemDelegate : public QItemDelegate
{
public:
    MessageItemDelegate(QObject *parent = 0) : QItemDelegate(parent)
    {
    }

    void paint (QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
    {
        QStyleOptionViewItem ownOption (option);

        ownOption.state |= QStyle::State_Enabled; // the item is disabled to get no focus, but draw the icon as enabled (no grayscale)

        QItemDelegate::paint (painter, ownOption, index);
    }
};

/** Constructor */
MessageComposer::MessageComposer(QWidget *parent, Qt::WFlags flags)
: QMainWindow(parent, flags)
{
    /* Invoke the Qt Designer generated object setup routine */
    ui.setupUi(this);

    m_msgType = NORMAL;
    // needed to send system flags with reply
    msgFlags = 0;

    setupFileActions();
    setupEditActions();
    setupViewActions();
    setupInsertActions();

    m_compareRole = new RSTreeWidgetItemCompareRole;
    m_compareRole->setRole(COLUMN_CONTACT_NAME, ROLE_CONTACT_SORT);

    m_completer = NULL;

    Settings->loadWidgetInformation(this);

    setAttribute ( Qt::WA_DeleteOnClose, true );

    ui.hashBox->hide();

    // connect up the buttons.
    connect( ui.actionSend, SIGNAL( triggered (bool)), this, SLOT( sendMessage( ) ) );
    //connect( ui.actionReply, SIGNAL( triggered (bool)), this, SLOT( replyMessage( ) ) );
    connect(ui.boldbtn, SIGNAL(clicked()), this, SLOT(textBold()));
    connect(ui.underlinebtn, SIGNAL(clicked()), this, SLOT(textUnderline()));
    connect(ui.italicbtn, SIGNAL(clicked()), this, SLOT(textItalic()));
    connect(ui.colorbtn, SIGNAL(clicked()), this, SLOT(textColor()));
    connect(ui.imagebtn, SIGNAL(clicked()), this, SLOT(addImage()));
    connect(ui.emoticonButton, SIGNAL(clicked()), this, SLOT(smileyWidget()));
    //connect(ui.linkbtn, SIGNAL(clicked()), this, SLOT(insertLink()));
    connect(ui.actionContactsView, SIGNAL(triggered()), this, SLOT(toggleContacts()));
    connect(ui.actionSaveas, SIGNAL(triggered()), this, SLOT(saveasDraft()));
    connect(ui.actionAttach, SIGNAL(triggered()), this, SLOT(attachFile()));
    connect(ui.titleEdit, SIGNAL(textChanged(const QString &)), this, SLOT(titleChanged()));

    connect(ui.sizeincreaseButton, SIGNAL (clicked()), this, SLOT (fontSizeIncrease()));
    connect(ui.sizedecreaseButton, SIGNAL (clicked()), this, SLOT (fontSizeDecrease()));
    connect(ui.actionQuote, SIGNAL(triggered()), this, SLOT(blockQuote()));
    connect(ui.codeButton, SIGNAL (clicked()), this, SLOT (toggleCode()));

    connect(ui.msgText, SIGNAL( checkSpellingChanged( bool ) ), this, SLOT( spellChecking( bool ) ) );

    connect(ui.msgText, SIGNAL(currentCharFormatChanged(const QTextCharFormat &)), this, SLOT(currentCharFormatChanged(const QTextCharFormat &)));
    connect(ui.msgText, SIGNAL(cursorPositionChanged()), this, SLOT(cursorPositionChanged()));
    connect(ui.msgText,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(contextMenu(QPoint)));

    connect(ui.msgText->document(), SIGNAL(modificationChanged(bool)), actionSave, SLOT(setEnabled(bool)));
    connect(ui.msgText->document(), SIGNAL(modificationChanged(bool)), this, SLOT(setWindowModified(bool)));
    connect(ui.msgText->document(), SIGNAL(undoAvailable(bool)), actionUndo, SLOT(setEnabled(bool)));
    connect(ui.msgText->document(), SIGNAL(redoAvailable(bool)), actionRedo, SLOT(setEnabled(bool)));

    connect(ui.msgFileList, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextMenuFileList(QPoint)));

    connect(ui.hashBox, SIGNAL(fileHashingStarted()), this, SLOT(fileHashingStarted()));
    connect(ui.hashBox, SIGNAL(fileHashingFinished(QList<HashedFile>)), this, SLOT(fileHashingFinished(QList<HashedFile>)));

    setWindowModified(ui.msgText->document()->isModified());
    actionSave->setEnabled(ui.msgText->document()->isModified());
    actionUndo->setEnabled(ui.msgText->document()->isUndoAvailable());
    actionRedo->setEnabled(ui.msgText->document()->isRedoAvailable());

    connect(actionUndo, SIGNAL(triggered()), ui.msgText, SLOT(undo()));
    connect(actionRedo, SIGNAL(triggered()), ui.msgText, SLOT(redo()));

    actionCut->setEnabled(false);
    actionCopy->setEnabled(false);

    connect(actionCut, SIGNAL(triggered()), ui.msgText, SLOT(cut()));
    connect(actionCopy, SIGNAL(triggered()), ui.msgText, SLOT(copy()));
    connect(actionPaste, SIGNAL(triggered()), ui.msgText, SLOT(paste()));

    connect(ui.msgText, SIGNAL(copyAvailable(bool)), actionCut, SLOT(setEnabled(bool)));
    connect(ui.msgText, SIGNAL(copyAvailable(bool)), actionCopy, SLOT(setEnabled(bool)));

    connect(QApplication::clipboard(), SIGNAL(dataChanged()), this, SLOT(clipboardDataChanged()));

    connect(ui.addToButton, SIGNAL(clicked(void)), this, SLOT(addTo()));
    connect(ui.addCcButton, SIGNAL(clicked(void)), this, SLOT(addCc()));
    connect(ui.addBccButton, SIGNAL(clicked(void)), this, SLOT(addBcc()));
    connect(ui.addRecommendButton, SIGNAL(clicked(void)), this, SLOT(addRecommend()));

    connect(NotifyQt::getInstance(), SIGNAL(peerStatusChanged(const QString&,int)), this, SLOT(peerStatusChanged(const QString&,int)));
    connect(ui.friendSelectionWidget, SIGNAL(contentChanged()), this, SLOT(buildCompleter()));
    connect(ui.friendSelectionWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(contextMenuMsgSendList(QPoint)));
    connect(ui.friendSelectionWidget, SIGNAL(doubleClicked(int,QString)), this, SLOT(addTo()));

    /* hide the Tree +/- */
    ui.msgFileList -> setRootIsDecorated( false );

    /* initialize friends list */
    ui.friendSelectionWidget->setHeaderText(tr("Send To:"));
    ui.friendSelectionWidget->setModus(FriendSelectionWidget::MODUS_MULTI);
    ui.friendSelectionWidget->setShowType(FriendSelectionWidget::SHOW_GROUP | FriendSelectionWidget::SHOW_SSL | FriendSelectionWidget::SHOW_NON_FRIEND_GPG );
    //ui.friendSelectionWidget->setShowType(FriendSelectionWidget::SHOW_GROUP | FriendSelectionWidget::SHOW_SSL );
    ui.friendSelectionWidget->start();

    QActionGroup *grp = new QActionGroup(this);
    connect(grp, SIGNAL(triggered(QAction *)), this, SLOT(textAlign(QAction *)));

    actionAlignLeft = new QAction(QIcon(":/images/textedit/textleft.png"), tr("&Left"), grp);
    actionAlignLeft->setShortcut(Qt::CTRL + Qt::Key_L);
    actionAlignLeft->setCheckable(true);
    actionAlignCenter = new QAction(QIcon(":/images/textedit/textcenter.png"), tr("C&enter"), grp);
    actionAlignCenter->setShortcut(Qt::CTRL + Qt::Key_E);
    actionAlignCenter->setCheckable(true);
    actionAlignRight = new QAction(QIcon(":/images/textedit/textright.png"), tr("&Right"), grp);
    actionAlignRight->setShortcut(Qt::CTRL + Qt::Key_R);
    actionAlignRight->setCheckable(true);
    actionAlignJustify = new QAction(QIcon(":/images/textedit/textjustify.png"), tr("&Justify"), grp);
    actionAlignJustify->setShortcut(Qt::CTRL + Qt::Key_J);
    actionAlignJustify->setCheckable(true);

    setupFormatActions();

    /*ui.comboStyle->addItem("Standard");
    ui.comboStyle->addItem("Bullet List (Disc)");
    ui.comboStyle->addItem("Bullet List (Circle)");
    ui.comboStyle->addItem("Bullet List (Square)");
    ui.comboStyle->addItem("Ordered List (Decimal)");
    ui.comboStyle->addItem("Ordered List (Alpha lower)");
    ui.comboStyle->addItem("Ordered List (Alpha upper)");*/
    //connect(ui.comboStyle, SIGNAL(activated(int)),this, SLOT(textStyle(int)));
    connect(ui.comboStyle, SIGNAL(activated(int)),this, SLOT(changeFormatType(int)));

    connect(ui.comboFont, SIGNAL(activated(const QString &)), this, SLOT(textFamily(const QString &)));

    ui.comboSize->setEditable(true);

    QFontDatabase db;
    foreach(int size, db.standardSizes())
        ui.comboSize->addItem(QString::number(size));

    connect(ui.comboSize, SIGNAL(activated(const QString &)),this, SLOT(textSize(const QString &)));
    ui.comboSize->setCurrentIndex(ui.comboSize->findText(QString::number(QApplication::font().pointSize())));

    ui.textalignmentbtn->setIcon(QIcon(QString(":/images/textedit/textcenter.png")));

    QMenu * alignmentmenu = new QMenu();
    alignmentmenu->addAction(actionAlignLeft);
    alignmentmenu->addAction(actionAlignCenter);
    alignmentmenu->addAction(actionAlignRight);
    alignmentmenu->addAction(actionAlignJustify);
    ui.textalignmentbtn->setMenu(alignmentmenu);

    QPixmap pxm(24,24);
    pxm.fill(Qt::black);
    ui.colorbtn->setIcon(pxm);

    /* Set header resize modes and initial section sizes */
    QHeaderView * _smheader = ui.msgFileList->header () ;

    _smheader->resizeSection(COLUMN_FILE_NAME, 200);
    _smheader->resizeSection(COLUMN_FILE_SIZE, 60);
    _smheader->resizeSection(COLUMN_FILE_HASH, 220);

    QPalette palette = QApplication::palette();
    codeBackground = palette.color( QPalette::Active, QPalette::Midlight );

    ui.recipientWidget->setColumnCount(COLUMN_RECIPIENT_COUNT);

    QHeaderView *header = ui.recipientWidget->horizontalHeader();
    header->resizeSection(COLUMN_RECIPIENT_TYPE, 50);
    header->resizeSection(COLUMN_RECIPIENT_ICON, 22);
    header->setResizeMode(COLUMN_RECIPIENT_TYPE, QHeaderView::Fixed);
    header->setResizeMode(COLUMN_RECIPIENT_ICON, QHeaderView::Fixed);
    header->setResizeMode(COLUMN_RECIPIENT_NAME, QHeaderView::Interactive);
    header->setStretchLastSection(true);

    /* Set own item delegate */
    QItemDelegate *delegate = new MessageItemDelegate(this);
    ui.recipientWidget->setItemDelegateForColumn(COLUMN_RECIPIENT_ICON, delegate);

    addEmptyRecipient();

    // load settings
    processSettings(true);

    buildCompleter();

    /* set focus to subject */
    ui.titleEdit->setFocus();

    // create tag menu
    TagsMenu *menu = new TagsMenu (tr("Tags"), this);
    connect(menu, SIGNAL(aboutToShow()), this, SLOT(tagAboutToShow()));
    connect(menu, SIGNAL(tagSet(int, bool)), this, SLOT(tagSet(int, bool)));
    connect(menu, SIGNAL(tagRemoveAll()), this, SLOT(tagRemoveAll()));

    ui.tagButton->setMenu(menu);

    setAcceptDrops(true);
    ui.hashBox->setDropWidget(this);
    ui.hashBox->setAutoHide(true);

#if QT_VERSION < 0x040700
    // embedded images are not supported before QT 4.7.0
    ui.imagebtn->setVisible(false);
#endif

    /* Hide platform specific features */
#ifdef Q_WS_WIN

#endif
}

MessageComposer::~MessageComposer()
{
    delete(m_compareRole);
}

void MessageComposer::processSettings(bool bLoad)
{
    Settings->beginGroup(QString("MessageComposer"));

    if (bLoad) {
        // load settings

        // state of contact sidebar
        ui.contactsdockWidget->setVisible(Settings->value("ContactSidebar", true).toBool());

        // state of splitter
        ui.splitter->restoreState(Settings->value("Splitter").toByteArray());
        ui.splitter_2->restoreState(Settings->value("Splitter2").toByteArray());
    } else {
        // save settings

        // state of contact sidebar
        Settings->setValue("ContactSidebar", ui.contactsdockWidget->isVisible());

        // state of splitter
        Settings->setValue("Splitter", ui.splitter->saveState());
        Settings->setValue("Splitter2", ui.splitter_2->saveState());
    }

    Settings->endGroup();
}
/*static*/ void MessageComposer::msgDistantPeer(const std::string& hash,const std::string& pgp_id)
{
//    std::cerr << "MessageComposer::msgfriend()" << std::endl;

    /* create a message */

    MessageComposer *pMsgDialog = MessageComposer::newMsg();
    if (pMsgDialog == NULL) 
        return;

	 pMsgDialog->addRecipient(TO, hash,pgp_id) ;
    pMsgDialog->show();

    /* window will destroy itself! */
}


/*static*/ void MessageComposer::msgFriend(const std::string &id, bool group)
{
//    std::cerr << "MessageComposer::msgfriend()" << std::endl;

    /* create a message */

    MessageComposer *pMsgDialog = MessageComposer::newMsg();
    if (pMsgDialog == NULL) {
        return;
    }

    if (group) {
        pMsgDialog->addRecipient(TO, id, true);
    } else {
        RsPeerDetails detail;
        if (rsPeers->getPeerDetails(id, detail) && detail.accept_connection) {
            if (detail.isOnlyGPGdetail) {
                //put all sslChilds in message list
                std::list<std::string> sslIds;
                rsPeers->getAssociatedSSLIds(id, sslIds);

                std::list<std::string>::const_iterator it;
                for (it = sslIds.begin(); it != sslIds.end(); it++) {
                    pMsgDialog->addRecipient(TO, *it, false);
                }
            } else {
                pMsgDialog->addRecipient(TO, detail.id, false);
            }
        }
    }

    pMsgDialog->show();

    /* window will destroy itself! */
}

static QString buildRecommendHtml(const std::list<std::string> &sslIds, const std::string &excludeId = "")
{
    QString text;

    /* process ssl ids */
    std::list <std::string>::const_iterator sslIt;
    for (sslIt = sslIds.begin(); sslIt != sslIds.end(); sslIt++) {
        if (*sslIt == excludeId) {
            continue;
        }
        RetroShareLink link;
        if (link.createCertificate(*sslIt)) {
            text += link.toHtml() + "<br>";
        }
    }

    return text;
}

QString MessageComposer::recommendMessage()
{
    return tr("Hello,<br>I recommend a good friend of mine; you can trust them too when you trust me. <br>");
}

void MessageComposer::recommendFriend(const std::list <std::string> &sslIds, const std::string &to, const QString &msg, bool autoSend)
{
//    std::cerr << "MessageComposer::recommendFriend()" << std::endl;

    if (sslIds.size() == 0) {
        return;
    }

    QString recommendHtml = buildRecommendHtml(sslIds, to);
    if (recommendHtml.isEmpty()) {
        return;
    }

    /* create a message */
    MessageComposer *composer = MessageComposer::newMsg();

    composer->setTitleText(tr("You have a friend recommendation"));
    composer->msgFlags |= RS_MSG_FRIEND_RECOMMENDATION;

    if (!to.empty()) {
        composer->addRecipient(TO, to, false);
    }
    std::string ownId = rsPeers->getOwnId();
    RetroShareLink link;
    link.createPerson(ownId);
    
    QString sMsgText = msg.isEmpty() ? recommendMessage() : msg;
    sMsgText += "<br><br>";
    sMsgText += recommendHtml;
    sMsgText += "<br>";
    sMsgText += tr("This friend is suggested by") + " " + link.toHtml() + "<br><br>" ;      
    sMsgText += tr("Thanks, <br>The RetroShare Team");
    composer->setMsgText(sMsgText);

    std::list <std::string>::const_iterator peerIt;
    for (peerIt = sslIds.begin(); peerIt != sslIds.end(); peerIt++) {
        if (*peerIt == to) {
            continue;
        }
        composer->addRecipient(CC, *peerIt, false);
    }

    if (autoSend) {
        if (composer->sendMessage_internal(false)) {
            composer->close();
            return;
        }
    }

    composer->show();

    /* window will destroy itself! */
}

void MessageComposer::sendConnectAttemptMsg(const std::string &gpgId, const std::string &sslId, const QString &sslName)
{
    if (gpgId.empty()) {
        return;
    }

    RetroShareLink link;
    if (link.createUnknwonSslCertificate(sslId, gpgId) == false) {
        return;
    }

    QString title = QString("%1 %2").arg(sslName, tr("wants to be friends with you on RetroShare"));

    /* search for an exisiting message in the inbox */
    std::list<MsgInfoSummary> msgList;
    std::list<MsgInfoSummary>::const_iterator it;

    rsMsgs->getMessageSummaries(msgList);
    for(it = msgList.begin(); it != msgList.end(); it++) {
        if (it->msgflags & RS_MSG_TRASH) {
            continue;
        }
        if ((it->msgflags & RS_MSG_BOXMASK) != RS_MSG_INBOX) {
            continue;
        }
        if ((it->msgflags & RS_MSG_USER_REQUEST) == 0) {
            continue;
        }
        if (it->title == title.toStdWString()) {
            return;
        }
    }

    /* create a message */
    QString msgText = tr("Hi %1,<br><br>%2 wants to be friends with you on RetroShare.<br><br>Respond now:<br>%3<br><br>Thanks,<br>The RetroShare Team").arg(QString::fromUtf8(rsPeers->getGPGName(rsPeers->getGPGOwnId()).c_str()), sslName, link.toHtml());
    rsMsgs->SystemMessage(title.toStdWString(), msgText.toStdWString(), RS_MSG_USER_REQUEST);
}

void MessageComposer::closeEvent (QCloseEvent * event)
{
    bool bClose = true;

    /* Save to Drafts? */

    if (ui.msgText->document()->isModified()) {
        QMessageBox::StandardButton ret;
        ret = QMessageBox::warning(this, tr("Save Message"),
                                   tr("Message has not been Sent.\n"
                                      "Do you want to save message to draft box?"),
                                   QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
        switch (ret) {
        case QMessageBox::Yes:
            sendMessage_internal(true);
            break;
        case QMessageBox::Cancel:
            bClose = false;
            break;
        default:
            break;
        }
    }

    if (bClose) {
        Settings->saveWidgetInformation(this);

        // save settings
        processSettings(false);

        QMainWindow::closeEvent(event);
    } else {
        event->ignore();
    }
}

void MessageComposer::contextMenu(QPoint point)
{
    QMenu *contextMnu = ui.msgText->createStandardContextMenu(point);

    contextMnu->addSeparator();
    QAction *action = contextMnu->addAction(QIcon(":/images/pasterslink.png"), tr("Paste RetroShare Link"), this, SLOT(pasteLink()));
    action->setDisabled(RSLinkClipboard::empty());

    contextMnu->exec(QCursor::pos());
    delete(contextMnu);
}

void MessageComposer::pasteLink()
{
    ui.msgText->insertHtml(RSLinkClipboard::toHtml()) ;
}

void MessageComposer::contextMenuFileList(QPoint)
{
    QMenu contextMnu(this);

    QAction *action = contextMnu.addAction(QIcon(":/images/pasterslink.png"), tr("Paste RetroShare Link"), this, SLOT(pasteRecommended()));
    action->setDisabled(RSLinkClipboard::empty(RetroShareLink::TYPE_FILE));

    contextMnu.exec(QCursor::pos());
}

void MessageComposer::contextMenuMsgSendList(QPoint)
{
    QMenu contextMnu(this);

    int selectedCount = ui.friendSelectionWidget->selectedItemCount();

    FriendSelectionWidget::IdType idType;
    ui.friendSelectionWidget->selectedId(idType);

    QAction *action = contextMnu.addAction(QIcon(), tr("Add to \"To\""), this, SLOT(addTo()));
    action->setEnabled(selectedCount);
    action = contextMnu.addAction(QIcon(), tr("Add to \"CC\""), this, SLOT(addCc()));
    action->setEnabled(selectedCount);
    action = contextMnu.addAction(QIcon(), tr("Add to \"BCC\""), this, SLOT(addBcc()));
    action->setEnabled(selectedCount);
    action = contextMnu.addAction(QIcon(), tr("Add as Recommend"), this, SLOT(addRecommend()));
    action->setEnabled(selectedCount);

    contextMnu.addSeparator();

    action = contextMnu.addAction(QIcon(IMAGE_FRIENDINFO), tr("Friend Details"), this, SLOT(friendDetails()));
    action->setEnabled(selectedCount == 1 && idType == FriendSelectionWidget::IDTYPE_SSL);

    contextMnu.exec(QCursor::pos());
}

void MessageComposer::pasteRecommended()
{
    QList<RetroShareLink> links;
    RSLinkClipboard::pasteLinks(links);

    for (int i = 0; i < links.size(); i++) {
        if (links[i].valid() && links[i].type() == RetroShareLink::TYPE_FILE) {
            FileInfo fileInfo;
            fileInfo.fname = links[i].name().toStdString();
            fileInfo.hash = links[i].hash().toStdString();
            fileInfo.size = links[i].size();

            addFile(fileInfo);
        }
    }
}

static void setNewCompleter(QTableWidget *tableWidget, QCompleter *completer)
{
    int rowCount = tableWidget->rowCount();
    int row;

    for (row = 0; row < rowCount; row++) {
        QLineEdit *lineEdit = dynamic_cast<QLineEdit*>(tableWidget->cellWidget(row, COLUMN_RECIPIENT_NAME));
        if (lineEdit) {
            lineEdit->setCompleter(completer);
        }
    }
}

void MessageComposer::buildCompleter()
{
    // get existing groups
    std::list<RsGroupInfo> groupInfoList;
    std::list<RsGroupInfo>::iterator groupIt;
    rsPeers->getGroupInfoList(groupInfoList);

    std::list<std::string> peers;
    std::list<std::string>::iterator peerIt;
    rsPeers->getFriendList(peers);

    // create completer list for friends
    QStringList completerList;
    QStringList completerGroupList;

    for (peerIt = peers.begin(); peerIt != peers.end(); peerIt++) {
        RsPeerDetails detail;
        if (!rsPeers->getPeerDetails(*peerIt, detail)) {
            continue; /* BAD */
        }

        QString name = PeerDefs::nameWithLocation(detail);
        if (completerList.indexOf(name) == -1) {
            completerList.append(name);
        }
    }

    completerList.sort();

    // create completer list for groups
    for (groupIt = groupInfoList.begin(); groupIt != groupInfoList.end(); groupIt++) {
        completerGroupList.append(GroupDefs::name(*groupIt));
    }

    completerGroupList.sort();
    completerList.append(completerGroupList);

    m_completer = new QCompleter(completerList, this);
    m_completer->setCaseSensitivity(Qt::CaseInsensitive);
    setNewCompleter(ui.recipientWidget, m_completer);
}

void MessageComposer::peerStatusChanged(const QString& peer_id, int status)
{
    int rowCount = ui.recipientWidget->rowCount();
    int row;

    for (row = 0; row < rowCount; row++) {
        enumType type;
        std::string id;
        bool group;

        if (getRecipientFromRow(row, type, id, group) && id.empty() == false) {
            if (group == false && QString::fromStdString(id) == peer_id) {
                QTableWidgetItem *item = ui.recipientWidget->item(row, COLUMN_RECIPIENT_ICON);
                if (item) {
                    item->setIcon(QIcon(StatusDefs::imageUser(status)));
                }
            }
        }
    }
}

void MessageComposer::setFileList(const std::list<DirDetails>& dir_info)
{
    std::list<FileInfo> files_info;
    std::list<DirDetails>::const_iterator it;

    /* convert dir_info to files_info */
    for(it = dir_info.begin(); it != dir_info.end(); it++)
    {
        FileInfo info ;
        info.fname = it->name ;
        info.hash = it->hash ;
        info.size = it->count ;
        files_info.push_back(info) ;
    }

    setFileList(files_info);
}

void MessageComposer::setFileList(const std::list<FileInfo>& files_info)
{
    _recList.clear() ;

    ui.msgFileList->clear();

    std::list<FileInfo>::const_iterator it;
    for(it = files_info.begin(); it != files_info.end(); it++) {
        addFile(*it);
    }

    ui.msgFileList->update(); /* update display */
}

void MessageComposer::addFile(const FileInfo &fileInfo)
{
    for(std::list<FileInfo>::iterator it = _recList.begin(); it != _recList.end(); it++) {
        if (it->hash == fileInfo.hash) {
            /* File already added */
            return;
        }
    }

    _recList.push_back(fileInfo) ;

    /* make a widget per person */
    QTreeWidgetItem *item = new QTreeWidgetItem;

    item->setText(COLUMN_FILE_NAME, QString::fromUtf8(fileInfo.fname.c_str()));
    item->setText(COLUMN_FILE_SIZE, misc::friendlyUnit(fileInfo.size));
    item->setText(COLUMN_FILE_HASH, QString::fromStdString(fileInfo.hash));
    item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
    item->setCheckState(COLUMN_FILE_CHECKED, Qt::Checked);

    /* add to the list */
    ui.msgFileList->addTopLevelItem(item);
}

/* title changed */
void MessageComposer::titleChanged()
{
    calculateTitle();
    ui.msgText->document()->setModified(true);
}

void MessageComposer::calculateTitle()
{
    setWindowTitle(tr("Compose") + ": " + misc::removeNewLine(ui.titleEdit->text()));
}

static void calculateGroupsOfSslIds(std::list<RsGroupInfo> &existingGroupInfos, std::list<std::string> &checkSslIds, std::list<std::string> &checkGroupIds)
{
    checkGroupIds.clear();

    if (checkSslIds.empty()) {
        // nothing to do
        return;
    }

    std::map<std::string, std::string> sslToGpg;
    std::map<std::string, std::list<std::string> > gpgToSslIds;

    std::list<RsGroupInfo> groupInfos;

    // iterate all groups
    std::list<RsGroupInfo>::iterator groupInfoIt;
    for (groupInfoIt = existingGroupInfos.begin(); groupInfoIt != existingGroupInfos.end(); groupInfoIt++) {
        if (groupInfoIt->peerIds.empty()) {
            continue;
        }

        // iterate all assigned peers (gpg id's)
        std::list<std::string>::iterator peerIt;
        for (peerIt = groupInfoIt->peerIds.begin(); peerIt != groupInfoIt->peerIds.end(); peerIt++) {
            std::list<std::string> sslIds;

            std::map<std::string, std::list<std::string> >::iterator it = gpgToSslIds.find(*peerIt);
            if (it == gpgToSslIds.end()) {
                rsPeers->getAssociatedSSLIds(*peerIt, sslIds);

                gpgToSslIds[*peerIt] = sslIds;
            } else {
                sslIds = it->second;
            }

            // iterate all ssl id's
            std::list<std::string>::const_iterator sslIt;
            for (sslIt = sslIds.begin(); sslIt != sslIds.end(); sslIt++) {
                // search in ssl list
                if (std::find(checkSslIds.begin(), checkSslIds.end(), *sslIt) == checkSslIds.end()) {
                    // not found
                    break;
                }
            }
            if (sslIt != sslIds.end()) {
                // one or more ssl id's not found
                break;
            }
        }

        if (peerIt == groupInfoIt->peerIds.end()) {
            // all ssl id's of all assigned gpg id's found
            groupInfos.push_back(*groupInfoIt);
        }
    }

    // remove all ssl id's of all found groups from the list
    for (groupInfoIt = groupInfos.begin(); groupInfoIt != groupInfos.end(); groupInfoIt++) {
        // iterate all assigned peers (gpg id's)
        std::list<std::string>::iterator peerIt;
        for (peerIt = groupInfoIt->peerIds.begin(); peerIt != groupInfoIt->peerIds.end(); peerIt++) {
            std::list<std::string> sslIds;

            std::map<std::string, std::list<std::string> >::iterator it = gpgToSslIds.find(*peerIt);
            if (it == gpgToSslIds.end()) {
                rsPeers->getAssociatedSSLIds(*peerIt, sslIds);

                gpgToSslIds[*peerIt] = sslIds;
            } else {
                sslIds = it->second;
            }

            // iterate all ssl id's
            std::list<std::string>::const_iterator sslIt;
            for (sslIt = sslIds.begin(); sslIt != sslIds.end(); sslIt++) {
                // search in ssl list
                std::list<std::string>::iterator it = std::find(checkSslIds.begin(), checkSslIds.end(), *sslIt);
                if (it != checkSslIds.end()) {
                    checkSslIds.erase(it);
                }
            }
        }

        checkGroupIds.push_back(groupInfoIt->id);
    }
}

MessageComposer *MessageComposer::newMsg(const std::string &msgId /* = ""*/)
{
    MessageComposer *msgComposer = new MessageComposer();

    msgComposer->addEmptyRecipient();

    if (msgId.empty() == false) {
        // fill existing message
        MessageInfo msgInfo;
        if (!rsMsgs->getMessage(msgId, msgInfo)) {
            std::cerr << "MessageComposer::newMsg() Couldn't find Msg" << std::endl;
            delete msgComposer;
            return NULL;
        }

        if (msgInfo.msgflags & RS_MSG_DRAFT) {
            msgComposer->m_sDraftMsgId = msgId;

            rsMsgs->getMsgParentId(msgId,  msgComposer->m_msgParentId);

            if (msgInfo.msgflags & RS_MSG_REPLIED) {
                msgComposer->m_msgType = REPLY;
            } else if (msgInfo.msgflags & RS_MSG_FORWARDED) {
                msgComposer->m_msgType = FORWARD;
            }
        }

        // needed to send system flags with reply
        msgComposer->msgFlags = (msgInfo.msgflags & RS_MSG_SYSTEM);

        msgComposer->setTitleText(QString::fromStdWString(msgInfo.title));
        msgComposer->setMsgText(QString::fromStdWString(msgInfo.msg));
        msgComposer->setFileList(msgInfo.files);

        // get existing groups
        std::list<RsGroupInfo> groupInfoList;
        rsPeers->getGroupInfoList(groupInfoList);

        std::list<std::string> groupIds;
        std::list<std::string>::iterator groupIt;
        std::list<std::string>::iterator it;

        calculateGroupsOfSslIds(groupInfoList, msgInfo.msgto, groupIds);
        for (groupIt = groupIds.begin(); groupIt != groupIds.end(); groupIt++ ) {
            msgComposer->addRecipient(MessageComposer::TO, *groupIt, true) ;
        }
        for (it = msgInfo.msgto.begin(); it != msgInfo.msgto.end(); it++ ) {
            msgComposer->addRecipient(MessageComposer::TO, *it, false) ;
        }

        calculateGroupsOfSslIds(groupInfoList, msgInfo.msgcc, groupIds);
        for (groupIt = groupIds.begin(); groupIt != groupIds.end(); groupIt++ ) {
            msgComposer->addRecipient(MessageComposer::CC, *groupIt, true) ;
        }
        for (it = msgInfo.msgcc.begin(); it != msgInfo.msgcc.end(); it++ ) {
            msgComposer->addRecipient(MessageComposer::CC, *it, false) ;
        }

        calculateGroupsOfSslIds(groupInfoList, msgInfo.msgbcc, groupIds);
        for (groupIt = groupIds.begin(); groupIt != groupIds.end(); groupIt++ ) {
            msgComposer->addRecipient(MessageComposer::BCC, *groupIt, true) ;
        }
        for (it = msgInfo.msgbcc.begin(); it != msgInfo.msgbcc.end(); it++ ) {
            msgComposer->addRecipient(MessageComposer::BCC, *it, false) ;
        }

        MsgTagInfo tagInfo;
        rsMsgs->getMessageTag(msgId, tagInfo);
        msgComposer->m_tagIds = tagInfo.tagIds;

        msgComposer->showTagLabels();

        msgComposer->ui.msgText->document()->setModified(false);
    }

    msgComposer->calculateTitle();

    return msgComposer;
}

QString MessageComposer::buildReplyHeader(const MessageInfo &msgInfo)
{
    RetroShareLink link;
    link.createMessage(msgInfo.srcId, "");
    QString from = link.toHtml();

    QString to;
    std::list<std::string>::const_iterator it;
    for (it = msgInfo.msgto.begin(); it != msgInfo.msgto.end(); it++) {
        if (link.createMessage(*it, "")) {
            if (!to.isEmpty()) {
                to += ", ";
            }
            to += link.toHtml();
        }
    }

    QString cc;
    for (it = msgInfo.msgcc.begin(); it != msgInfo.msgcc.end(); it++) {
        if (link.createMessage(*it, "")) {
            if (!cc.isEmpty()) {
                cc += ", ";
            }
            cc += link.toHtml();
        }
    }

    QString header = QString("<span>-----%1-----").arg(tr("Original Message"));
    header += QString("<br><font size='3'><strong>%1: </strong>%2</font><br>").arg(tr("From"), from);
    header += QString("<font size='3'><strong>%1: </strong>%2</font><br>").arg(tr("To"), to);

    if (!cc.isEmpty()) {
        header += QString("<font size='3'><strong>%1: </strong>%2</font><br>").arg(tr("Cc"), cc);
    }

    header += QString("<br><font size='3'><strong>%1: </strong>%2</font><br>").arg(tr("Sent"), DateTime::formatLongDateTime(msgInfo.ts));
    header += QString("<font size='3'><strong>%1: </strong>%2</font></span><br>").arg(tr("Subject"), QString::fromStdWString(msgInfo.title));
    header += "<br>";

    header += tr("On %1, %2 wrote:").arg(DateTime::formatDateTime(msgInfo.ts), from);

    return header;
}

void MessageComposer::setQuotedMsg(const QString &msg, const QString &header)
{
    ui.msgText->setUndoRedoEnabled(false);

    ui.msgText->setText(msg);

    QTextBlock block = ui.msgText->document()->firstBlock();
    while (block.isValid()) {
        QTextCursor cursor = ui.msgText->textCursor();
        cursor.setPosition(block.position());

        QTextBlockFormat format;
        format.setProperty(TextFormat::IsBlockQuote, true);
        format.setLeftMargin(block.blockFormat().leftMargin() + 20);
        format.setRightMargin(block.blockFormat().rightMargin() + 20);
        cursor.mergeBlockFormat(format);

        block = block.next();
    }

    ui.msgText->moveCursor(QTextCursor::Start);
    ui.msgText->textCursor().insertBlock();
    ui.msgText->moveCursor(QTextCursor::Start);

    if (header.isEmpty()) {
        ui.msgText->insertHtml("<br><br>");
    } else {
        ui.msgText->insertHtml("<br><br>" + header + "<br>");
    }

    ui.msgText->moveCursor(QTextCursor::Start);

    ui.msgText->setUndoRedoEnabled(true);
    ui.msgText->document()->setModified(true);

    ui.msgText->setFocus( Qt::OtherFocusReason );
}

MessageComposer *MessageComposer::replyMsg(const std::string &msgId, bool all)
{
    MessageInfo msgInfo;
    if (!rsMsgs->getMessage(msgId, msgInfo)) {
        return NULL;
    }

    MessageComposer *msgComposer = MessageComposer::newMsg();
    msgComposer->m_msgParentId = msgId;
    msgComposer->m_msgType = REPLY;

    /* fill it in */

    msgComposer->setTitleText(QString::fromStdWString(msgInfo.title), REPLY);
    msgComposer->setQuotedMsg(QString::fromStdWString(msgInfo.msg), buildReplyHeader(msgInfo));

    msgComposer->addRecipient(MessageComposer::TO, msgInfo.srcId, false);

    if (all) {
        std::string ownId = rsPeers->getOwnId();

        for (std::list<std::string>::iterator tli = msgInfo.msgto.begin(); tli != msgInfo.msgto.end(); tli++) {
            if (ownId != *tli) {
                msgComposer->addRecipient(MessageComposer::TO, *tli, false) ;
            }
        }

        for (std::list<std::string>::iterator tli = msgInfo.msgcc.begin(); tli != msgInfo.msgcc.end(); tli++) {
            if (ownId != *tli) {
                msgComposer->addRecipient(MessageComposer::TO, *tli, false) ;
            }
        }
    }

    // needed to send system flags with reply
    msgComposer->msgFlags = (msgInfo.msgflags & RS_MSG_SYSTEM);

    msgComposer->calculateTitle();

    /* window will destroy itself! */

    return msgComposer;
}

MessageComposer *MessageComposer::forwardMsg(const std::string &msgId)
{
    MessageInfo msgInfo;
    if (!rsMsgs->getMessage(msgId, msgInfo)) {
        return NULL;
    }

    MessageComposer *msgComposer = MessageComposer::newMsg();
    msgComposer->m_msgParentId = msgId;
    msgComposer->m_msgType = FORWARD;

    /* fill it in */

    msgComposer->setTitleText(QString::fromStdWString(msgInfo.title), FORWARD);
    msgComposer->setQuotedMsg(QString::fromStdWString(msgInfo.msg), buildReplyHeader(msgInfo));

    std::list<FileInfo>& files_info = msgInfo.files;

    msgComposer->setFileList(files_info);

    // needed to send system flags with reply
    msgComposer->msgFlags = (msgInfo.msgflags & RS_MSG_SYSTEM);

    msgComposer->calculateTitle();

    /* window will destroy itself! */

    return msgComposer;
}

void MessageComposer::setTitleText(const QString &title, enumMessageType type)
{
    QString titleText;

    switch (type) {
    case NORMAL:
        titleText = title;
        break;
    case REPLY:
        if (title.startsWith("Re:", Qt::CaseInsensitive)) {
            titleText = title;
        } else {
            titleText = tr("Re:") + " " + title;
        }
        break;
    case FORWARD:
        if (title.startsWith("Fwd:", Qt::CaseInsensitive)) {
            titleText = title;
        } else {
            titleText = tr("Fwd:") + " " + title;
        }
        break;
    }

    ui.titleEdit->setText(misc::removeNewLine(titleText));
}

void MessageComposer::setMsgText(const QString &msg, bool asHtml)
{
    if (asHtml) {
        ui.msgText->setHtml(msg);
    } else {
        ui.msgText->setText(msg);
    }

    ui.msgText->setFocus( Qt::OtherFocusReason );

    QTextCursor c =  ui.msgText->textCursor();
    c.movePosition(QTextCursor::End);
    ui.msgText->setTextCursor(c);

    ui.msgText->document()->setModified(true);
}

void MessageComposer::sendMessage()
{
    if (sendMessage_internal(false)) {
        close();
    }
}

bool MessageComposer::sendMessage_internal(bool bDraftbox)
{
    /* construct a message */
    MessageInfo mi;

    mi.title = misc::removeNewLine(ui.titleEdit->text()).toStdWString();
    // needed to send system flags with reply
    mi.msgflags = msgFlags;

    QString text;
    RsHtml::optimizeHtml(ui.msgText, text);
    mi.msg = text.toStdWString();

    /* check for existing title */
    if (bDraftbox == false && mi.title.empty()) {
        if (QMessageBox::warning(this, tr("RetroShare"), tr("Do you want to send the message without a subject ?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::No) {
            ui.titleEdit->setFocus();
            return false; // Don't send with an empty subject
        }
    }

    int filesCount = ui.msgFileList->topLevelItemCount();
    for (int i = 0; i < filesCount; i++) {
        QTreeWidgetItem *item = ui.msgFileList->topLevelItem(i);
        if (item->checkState(COLUMN_FILE_CHECKED)) {
            std::string hash = item->text(COLUMN_FILE_HASH).toStdString();
            for(std::list<FileInfo>::iterator it = _recList.begin(); it != _recList.end(); it++) {
                if (it->hash == hash) {
                    mi.files.push_back(*it);
                    break;
                }
            }
        }
    }

    /* get the ids from the send list */
    std::list<std::string> peers;
    rsPeers->getFriendList(peers);

    /* add own id */
    peers.push_back(rsPeers->getOwnId());

    int rowCount = ui.recipientWidget->rowCount();
    int row;

    for (row = 0; row < rowCount; row++) {
        enumType type;
        std::string id;
        bool group;

        if (getRecipientFromRow(row, type, id, group) && id.empty() == false) {
            if (group) {
                RsGroupInfo groupInfo;
                if (rsPeers->getGroupInfo(id, groupInfo) == false) {
                    // group not found
                    continue;
                }

                std::list<std::string>::const_iterator groupIt;
                for (groupIt = groupInfo.peerIds.begin(); groupIt != groupInfo.peerIds.end(); groupIt++) {
                    std::list<std::string> sslIds;
                    rsPeers->getAssociatedSSLIds(*groupIt, sslIds);

                    std::list<std::string>::const_iterator sslIt;
                    for (sslIt = sslIds.begin(); sslIt != sslIds.end(); sslIt++) {
                        if (std::find(peers.begin(), peers.end(), *sslIt) == peers.end()) {
                            // no friend
                            continue;
                        }

                        switch (type) {
                        case TO:
                                if (std::find(mi.msgto.begin(), mi.msgto.end(), *sslIt) == mi.msgto.end()) {
                                    mi.msgto.push_back(*sslIt);
                                }
                                break;
                        case CC:
                                if (std::find(mi.msgcc.begin(), mi.msgcc.end(), *sslIt) == mi.msgcc.end()) {
                                    mi.msgcc.push_back(*sslIt);
                                }
                                break;
                        case BCC:
                                if (std::find(mi.msgbcc.begin(), mi.msgbcc.end(), *sslIt) == mi.msgbcc.end()) {
                                    mi.msgbcc.push_back(*sslIt);
                                }
                                break;
                        }
                    }
                }
            } else {
                if (_distant_peers.find(id)==_distant_peers.end() && std::find(peers.begin(), peers.end(), id) == peers.end()) {
                    // no friend
                    continue;
                }

                switch (type) {
                case TO:
                        if (std::find(mi.msgto.begin(), mi.msgto.end(), id) == mi.msgto.end()) {
                            mi.msgto.push_back(id);
                        }
                        break;
                case CC:
                        if (std::find(mi.msgcc.begin(), mi.msgcc.end(), id) == mi.msgcc.end()) {
                            mi.msgcc.push_back(id);
                        }
                        break;
                case BCC:
                        if (std::find(mi.msgbcc.begin(), mi.msgbcc.end(), id) == mi.msgbcc.end()) {
                            mi.msgbcc.push_back(id);
                        }
                        break;
                }
            }
        }
    }

    if (bDraftbox) {
        mi.msgId = m_sDraftMsgId;

        rsMsgs->MessageToDraft(mi, m_msgParentId);

        // use new message id
        m_sDraftMsgId = mi.msgId;

        switch (m_msgType) {
        case NORMAL:
            break;
        case REPLY:
            rsMsgs->MessageReplied(m_sDraftMsgId, true);
            break;
        case FORWARD:
            rsMsgs->MessageForwarded(m_sDraftMsgId, true);
            break;
        }
    } else {
        /* check for the recipient */
        if (mi.msgto.empty()) {
            QMessageBox::warning(this, tr("RetroShare"), tr("Please insert at least one recipient."), QMessageBox::Ok);
            return false; // Don't send with no recipient
        }
		  mi.encryption_keys = _distant_peers ;

		  if(ui.signMessage_CB->isChecked())
			  mi.msgflags |= RS_MSG_SIGNED ;

        if (rsMsgs->MessageSend(mi) == false) {
            return false;
        }

        if (m_msgParentId.empty() == false) {
            switch (m_msgType) {
            case NORMAL:
                break;
            case REPLY:
                rsMsgs->MessageReplied(m_msgParentId, true);
                break;
            case FORWARD:
                rsMsgs->MessageForwarded(m_msgParentId, true);
                break;
            }
        }
    }

    if (mi.msgId.empty() == false) {
        MsgTagInfo tagInfo;
        rsMsgs->getMessageTag(mi.msgId, tagInfo);

        /* insert new tags */
        std::list<uint32_t>::iterator tag;
        for (tag = m_tagIds.begin(); tag != m_tagIds.end(); tag++) {
            if (std::find(tagInfo.tagIds.begin(), tagInfo.tagIds.end(), *tag) == tagInfo.tagIds.end()) {
                rsMsgs->setMessageTag(mi.msgId, *tag, true);
            } else {
                tagInfo.tagIds.remove(*tag);
            }
        }

        /* remove deleted tags */
        for (tag = tagInfo.tagIds.begin(); tag != tagInfo.tagIds.end(); tag++) {
            rsMsgs->setMessageTag(mi.msgId, *tag, false);
        }
    }
    ui.msgText->document()->setModified(false);
    return true;
}

void  MessageComposer::cancelMessage()
{
    close();
    return;
}

/* Toggling .... Check Boxes.....
 * This is dependent on whether we are a
 *
 * Chan or Msg Dialog.
 */

void MessageComposer::addEmptyRecipient()
{
    int lastRow = ui.recipientWidget->rowCount();
    if (lastRow > 0) {
        QTableWidgetItem *item = ui.recipientWidget->item(lastRow - 1, COLUMN_RECIPIENT_DATA);
        if (item && item->data(ROLE_RECIPIENT_ID).toString().isEmpty()) {
            return;
        }
    }

    setRecipientToRow(lastRow, TO, "", false);
}

bool MessageComposer::getRecipientFromRow(int row, enumType &type, std::string &id, bool &group)
{
    if (row >= ui.recipientWidget->rowCount()) {
        return false;
    }

    QComboBox *cb = dynamic_cast<QComboBox*>(ui.recipientWidget->cellWidget(row, COLUMN_RECIPIENT_TYPE));
    if (cb == NULL) {
        return false;
    }

    type = (enumType) cb->itemData(cb->currentIndex(), Qt::UserRole).toInt();
    id = ui.recipientWidget->item(row, COLUMN_RECIPIENT_DATA)->data(ROLE_RECIPIENT_ID).toString().toStdString();
    group = ui.recipientWidget->item(row, COLUMN_RECIPIENT_DATA)->data(ROLE_RECIPIENT_GROUP).toBool();

    return true;
}

void MessageComposer::setRecipientToRow(int row, enumType type, std::string id, bool group)
{
    if (row + 1 > ui.recipientWidget->rowCount()) {
        ui.recipientWidget->setRowCount(row + 1);
    }

    QComboBox *comboBox = dynamic_cast<QComboBox*>(ui.recipientWidget->cellWidget(row, COLUMN_RECIPIENT_TYPE));
    if (comboBox == NULL) {
        comboBox = new QComboBox;
        comboBox->addItem(tr("To"), TO);
        comboBox->addItem(tr("Cc"), CC);
        comboBox->addItem(tr("Bcc"), BCC);

        ui.recipientWidget->setCellWidget(row, COLUMN_RECIPIENT_TYPE, comboBox);

        comboBox->installEventFilter(this);
    }

    QLineEdit *lineEdit = dynamic_cast<QLineEdit*>(ui.recipientWidget->cellWidget(row, COLUMN_RECIPIENT_NAME));
    if (lineEdit == NULL) {
        lineEdit = new QLineEdit;

        QString objectName = "lineEdit" + QString::number(row);
        lineEdit->setObjectName(objectName);

        lineEdit->setCompleter(m_completer);

        ui.recipientWidget->setCellWidget(row, COLUMN_RECIPIENT_NAME, lineEdit);

        connect(lineEdit, SIGNAL(editingFinished()), this, SLOT(editingRecipientFinished()));
        lineEdit->installEventFilter(this);
    }

    QIcon icon;
    QString name;
    if (!id.empty()) 
	 {
        if (group) 
		  {
            icon = QIcon(IMAGE_GROUP16);

            RsGroupInfo groupInfo;
            if (rsPeers->getGroupInfo(id, groupInfo)) {
                name = GroupDefs::name(groupInfo);
            } else {
                name = tr("Unknown");
                id.clear();
            }
        } 
		  else 
		  {
			  RsPeerDetails details;

			  if(_distant_peers.find(id) != _distant_peers.end())
			  {
			  	  if(!rsPeers->getPeerDetails(_distant_peers[id], details))
				  {
					  std::cerr << "Can't get peer details from " << _distant_peers[id] << std::endl;
					  return ;
				  }

				  name = tr("Distant peer (name: %2, PGP key: %1)").arg(QString::fromStdString(_distant_peers[id])).arg(QString::fromStdString(details.name)) ;
				  icon = QIcon(StatusDefs::imageUser(RS_STATUS_ONLINE));
			  }
			  else if(rsPeers->getPeerDetails(id, details) && (!details.isOnlyGPGdetail))
			  {
				  name = PeerDefs::nameWithLocation(details);

				  StatusInfo peerStatusInfo;
				  // No check of return value. Non existing status info is handled as offline.
				  rsStatus->getStatus(id, peerStatusInfo);

				  icon = QIcon(StatusDefs::imageUser(peerStatusInfo.status));
			  } 
			  else 
			  {
				  icon = QIcon(StatusDefs::imageUser(RS_STATUS_OFFLINE));
				  name = tr("Unknown friend");
				  id.clear();
			  }
		  }
    }

    comboBox->setCurrentIndex(comboBox->findData(type, Qt::UserRole));

    lineEdit->setText(name);
    if (id.empty()) {
        lineEdit->setStyleSheet(QString(STYLE_FAIL).arg(lineEdit->objectName()));
    } else {
        lineEdit->setStyleSheet(QString(STYLE_NORMAL).arg(lineEdit->objectName()));
    }

    QTableWidgetItem *item = new QTableWidgetItem(icon, "", 0);
    item->setFlags(Qt::NoItemFlags);
    ui.recipientWidget->setItem(row, COLUMN_RECIPIENT_ICON, item);
    ui.recipientWidget->item(row, COLUMN_RECIPIENT_DATA)->setData(ROLE_RECIPIENT_ID, QString::fromStdString(id));
    ui.recipientWidget->item(row, COLUMN_RECIPIENT_DATA)->setData(ROLE_RECIPIENT_GROUP, group);

    addEmptyRecipient();
}

bool MessageComposer::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::FocusIn) {
        QLineEdit *lineEdit = dynamic_cast<QLineEdit*>(obj);
        if (lineEdit) {
            int rowCount = ui.recipientWidget->rowCount();
            int row;
            for (row = 0; row < rowCount; row++) {
                if (ui.recipientWidget->cellWidget(row, COLUMN_RECIPIENT_NAME) == lineEdit) {
                    break;
                }
            }
            if (row < rowCount) {
                ui.recipientWidget->setCurrentCell(row, COLUMN_RECIPIENT_NAME);
//                lineEdit->setFocus();
            }
        } else {
            QComboBox *comboBox = dynamic_cast<QComboBox*>(obj);
            if (comboBox) {
                int rowCount = ui.recipientWidget->rowCount();
                int row;
                for (row = 0; row < rowCount; row++) {
                    if (ui.recipientWidget->cellWidget(row, COLUMN_RECIPIENT_TYPE) == comboBox) {
                        break;
                    }
                }
                if (row < rowCount) {
                    ui.recipientWidget->setCurrentCell(row, COLUMN_RECIPIENT_TYPE);
//                    comboBox->setFocus();
                }
            }
        }
    }

    // pass the event on to the parent class
    return QMainWindow::eventFilter(obj, event);
}

void MessageComposer::editingRecipientFinished()
{
    QLineEdit *lineEdit = dynamic_cast<QLineEdit*>(QObject::sender());
    if (lineEdit == NULL) {
        return;
    }

    lineEdit->setStyleSheet(QString(STYLE_NORMAL).arg(lineEdit->objectName()));

    // find row of the widget
    int rowCount = ui.recipientWidget->rowCount();
    int row;
    for (row = 0; row < rowCount; row++) {
        if (ui.recipientWidget->cellWidget(row, COLUMN_RECIPIENT_NAME) == lineEdit) {
            break;
        }
    }
    if (row >= rowCount) {
        // not found
        return;
    }

    enumType type;
    std::string id; // dummy
    bool group; // dummy
    getRecipientFromRow(row, type, id, group);

    QString text = lineEdit->text();

    if (text.isEmpty()) {
        setRecipientToRow(row, type, "", false);
        return;
    }

    // start with peers
    std::list<std::string> peers;
    rsPeers->getFriendList(peers);

    std::list<std::string>::iterator peerIt;
    for (peerIt = peers.begin(); peerIt != peers.end(); peerIt++) {
        RsPeerDetails details;
        if (!rsPeers->getPeerDetails(*peerIt, details)) {
            continue; /* BAD */
        }

        QString name = PeerDefs::nameWithLocation(details);
        if (text.compare(name, Qt::CaseSensitive) == 0) {
            // found it
            setRecipientToRow(row, type, details.id, false);
            return;
        }
    }

    // then groups
    std::list<RsGroupInfo> groupInfoList;
    rsPeers->getGroupInfoList(groupInfoList);

    std::list<RsGroupInfo>::iterator groupIt;
    for (groupIt = groupInfoList.begin(); groupIt != groupInfoList.end(); groupIt++) {
        QString groupName = GroupDefs::name(*groupIt);
        if (text.compare(groupName, Qt::CaseSensitive) == 0) {
            // found it
            setRecipientToRow(row, type, groupIt->id, true);
            return;
        }
    }

    setRecipientToRow(row, type, "", false);
    lineEdit->setStyleSheet(QString(STYLE_FAIL).arg(lineEdit->objectName()));
    lineEdit->setText(text);
}

void MessageComposer::addRecipient(enumType type, const std::string& hash,const std::string& pgp_id)
{
	_distant_peers[hash] = pgp_id ;

	int rowCount = ui.recipientWidget->rowCount();
	int row;
	for (row = 0; row < rowCount; row++) 
	{
		enumType rowType;
		std::string rowId;
		bool rowGroup;

		if (getRecipientFromRow(row, rowType, rowId, rowGroup) == true) 
		{
			if (rowId.empty()) // use row
				break;

			if (rowId == hash && rowType == type) // existing row
				break;
		} 
		else // use row
			break;
	}

	setRecipientToRow(row, type, hash, false);
}
void MessageComposer::addRecipient(enumType type, const std::string &id, bool group)
{
    std::list<std::string> sslIds;
    if (group) {
        sslIds.push_back(id);
    } else {
        // check for gpg id or ssl id
        RsPeerDetails detail;
        if (!rsPeers->getPeerDetails(id, detail)) {
            return;
        }

        if (detail.isOnlyGPGdetail) {
            if (!rsPeers->getAssociatedSSLIds(id, sslIds)) {
                return;
            }
        } else {
            sslIds.push_back(id);
        }
    }
    std::list<std::string>::const_iterator sslIt;
    for (sslIt = sslIds.begin(); sslIt != sslIds.end(); sslIt++) {
        // search existing or empty row
        int rowCount = ui.recipientWidget->rowCount();
        int row;
        for (row = 0; row < rowCount; row++) {
            enumType rowType;
            std::string rowId;
            bool rowGroup;

            if (getRecipientFromRow(row, rowType, rowId, rowGroup) == true) {
                if (rowId.empty()) {
                    // use row
                    break;
                }

                if (rowId == *sslIt && rowType == type && group == rowGroup) {
                    // existing row
                    break;
                }
            } else {
                // use row
                break;
            }
        }

        setRecipientToRow(row, type, *sslIt, group);
    }
}

void MessageComposer::setupFileActions()
{
    QMenu *menu = new QMenu(tr("&File"), this);
    menuBar()->addMenu(menu);

    QAction *a;

    a = new QAction(QIcon(":/images/textedit/filenew.png"), tr("&New"), this);
    a->setShortcut(QKeySequence::New);
    connect(a, SIGNAL(triggered()), this, SLOT(fileNew()));
    menu->addAction(a);

    a = new QAction(QIcon(":/images/textedit/fileopen.png"), tr("&Open..."), this);
    a->setShortcut(QKeySequence::Open);
    connect(a, SIGNAL(triggered()), this, SLOT(fileOpen()));
    menu->addAction(a);

    menu->addSeparator();

    actionSave = a = new QAction(QIcon(":/images/textedit/filesave.png"), tr("&Save"), this);
    a->setShortcut(QKeySequence::Save);
    connect(a, SIGNAL(triggered()), this, SLOT(saveasDraft()));
    a->setEnabled(false);
    menu->addAction(a);

    a = new QAction(tr("Save &As File"), this);
    connect(a, SIGNAL(triggered()), this, SLOT(fileSaveAs()));
    menu->addAction(a);

    a = new QAction(tr("Save &As Draft"), this);
    connect(a, SIGNAL(triggered()), this, SLOT(saveasDraft()));
    menu->addAction(a);
    menu->addSeparator();

    a = new QAction(QIcon(":/images/textedit/fileprint.png"), tr("&Print..."), this);
    a->setShortcut(QKeySequence::Print);
    connect(a, SIGNAL(triggered()), this, SLOT(filePrint()));
    menu->addAction(a);

    /*a = new QAction(QIcon(":/images/textedit/fileprint.png"), tr("Print Preview..."), this);
    connect(a, SIGNAL(triggered()), this, SLOT(filePrintPreview()));
    menu->addAction(a);*/

    a = new QAction(QIcon(":/images/textedit/exportpdf.png"), tr("&Export PDF..."), this);
    a->setShortcut(Qt::CTRL + Qt::Key_D);
    connect(a, SIGNAL(triggered()), this, SLOT(filePrintPdf()));
    menu->addAction(a);

    menu->addSeparator();

    a = new QAction(tr("&Quit"), this);
    a->setShortcut(Qt::CTRL + Qt::Key_Q);
    connect(a, SIGNAL(triggered()), this, SLOT(close()));
    menu->addAction(a);
}

void MessageComposer::setupEditActions()
{
    QMenu *menu = new QMenu(tr("&Edit"), this);
    menuBar()->addMenu(menu);

    QAction *a;
    a = actionUndo = new QAction(QIcon(":/images/textedit/editundo.png"), tr("&Undo"), this);
    a->setShortcut(QKeySequence::Undo);
    menu->addAction(a);
    a = actionRedo = new QAction(QIcon(":/images/textedit/editredo.png"), tr("&Redo"), this);
    a->setShortcut(QKeySequence::Redo);
    menu->addAction(a);
    menu->addSeparator();
    a = actionCut = new QAction(QIcon(":/images/textedit/editcut.png"), tr("Cu&t"), this);
    a->setShortcut(QKeySequence::Cut);
    menu->addAction(a);
    a = actionCopy = new QAction(QIcon(":/images/textedit/editcopy.png"), tr("&Copy"), this);
    a->setShortcut(QKeySequence::Copy);
    menu->addAction(a);
    a = actionPaste = new QAction(QIcon(":/images/textedit/editpaste.png"), tr("&Paste"), this);
    a->setShortcut(QKeySequence::Paste);
    menu->addAction(a);
    actionPaste->setEnabled(!QApplication::clipboard()->text().isEmpty());
}

void MessageComposer::setupViewActions()
{
    QMenu *menu = new QMenu(tr("&View"), this);
    menuBar()->addMenu(menu);

    contactSidebarAction = menu->addAction(QIcon(), tr("&Contacts Sidebar"), this, SLOT(toggleContacts()));
    contactSidebarAction->setCheckable(true);
}

void MessageComposer::setupInsertActions()
{
    QMenu *menu = new QMenu(tr("&Insert"), this);
    menuBar()->addMenu(menu);

    QAction *a;

#if QT_VERSION >= 0x040700
	// embedded images are not supported before QT 4.7.0
    a = new QAction(QIcon(""), tr("&Image"), this);
    connect(a, SIGNAL(triggered()), this, SLOT(addImage()));
    menu->addAction(a);
#endif

    a = new QAction(QIcon(""), tr("&Horizontal Line"), this);
    connect(a, SIGNAL(triggered()), this, SLOT(addPostSplitter()));
    menu->addAction(a);

}

void MessageComposer::setupFormatActions()
{
    QMenu *menu = new QMenu(tr("&Format"), this);
    menuBar()->addMenu(menu);

    menu->addAction(actionAlignLeft);
    menu->addAction(actionAlignCenter);
    menu->addAction(actionAlignRight);
    menu->addAction(actionAlignJustify);

}

void MessageComposer::textBold()
{
    QTextCharFormat fmt;
    fmt.setFontWeight(ui.boldbtn->isChecked() ? QFont::Bold : QFont::Normal);
    mergeFormatOnWordOrSelection(fmt);
}

void MessageComposer::textUnderline()
{
    QTextCharFormat fmt;
    fmt.setFontUnderline(ui.underlinebtn->isChecked());
    mergeFormatOnWordOrSelection(fmt);
}

void MessageComposer::textItalic()
{
    QTextCharFormat fmt;
    fmt.setFontItalic(ui.italicbtn->isChecked());
    mergeFormatOnWordOrSelection(fmt);
}

void MessageComposer::textFamily(const QString &f)
{
    QTextCharFormat fmt;
    fmt.setFontFamily(f);
    mergeFormatOnWordOrSelection(fmt);
}

void MessageComposer::textSize(const QString &p)
{
    qreal pointSize = p.toFloat();
    if (p.toFloat() > 0) {
        QTextCharFormat fmt;
        fmt.setFontPointSize(pointSize);
        mergeFormatOnWordOrSelection(fmt);
    }
}

void MessageComposer::changeFormatType(int styleIndex )
{
    ui.msgText->setFocus( Qt::OtherFocusReason );

    QTextCursor cursor = ui.msgText->textCursor();
    //QTextBlockFormat bformat = cursor.blockFormat();
    QTextBlockFormat bformat;
    QTextCharFormat cformat;

    switch (styleIndex) {
         default:
            case 0:
            bformat.setProperty( TextFormat::HtmlHeading, QVariant( 0 ) );
            cformat.setFontWeight( QFont::Normal );
            cformat.setProperty( QTextFormat::FontSizeAdjustment, QVariant( 0 ) );
            break;
        case 1:
            bformat.setProperty( TextFormat::HtmlHeading, QVariant( 1 ) );
            cformat.setFontWeight( QFont::Bold );
            cformat.setProperty( QTextFormat::FontSizeAdjustment, QVariant( 3 ) );
            break;
        case 2:
            bformat.setProperty( TextFormat::HtmlHeading, QVariant( 2 ) );
            cformat.setFontWeight( QFont::Bold );
            cformat.setProperty( QTextFormat::FontSizeAdjustment, QVariant( 2 ) );
            break;
        case 3:
            bformat.setProperty( TextFormat::HtmlHeading, QVariant( 3 ) );
            cformat.setFontWeight( QFont::Bold );
            cformat.setProperty( QTextFormat::FontSizeAdjustment, QVariant( 1 ) );
            break;
        case 4:
            bformat.setProperty( TextFormat::HtmlHeading, QVariant( 4 ) );
            cformat.setFontWeight( QFont::Bold );
            cformat.setProperty( QTextFormat::FontSizeAdjustment, QVariant( 0 ) );
            break;
        case 5:
            bformat.setProperty( TextFormat::HtmlHeading, QVariant( 5 ) );
            cformat.setFontWeight( QFont::Bold );
            cformat.setProperty( QTextFormat::FontSizeAdjustment, QVariant( -1 ) );
            break;
        case 6:
            bformat.setProperty( TextFormat::HtmlHeading, QVariant( 6 ) );
            cformat.setFontWeight( QFont::Bold );
            cformat.setProperty( QTextFormat::FontSizeAdjustment, QVariant( -2 ) );
            break;
    }
    //cformat.clearProperty( TextFormat::HasCodeStyle );

    cursor.beginEditBlock();
    cursor.mergeBlockFormat( bformat );
    cursor.select( QTextCursor::BlockUnderCursor );
    cursor.mergeCharFormat( cformat );
    cursor.endEditBlock();
}

void MessageComposer::textStyle(int styleIndex)
{
    QTextCursor cursor = ui.msgText->textCursor();

    if (styleIndex != 0) {
        QTextListFormat::Style style = QTextListFormat::ListDisc;

        switch (styleIndex) {
            default:
            case 1:
                style = QTextListFormat::ListDisc;
                break;
            case 2:
                style = QTextListFormat::ListCircle;
                break;
            case 3:
                style = QTextListFormat::ListSquare;
                break;
            case 4:
                style = QTextListFormat::ListDecimal;
                break;
            case 5:
                style = QTextListFormat::ListLowerAlpha;
                break;
            case 6:
                style = QTextListFormat::ListUpperAlpha;
                break;
        }

        cursor.beginEditBlock();

        QTextBlockFormat blockFmt = cursor.blockFormat();

        QTextListFormat listFmt;

        if (cursor.currentList()) {
            listFmt = cursor.currentList()->format();
        } else {
            listFmt.setIndent(blockFmt.indent() + 1);
            blockFmt.setIndent(0);
            cursor.setBlockFormat(blockFmt);
        }

        listFmt.setStyle(style);

        cursor.createList(listFmt);

        cursor.endEditBlock();
    } else {
        // ####
        QTextBlockFormat bfmt;
        bfmt.setObjectIndex(-1);
        cursor.mergeBlockFormat(bfmt);
    }
}

void MessageComposer::textColor()
{
    QColor col = QColorDialog::getColor(ui.msgText->textColor(), this);
    if (!col.isValid())
        return;
    QTextCharFormat fmt;
    fmt.setForeground(col);
    mergeFormatOnWordOrSelection(fmt);
    colorChanged(col);
}

void MessageComposer::textAlign(QAction *a)
{
    if (a == actionAlignLeft)
        ui.msgText->setAlignment(Qt::AlignLeft);
    else if (a == actionAlignCenter)
        ui.msgText->setAlignment(Qt::AlignHCenter);
    else if (a == actionAlignRight)
        ui.msgText->setAlignment(Qt::AlignRight);
    else if (a == actionAlignJustify)
        ui.msgText->setAlignment(Qt::AlignJustify);
}

void MessageComposer::smileyWidget()
{
    Emoticons::showSmileyWidget(this, ui.emoticonButton, SLOT(addSmileys()), false);
}

void MessageComposer::addSmileys()
{
    ui.msgText->textCursor().insertText(qobject_cast<QPushButton*>(sender())->toolTip().split("|").first());
}

void MessageComposer::currentCharFormatChanged(const QTextCharFormat &format)
{
    fontChanged(format.font());
    colorChanged(format.foreground().color());
}

void MessageComposer::cursorPositionChanged()
{
    alignmentChanged(ui.msgText->alignment());
}

void MessageComposer::mergeFormatOnWordOrSelection(const QTextCharFormat &format)
{
    QTextCursor cursor = ui.msgText->textCursor();
    if (!cursor.hasSelection())
        cursor.select(QTextCursor::WordUnderCursor);
    cursor.mergeCharFormat(format);
    ui.msgText->mergeCurrentCharFormat(format);
}

void MessageComposer::fontChanged(const QFont &f)
{
    ui.comboFont->setCurrentIndex(ui.comboFont->findText(QFontInfo(f).family()));
    ui.comboSize->setCurrentIndex(ui.comboSize->findText(QString::number(f.pointSize())));
    ui.boldbtn->setChecked(f.bold());
    ui.italicbtn->setChecked(f.italic());
    ui.underlinebtn->setChecked(f.underline());
}

void MessageComposer::colorChanged(const QColor &c)
{
    QPixmap pix(16, 16);
    pix.fill(c);
    ui.colorbtn->setIcon(pix);
}

void MessageComposer::alignmentChanged(Qt::Alignment a)
{
    if (a & Qt::AlignLeft) {
        actionAlignLeft->setChecked(true);
    } else if (a & Qt::AlignHCenter) {
        actionAlignCenter->setChecked(true);
    } else if (a & Qt::AlignRight) {
        actionAlignRight->setChecked(true);
    } else if (a & Qt::AlignJustify) {
        actionAlignJustify->setChecked(true);
    }
}

void MessageComposer::clipboardDataChanged()
{
    actionPaste->setEnabled(!QApplication::clipboard()->text().isEmpty());
}

void MessageComposer::fileNew()
{
    if (maybeSave()) {
        ui.msgText->clear();
        //setCurrentFileName(QString());
    }
}

void MessageComposer::fileOpen()
{
    QString fn;
    if (misc::getOpenFileName(this, RshareSettings::LASTDIR_MESSAGES, tr("Open File..."), tr("HTML-Files (*.htm *.html);;All Files (*)"), fn)) {
        load(fn);
    }
}

bool MessageComposer::fileSave()
{
    if (fileName.isEmpty())
        return fileSaveAs();

    QFile file(fileName);
    if (!file.open(QFile::WriteOnly))
        return false;
    QTextStream ts(&file);
    ts.setCodec(QTextCodec::codecForName("UTF-8"));
    ts << ui.msgText->document()->toHtml("UTF-8");
    ui.msgText->document()->setModified(false);
    return true;
}

bool MessageComposer::fileSaveAs()
{
    QString fn;
    if (misc::getSaveFileName(this, RshareSettings::LASTDIR_MESSAGES, tr("Save as..."), tr("HTML-Files (*.htm *.html);;All Files (*)"), fn)) {
        setCurrentFileName(fn);
        return fileSave();
    }

    return false;
}

void MessageComposer::saveasDraft()
{
    sendMessage_internal(true);
}

void MessageComposer::filePrint()
{
#ifndef QT_NO_PRINTER
    QPrinter printer(QPrinter::HighResolution);
    printer.setFullPage(true);
    QPrintDialog *dlg = new QPrintDialog(&printer, this);
    if (ui.msgText->textCursor().hasSelection())
        dlg->addEnabledOption(QAbstractPrintDialog::PrintSelection);
    dlg->setWindowTitle(tr("Print Document"));
    if (dlg->exec() == QDialog::Accepted) {
        ui.msgText->print(&printer);
    }
    delete dlg;
#endif
}

void MessageComposer::filePrintPdf()
{
#ifndef QT_NO_PRINTER
    QString fileName;
    if (misc::getSaveFileName(this, RshareSettings::LASTDIR_MESSAGES, tr("Export PDF"), "*.pdf", fileName)) {
        if (QFileInfo(fileName).suffix().isEmpty())
            fileName.append(".pdf");
        QPrinter printer(QPrinter::HighResolution);
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setOutputFileName(fileName);
        ui.msgText->document()->print(&printer);
    }
#endif
}

void MessageComposer::setCurrentFileName(const QString &fileName)
{
    this->fileName = fileName;
    ui.msgText->document()->setModified(false);

    setWindowModified(false);
}

bool MessageComposer::load(const QString &f)
{
    if (!QFile::exists(f))
        return false;
    QFile file(f);
    if (!file.open(QFile::ReadOnly))
        return false;

    QByteArray data = file.readAll();
    QTextCodec *codec = Qt::codecForHtml(data);
    QString str = codec->toUnicode(data);
    if (Qt::mightBeRichText(str)) {
        ui.msgText->setHtml(str);
    } else {
        str = QString::fromLocal8Bit(data);
        ui.msgText->setPlainText(str);
    }

    setCurrentFileName(f);
    return true;
}

bool MessageComposer::maybeSave()
{
    if (!ui.msgText->document()->isModified())
        return true;
    if (fileName.startsWith(QLatin1String(":/")))
        return true;
    QMessageBox::StandardButton ret;
    ret = QMessageBox::warning(this, tr("Save Message"),
                               tr("Message has not been Sent.\n"
                                  "Do you want to save message ?"),
                               QMessageBox::Save | QMessageBox::Discard
                               | QMessageBox::Cancel);
    if (ret == QMessageBox::Save)
        return fileSave();
    else if (ret == QMessageBox::Cancel)
        return false;
    return true;
}


void MessageComposer::toggleContacts()
{
    ui.contactsdockWidget->setVisible(!ui.contactsdockWidget->isVisible());
}

void MessageComposer::on_contactsdockWidget_visibilityChanged(bool visible)
{
    contactSidebarAction->setChecked(visible);
}

void  MessageComposer::addImage()
{
	QString file;
	if (misc::getOpenFileName(this, RshareSettings::LASTDIR_IMAGES, tr("Choose Image"), tr("Image Files supported (*.png *.jpeg *.jpg *.gif)"), file)) {
		QString encodedImage;
		if (RsHtml::makeEmbeddedImage(file, encodedImage, 640*480)) {
			QTextDocumentFragment fragment = QTextDocumentFragment::fromHtml(encodedImage);
			ui.msgText->textCursor().insertFragment(fragment);
		}
    }
}

void MessageComposer::fontSizeIncrease()
{
    if ( !( ui.msgText->textCursor().blockFormat().hasProperty( TextFormat::HtmlHeading ) &&
        ui.msgText->textCursor().blockFormat().intProperty( TextFormat::HtmlHeading ) ) ) {
        QTextCharFormat format;
        int idx = ui.msgText->currentCharFormat().intProperty( QTextFormat::FontSizeAdjustment );
        if ( idx < 3 ) {
            format.setProperty( QTextFormat::FontSizeAdjustment, QVariant( ++idx ) );
            ui.msgText->textCursor().mergeCharFormat( format );
        }
    }
    ui.msgText->setFocus( Qt::OtherFocusReason );
}

void MessageComposer::fontSizeDecrease()
{
    if ( !( ui.msgText->textCursor().blockFormat().hasProperty( TextFormat::HtmlHeading ) &&
        ui.msgText->textCursor().blockFormat().intProperty( TextFormat::HtmlHeading ) ) ) {
        QTextCharFormat format;
        int idx = ui.msgText->currentCharFormat().intProperty( QTextFormat::FontSizeAdjustment );
        if ( idx > -1 ) {
            format.setProperty( QTextFormat::FontSizeAdjustment, QVariant( --idx ) );
            ui.msgText->textCursor().mergeCharFormat( format );
        }
    }
    ui.msgText->setFocus( Qt::OtherFocusReason );
}

void MessageComposer::blockQuote()
{
    QTextBlockFormat blockFormat = ui.msgText->textCursor().blockFormat();
    QTextBlockFormat f;

    if ( blockFormat.hasProperty( TextFormat::IsBlockQuote ) &&
         blockFormat.boolProperty( TextFormat::IsBlockQuote ) ) {
        f.setProperty( TextFormat::IsBlockQuote, QVariant( false ) );
        f.setLeftMargin( 0 );
        f.setRightMargin( 0 );
    } else {
        f.setProperty( TextFormat::IsBlockQuote, QVariant( true ) );
        f.setLeftMargin( 40 );
        f.setRightMargin( 40 );
    }
    ui.msgText->textCursor().mergeBlockFormat( f );
}

void MessageComposer::toggleCode()
{
    static QString preFontFamily;

    QTextCharFormat charFormat = ui.msgText->currentCharFormat();
    QTextCharFormat f;

    if ( charFormat.hasProperty( TextFormat::HasCodeStyle ) &&
         charFormat.boolProperty( TextFormat::HasCodeStyle ) ) {
        f.setProperty( TextFormat::HasCodeStyle, QVariant( false ) );
        f.setBackground( defaultCharFormat.background() );
        f.setFontFamily( preFontFamily );
        ui.msgText->textCursor().mergeCharFormat( f );

    } else {
        preFontFamily = ui.msgText->fontFamily();
        f.setProperty( TextFormat::HasCodeStyle, QVariant( true ) );
        f.setBackground( codeBackground );
        f.setFontFamily( "Dejavu Sans Mono" );
        ui.msgText->textCursor().mergeCharFormat( f );
    }
    ui.msgText->setFocus( Qt::OtherFocusReason );
}

void MessageComposer::addPostSplitter()
{
    QTextBlockFormat f = ui.msgText->textCursor().blockFormat();
    QTextBlockFormat f1 = f;

    f.setProperty( TextFormat::IsHtmlTagSign, true );
    f.setProperty( QTextFormat::BlockTrailingHorizontalRulerWidth,
             QTextLength( QTextLength::PercentageLength, 80 ) );
    if ( ui.msgText->textCursor().block().text().isEmpty() ) {
        ui.msgText->textCursor().mergeBlockFormat( f );
    } else {
        ui.msgText->textCursor().insertBlock( f );
    }
    ui.msgText->textCursor().insertBlock( f1 );
}

void MessageComposer::attachFile()
{
    // select a file
    QStringList files;
    if (misc::getOpenFileNames(this, RshareSettings::LASTDIR_EXTRAFILE, tr("Add Extra File"), "", files)) {
        ui.hashBox->addAttachments(files,TransferRequestFlags(0u));
    }
}

void MessageComposer::fileHashingStarted()
{
    std::cerr << "MessageComposer::fileHashingStarted() started." << std::endl;

    /* add widget in for new destination */
    ui.msgFileList->hide();
    ui.hashBox->show();
}

void MessageComposer::fileHashingFinished(QList<HashedFile> hashedFiles)
{
    std::cerr << "MessageComposer::fileHashingFinished() started." << std::endl;

    QList<HashedFile>::iterator it;
    for (it = hashedFiles.begin(); it != hashedFiles.end(); ++it) {
        FileInfo info;
        info.fname = it->filename.toUtf8().constData();
        info.hash = it->hash;
        info.size = it->size;
        addFile(info);
    }

    ui.actionSend->setEnabled(true);
    ui.hashBox->hide();
    ui.msgFileList->show();
}

void MessageComposer::addContact(enumType type)
{
    std::list<std::string> ids;
    ui.friendSelectionWidget->selectedGroupIds(ids);

    std::list<std::string>::iterator idIt;
    for (idIt = ids.begin(); idIt != ids.end(); idIt++) {
        addRecipient(type, *idIt, true);
    }

    ids.empty();
    ui.friendSelectionWidget->selectedSslIds(ids, true);
    for (idIt = ids.begin(); idIt != ids.end(); idIt++) {
        addRecipient(type, *idIt, false);
    }

    ids.empty();
    ui.friendSelectionWidget->selectedGpgIds(ids, true);
    for (idIt = ids.begin(); idIt != ids.end(); idIt++) 
	 {
		 std::string hash ;

		 if(rsMsgs->getDistantMessageHash(*idIt,hash))
			 addRecipient(type, hash, *idIt);
	 }
}

void MessageComposer::addTo()
{
    addContact(TO);
}

void MessageComposer::addCc()
{
    addContact(CC);
}

void MessageComposer::addBcc()
{
    addContact(BCC);
}

void MessageComposer::addRecommend()
{
    std::list<std::string> sslIds;
    ui.friendSelectionWidget->selectedSslIds(sslIds, false);

    if (sslIds.empty()) {
        return;
    }

    std::list <std::string>::iterator it;
    for (it = sslIds.begin(); it != sslIds.end(); it++) {
        addRecipient(CC, *it, false);
    }

    QString text = buildRecommendHtml(sslIds);
    ui.msgText->textCursor().insertHtml(text);
    ui.msgText->setFocus(Qt::OtherFocusReason);
}

void MessageComposer::friendDetails()
{
    FriendSelectionWidget::IdType idType;
    std::string id = ui.friendSelectionWidget->selectedId(idType);

    if (id.empty() || idType != FriendSelectionWidget::IDTYPE_SSL) {
        return;
    }

    ConfCertDialog::showIt(id, ConfCertDialog::PageDetails);
}

void MessageComposer::tagAboutToShow()
{
	TagsMenu *menu = dynamic_cast<TagsMenu*>(ui.tagButton->menu());
	if (menu == NULL) {
		return;
	}

	menu->activateActions(m_tagIds);
}

void MessageComposer::tagRemoveAll()
{
	m_tagIds.clear();

	showTagLabels();
}

void MessageComposer::tagSet(int tagId, bool set)
{
	if (tagId == 0) {
		return;
	}

	std::list<uint32_t>::iterator tag = std::find(m_tagIds.begin(), m_tagIds.end(), tagId);
	if (tag == m_tagIds.end()) {
		if (set) {
			m_tagIds.push_back(tagId);
			/* Keep the list sorted */
			m_tagIds.sort();
		}
	} else {
		if (set == false) {
			m_tagIds.remove(tagId);
		}
	}

	showTagLabels();
}

void MessageComposer::clearTagLabels()
{
	/* clear all tags */
	while (tagLabels.size()) {
		delete tagLabels.front();
		tagLabels.pop_front();
	}
	while (ui.tagLayout->count()) {
		delete ui.tagLayout->takeAt(0);
	}
}

void MessageComposer::showTagLabels()
{
	clearTagLabels();

	if (m_tagIds.empty() == false) {
		MsgTagType tags;
		rsMsgs->getMessageTagTypes(tags);

		std::map<uint32_t, std::pair<std::string, uint32_t> >::iterator tag;
		for (std::list<uint32_t>::iterator tagId = m_tagIds.begin(); tagId != m_tagIds.end(); tagId++) {
			tag = tags.types.find(*tagId);
			if (tag != tags.types.end()) {
				QLabel *tagLabel = new QLabel(TagDefs::name(tag->first, tag->second.first), this);
				tagLabel->setMaximumHeight(16);
				tagLabel->setStyleSheet(TagDefs::labelStyleSheet(tag->second.second));
				tagLabels.push_back(tagLabel);
				ui.tagLayout->addWidget(tagLabel);
				ui.tagLayout->addSpacing(3);
			}
		}
		ui.tagLayout->addStretch();
	}
}
