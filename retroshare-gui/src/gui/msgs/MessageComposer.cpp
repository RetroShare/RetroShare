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
#include <QFileDialog>
#include <QTextList>
#include <QColorDialog>
#include <QTextDocumentFragment>
#include <QTimer>
#include <QCompleter>

#include <algorithm>

#include "rshare.h"
#include "MessageComposer.h"

#include <retroshare/rsiface.h>
#include <retroshare/rspeers.h>
#include <retroshare/rsmsgs.h>
#include <retroshare/rsstatus.h>

#include "gui/notifyqt.h"
#include "gui/common/RSTreeWidgetItem.h"
#include "gui/common/GroupDefs.h"
#include "gui/common/StatusDefs.h"
#include "gui/RetroShareLink.h"
#include "gui/settings/rsharesettings.h"
#include "gui/feeds/AttachFileItem.h"
#include "textformat.h"
#include "util/misc.h"

#define IMAGE_GROUP16            ":/images/user/group16.png"

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

static QString BuildPeerName(RsPeerDetails &detail)
{
    QString name = QString::fromUtf8(detail.name.c_str());
    if (detail.location.empty() == false) {
        name += " - " + QString::fromUtf8(detail.location.c_str());
    }

    return name;
}

/** Constructor */
MessageComposer::MessageComposer(QWidget *parent, Qt::WFlags flags)
: QMainWindow(parent, flags), mCheckAttachment(true)
{
    /* Invoke the Qt Designer generated object setup routine */
    ui.setupUi(this);

    setupFileActions();
    setupEditActions();
    setupViewActions();
    setupInsertActions();

    m_compareRole = new RSTreeWidgetItemCompareRole;
    m_compareRole->addRole(COLUMN_CONTACT_NAME, ROLE_CONTACT_SORT);

    m_completer = NULL;

    Settings->loadWidgetInformation(this);

    setAttribute ( Qt::WA_DeleteOnClose, true );

    // connect up the buttons.
    connect( ui.actionSend, SIGNAL( triggered (bool)), this, SLOT( sendMessage( ) ) );
    //connect( ui.actionReply, SIGNAL( triggered (bool)), this, SLOT( replyMessage( ) ) );
    connect(ui.boldbtn, SIGNAL(clicked()), this, SLOT(textBold()));
    connect(ui.underlinebtn, SIGNAL(clicked()), this, SLOT(textUnderline()));
    connect(ui.italicbtn, SIGNAL(clicked()), this, SLOT(textItalic()));
    connect(ui.colorbtn, SIGNAL(clicked()), this, SLOT(textColor()));
    connect(ui.imagebtn, SIGNAL(clicked()), this, SLOT(addImage()));
    //connect(ui.linkbtn, SIGNAL(clicked()), this, SLOT(insertLink()));
    connect(ui.actionContactsView, SIGNAL(triggered()), this, SLOT(toggleContacts()));
    connect(ui.actionSaveas, SIGNAL(triggered()), this, SLOT(saveasDraft()));
    connect(ui.actionAttach, SIGNAL(triggered()), this, SLOT(attachFile()));
    connect(ui.filterPatternLineEdit, SIGNAL(textChanged(const QString &)), this, SLOT(filterRegExpChanged()));
    connect(ui.clearButton, SIGNAL(clicked()), this, SLOT(clearFilter()));
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

    connect(ui.msgFileList, SIGNAL(itemChanged( QTreeWidgetItem *, int ) ), this, SLOT(toggleRecommendItem( QTreeWidgetItem *, int ) ));

    connect(ui.addToButton, SIGNAL(clicked(void)), this, SLOT(btnClickEvent()));
    connect(ui.addCcButton, SIGNAL(clicked(void)), this, SLOT(btnClickEvent()));
    connect(ui.addBccButton, SIGNAL(clicked(void)), this, SLOT(btnClickEvent()));
    connect(ui.addRecommendButton, SIGNAL(clicked(void)), this, SLOT(recommendButtonClicked()));
    connect(ui.msgSendList, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(btnClickEvent()));

    connect(NotifyQt::getInstance(), SIGNAL(groupsChanged(int)), this, SLOT(groupsChanged(int)));
    connect(NotifyQt::getInstance(), SIGNAL(peerStatusChanged(const QString&,int)), this, SLOT(peerStatusChanged(const QString&,int)));

    /* hide the Tree +/- */
    ui.msgFileList -> setRootIsDecorated( false );

    /* to hide the header  */
    //ui.msgSendList->header()->hide();

    /* sort send list by name ascending */
    ui.msgSendList->sortItems(0, Qt::AscendingOrder);

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
    ui.msgFileList->setColumnCount(5);
    ui.msgFileList->setColumnHidden ( 4, true);

    QHeaderView * _smheader = ui.msgFileList->header () ;

    _smheader->resizeSection ( 0, 200 );
    _smheader->resizeSection ( 1, 60 );
    _smheader->resizeSection ( 2, 60 );
    _smheader->resizeSection ( 3, 220 );
    _smheader->resizeSection ( 4, 10 );

    QPalette palette = QApplication::palette();
    codeBackground = palette.color( QPalette::Active, QPalette::Midlight );

    ui.clearButton->hide();

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

    /* set focus to subject */
    ui.titleEdit->setFocus();

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

/*static*/ void MessageComposer::msgFriend(std::string id, bool group)
{
//    std::cerr << "MessageComposer::msgfriend()" << std::endl;

    /* create a message */

    MessageComposer *pMsgDialog = new MessageComposer();

    pMsgDialog->newMsg();

    if (group) {
        pMsgDialog->addRecipient(TO, id, true);
    } else {
        //put all sslChilds in message list
        std::list<std::string> sslIds;
        rsPeers->getSSLChildListOfGPGId(id, sslIds);

        std::list<std::string>::const_iterator it;
        for (it = sslIds.begin(); it != sslIds.end(); it++) {
            pMsgDialog->addRecipient(TO, *it, false);
        }
    }

    pMsgDialog->show();

    /* window will destroy itself! */
}

static QString BuildRecommendHtml(std::list<std::string> &peerids)
{
    QString text;

    /* process peer ids */
    std::list <std::string>::iterator peerid;
    for (peerid = peerids.begin(); peerid != peerids.end(); peerid++) {
        RsPeerDetails detail;
        if (rsPeers->getPeerDetails(*peerid, detail) == false) {
            std::cerr << "MessageComposer::recommendFriend() Couldn't find peer id " << *peerid << std::endl;
            continue;
        }

        RetroShareLink link(QString::fromUtf8(detail.name.c_str()), QString::fromStdString(detail.id));
        if (link.valid() == false || link.type() != RetroShareLink::TYPE_PERSON) {
            continue;
        }
        text += link.toHtmlFull() + "<br>";
    }

    return text;
}

void MessageComposer::recommendFriend(std::list <std::string> &peerids)
{
//    std::cerr << "MessageComposer::recommendFriend()" << std::endl;

    if (peerids.size() == 0) {
        return;
    }

    /* create a message */
    MessageComposer *pMsgDialog = new MessageComposer();

    pMsgDialog->newMsg();
    pMsgDialog->setWindowTitle(tr("Compose") + ": " + tr("Friend Recommendation")) ;
    pMsgDialog->insertTitleText(tr("Friend Recommendation(s)").toStdString());

    std::string sMsgText = tr("I recommend a good friend of me, you can trust him too when you trust me. <br> Copy friend link and paste to Friends list").toStdString();
    sMsgText += "<br><br>";
    sMsgText += BuildRecommendHtml(peerids).toStdString();
    pMsgDialog->insertMsgText(sMsgText);

//    pMsgDialog->insertFileList(files_info);
    pMsgDialog->show();

    /* window will destroy itself! */
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

        //=== uncheck all repecient's boxes =======
        QTreeWidget *sendWidget = ui.msgSendList;

        for (int i=0;i<sendWidget->topLevelItemCount();++i)
            sendWidget->topLevelItem(i)->setCheckState(0,Qt::Unchecked) ;

        // save settings
        processSettings(false);

        QMainWindow::closeEvent(event);
    } else {
        event->ignore();
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

void MessageComposer::insertSendList()
{
    /* get a link to the table */
    QTreeWidget *sendWidget = ui.msgSendList;

    /* remove old items */
    sendWidget->clear();
    sendWidget->setColumnCount(1);

    setNewCompleter(ui.recipientWidget, NULL);
    if (m_completer) {
        delete(m_completer);
        m_completer = NULL;
    }

    if (!rsPeers)
    {
        /* not ready yet! */
        return;
    }

    // get existing groups
    std::list<RsGroupInfo> groupInfoList;
    std::list<RsGroupInfo>::iterator groupIt;
    rsPeers->getGroupInfoList(groupInfoList);

    std::list<std::string> peers;
    std::list<std::string>::iterator peerIt;
    rsPeers->getFriendList(peers);

    std::list<std::string> fillPeerIds;

    // start with groups
    groupIt = groupInfoList.begin();
    while (true) {
        QTreeWidgetItem *groupItem = NULL;
        RsGroupInfo *groupInfo = NULL;

        if (groupIt != groupInfoList.end()) {
            groupInfo = &(*groupIt);

            if (groupInfo->peerIds.size() == 0) {
                // don't show empty groups
                groupIt++;
                continue;
            }

            // add group item
            groupItem = new RSTreeWidgetItem(m_compareRole, TYPE_GROUP);

            // Add item to the list
            sendWidget->addTopLevelItem(groupItem);

            groupItem->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
//            groupItem->setSizeHint(COLUMN_NAME, QSize(26, 26));
            groupItem->setTextAlignment(COLUMN_CONTACT_NAME, Qt::AlignLeft | Qt::AlignVCenter);
            groupItem->setIcon(COLUMN_CONTACT_NAME, QIcon(IMAGE_GROUP16));

            /* used to find back the item */
            groupItem->setData(COLUMN_CONTACT_DATA, ROLE_CONTACT_ID, QString::fromStdString(groupInfo->id));

            groupItem->setExpanded(true);

            QString groupName = GroupDefs::name(*groupInfo);
            groupItem->setText(COLUMN_CONTACT_NAME, groupName);
            groupItem->setData(COLUMN_CONTACT_DATA, ROLE_CONTACT_SORT, ((groupInfo->flag & RS_GROUP_FLAG_STANDARD) ? "0 " : "1 ") + groupName);
        }

        // iterate through peers
        for (peerIt = peers.begin(); peerIt != peers.end(); peerIt++) {
            RsPeerDetails detail;
            if (!rsPeers->getPeerDetails(*peerIt, detail)) {
                continue; /* BAD */
            }

            if (groupInfo) {
                // we fill a group, check if gpg id is assigned
                if (std::find(groupInfo->peerIds.begin(), groupInfo->peerIds.end(), detail.gpg_id) == groupInfo->peerIds.end()) {
                    continue;
                }
            } else {
                // we fill the not assigned gpg ids
                if (std::find(fillPeerIds.begin(), fillPeerIds.end(), *peerIt) != fillPeerIds.end()) {
                    continue;
                }
            }

            // add equal too, its no problem
            fillPeerIds.push_back(detail.id);


            /* make a widget per friend */
            QTreeWidgetItem *item = new RSTreeWidgetItem(m_compareRole, TYPE_SSL);

            /* add all the labels */
            /* (0) Person */
            QString name = BuildPeerName(detail);
            item->setText(COLUMN_CONTACT_NAME, name);

            int state = RS_STATUS_OFFLINE;
            if (detail.state & RS_PEER_STATE_CONNECTED) {
                item->setTextColor(COLUMN_CONTACT_NAME, COLOR_CONNECT);
                state = RS_STATUS_ONLINE;
            }

            item->setIcon(COLUMN_CONTACT_NAME, QIcon(StatusDefs::imageUser(state)));
            item->setData(COLUMN_CONTACT_DATA, ROLE_CONTACT_ID, QString::fromStdString(detail.id));
            item->setData(COLUMN_CONTACT_DATA, ROLE_CONTACT_SORT, "2 " + name);

            // add to the list
            if (groupItem) {
                groupItem->addChild(item);
            } else {
                sendWidget->addTopLevelItem(item);
            }
        }

        if (groupIt != groupInfoList.end()) {
            groupIt++;
        } else {
            // all done
            break;
        }
    }

    if (ui.filterPatternLineEdit->text().isEmpty() == false) {
        FilterItems();
    }

    sendWidget->update(); /* update display */

    // create completer list for friends
    QStringList completerList;
    QStringList completerGroupList;

    for (peerIt = peers.begin(); peerIt != peers.end(); peerIt++) {
        RsPeerDetails detail;
        if (!rsPeers->getPeerDetails(*peerIt, detail)) {
            continue; /* BAD */
        }

        QString name = BuildPeerName(detail);
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

void MessageComposer::groupsChanged(int type)
{
    Q_UNUSED(type);

    insertSendList();
}

void MessageComposer::peerStatusChanged(const QString& peer_id, int status)
{
    QTreeWidgetItemIterator itemIterator(ui.msgSendList);
    QTreeWidgetItem *item;
    while ((item = *itemIterator) != NULL) {
        itemIterator++;

        if (item->data(COLUMN_CONTACT_DATA, ROLE_CONTACT_ID).toString() == peer_id) {
            QColor color;
            if (status != (int) RS_STATUS_OFFLINE) {
                color = COLOR_CONNECT;
            } else {
                color = Qt::black;
            }

            item->setTextColor(COLUMN_CONTACT_NAME, color);
            item->setIcon(COLUMN_CONTACT_NAME, QIcon(StatusDefs::imageUser(status)));
            //break; no break, peers can assigned to groups more than one
        }
    }

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

void  MessageComposer::insertFileList(const std::list<DirDetails>& dir_info)
{
    std::list<FileInfo> files_info;
    std::list<DirDetails>::const_iterator it;

    /* convert dir_info to files_info */
    for(it = dir_info.begin(); it != dir_info.end(); it++)
    {
        FileInfo info ;
        info.fname = it->name ;
        info.hash = it->hash ;
        info.rank = 0;//it->rank ;
        info.size = it->count ;
        info.inRecommend = true;
        files_info.push_back(info) ;
    }

    insertFileList(files_info);
}

void  MessageComposer::insertFileList(const std::list<FileInfo>& files_info)
{
    _recList.clear() ;

    std::list<FileInfo>::const_iterator it;

    /* get a link to the table */
    QTreeWidget *tree = ui.msgFileList;

    tree->clear();
    tree->setColumnCount(5);

    QList<QTreeWidgetItem *> items;
    for(it = files_info.begin(); it != files_info.end(); it++)
    {
        _recList.push_back(*it) ;

        /* make a widget per person */
        QTreeWidgetItem *item = new QTreeWidgetItem((QTreeWidget*)0);

        item->setText(0, QString::fromStdString(it->fname));			/* (0) Filename */
        item->setText(1, misc::friendlyUnit(it->size));			 		/* (1) Size */
        item->setText(2, QString::number(0)) ;//it->rank));
        item->setText(3, QString::fromStdString(it->hash));
        item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        item->setCheckState(0, it->inRecommend ? Qt::Checked : Qt::Unchecked);

        /* add to the list */
        items.append(item);
    }

    /* add the items in! */
    tree->insertTopLevelItems(0, items);

    tree->update(); /* update display */
}

/* title changed */
void MessageComposer::titleChanged()
{
    calculateTitle();
    ui.msgText->document()->setModified(true);
}

void MessageComposer::calculateTitle()
{
    setWindowTitle(tr("Compose") + ": " + ui.titleEdit->text());
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
                rsPeers->getSSLChildListOfGPGId(*peerIt, sslIds);

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
                rsPeers->getSSLChildListOfGPGId(*peerIt, sslIds);

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

void  MessageComposer::newMsg(std::string msgId /*= ""*/)
{
    /* clear all */
    ui.msgText->setText("");

    /* worker fns */
    insertSendList();

    ui.recipientWidget->setRowCount(0);
    addEmptyRecipient();

    m_sMsgId = msgId;
    m_sDraftMsgId.clear();

    if (m_sMsgId.empty() == false) {
        // fill existing message
        MessageInfo msgInfo;
        if (!rsMsgs->getMessage(m_sMsgId, msgInfo)) {
            std::cerr << "MessageComposer::newMsg() Couldn't find Msg" << std::endl;
            m_sMsgId.clear();
            return;
        }

        if (msgInfo.msgflags & RS_MSG_DRAFT) {
            m_sDraftMsgId = msgId;
        }

        insertTitleText( QString::fromStdWString(msgInfo.title).toStdString());

        insertMsgText(QString::fromStdWString(msgInfo.msg).toStdString());

        insertFileList(msgInfo.files);

        // get existing groups
        std::list<RsGroupInfo> groupInfoList;
        rsPeers->getGroupInfoList(groupInfoList);

        std::list<std::string> groupIds;
        std::list<std::string>::iterator groupIt;
        std::list<std::string>::iterator it;

        calculateGroupsOfSslIds(groupInfoList, msgInfo.msgto, groupIds);
        for (groupIt = groupIds.begin(); groupIt != groupIds.end(); groupIt++ ) {
            addRecipient(MessageComposer::TO, *groupIt, true) ;
        }
        for (it = msgInfo.msgto.begin(); it != msgInfo.msgto.end(); it++ ) {
            addRecipient(MessageComposer::TO, *it, false) ;
        }

        calculateGroupsOfSslIds(groupInfoList, msgInfo.msgcc, groupIds);
        for (groupIt = groupIds.begin(); groupIt != groupIds.end(); groupIt++ ) {
            addRecipient(MessageComposer::CC, *groupIt, true) ;
        }
        for (it = msgInfo.msgcc.begin(); it != msgInfo.msgcc.end(); it++ ) {
            addRecipient(MessageComposer::CC, *it, false) ;
        }

        calculateGroupsOfSslIds(groupInfoList, msgInfo.msgbcc, groupIds);
        for (groupIt = groupIds.begin(); groupIt != groupIds.end(); groupIt++ ) {
            addRecipient(MessageComposer::BCC, *groupIt, true) ;
        }
        for (it = msgInfo.msgbcc.begin(); it != msgInfo.msgbcc.end(); it++ ) {
            addRecipient(MessageComposer::BCC, *it, false) ;
        }

        ui.msgText->document()->setModified(false);
    } else {
        insertTitleText(tr("No Title").toStdString());
    }

    calculateTitle();
}

void  MessageComposer::insertTitleText(std::string title)
{
    ui.titleEdit->setText(QString::fromStdString(title));
}

void  MessageComposer::insertPastedText(std::string msg)
{
    std::string::size_type i=0 ;
    while( (i=msg.find_first_of('\n',i+1)) < msg.size())
        msg.replace(i,1,std::string("\n<BR/>> ")) ;

    ui.msgText->setHtml(QString("<HTML><font color=\"blue\">")+QString::fromStdString(std::string("> ") + msg)+"</font><br/><br/></HTML>") ;

    ui.msgText->setFocus( Qt::OtherFocusReason );

    QTextCursor c =  ui.msgText->textCursor();
    c.movePosition(QTextCursor::End);
    ui.msgText->setTextCursor(c);

    ui.msgText->document()->setModified(true);
}

void  MessageComposer::insertForwardPastedText(std::string msg)
{
    std::string::size_type i=0 ;
    while( (i=msg.find_first_of('\n',i+1)) < msg.size())
        msg.replace(i,1,std::string("\n<BR/>> ")) ;

    ui.msgText->setHtml(QString("<HTML><blockquote [type=cite]><font color=\"blue\">")+QString::fromStdString(std::string("") + msg)+"</font><br/><br/></blockquote></HTML>") ;

    ui.msgText->setFocus( Qt::OtherFocusReason );

    QTextCursor c =  ui.msgText->textCursor();
    c.movePosition(QTextCursor::End);
    ui.msgText->setTextCursor(c);

    ui.msgText->document()->setModified(true);
}

void  MessageComposer::insertMsgText(std::string msg)
{
    ui.msgText->setText(QString::fromStdString(msg));

    ui.msgText->setFocus( Qt::OtherFocusReason );

    QTextCursor c =  ui.msgText->textCursor();
    c.movePosition(QTextCursor::End);
    ui.msgText->setTextCursor(c);

    ui.msgText->document()->setModified(true);
}

void  MessageComposer::insertHtmlText(std::string msg)
{
    ui.msgText->setHtml(QString("<a href='") + QString::fromStdString(std::string(msg + "'> ") ) + QString::fromStdString(std::string(msg)) + "</a>") ;

    ui.msgText->document()->setModified(true);
}

void  MessageComposer::sendMessage()
{
    sendMessage_internal(false);
    close();
}

void MessageComposer::sendMessage_internal(bool bDraftbox)
{
    /* construct a message */
    MessageInfo mi;

    mi.title = ui.titleEdit->text().toStdWString();
    mi.msg =   ui.msgText->toHtml().toStdWString();

    for(std::list<FileInfo>::const_iterator it(_recList.begin()); it != _recList.end(); ++it)
        if (it -> inRecommend)
            mi.files.push_back(*it);

    /* get the ids from the send list */
    std::list<std::string> peers;
    rsPeers->getFriendList(peers);

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
                    rsPeers->getSSLChildListOfGPGId(*groupIt, sslIds);

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
                if (std::find(peers.begin(), peers.end(), id) == peers.end()) {
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
        rsMsgs->MessageToDraft(mi);

        // use new message id
        m_sDraftMsgId = mi.msgId;
    } else {
        rsMsgs->MessageSend(mi);
    }

    ui.msgText->document()->setModified(false);
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
        lineEdit->setStyleSheet(QString(STYLE_NORMAL).arg(objectName));

        lineEdit->setCompleter(m_completer);

        ui.recipientWidget->setCellWidget(row, COLUMN_RECIPIENT_NAME, lineEdit);

        connect(lineEdit, SIGNAL(editingFinished()), this, SLOT(editingRecipientFinished()));
        lineEdit->installEventFilter(this);
    }

    QIcon icon;
    QString name;
    if (id.empty() == FALSE) {
        if (group) {
            icon = QIcon(IMAGE_GROUP16);

            RsGroupInfo groupInfo;
            if (rsPeers->getGroupInfo(id, groupInfo)) {
                name = GroupDefs::name(groupInfo);
            } else {
                name = tr("Unknown");
            }
        } else {
            RsPeerDetails detail;
            if (rsPeers->getPeerDetails(id, detail)) {
                name = BuildPeerName(detail);

                StatusInfo peerStatusInfo;
                // No check of return value. Non existing status info is handled as offline.
                rsStatus->getStatus(id, peerStatusInfo);

                icon = QIcon(StatusDefs::imageUser(peerStatusInfo.status));
            } else {
                icon = QIcon(StatusDefs::imageUser(RS_STATUS_OFFLINE));
                name = tr("Unknown friend");
            }
        }
    }

    comboBox->setCurrentIndex(comboBox->findData(type, Qt::UserRole));

    lineEdit->setText(name);

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
        RsPeerDetails detail;
        if (!rsPeers->getPeerDetails(*peerIt, detail)) {
            continue; /* BAD */
        }

        QString name = BuildPeerName(detail);
        if (text.compare(name, Qt::CaseSensitive) == 0) {
            // found it
            setRecipientToRow(row, type, detail.id, false);
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

void MessageComposer::addRecipient(enumType type, const std::string &id, bool group)
{
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

            if (rowId == id && rowType == type && group == rowGroup) {
                // existing row
                break;
            }
        } else {
            // use row
            break;
        }
    }

    setRecipientToRow(row, type, id, group);
}

void MessageComposer::toggleRecommendItem( QTreeWidgetItem *item, int col )
{
    //std::cerr << "ToggleRecommendItem()" << std::endl;

    /* extract hash */
    std::string hash = (item -> text(3)).toStdString();

    /* get state */
    bool inRecommend = (Qt::Checked == item -> checkState(0)); /* alway column 0 */

    for(std::list<FileInfo>::iterator it(_recList.begin()); it != _recList.end(); ++it) {
        if (it->hash == hash) {
            it->inRecommend = inRecommend;
        }
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

    a = new QAction(QIcon(""), tr("&Image"), this);
    connect(a, SIGNAL(triggered()), this, SLOT(addImage()));
    menu->addAction(a);

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
    QString fn = QFileDialog::getOpenFileName(this, tr("Open File..."),
                                              QString(), tr("HTML-Files (*.htm *.html);;All Files (*)"));
    if (!fn.isEmpty())
        load(fn);
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
    QString fn = QFileDialog::getSaveFileName(this, tr("Save as..."),
                                              QString(), tr("HTML-Files (*.htm *.html);;All Files (*)"));
    if (fn.isEmpty())
        return false;
    setCurrentFileName(fn);
    return fileSave();
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
    QString fileName = QFileDialog::getSaveFileName(this, "Export PDF",
                                                    QString(), "*.pdf");
    if (!fileName.isEmpty()) {
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
    QString fileimg = QFileDialog::getOpenFileName( this, tr( "Choose Image" ),
                                                    QString(Settings->valueFromGroup("MessageComposer", "LastDir").toString()) ,tr("Image Files supported (*.png *.jpeg *.jpg *.gif)"));

    if ( fileimg.isEmpty() ) {
        return;
    }

    QImage base(fileimg);

    QString pathimage = fileimg.left(fileimg.lastIndexOf("/"))+"/";
    Settings->setValueToGroup("MessageComposer", "LastDir", pathimage);

    Create_New_Image_Tag(fileimg);
}

void  MessageComposer::Create_New_Image_Tag( const QString urlremoteorlocal )
{
   /*if (image_extension(urlremoteorlocal)) {*/
       QString subtext = QString("<p><img src=\"%1\" />").arg(urlremoteorlocal);
               ///////////subtext.append("<br/><br/>Description on image.</p>");
       QTextDocumentFragment fragment = QTextDocumentFragment::fromHtml(subtext);
       ui.msgText->textCursor().insertFragment(fragment);
       //emit statusMessage(QString("Image new :").arg(urlremoteorlocal));
   //}
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
    QString qfile = QFileDialog::getOpenFileName(this, tr("Add Extra File"), "", "", 0,
                                                 QFileDialog::DontResolveSymlinks);
    std::string filePath = qfile.toUtf8().constData();
    if (filePath != "")
    {
        MessageComposer::addAttachment(filePath);
    }
}

void MessageComposer::addAttachment(std::string filePath)
{
    /* add a AttachFileItem to the attachment section */
    std::cerr << "MessageComposer::addFile() hashing file.";
    std::cerr << std::endl;

    /* add widget in for new destination */
    AttachFileItem *file = new AttachFileItem(filePath);
    //file->

    ui.verticalLayout->addWidget(file, 1, 0);

    //when the file is local or is finished hashing, call the fileHashingFinished method to send a chat message
    if (file->getState() == AFI_STATE_LOCAL) {
        fileHashingFinished(file);
    } else {
        QObject::connect(file,SIGNAL(fileFinished(AttachFileItem *)),this, SLOT(fileHashingFinished(AttachFileItem *))) ;
    }
    mAttachments.push_back(file);

    if (mCheckAttachment)
    {
        checkAttachmentReady();
    }
}

void MessageComposer::fileHashingFinished(AttachFileItem* file)
{
    std::cerr << "MessageComposer::fileHashingFinished() started.";
    std::cerr << std::endl;

    //check that the file is ok tos end
    if (file->getState() == AFI_STATE_ERROR) {
#ifdef CHAT_DEBUG
        std::cerr << "MessageComposer::fileHashingFinished error file is not hashed.";
#endif
        return;
    }

    RetroShareLink message(QString::fromUtf8(file->FileName().c_str()), file->FileSize(), QString::fromStdString(file->FileHash()));
#ifdef CHAT_DEBUG
    std::cerr << "MessageComposer::anchorClicked message : " << message.toHtmlFull().toStdString() << std::endl;
#endif

    ui.msgText->textCursor().insertHtml(message.toHtmlFull() + QString("<br>"));
    ui.msgText->setFocus( Qt::OtherFocusReason );
}

void MessageComposer::checkAttachmentReady()
{
    std::list<AttachFileItem *>::iterator fit;

    mCheckAttachment = false;

    for(fit = mAttachments.begin(); fit != mAttachments.end(); fit++)
    {
        if (!(*fit)->isHidden())
        {
            if (!(*fit)->ready())
            {
                /*
				 */
                ui.actionSend->setEnabled(false);
                break;
            }
        }
    }

    if (fit == mAttachments.end())
    {
        ui.actionSend->setEnabled(true);
    }

    /* repeat... */
    int msec_rate = 1000;
    QTimer::singleShot( msec_rate, this, SLOT(checkAttachmentReady(void)));
}

/* clear Filter */
void MessageComposer::clearFilter()
{
    ui.filterPatternLineEdit->clear();
    ui.filterPatternLineEdit->setFocus();
}

void MessageComposer::filterRegExpChanged()
{
    QString text = ui.filterPatternLineEdit->text();

    if (text.isEmpty()) {
        ui.clearButton->hide();
    } else {
        ui.clearButton->show();
    }

    FilterItems();
}

void MessageComposer::FilterItems()
{
    QString sPattern = ui.filterPatternLineEdit->text();

    int nCount = ui.msgSendList->topLevelItemCount ();
    for (int nIndex = 0; nIndex < nCount; nIndex++) {
        FilterItem(ui.msgSendList->topLevelItem(nIndex), sPattern);
    }
}

bool MessageComposer::FilterItem(QTreeWidgetItem *pItem, QString &sPattern)
{
    bool bVisible = true;

    if (sPattern.isEmpty() == false) {
        if (pItem->text(0).contains(sPattern, Qt::CaseInsensitive) == false) {
            bVisible = false;
        }
    }

    int nVisibleChildCount = 0;
    int nCount = pItem->childCount();
    for (int nIndex = 0; nIndex < nCount; nIndex++) {
        if (FilterItem(pItem->child(nIndex), sPattern)) {
            nVisibleChildCount++;
        }
    }

    if (bVisible || nVisibleChildCount) {
        pItem->setHidden(false);
    } else {
        pItem->setHidden(true);
    }

    return (bVisible || nVisibleChildCount);
}

void MessageComposer::btnClickEvent()
{
    enumType type;
    if (QObject::sender() == ui.addToButton || QObject::sender() == ui.msgSendList) {
        type = TO;
    } else if (QObject::sender() == ui.addCcButton) {
        type = CC;
    } else if (QObject::sender() == ui.addBccButton) {
        type = BCC;
    } else {
        return;
    }

    QTreeWidgetItemIterator itemIterator(ui.msgSendList);
    QTreeWidgetItem *item;
    while ((item = *itemIterator) != NULL) {
        itemIterator++;

        if (item->isSelected()) {
            std::string id = item->data(COLUMN_CONTACT_DATA, ROLE_CONTACT_ID).toString().toStdString();
            bool group = (item->type() == TYPE_GROUP);
            addRecipient(type, id, group);
        }
    }
}

void MessageComposer::recommendButtonClicked()
{
    std::list<std::string> gpgIds;

    QTreeWidgetItemIterator itemIterator(ui.msgSendList);
    QTreeWidgetItem *item;
    while ((item = *itemIterator) != NULL) {
        itemIterator++;

        if (item->isSelected()) {
            std::string id = item->data(COLUMN_CONTACT_DATA, ROLE_CONTACT_ID).toString().toStdString();
            bool group = (item->type() == TYPE_GROUP);

            if (group) {
                RsGroupInfo groupInfo;
                if (rsPeers->getGroupInfo(id, groupInfo) == false) {
                    continue;
                }

                std::list<std::string>::iterator it;
                for (it = groupInfo.peerIds.begin(); it != groupInfo.peerIds.end(); it++) {
                    if (std::find(gpgIds.begin(), gpgIds.end(), *it) == gpgIds.end()) {
                        gpgIds.push_back(*it);
                    }
                }
            } else {
                RsPeerDetails detail;
                if (rsPeers->getPeerDetails(id, detail) == false) {
                    continue;
                }

                if (std::find(gpgIds.begin(), gpgIds.end(), detail.gpg_id) == gpgIds.end()) {
                    gpgIds.push_back(detail.gpg_id);
                }
            }
        }
    }

    if (gpgIds.empty()) {
        return;
    }

    QString text = BuildRecommendHtml(gpgIds);
    ui.msgText->textCursor().insertHtml(text);
    ui.msgText->setFocus(Qt::OtherFocusReason);
}
