/*******************************************************************************
 * retroshare-gui/src/gui/msgs/MessageComposer.cpp                             *
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

#include "gui/common/FilesDefs.h"
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
#include <retroshare/rsidentity.h>
#include <retroshare/rsgxschannels.h>
#include <retroshare/rsgxsforums.h>

#include "gui/notifyqt.h"
#include "gui/common/RSTreeWidgetItem.h"
#include "gui/common/GroupDefs.h"
#include "gui/common/StatusDefs.h"
#include "gui/common/PeerDefs.h"
#include "gui/common/TagDefs.h"
#include "gui/common/Emoticons.h"
#include "gui/RetroShareLink.h"
#include "gui/settings/rsharesettings.h"
#include "gui/connect/ConfCertDialog.h"
#include "gui/Identity/IdDetailsDialog.h"
#include "gui/gxs/GxsIdDetails.h"
#include "util/misc.h"
#include "util/DateTime.h"
#include "util/HandleRichText.h"
#include "util/QtVersion.h"
#include "textformat.h"
#include "TagsMenu.h"

#define IMAGE_GROUP16    ":/images/user/group16.png"
#define IMAGE_FRIENDINFO ":/images/peerdetails_16x16.png"

#define COLUMN_CONTACT_NAME   0
#define COLUMN_CONTACT_DATA   0

#define ROLE_CONTACT_ID       Qt::UserRole
#define ROLE_CONTACT_SORT     Qt::UserRole + 1

#define COLUMN_RECIPIENT_TYPE  0
#define COLUMN_RECIPIENT_ICON  1
#define COLUMN_RECIPIENT_NAME  2
#define COLUMN_RECIPIENT_COUNT 3
#define COLUMN_RECIPIENT_DATA COLUMN_RECIPIENT_ICON // the column with a QTableWidgetItem

#define ROLE_RECIPIENT_ID     Qt::UserRole
#define ROLE_RECIPIENT_TYPE   Qt::UserRole + 1

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
MessageComposer::MessageComposer(QWidget *parent, Qt::WindowFlags flags)
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
    setupContactActions();

    m_compareRole = new RSTreeWidgetItemCompareRole;
    m_compareRole->setRole(COLUMN_CONTACT_NAME, ROLE_CONTACT_SORT);

    m_completer = NULL;
    
    ui.distantFrame->hide();
    ui.sizeLimitFrame->hide();
    ui.respond_to_CB->hide();
    ui.fromLabel->hide();

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
    connect(ui.color2btn, SIGNAL(clicked()), this, SLOT(textbackgroundColor()));

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
    
    connect(ui.filterComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(filterComboBoxChanged(int)));

    connect(ui.addToButton, SIGNAL(clicked(void)), this, SLOT(addTo()));
    connect(ui.addCcButton, SIGNAL(clicked(void)), this, SLOT(addCc()));
    connect(ui.addBccButton, SIGNAL(clicked(void)), this, SLOT(addBcc()));
    connect(ui.addRecommendButton, SIGNAL(clicked(void)), this, SLOT(addRecommend()));

    connect(NotifyQt::getInstance(), SIGNAL(peerStatusChanged(const QString&,int)), this, SLOT(peerStatusChanged(const QString&,int)));
    connect(ui.friendSelectionWidget, SIGNAL(contentChanged()), this, SLOT(buildCompleter()));
    connect(ui.friendSelectionWidget, SIGNAL(doubleClicked(int,QString)), this, SLOT(addTo()));
    connect(ui.friendSelectionWidget, SIGNAL(itemSelectionChanged()), this, SLOT(friendSelectionChanged()));

    /* hide the Tree +/- */
    ui.msgFileList -> setRootIsDecorated( false );

    /* initialize friends list */
    ui.friendSelectionWidget->setHeaderText(tr("Send To:"));
    ui.friendSelectionWidget->setModus(FriendSelectionWidget::MODUS_MULTI);
	ui.friendSelectionWidget->setShowType(FriendSelectionWidget::SHOW_GXS);
    ui.friendSelectionWidget->start();

    QActionGroup *grp = new QActionGroup(this);
    connect(grp, SIGNAL(triggered(QAction *)), this, SLOT(textAlign(QAction *)));

    actionAlignLeft = new QAction(QIcon(""), tr("&Left"), grp);
    actionAlignLeft->setShortcut(Qt::CTRL + Qt::Key_L);
    actionAlignLeft->setCheckable(true);
    actionAlignCenter = new QAction(QIcon(""), tr("C&enter"), grp);
    actionAlignCenter->setShortcut(Qt::CTRL + Qt::Key_E);
    actionAlignCenter->setCheckable(true);
    actionAlignRight = new QAction(QIcon(""), tr("&Right"), grp);
    actionAlignRight->setShortcut(Qt::CTRL + Qt::Key_R);
    actionAlignRight->setCheckable(true);
    actionAlignJustify = new QAction(QIcon(""), tr("&Justify"), grp);
    actionAlignJustify->setShortcut(Qt::CTRL + Qt::Key_J);
    actionAlignJustify->setCheckable(true);
    
    QActionGroup *grp2 = new QActionGroup(this);
    connect(grp2, SIGNAL(triggered(QAction *)), this, SLOT(textStyle(QAction *)));
    
    actionDisc = new QAction(QIcon(""), tr("Bullet list (disc)"), grp2);
    actionDisc->setCheckable(true);
    actionCircle = new QAction(QIcon(""), tr("Bullet list (circle)"), grp2);
    actionCircle->setCheckable(true);
    actionSquare = new QAction(QIcon(""), tr("Bullet list (square)"), grp2);
    actionSquare->setCheckable(true);
    actionDecimal= new QAction(QIcon(""), tr("Ordered list (decimal)"), grp2);
    actionDecimal->setCheckable(true);
    actionLowerAlpha = new QAction(QIcon(""), tr("Ordered list (alpha lower)"), grp2);
    actionLowerAlpha->setCheckable(true);
    actionUpperAlpha = new QAction(QIcon(""), tr("Ordered list (alpha upper)"), grp2);
    actionUpperAlpha->setCheckable(true);
    actionLowerRoman = new QAction(QIcon(""), tr("Ordered list (roman lower)"), grp2);
    actionLowerRoman->setCheckable(true);
    actionUpperRoman = new QAction(QIcon(""), tr("Ordered list (roman upper)"), grp2);
    actionUpperRoman->setCheckable(true);

    setupFormatActions();

    ui.respond_to_CB->setFlags(IDCHOOSER_ID_REQUIRED) ;
    
    /* Add filter types */
    ui.filterComboBox->addItem(tr("All people"));
    ui.filterComboBox->addItem(tr("My contacts"));
	ui.filterComboBox->setCurrentIndex(0);

    if(rsIdentity->nbRegularContacts() > 0)
    	ui.filterComboBox->setCurrentIndex(3);

    connect(ui.comboStyle, SIGNAL(activated(int)),this, SLOT(changeFormatType(int)));
    connect(ui.comboFont,  SIGNAL(activated(const QString &)), this, SLOT(textFamily(const QString &)));

    ui.comboSize->setEditable(true);

    QFontDatabase db;
    foreach(int size, db.standardSizes())
    ui.comboSize->addItem(QString::number(size));

    connect(ui.comboSize, SIGNAL(activated(const QString &)),this, SLOT(textSize(const QString &)));
    ui.comboSize->setCurrentIndex(ui.comboSize->findText(QString::number(QApplication::font().pointSize())));

    QMenu * alignmentmenu = new QMenu();
    alignmentmenu->addAction(actionAlignLeft);
    alignmentmenu->addAction(actionAlignCenter);
    alignmentmenu->addAction(actionAlignRight);
    alignmentmenu->addAction(actionAlignJustify);
    ui.textalignmentbtn->setMenu(alignmentmenu);
    
    QMenu * formatlistmenu = new QMenu();
    formatlistmenu->addAction(actionDisc);
    formatlistmenu->addAction(actionCircle);
    formatlistmenu->addAction(actionSquare);
    formatlistmenu->addAction(actionDecimal);
    formatlistmenu->addAction(actionLowerAlpha);
    formatlistmenu->addAction(actionUpperAlpha);
    formatlistmenu->addAction(actionLowerRoman);
    formatlistmenu->addAction(actionUpperRoman);
    ui.styleButton->setMenu(formatlistmenu);

    QPixmap pxm(24,24);
    pxm.fill(Qt::black);
    ui.colorbtn->setIcon(pxm);
    
    QPixmap pxm2(24,24);
    pxm2.fill(Qt::white);
    ui.color2btn->setIcon(pxm2);

    /* Set header resize modes and initial section sizes */
    QHeaderView * _smheader = ui.msgFileList->header () ;

    _smheader->resizeSection(COLUMN_FILE_NAME, 200);
    _smheader->resizeSection(COLUMN_FILE_SIZE, 60);
    _smheader->resizeSection(COLUMN_FILE_HASH, 220);

    QPalette palette = QApplication::palette();
    codeBackground = palette.color( QPalette::Active, QPalette::Midlight );

    ui.recipientWidget->setColumnCount(COLUMN_RECIPIENT_COUNT);

    QHeaderView *header = ui.recipientWidget->horizontalHeader();
//    header->resizeSection(COLUMN_RECIPIENT_TYPE, ui.fromLabel->size().width()); // see ::eventFilter
    header->resizeSection(COLUMN_RECIPIENT_ICON, 22);
    QHeaderView_setSectionResizeModeColumn(header, COLUMN_RECIPIENT_TYPE, QHeaderView::Fixed);
    QHeaderView_setSectionResizeModeColumn(header, COLUMN_RECIPIENT_ICON, QHeaderView::Fixed);
    QHeaderView_setSectionResizeModeColumn(header, COLUMN_RECIPIENT_NAME, QHeaderView::Fixed);
    header->setStretchLastSection(true);
    ui.fromLabel->installEventFilter(this);

    /* Set own item delegate */
    QItemDelegate *delegate = new MessageItemDelegate(this);
    ui.recipientWidget->setItemDelegateForColumn(COLUMN_RECIPIENT_ICON, delegate);

    connect(ui.recipientWidget,SIGNAL(cellChanged(int,int)),this,SLOT(updateCells(int,int))) ;

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
}

MessageComposer::~MessageComposer()
{
    delete(m_compareRole);
}

void MessageComposer::updateCells(int,int)
{
    int rowCount = ui.recipientWidget->rowCount();
    int row;
    bool has_gxs = false ;

    for (row = 0; row < rowCount; ++row)
    {
        enumType type;
        destinationType dtype ;
        std::string id;

        if (getRecipientFromRow(row, type,dtype, id) && !id.empty() )
            if(dtype == PEER_TYPE_GXS)
                has_gxs = true ;
    }
    if(has_gxs)
    {
        ui.respond_to_CB->show();
        ui.distantFrame->show();
        ui.fromLabel->show();
    }
    else
    {
        ui.respond_to_CB->hide();
        ui.distantFrame->hide() ;
        ui.fromLabel->hide();
    }

    if(rowCount > 20)
        ui.sizeLimitFrame->show();
    else
        ui.sizeLimitFrame->hide();
}

void MessageComposer::processSettings(bool bLoad)
{
    Settings->beginGroup(QString("MessageComposer"));

    if (bLoad) {
        // load settings

        // state of contact sidebar
        ui.contactsdockWidget->setVisible(Settings->value("ContactSidebar", true).toBool());

        // state of splitter
        ui.messageSplitter->restoreState(Settings->value("Splitter").toByteArray());
        ui.centralwidgetHSplitter->restoreState(Settings->value("Splitter2").toByteArray());
        
        // state of filter combobox
        int index = Settings->value("ShowType", 0).toInt();
        ui.filterComboBox->setCurrentIndex(index);
        
        RsGxsId resp_to_id ( Settings->value("LastRespondTo").toString().toStdString());
        
       	if(!resp_to_id.isNull())
		ui.respond_to_CB->setDefaultId(resp_to_id);
    } else {
        // save settings

        // state of contact sidebar
        Settings->setValue("ContactSidebar", ui.contactsdockWidget->isVisible());

        // state of splitter
        Settings->setValue("Splitter", ui.messageSplitter->saveState());
        Settings->setValue("Splitter2", ui.centralwidgetHSplitter->saveState());
        
        // state of filter combobox
        Settings->setValue("ShowType", ui.filterComboBox->currentIndex());
 
        Settings->setValue("LastRespondTo",ui.respond_to_CB->itemData(ui.respond_to_CB->currentIndex()).toString());
    }

    Settings->endGroup();
}

/*static*/ void MessageComposer::msgGxsIdentity(const RsGxsId &id)
{
//    std::cerr << "MessageComposer::msgfriend()" << std::endl;

    /* create a message */

    MessageComposer *pMsgDialog = MessageComposer::newMsg();

    if (pMsgDialog == NULL)
        return;

    pMsgDialog->addRecipient(TO, id);
    pMsgDialog->show();

    /* window will destroy itself! */
}

/*static*/ void MessageComposer::msgFriend(const RsPeerId &id)
{
    //    std::cerr << "MessageComposer::msgfriend()" << std::endl;

    /* create a message */

    MessageComposer *pMsgDialog = MessageComposer::newMsg();
    if (pMsgDialog == NULL) {
        return;
    }

    RsPeerDetails detail;
    if (rsPeers->getPeerDetails(id, detail) && detail.accept_connection)
    {
        pMsgDialog->addRecipient(TO, id);
    }

    pMsgDialog->show();

    /* window will destroy itself! */
}

/*static*/ void MessageComposer::msgFriend(const RsPgpId &id)
{
    //    std::cerr << "MessageComposer::msgfriend()" << std::endl;

    /* check if pgp id is a friend */
    std::list<RsPgpId> friends;
    rsPeers->getGPGAcceptedList(friends);
    if(std::find(friends.begin(), friends.end(), id) == friends.end())
        return;

    /* create a message */
    MessageComposer *pMsgDialog = MessageComposer::newMsg();
    if (pMsgDialog == NULL) {
        return;
    }

    /* add all locations */
    std::list<RsPeerId> locations;
    rsPeers->getAssociatedSSLIds(id, locations);
    for(std::list<RsPeerId>::iterator it = locations.begin(); it != locations.end(); ++it)
    {
        pMsgDialog->addRecipient(TO, *it);
    }

    pMsgDialog->show();

    /* window will destroy itself! */
}

static QString buildRecommendHtml(const std::set<RsPeerId> &sslIds, const RsPeerId& excludeId = RsPeerId())
{
    QString text;

    /* process ssl ids */
    std::set <RsPeerId>::const_iterator sslIt;
    for (sslIt = sslIds.begin(); sslIt != sslIds.end(); ++sslIt) {
        if (*sslIt == excludeId) {
            continue;
        }
        RetroShareLink link = RetroShareLink::createCertificate(*sslIt);
        if (link.valid()) {
            text += link.toHtml() + "<br>";
        }
    }

    return text;
}

QString MessageComposer::recommendMessage()
{
    return tr("Hello,<br>I recommend a good friend of mine; you can trust them too when you trust me. <br>");
}

void MessageComposer::recommendFriend(const std::set <RsPeerId> &sslIds, const RsPeerId &to, const QString &msg, bool autoSend)
{
//    std::cerr << "MessageComposer::recommendFriend()" << std::endl;

    if (sslIds.empty()) {
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

    if (!to.isNull()) {
        composer->addRecipient(TO, to);
    }
    RsPgpId ownPgpId = rsPeers->getGPGOwnId();
    RetroShareLink link = RetroShareLink::createPerson(ownPgpId);
    
    QString sMsgText = msg.isEmpty() ? recommendMessage() : msg;
    sMsgText += "<br><br>";
    sMsgText += recommendHtml;
    sMsgText += "<br>";
    sMsgText += tr("This friend is suggested by") + " " + link.toHtml() + "<br><br>" ;      
    sMsgText += tr("Thanks, <br>") + QString::fromUtf8(rsPeers->getGPGName(rsPeers->getGPGOwnId()).c_str());
    composer->setMsgText(sMsgText);

    std::set <RsPeerId>::const_iterator peerIt;
    for (peerIt = sslIds.begin(); peerIt != sslIds.end(); ++peerIt) {
        if (*peerIt == to) {
            continue;
        }
        composer->addRecipient(CC, *peerIt);
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

void MessageComposer::addConnectAttemptMsg(const RsPgpId &gpgId, const RsPeerId &sslId, const QString &/*sslName*/)
{
    if (gpgId.isNull())
        return;

    // PGPId+SslId are always here.  But if the peer is not a friend the SSL id cannot be used.
    // (todo) If the PGP id doesn't get us a PGP key from the keyring, we need to create a short invite

	RetroShareLink link = RetroShareLink::createUnknownSslCertificate(sslId);

    if (!link.valid())
        return;

    QString title = QString("%1 %2").arg(link.name(), tr("wants to be friends with you on RetroShare"));

    /* search for an exisiting message in the inbox */
    std::list<MsgInfoSummary> msgList;
    std::list<MsgInfoSummary>::const_iterator it;

    rsMail->getMessageSummaries(msgList);
    for(it = msgList.begin(); it != msgList.end(); ++it) {
        if (it->msgflags & RS_MSG_TRASH) {
            continue;
        }
        if ((it->msgflags & RS_MSG_BOXMASK) != RS_MSG_INBOX) {
            continue;
        }
        if ((it->msgflags & RS_MSG_USER_REQUEST) == 0) {
            continue;
        }
        if (it->title == title.toUtf8().constData()) {
            return;
        }
    }

    /* create a message */
    QString msgText = tr("Hi %1,<br><br>%2 wants to be friends with you on RetroShare.<br><br>Respond now:<br>%3<br><br>Thanks,<br>The RetroShare Team").arg(QString::fromUtf8(rsPeers->getGPGName(rsPeers->getGPGOwnId()).c_str()), link.name(), link.toHtml());
    rsMail->SystemMessage(title.toUtf8().constData(), msgText.toUtf8().constData(), RS_MSG_USER_REQUEST);
}

#ifdef UNUSED_CODE
void MessageComposer::sendChannelPublishKey(RsGxsChannelGroup &group)
{
//    QString channelName = QString::fromUtf8(group.mMeta.mGroupName.c_str());

//    RetroShareLink link;
//    if (!link.createGxsGroupLink(RetroShareLink::TYPE_CHANNEL, group.mMeta.mGroupId, channelName)) {
//        return;
//    }

//    QString title = tr("Publish key for channel %1").arg(channelName);

//    /* create a message */
//    QString msgText = tr("... %1 ...<br>%2").arg(channelName, link.toHtml());
//    rsMail->SystemMessage(title.toUtf8().constData(), msgText.toUtf8().constData(), RS_MSG_PUBLISH_KEY);
}

void MessageComposer::sendForumPublishKey(RsGxsForumGroup &group)
{
//    QString forumName = QString::fromUtf8(group.mMeta.mGroupName.c_str());

//    RetroShareLink link;
//    if (!link.createGxsGroupLink(RetroShareLink::TYPE_FORUM, group.mMeta.mGroupId, forumName)) {
//        return;
//    }

//    QString title = tr("Publish key for forum %1").arg(forumName);

//    /* create a message */
//    QString msgText = tr("... %1 ...<br>%2").arg(forumName, link.toHtml());
//    rsMail->SystemMessage(title.toUtf8().constData(), msgText.toUtf8().constData(), RS_MSG_PUBLISH_KEY);
}
#endif

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

void MessageComposer::contextMenuFileList(QPoint)
{
    QMenu contextMnu(this);

    QAction *action = contextMnu.addAction(FilesDefs::getIconFromQtResourcePath(":/images/pasterslink.png"), tr("Paste RetroShare Link"), this, SLOT(pasteRecommended()));
    action->setDisabled(RSLinkClipboard::empty(RetroShareLink::TYPE_FILE));

    contextMnu.exec(QCursor::pos());
}

void MessageComposer::pasteRecommended()
{
    QList<RetroShareLink> links;
    RSLinkClipboard::pasteLinks(links);

    for (int i = 0; i < links.size(); ++i) {
        if (links[i].valid() && links[i].type() == RetroShareLink::TYPE_FILE) {
            FileInfo fileInfo;
            fileInfo.fname = links[i].name().toStdString();
            fileInfo.hash = RsFileHash(links[i].hash().toStdString());
            fileInfo.size = links[i].size();

            addFile(fileInfo);
        }
    }
}

static void setNewCompleter(QTableWidget *tableWidget, QCompleter *completer)
{
    int rowCount = tableWidget->rowCount();
    int row;

    for (row = 0; row < rowCount; ++row) {
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
    
    std::list<RsPeerId> peers;
    std::list<RsPeerId>::iterator peerIt;
    rsPeers->getFriendList(peers);
    
    std::list<RsGxsId> gxsIds;
    QList<QTreeWidgetItem*> gxsitems ;

    ui.friendSelectionWidget->items(gxsitems,FriendSelectionWidget::IDTYPE_GXS) ;

    // create completer list for friends
    QStringList completerList;
    QStringList completerGroupList;
    
    for (QList<QTreeWidgetItem*>::const_iterator idIt = gxsitems.begin(); idIt != gxsitems.end(); ++idIt)
    {
        RsGxsId id ( ui.friendSelectionWidget->idFromItem( *idIt ) );
        RsIdentityDetails detail;

        if(rsIdentity->getIdDetails(id, detail))
            completerList.append( getRecipientEmailAddress(id,detail)) ;
    }

    for (peerIt = peers.begin(); peerIt != peers.end(); ++peerIt)
    {
        RsPeerDetails detail;

        if (rsPeers->getPeerDetails(*peerIt, detail))
        completerList.append( getRecipientEmailAddress(*peerIt,detail)) ;

//        QString name = PeerDefs::nameWithLocation(detail);
//        if (completerList.indexOf(name) == -1) {
//            completerList.append(name);
//        }
    }

    completerList.sort();

    // create completer list for groups
    for (groupIt = groupInfoList.begin(); groupIt != groupInfoList.end(); ++groupIt) {
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

    for (row = 0; row < rowCount; ++row) {
        enumType type;
        destinationType dtype ;
        std::string id;

        if (getRecipientFromRow(row, type,dtype, id) && !id.empty() ) {
            if (dtype == PEER_TYPE_SSL && QString::fromStdString(id) == peer_id)
            {
                QTableWidgetItem *item = ui.recipientWidget->item(row, COLUMN_RECIPIENT_ICON);
                if (item)
                    item->setIcon(FilesDefs::getIconFromQtResourcePath(StatusDefs::imageUser(status)));
            }
        }
    }


}

void MessageComposer::setFileList(const std::list<DirDetails>& dir_info)
{
    std::list<FileInfo> files_info;
    std::list<DirDetails>::const_iterator it;

    /* convert dir_info to files_info */
    for(it = dir_info.begin(); it != dir_info.end(); ++it)
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
    for(it = files_info.begin(); it != files_info.end(); ++it) {
        addFile(*it);
    }

    ui.msgFileList->update(); /* update display */
}

void MessageComposer::addFile(const FileInfo &fileInfo)
{
    for(std::list<FileInfo>::iterator it = _recList.begin(); it != _recList.end(); ++it) {
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
    item->setTextAlignment(COLUMN_FILE_SIZE, Qt::AlignRight);
    item->setText(COLUMN_FILE_HASH, QString::fromStdString(fileInfo.hash.toStdString()));
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

/*static void calculateGroupsOfSslIds(const std::list<RsGroupInfo> &existingGroupInfos, std::list<RsPeerId> &checkSslIds, std::list<RsNodeGroupId> &checkGroupIds)
{
    checkGroupIds.clear();

    if (checkSslIds.empty()) {
        // nothing to do
        return;
    }

    std::map<RsPeerId, RsPgpId> sslToGpg;
    std::map<RsPgpId, std::list<RsPeerId> > gpgToSslIds;

    std::list<RsGroupInfo> groupInfos;

    // iterate all groups
    std::list<RsGroupInfo>::const_iterator groupInfoIt;
    for (groupInfoIt = existingGroupInfos.begin(); groupInfoIt != existingGroupInfos.end(); ++groupInfoIt) {
        if (groupInfoIt->peerIds.empty()) {
            continue;
        }

        // iterate all assigned peers (gpg id's)
        std::set<RsPgpId>::const_iterator peerIt;
        for (peerIt = groupInfoIt->peerIds.begin(); peerIt != groupInfoIt->peerIds.end(); ++peerIt) {
            std::list<RsPeerId> sslIds;

            std::map<RsPgpId, std::list<RsPeerId> >::const_iterator it = gpgToSslIds.find(*peerIt);
            if (it == gpgToSslIds.end()) {
                rsPeers->getAssociatedSSLIds(*peerIt, sslIds);

                gpgToSslIds[*peerIt] = sslIds;
            } else {
                sslIds = it->second;
            }

            // iterate all ssl id's
            std::list<RsPeerId>::const_iterator sslIt;
            for (sslIt = sslIds.begin(); sslIt != sslIds.end(); ++sslIt) {
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
    for (groupInfoIt = groupInfos.begin(); groupInfoIt != groupInfos.end(); ++groupInfoIt) {
        // iterate all assigned peers (gpg id's)
        std::set<RsPgpId>::const_iterator peerIt;
        for (peerIt = groupInfoIt->peerIds.begin(); peerIt != groupInfoIt->peerIds.end(); ++peerIt) {
            std::list<RsPeerId> sslIds;

            std::map<RsPgpId, std::list<RsPeerId> >::iterator it = gpgToSslIds.find(*peerIt);
            if (it == gpgToSslIds.end()) {
                rsPeers->getAssociatedSSLIds(*peerIt, sslIds);

                gpgToSslIds[*peerIt] = sslIds;
            } else {
                sslIds = it->second;
            }

            // iterate all ssl id's
            std::list<RsPeerId>::const_iterator sslIt;
            for (sslIt = sslIds.begin(); sslIt != sslIds.end(); ++sslIt) {
                // search in ssl list
                std::list<RsPeerId>::iterator it = std::find(checkSslIds.begin(), checkSslIds.end(), *sslIt);
                if (it != checkSslIds.end()) {
                    checkSslIds.erase(it);
                }
            }
        }

        checkGroupIds.push_back(groupInfoIt->id);
    }
}
*/

MessageComposer *MessageComposer::newMsg(const std::string &msgId /* = ""*/)
{
    MessageComposer *msgComposer = new MessageComposer();

    msgComposer->addEmptyRecipient();

    if (!msgId.empty())
    {
        // fill existing message
        MessageInfo msgInfo;
        if (!rsMail->getMessage(msgId, msgInfo)) {
            std::cerr << "MessageComposer::newMsg() Couldn't find Msg" << std::endl;
            delete msgComposer;
            return NULL;
        }

        if (msgInfo.msgflags & RS_MSG_DRAFT) {
            msgComposer->m_sDraftMsgId = msgId;

            rsMail->getMsgParentId(msgId,  msgComposer->m_msgParentId);

            if (msgInfo.msgflags & RS_MSG_REPLIED) {
                msgComposer->m_msgType = REPLY;
            } else if (msgInfo.msgflags & RS_MSG_FORWARDED) {
                msgComposer->m_msgType = FORWARD;
            }
        }

        // needed to send system flags with reply
        msgComposer->msgFlags = (msgInfo.msgflags & RS_MSG_SYSTEM);

        msgComposer->setTitleText(QString::fromUtf8(msgInfo.title.c_str()));
        msgComposer->setMsgText(QString::fromUtf8(msgInfo.msg.c_str()));
        msgComposer->setFileList(msgInfo.files);

        // get existing groups
        std::list<RsGroupInfo> groupInfoList;
        rsPeers->getGroupInfoList(groupInfoList);

        std::list<std::string> groupIds;
        std::list<std::string>::iterator groupIt;

    //       calculateGroupsOfSslIds(groupInfoList, msgInfo.msgto, groupIds);
    //       for (groupIt = groupIds.begin(); groupIt != groupIds.end(); ++groupIt ) {
    //           msgComposer->addRecipient(MessageComposer::TO, *groupIt, true) ;
    //       }

    //     calculateGroupsOfSslIds(groupInfoList, msgInfo.msgcc, groupIds);
    //     for (groupIt = groupIds.begin(); groupIt != groupIds.end(); ++groupIt ) {
    //         msgComposer->addRecipient(MessageComposer::CC, *groupIt, true) ;
    //     }

    //        calculateGroupsOfSslIds(groupInfoList, msgInfo.msgbcc, groupIds);
    //        for (groupIt = groupIds.begin(); groupIt != groupIds.end(); ++groupIt ) {
    //            msgComposer->addRecipient(MessageComposer::BCC, *groupIt, true) ;
    //        }

        for (std::set<RsPeerId>::const_iterator it = msgInfo.rspeerid_msgto.begin();  it != msgInfo.rspeerid_msgto.end(); ++it )  msgComposer->addRecipient(MessageComposer::TO, *it) ;
        for (std::set<RsPeerId>::const_iterator it = msgInfo.rspeerid_msgcc.begin();  it != msgInfo.rspeerid_msgcc.end(); ++it )  msgComposer->addRecipient(MessageComposer::CC, *it) ;
        for (std::set<RsPeerId>::const_iterator it = msgInfo.rspeerid_msgbcc.begin(); it != msgInfo.rspeerid_msgbcc.end(); ++it )  msgComposer->addRecipient(MessageComposer::BCC, *it) ;
        for (std::set<RsGxsId>::const_iterator it = msgInfo.rsgxsid_msgto.begin();   it != msgInfo.rsgxsid_msgto.end(); ++it )  msgComposer->addRecipient(MessageComposer::TO, *it) ;
        for (std::set<RsGxsId>::const_iterator it = msgInfo.rsgxsid_msgcc.begin();   it != msgInfo.rsgxsid_msgcc.end(); ++it )  msgComposer->addRecipient(MessageComposer::CC, *it) ;
        for (std::set<RsGxsId>::const_iterator it = msgInfo.rsgxsid_msgbcc.begin();  it != msgInfo.rsgxsid_msgbcc.end(); ++it )  msgComposer->addRecipient(MessageComposer::BCC, *it) ;

        MsgTagInfo tagInfo;
        rsMail->getMessageTag(msgId, tagInfo);

        msgComposer->m_tagIds = tagInfo.tagIds;
        msgComposer->showTagLabels();
        msgComposer->ui.msgText->document()->setModified(false);
    }

    msgComposer->calculateTitle();

    return msgComposer;
}

QString MessageComposer::buildReplyHeader(const MessageInfo &msgInfo)
{
    RetroShareLink link = RetroShareLink::createMessage(msgInfo.rspeerid_srcId, "");
    QString from = link.toHtml();

    QString to;
    for ( std::set<RsPeerId>::const_iterator  it = msgInfo.rspeerid_msgto.begin(); it != msgInfo.rspeerid_msgto.end(); ++it)
    {
        link = RetroShareLink::createMessage(*it, "");
        if (link.valid())
        {
            if (!to.isEmpty())
                to += ", ";

            to += link.toHtml();
        }
    }
    for ( std::set<RsGxsId>::const_iterator  it = msgInfo.rsgxsid_msgto.begin(); it != msgInfo.rsgxsid_msgto.end(); ++it)
    {
        link = RetroShareLink::createMessage(*it, "");
        if (link.valid())
        {
            if (!to.isEmpty())
                to += ", ";

            to += link.toHtml();
        }
    }

    QString cc;
    for (std::set<RsPeerId>::const_iterator it = msgInfo.rspeerid_msgcc.begin(); it != msgInfo.rspeerid_msgcc.end(); ++it) {
        link = RetroShareLink::createMessage(*it, "");
        if (link.valid()) {
            if (!cc.isEmpty()) {
                cc += ", ";
            }
            cc += link.toHtml();
        }
    }
    for (std::set<RsGxsId>::const_iterator it = msgInfo.rsgxsid_msgcc.begin(); it != msgInfo.rsgxsid_msgcc.end(); ++it) {
        link = RetroShareLink::createMessage(*it, "");
        if (link.valid()) {
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
    header += QString("<font size='3'><strong>%1: </strong>%2</font></span><br>").arg(tr("Subject"), QString::fromUtf8(msgInfo.title.c_str()));
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
    if (!rsMail->getMessage(msgId, msgInfo)) {
        return NULL;
    }

    MessageComposer *msgComposer = MessageComposer::newMsg();
    msgComposer->m_msgParentId = msgId;
    msgComposer->m_msgType = REPLY;

    /* fill it in */

    msgComposer->setTitleText(QString::fromUtf8(msgInfo.title.c_str()), REPLY);
    msgComposer->setQuotedMsg(QString::fromUtf8(msgInfo.msg.c_str()), buildReplyHeader(msgInfo));

    if(!msgInfo.rspeerid_srcId.isNull()) msgComposer->addRecipient(MessageComposer::TO, msgInfo.rspeerid_srcId);
    if(!msgInfo.rsgxsid_srcId.isNull()) msgComposer->addRecipient(MessageComposer::TO, msgInfo.rsgxsid_srcId);

    // make sure the current ID is among the ones the msg was actually sent to.
    for(auto it(msgInfo.rsgxsid_msgto.begin());it!=msgInfo.rsgxsid_msgto.end();++it)
        if(rsIdentity->isOwnId(*it))
        {
            msgComposer->ui.respond_to_CB->setDefaultId(*it) ;
            break ;
        }
    // Note: another solution is to do
    //		msgComposer->ui.respond_to_CB->setIdConstraintSet(msgInfo.rsgxsid_msgto);	// always choose one of the destinations to originate the response!
    // but that prevent any use of IDs tht are not in the destination set to be chosen to author the msg.

    if (all)
    {
        RsPeerId ownId = rsPeers->getOwnId();

        for (std::set<RsPeerId>::iterator tli = msgInfo.rspeerid_msgto.begin(); tli != msgInfo.rspeerid_msgto.end(); ++tli)
            if (ownId != *tli)
                msgComposer->addRecipient(MessageComposer::TO, *tli) ;

        for (std::set<RsPeerId>::iterator tli = msgInfo.rspeerid_msgcc.begin(); tli != msgInfo.rspeerid_msgcc.end(); ++tli)
            if (ownId != *tli)
                msgComposer->addRecipient(MessageComposer::TO, *tli) ;

        for (std::set<RsGxsId>::iterator tli = msgInfo.rsgxsid_msgto.begin(); tli != msgInfo.rsgxsid_msgto.end(); ++tli)
            //if (ownId != *tli)
                msgComposer->addRecipient(MessageComposer::TO, *tli) ;

        for (std::set<RsGxsId>::iterator tli = msgInfo.rsgxsid_msgcc.begin(); tli != msgInfo.rsgxsid_msgcc.end(); ++tli)
            //if (ownId != *tli)
                msgComposer->addRecipient(MessageComposer::TO, *tli) ;
    }

    // needed to send system flags with reply
    msgComposer->msgFlags = (msgInfo.msgflags & RS_MSG_SYSTEM);

	MsgTagInfo tagInfo;
	rsMail->getMessageTag(msgId, tagInfo);

	msgComposer->m_tagIds = tagInfo.tagIds;
	msgComposer->showTagLabels();

    msgComposer->calculateTitle();

    /* window will destroy itself! */

    return msgComposer;
}

MessageComposer *MessageComposer::forwardMsg(const std::string &msgId)
{
    MessageInfo msgInfo;
    if (!rsMail->getMessage(msgId, msgInfo)) {
        return NULL;
    }

    MessageComposer *msgComposer = MessageComposer::newMsg();
    msgComposer->m_msgParentId = msgId;
    msgComposer->m_msgType = FORWARD;

    /* fill it in */

    msgComposer->setTitleText(QString::fromUtf8(msgInfo.title.c_str()), FORWARD);
    msgComposer->setQuotedMsg(QString::fromUtf8(msgInfo.msg.c_str()), buildReplyHeader(msgInfo));

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

	 // add a GXS signer/from in case the message is to be sent to a distant peer

	 mi.rsgxsid_srcId = RsGxsId(ui.respond_to_CB->itemData(ui.respond_to_CB->currentIndex()).toString().toStdString()) ;

	 //std::cerr << "MessageSend: setting 'from' field to GXS id = " << mi.rsgxsid_srcId << std::endl;

    mi.title = misc::removeNewLine(ui.titleEdit->text()).toUtf8().constData();
    // needed to send system flags with reply
    mi.msgflags = msgFlags;

    QString text;
    RsHtml::optimizeHtml(ui.msgText, text);
    mi.msg = text.toUtf8().constData();

    /* check for existing title */
    if (bDraftbox == false && mi.title.empty()) {
        if (QMessageBox::warning(this, tr("RetroShare"), tr("Do you want to send the message without a subject ?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::No) {
            ui.titleEdit->setFocus();
            return false; // Don't send with an empty subject
        }
    }

    int filesCount = ui.msgFileList->topLevelItemCount();
    for (int i = 0; i < filesCount; ++i) {
        QTreeWidgetItem *item = ui.msgFileList->topLevelItem(i);
        if (item->checkState(COLUMN_FILE_CHECKED)) {
            RsFileHash hash ( item->text(COLUMN_FILE_HASH).toStdString() );
            for(std::list<FileInfo>::iterator it = _recList.begin(); it != _recList.end(); ++it) {
                if (it->hash == hash) {
                    mi.files.push_back(*it);
                    break;
                }
            }
        }
    }

    /* get the ids from the send list */
    std::list<RsPeerId> peers;
    rsPeers->getFriendList(peers);

    /* add own id */
    peers.push_back(rsPeers->getOwnId());

    int rowCount = ui.recipientWidget->rowCount();
    int row;

    for (row = 0; row < rowCount; ++row)
    {
        enumType type;
        destinationType dtype ;
        std::string id;

        if (!getRecipientFromRow(row, type,dtype, id) || id.empty())
            continue ;

        switch(dtype)
        {
        case PEER_TYPE_GROUP: {
            RsGroupInfo groupInfo;
            if (rsPeers->getGroupInfo(RsNodeGroupId(id), groupInfo) == false) {
                // group not found
                continue;
            }

            std::set<RsPgpId>::const_iterator groupIt;
            for (groupIt = groupInfo.peerIds.begin(); groupIt != groupInfo.peerIds.end(); ++groupIt) {
                std::list<RsPeerId> sslIds;
                rsPeers->getAssociatedSSLIds(*groupIt, sslIds);

                std::list<RsPeerId>::const_iterator sslIt;
                for (sslIt = sslIds.begin(); sslIt != sslIds.end(); ++sslIt) {
                    if (std::find(peers.begin(), peers.end(), *sslIt) == peers.end()) {
                        // no friend
                        continue;
                    }

                    switch (type)
                    {
                    case TO: mi.rspeerid_msgto.insert(*sslIt);
                        break;
                    case CC: mi.rspeerid_msgcc.insert(*sslIt);
                        break;
                    case BCC:mi.rspeerid_msgbcc.insert(*sslIt);
                        break;
                    }
                }
            }
        }
            break ;
        case PEER_TYPE_SSL:
        {
            RsPeerId pid(id) ;

            switch (type)
            {
            case TO: mi.rspeerid_msgto.insert(pid);
            break ;
            case CC: mi.rspeerid_msgcc.insert(pid);
            break ;
            case BCC:mi.rspeerid_msgbcc.insert(pid);
            break ;
            }
        }
            break ;
        case PEER_TYPE_GXS:
        {
            RsGxsId gid(id) ;

            switch (type)
            {
            case TO: mi.rsgxsid_msgto.insert(gid) ;
            break ;
            case CC: mi.rsgxsid_msgcc.insert(gid) ;
            break ;
            case BCC:mi.rsgxsid_msgbcc.insert(gid) ;
            break ;
            }
        }
            break ;

        default:
            std::cerr << __PRETTY_FUNCTION__ << ": Unhandled destination type " << dtype << std::endl;
            break ;
        }
    }

    if (bDraftbox)
    {
        mi.msgId = m_sDraftMsgId;

        rsMail->MessageToDraft(mi, m_msgParentId);

        // use new message id
        m_sDraftMsgId = mi.msgId;

        switch (m_msgType) {
        case NORMAL:
            break;
        case REPLY:
            rsMail->MessageReplied(m_sDraftMsgId, true);
            break;
        case FORWARD:
            rsMail->MessageForwarded(m_sDraftMsgId, true);
            break;
        }
    }
    else
    {
        /* check for the recipient */
        if (mi.rspeerid_msgto.empty() && mi.rspeerid_msgcc.empty() && mi.rspeerid_msgbcc.empty()
                        && mi.rsgxsid_msgto.empty() && mi.rsgxsid_msgcc.empty() && mi.rsgxsid_msgbcc.empty())
        {
            QMessageBox::warning(this, tr("RetroShare"), tr("Please insert at least one recipient."), QMessageBox::Ok);
            return false; // Don't send with no recipient
        }

     if(mi.rsgxsid_srcId.isNull() && !(mi.rsgxsid_msgto.empty() && mi.rsgxsid_msgcc.empty() && mi.rsgxsid_msgbcc.empty()))
     {
            QMessageBox::warning(this, tr("RetroShare"), tr("Please create an identity to sign distant messages, or remove the distant peers from the destination list."), QMessageBox::Ok);
            return false; // Don't send if cannot sign.
     }
        if (rsMail->MessageSend(mi) == false) {
            return false;
        }

        if (m_msgParentId.empty() == false) {
            switch (m_msgType) {
            case NORMAL:
                break;
            case REPLY:
                rsMail->MessageReplied(m_msgParentId, true);
                break;
            case FORWARD:
                rsMail->MessageForwarded(m_msgParentId, true);
                break;
            }
        }
    }

    if (mi.msgId.empty() == false) {
        MsgTagInfo tagInfo;
        rsMail->getMessageTag(mi.msgId, tagInfo);

        /* insert new tags */
        std::list<uint32_t>::iterator tag;
        for (tag = m_tagIds.begin(); tag != m_tagIds.end(); ++tag) {
            if (std::find(tagInfo.tagIds.begin(), tagInfo.tagIds.end(), *tag) == tagInfo.tagIds.end()) {
                rsMail->setMessageTag(mi.msgId, *tag, true);
            } else {
                tagInfo.tagIds.remove(*tag);
            }
        }

        /* remove deleted tags */
        for (tag = tagInfo.tagIds.begin(); tag != tagInfo.tagIds.end(); ++tag) {
            rsMail->setMessageTag(mi.msgId, *tag, false);
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

    setRecipientToRow(lastRow, TO,PEER_TYPE_SSL,"");
}

bool MessageComposer::getRecipientFromRow(int row, enumType &type, destinationType& dest_type,std::string &id)
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
    dest_type = (destinationType)ui.recipientWidget->item(row, COLUMN_RECIPIENT_DATA)->data(ROLE_RECIPIENT_TYPE).toInt();

    return true;
}

QString MessageComposer::getRecipientEmailAddress(const RsGxsId& id,const RsIdentityDetails& detail)
{
    return (QString("%2 <")+tr("Distant identity:")+" %2@%1>").arg(QString::fromStdString(id.toStdString())).arg(QString::fromUtf8(detail.mNickname.c_str())) ;
}

QString MessageComposer::getRecipientEmailAddress(const RsPeerId& /* id */,const RsPeerDetails& detail)
{
    QString location_name = detail.location.empty()?tr("[Missing]"):QString::fromUtf8(detail.location.c_str()) ;

    return (QString("%1 (")+tr("Node name & id:")+" %2, %3)").arg(QString::fromUtf8(detail.name.c_str()))
                 .arg(location_name)
                 .arg(QString::fromUtf8(detail.id.toStdString().c_str())) ;
}

void MessageComposer::setRecipientToRow(int row, enumType type, destinationType dest_type, const std::string &id)
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
        
        comboBox->setLayoutDirection(Qt::RightToLeft);
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
        switch(dest_type)
        {
        case PEER_TYPE_GROUP: {
            icon = FilesDefs::getIconFromQtResourcePath(IMAGE_GROUP16);

            RsGroupInfo groupInfo;
            if (rsPeers->getGroupInfo(RsNodeGroupId(id), groupInfo)) {
                name = GroupDefs::name(groupInfo);
            } else {
                name = tr("Unknown");
            }
        }
            break ;

        case PEER_TYPE_GXS: {

            RsIdentityDetails detail;
            RsGxsId gid(id) ;

            if(!rsIdentity->getIdDetails(gid, detail))
            {
                std::cerr << "Can't get peer details from " << gid << std::endl;
                return ;
            }

        QList<QIcon> icons ;
        GxsIdDetails::getIcons(detail,icons,GxsIdDetails::ICON_TYPE_AVATAR) ;

            name = getRecipientEmailAddress(gid,detail) ;

        if(!icons.empty())
            icon = icons.front() ;
        }
            break ;

        case PEER_TYPE_SSL: {
            RsPeerDetails details ;

            if(!rsPeers->getPeerDetails(RsPeerId(id), details))
            {
                std::cerr << "Can't get peer details from " << id << std::endl;
                return ;
            }
            name = getRecipientEmailAddress(RsPeerId(id),details) ;

            StatusInfo peerStatusInfo;
            // No check of return value. Non existing status info is handled as offline.
            rsStatus->getStatus(RsPeerId(id), peerStatusInfo);

            icon = FilesDefs::getIconFromQtResourcePath(StatusDefs::imageUser(peerStatusInfo.status));
        }
        break ;
        default:
            std::cerr << __PRETTY_FUNCTION__ << ": Unhandled type " << dest_type << std::endl;
            return ;
        }
    }

    comboBox->setCurrentIndex(comboBox->findData(type, Qt::UserRole));

    lineEdit->setText(name);
    if (id.empty())
        lineEdit->setStyleSheet(QString(STYLE_FAIL).arg(lineEdit->objectName()));
    else
        lineEdit->setStyleSheet(QString(STYLE_NORMAL).arg(lineEdit->objectName()));

    QTableWidgetItem *item = new QTableWidgetItem(icon, "", 0);
    item->setFlags(Qt::NoItemFlags);
    ui.recipientWidget->setItem(row, COLUMN_RECIPIENT_ICON, item);
    ui.recipientWidget->item(row, COLUMN_RECIPIENT_DATA)->setData(ROLE_RECIPIENT_ID, QString::fromStdString(id));
    ui.recipientWidget->item(row, COLUMN_RECIPIENT_DATA)->setData(ROLE_RECIPIENT_TYPE, dest_type);

    addEmptyRecipient();
}

bool MessageComposer::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::FocusIn) {
        QLineEdit *lineEdit = dynamic_cast<QLineEdit*>(obj);
        if (lineEdit) {
            int rowCount = ui.recipientWidget->rowCount();
            int row;
            for (row = 0; row < rowCount; ++row) {
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
                for (row = 0; row < rowCount; ++row) {
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

    if (event->type() == QEvent::Resize) {
        if (obj == ui.fromLabel) {
            // Resize "Recipient" column
            QHeaderView *header = ui.recipientWidget->horizontalHeader();
            header->resizeSection(COLUMN_RECIPIENT_TYPE, ui.fromLabel->size().width());
        }
    }

    // pass the event on to the parent class
    return QMainWindow::eventFilter(obj, event);
}

void MessageComposer::editingRecipientFinished()
{
    QLineEdit *lineEdit = dynamic_cast<QLineEdit*>(QObject::sender());

    if (lineEdit == NULL)
        return;

    lineEdit->setStyleSheet(QString(STYLE_NORMAL).arg(lineEdit->objectName()));

    // find row of the widget
    int rowCount = ui.recipientWidget->rowCount();
    int row;
    for (row = 0; row < rowCount; ++row)
        if (ui.recipientWidget->cellWidget(row, COLUMN_RECIPIENT_NAME) == lineEdit)
            break;

    if (row >= rowCount)  // not found
        return;

    enumType type;
    std::string id; // dummy
    destinationType dtype ;

    getRecipientFromRow(row, type, dtype, id);

    QString text = lineEdit->text();

    if (text.isEmpty()) {
        setRecipientToRow(row, type,PEER_TYPE_SSL, "");
        return;
    }

    // start with peers
    std::list<RsPeerId> peers;
    rsPeers->getFriendList(peers);
    RsPeerDetails details;

    for (std::list<RsPeerId>::iterator  peerIt = peers.begin(); peerIt != peers.end(); ++peerIt)
        if (rsPeers->getPeerDetails(*peerIt, details) && text == getRecipientEmailAddress(*peerIt,details))
    {
        setRecipientToRow(row, type, PEER_TYPE_SSL, details.id.toStdString());
        return ;
    }

    QList<QTreeWidgetItem*> gxsitems ;
    ui.friendSelectionWidget->items(gxsitems,FriendSelectionWidget::IDTYPE_GXS) ;
    RsIdentityDetails detail;

    for (QList<QTreeWidgetItem*>::const_iterator idIt = gxsitems.begin(); idIt != gxsitems.end(); ++idIt)
    {
        RsGxsId id ( ui.friendSelectionWidget->idFromItem( *idIt ) );

        if(rsIdentity->getIdDetails(id, detail) && text == getRecipientEmailAddress(id,detail))
    {
            setRecipientToRow(row, type, PEER_TYPE_GXS, id.toStdString());
        return ;
    }
    }


    // then groups
    std::list<RsGroupInfo> groupInfoList;
    rsPeers->getGroupInfoList(groupInfoList);

    std::list<RsGroupInfo>::iterator groupIt;
    for (groupIt = groupInfoList.begin(); groupIt != groupInfoList.end(); ++groupIt) {
        QString groupName = GroupDefs::name(*groupIt);
        if (text.compare(groupName, Qt::CaseSensitive) == 0) {
            // found it
            setRecipientToRow(row, type, PEER_TYPE_GROUP, groupIt->id.toStdString());
            return;
        }
    }

    setRecipientToRow(row, type, PEER_TYPE_SSL, "");
    lineEdit->setStyleSheet(QString(STYLE_FAIL).arg(lineEdit->objectName()));
    lineEdit->setText(text);
}

void MessageComposer::addRecipient(enumType type, const RsPeerId& pid)
{
    int rowCount = ui.recipientWidget->rowCount();
    int row;
    for (row = 0; row < rowCount; ++row)
    {
        enumType rowType;
        std::string rowId;
    destinationType dtype ;

        if (getRecipientFromRow(row, rowType, dtype, rowId))
        {
            if (rowId.empty()) // use row
                break;

            if (RsPeerId(rowId) == pid && rowType == type) // existing row
                break;
        }
        else // use row
            break;
    }

    setRecipientToRow(row, type, PEER_TYPE_SSL,pid.toStdString());
}
void MessageComposer::addRecipient(enumType type, const RsGxsId& gxs_id)
{
//    static bool already = false ;
//    if(!already)
//	 {
//        	QMessageBox::warning(NULL,"Distant messaging not stable","Distant messaging is currently unstable. Do not expect too much from it.") ;
//		already = true ;
//	 }

    int rowCount = ui.recipientWidget->rowCount();
    int row;
    for (row = 0; row < rowCount; ++row)
    {
        enumType rowType;
        std::string rowId;
    destinationType dtype ;

        if (getRecipientFromRow(row, rowType, dtype, rowId))
        {
            if (rowId.empty()) // use row
                break;

            if (RsGxsId(rowId) == gxs_id && rowType == type) // existing row
                break;
        }
        else // use row
            break;
    }

    setRecipientToRow(row, type, PEER_TYPE_GXS,gxs_id.toStdString());
}

void MessageComposer::addRecipient(enumType type, const std::string& id)
{
    // search existing or empty row
    int rowCount = ui.recipientWidget->rowCount();
    int row;
    for (row = 0; row < rowCount; ++row) {
        enumType rowType;
        std::string rowId;
        destinationType dtype ;

        if (getRecipientFromRow(row, rowType, dtype,rowId))
        {
            if (rowId.empty()) {
                // use row
                break;
            }

            if (rowId == id && rowType == type && dtype == PEER_TYPE_GROUP) {
                // existing row
                break;
            }
        } else {
            // use row
            break;
        }
    }

    setRecipientToRow(row, type, PEER_TYPE_GROUP, id);
}

void MessageComposer::setupFileActions()
{
    QMenu *menu = new QMenu(tr("&File"), this);
    menuBar()->addMenu(menu);

    QAction *a;

    a = new QAction(QIcon(""), tr("&New"), this);
    a->setShortcut(QKeySequence::New);
    connect(a, SIGNAL(triggered()), this, SLOT(fileNew()));
    menu->addAction(a);

    a = new QAction(QIcon(""), tr("&Open..."), this);
    a->setShortcut(QKeySequence::Open);
    connect(a, SIGNAL(triggered()), this, SLOT(fileOpen()));
    menu->addAction(a);

    menu->addSeparator();

    actionSave = a = new QAction(QIcon(""), tr("&Save"), this);
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

    a = new QAction(QIcon(""), tr("&Print..."), this);
    a->setShortcut(QKeySequence::Print);
    connect(a, SIGNAL(triggered()), this, SLOT(filePrint()));
    menu->addAction(a);

    /*a = new QAction(FilesDefs::getIconFromQtResourcePath(":/images/textedit/fileprint.png"), tr("Print Preview..."), this);
    connect(a, SIGNAL(triggered()), this, SLOT(filePrintPreview()));
    menu->addAction(a);*/

    a = new QAction(QIcon(""), tr("&Export PDF..."), this);
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
    a = actionUndo = new QAction(QIcon(""), tr("&Undo"), this);
    a->setShortcut(QKeySequence::Undo);
    menu->addAction(a);
    a = actionRedo = new QAction(QIcon(""), tr("&Redo"), this);
    a->setShortcut(QKeySequence::Redo);
    menu->addAction(a);
    menu->addSeparator();
    a = actionCut = new QAction(QIcon(""), tr("Cu&t"), this);
    a->setShortcut(QKeySequence::Cut);
    menu->addAction(a);
    a = actionCopy = new QAction(QIcon(""), tr("&Copy"), this);
    a->setShortcut(QKeySequence::Copy);
    menu->addAction(a);
    a = actionPaste = new QAction(QIcon(""), tr("&Paste"), this);
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

void MessageComposer::setupContactActions()
{
    mActionAddTo = new QAction(tr("Add to \"To\""), this);
    connect(mActionAddTo, SIGNAL(triggered(bool)), this, SLOT(addTo()));
    mActionAddCC = new QAction(tr("Add to \"CC\""), this);
    connect(mActionAddCC, SIGNAL(triggered(bool)), this, SLOT(addCc()));
    mActionAddBCC = new QAction(tr("Add to \"BCC\""), this);
    connect(mActionAddBCC, SIGNAL(triggered(bool)), this, SLOT(addBcc()));
    mActionAddRecommend = new QAction(tr("Add as Recommend"), this);
    connect(mActionAddRecommend, SIGNAL(triggered(bool)), this, SLOT(addRecommend()));
    mActionContactDetails = new QAction(FilesDefs::getIconFromQtResourcePath(IMAGE_FRIENDINFO), tr("Details"), this);
    connect(mActionContactDetails, SIGNAL(triggered(bool)), this, SLOT(contactDetails()));

    ui.friendSelectionWidget->addContextMenuAction(mActionAddTo);
    ui.friendSelectionWidget->addContextMenuAction(mActionAddCC);
    ui.friendSelectionWidget->addContextMenuAction(mActionAddBCC);
    ui.friendSelectionWidget->addContextMenuAction(mActionAddRecommend);

    QAction *action = new QAction(this);
    action->setSeparator(true);
    ui.friendSelectionWidget->addContextMenuAction(action);

    ui.friendSelectionWidget->addContextMenuAction(mActionContactDetails);
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

void MessageComposer::textStyle(QAction *a)
{
    QTextCursor cursor = ui.msgText->textCursor();

        QTextListFormat::Style style = QTextListFormat::ListDisc;
        
        if (a == actionDisc)
                style = QTextListFormat::ListDisc;
        else if (a == actionCircle)
                style = QTextListFormat::ListCircle;
        else if (a == actionSquare)
                style = QTextListFormat::ListSquare;
        else if (a == actionDecimal)
                style = QTextListFormat::ListDecimal;
        else if (a == actionLowerAlpha)
                style = QTextListFormat::ListLowerAlpha;
        else if (a == actionUpperAlpha)
                style = QTextListFormat::ListUpperAlpha;
        else if (a == actionLowerRoman)
                style = QTextListFormat::ListLowerRoman;
        else if (a == actionUpperRoman)
                style = QTextListFormat::ListUpperRoman;

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
    /*} else {
        // ####
        QTextBlockFormat bfmt;
        bfmt.setObjectIndex(-1);
        cursor.mergeBlockFormat(bfmt);
    }*/
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

void MessageComposer::textbackgroundColor()
{
    QColor col2 = QColorDialog::getColor(ui.msgText->textBackgroundColor(), this);
    if (!col2.isValid())
        return;
    QTextCharFormat fmt;
    fmt.setBackground(col2);
    mergeFormatOnWordOrSelection(fmt);
    colorChanged2(col2);
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
	QString smiley = qobject_cast<QPushButton*>(sender())->toolTip().split("|").first();
	// add trailing space
	smiley += QString(" ");
	// add preceding space when needed (not at start of text or preceding space already exists)
	if(!ui.msgText->textCursor().atStart() && ui.msgText->toPlainText()[ui.msgText->textCursor().position() - 1] != QChar(' '))
		smiley = QString(" ") + smiley;
	ui.msgText->textCursor().insertText(smiley);
}

void MessageComposer::currentCharFormatChanged(const QTextCharFormat &format)
{
    fontChanged(format.font());
    colorChanged(format.foreground().color());
    colorChanged2(format.background().color());
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

void MessageComposer::colorChanged2(const QColor &c)
{
    QPixmap pix(16, 16);
    pix.fill(c);
    ui.color2btn->setIcon(pix);
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
    updatecontactsviewicons();
}

void MessageComposer::on_contactsdockWidget_visibilityChanged(bool visible)
{
    contactSidebarAction->setChecked(visible);
    updatecontactsviewicons();
}

void MessageComposer::updatecontactsviewicons()
{
    if(!ui.contactsdockWidget->isVisible()){
      ui.actionContactsView->setIcon(FilesDefs::getIconFromQtResourcePath(":/icons/mail/contacts.png"));
    }else{
      ui.actionContactsView->setIcon(FilesDefs::getIconFromQtResourcePath(":/icons/mail/contacts.png"));
    } 
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
    std::set<RsPeerId> peerIds ;
	ui.friendSelectionWidget->selectedIds<RsPeerId,FriendSelectionWidget::IDTYPE_SSL>(peerIds, false);

    for (std::set<RsPeerId>::const_iterator idIt = peerIds.begin(); idIt != peerIds.end(); ++idIt)
		addRecipient(type, *idIt);

    std::set<RsGxsId> gxsIds ;
    ui.friendSelectionWidget->selectedIds<RsGxsId,FriendSelectionWidget::IDTYPE_GXS>(gxsIds, false);

    for (std::set<RsGxsId>::const_iterator idIt = gxsIds.begin(); idIt != gxsIds.end(); ++idIt)
		addRecipient(type, *idIt);
}

void MessageComposer::filterComboBoxChanged(int i)
{
	switch(i)
	{
	default:
	case 0:
		ui.friendSelectionWidget->setShowType(FriendSelectionWidget::SHOW_GXS);
		break;
	case 1:
		ui.friendSelectionWidget->setShowType(FriendSelectionWidget::SHOW_CONTACTS);
		break;
	}
}

void MessageComposer::friendSelectionChanged()
{
	std::set<RsPeerId> peerIds;
	ui.friendSelectionWidget->selectedIds<RsPeerId,FriendSelectionWidget::IDTYPE_SSL>(peerIds, false);

	std::set<RsGxsId> gxsIds;
	ui.friendSelectionWidget->selectedIds<RsGxsId,FriendSelectionWidget::IDTYPE_GXS>(gxsIds, false);

	int selectedCount = peerIds.size() + gxsIds.size();

	mActionAddTo->setEnabled(selectedCount);
	mActionAddCC->setEnabled(selectedCount);
	mActionAddBCC->setEnabled(selectedCount);
	mActionAddRecommend->setEnabled(selectedCount);

	FriendSelectionWidget::IdType idType;
	ui.friendSelectionWidget->selectedId(idType);

	mActionContactDetails->setEnabled(selectedCount == 1 && (idType == FriendSelectionWidget::IDTYPE_SSL || idType == FriendSelectionWidget::IDTYPE_GXS));
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
    std::set<RsPeerId> sslIds;
	ui.friendSelectionWidget->selectedIds<RsPeerId,FriendSelectionWidget::IDTYPE_SSL>(sslIds, false);

    std::set<RsGxsId> gxsIds ;
	ui.friendSelectionWidget->selectedIds<RsGxsId,FriendSelectionWidget::IDTYPE_GXS>(gxsIds, true);

	if (sslIds.empty() && gxsIds.empty()) 
		return;

	for( std::set <RsPeerId>::iterator it = sslIds.begin(); it != sslIds.end(); ++it)
		addRecipient(CC, *it) ;

	for( std::set<RsGxsId>::const_iterator it = gxsIds.begin(); it != gxsIds.end(); ++it)
		addRecipient(TO, *it) ;

	QString text = buildRecommendHtml(sslIds);
	ui.msgText->textCursor().insertHtml(text);
	ui.msgText->setFocus(Qt::OtherFocusReason);
}

void MessageComposer::contactDetails()
{
    FriendSelectionWidget::IdType idType;
    std::string id = ui.friendSelectionWidget->selectedId(idType);

    if (id.empty()) {
        return;
    }

    switch (idType) {
    case FriendSelectionWidget::IDTYPE_NONE:
    case FriendSelectionWidget::IDTYPE_GROUP:
    case FriendSelectionWidget::IDTYPE_GPG:
        break;
    case FriendSelectionWidget::IDTYPE_SSL:
        ConfCertDialog::showIt(RsPeerId(id), ConfCertDialog::PageDetails);
        break;
    case FriendSelectionWidget::IDTYPE_GXS:
        {
            if (RsGxsGroupId(id).isNull()) {
                return;
            }

            IdDetailsDialog *dialog = new IdDetailsDialog(RsGxsGroupId(id));
            dialog->show();

            /* Dialog will destroy itself */
        }
        break;
    }

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
		rsMail->getMessageTagTypes(tags);

		std::map<uint32_t, std::pair<std::string, uint32_t> >::iterator tag;
		for (std::list<uint32_t>::iterator tagId = m_tagIds.begin(); tagId != m_tagIds.end(); ++tagId) {
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
void MessageComposer::on_closeSizeLimitFrameButton_clicked()
{
    ui.sizeLimitFrame->setVisible(false);
}
void MessageComposer::on_closeInfoFrameButton_clicked()
{
	ui.distantFrame->setVisible(false);
}

QString MessageComposer::inviteMessage()
{
    return tr("Hi,<br>I want to be friends with you on RetroShare.<br>");
}

void MessageComposer::sendInvite(const RsGxsId &to, bool autoSend)
{
    /* create a message */
    MessageComposer *composer = MessageComposer::newMsg();

    composer->setTitleText(tr("Invite message"));
    composer->msgFlags |= RS_MSG_USER_REQUEST;


    RsPeerId ownId = rsPeers->getOwnId();
    RetroShareLink link = RetroShareLink::createCertificate(ownId);
        
    QString sMsgText = inviteMessage();
    sMsgText += "<br><br>";
    sMsgText += tr("Respond now:") + "<br>";
    sMsgText += link.toHtml() + "<br>";
    sMsgText += "<br>";
    sMsgText += tr("Thanks, <br>") + QString::fromUtf8(rsPeers->getGPGName(rsPeers->getGPGOwnId()).c_str());
    composer->setMsgText(sMsgText);
    composer->addRecipient(MessageComposer::TO,  RsGxsId(to));
    
    
    if (autoSend) {
        if (composer->sendMessage_internal(false)) {
            composer->close();
            return;
        }
    }
	else
		composer->show();

    /* window will destroy itself! */
}

