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

#include <QStandardItemModel>
#include <QShortcut>
#include <QTimer>
#include <QDateTime>
#include <QMessageBox>
#include <QKeyEvent>

#include "MessagesDialog.h"
#include "msgs/MessageComposer.h"
#include "msgs/MessageWidget.h"
#include "msgs/TagsMenu.h"
#include "msgs/MessageUserNotify.h"
#include "settings/rsharesettings.h"
#include "common/TagDefs.h"
#include "common/PeerDefs.h"
#include "common/RSItemDelegate.h"
#include "gui/gxs/GxsIdTreeWidgetItem.h"
#include "util/DateTime.h"
#include "util/RsProtectedTimer.h"
#include "util/QtVersion.h"

#include <retroshare/rspeers.h>
#include <retroshare/rsmsgs.h>

#include <algorithm>

/* Images for context menu icons */
#define IMAGE_MESSAGE          ":/images/folder-draft.png"
#define IMAGE_MESSAGEREMOVE    ":/images/message-mail-imapdelete.png"
#define IMAGE_STAR_ON          ":/images/star-on-16.png"
#define IMAGE_STAR_OFF         ":/images/star-off-16.png"
#define IMAGE_SYSTEM           ":/images/user/user_request16.png"
#define IMAGE_DECRYPTMESSAGE  ":/images/decrypt-mail.png"

#define COLUMN_STAR          0
#define COLUMN_ATTACHEMENTS  1
#define COLUMN_SUBJECT       2
#define COLUMN_UNREAD        3
#define COLUMN_FROM          4
#define COLUMN_SIGNATURE     5
#define COLUMN_DATE          6
#define COLUMN_CONTENT       7
#define COLUMN_TAGS          8
#define COLUMN_COUNT         9

#define COLUMN_DATA          0 // column for storing the userdata like msgid and srcid

#define ROLE_SORT     Qt::UserRole
#define ROLE_MSGID    Qt::UserRole + 1
#define ROLE_SRCID    Qt::UserRole + 2
#define ROLE_UNREAD   Qt::UserRole + 3
#define ROLE_MSGFLAGS Qt::UserRole + 4

#define ROLE_QUICKVIEW_TYPE Qt::UserRole
#define ROLE_QUICKVIEW_ID   Qt::UserRole + 1
#define ROLE_QUICKVIEW_TEXT Qt::UserRole + 2

#define QUICKVIEW_TYPE_NOTHING 0
#define QUICKVIEW_TYPE_STATIC  1
#define QUICKVIEW_TYPE_TAG     2

#define QUICKVIEW_STATIC_ID_STARRED 1
#define QUICKVIEW_STATIC_ID_SYSTEM  2

#define ROW_INBOX         0
#define ROW_OUTBOX        1
#define ROW_DRAFTBOX      2
#define ROW_SENTBOX       3
#define ROW_TRASHBOX      4


MessagesDialog::LockUpdate::LockUpdate (MessagesDialog *pDialog, bool bUpdate)
{
    m_pDialog = pDialog;
    m_bUpdate = bUpdate;

    ++m_pDialog->lockUpdate;
}

MessagesDialog::LockUpdate::~LockUpdate ()
{
    if(--m_pDialog->lockUpdate < 0)
        m_pDialog->lockUpdate = 0;

    if (m_bUpdate && m_pDialog->lockUpdate == 0) {
        m_pDialog->insertMessages();
    }
}

void MessagesDialog::LockUpdate::setUpdate(bool bUpdate)
{
    m_bUpdate = bUpdate;
}

/** Constructor */
MessagesDialog::MessagesDialog(QWidget *parent)
: MainPage(parent)
{
    /* Invoke the Qt Designer generated object setup routine */
    ui.setupUi(this);

    inProcessSettings = false;
    inChange = false;
    lockUpdate = 0;

    connect(ui.messageTreeWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(messageTreeWidgetCustomPopupMenu(QPoint)));
    connect(ui.listWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(folderlistWidgetCustomPopupMenu(QPoint)));
    connect(ui.messageTreeWidget, SIGNAL(itemClicked(QTreeWidgetItem*,int)) , this, SLOT(clicked(QTreeWidgetItem*,int)));
    connect(ui.messageTreeWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)) , this, SLOT(doubleClicked(QTreeWidgetItem*,int)));
    connect(ui.messageTreeWidget, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(currentItemChanged(QTreeWidgetItem*)));
    connect(ui.messageTreeWidget->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)), this, SLOT(currentChanged(const QModelIndex&)));
    connect(ui.listWidget, SIGNAL(currentRowChanged(int)), this, SLOT(changeBox(int)));
    connect(ui.quickViewWidget, SIGNAL(currentRowChanged(int)), this, SLOT(changeQuickView(int)));
    connect(ui.tabWidget, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));
    connect(ui.tabWidget, SIGNAL(tabCloseRequested(int)), this, SLOT(tabCloseRequested(int)));
    connect(ui.newmessageButton, SIGNAL(clicked()), this, SLOT(newmessage()));

    connect(ui.actionTextBesideIcon, SIGNAL(triggered()), this, SLOT(buttonStyle()));
    connect(ui.actionIconOnly, SIGNAL(triggered()), this, SLOT(buttonStyle()));
    connect(ui.actionTextUnderIcon, SIGNAL(triggered()), this, SLOT(buttonStyle()));

    ui.actionTextBesideIcon->setData(Qt::ToolButtonTextBesideIcon);
    ui.actionIconOnly->setData(Qt::ToolButtonIconOnly);
    ui.actionTextUnderIcon->setData(Qt::ToolButtonTextUnderIcon);

    connect(ui.filterLineEdit, SIGNAL(textChanged(QString)), this, SLOT(filterChanged(QString)));
    connect(ui.filterLineEdit, SIGNAL(filterChanged(int)), this, SLOT(filterColumnChanged(int)));

    msgWidget = new MessageWidget(true, this);
	ui.msgLayout->addWidget(msgWidget);

    connectActions();

    listMode = LIST_NOTHING;

    mCurrMsgId  = "";

    // Set the QStandardItemModel
    ui.messageTreeWidget->setColumnCount(COLUMN_COUNT);
    QTreeWidgetItem *headerItem = ui.messageTreeWidget->headerItem();
    headerItem->setText(COLUMN_ATTACHEMENTS, "");
    headerItem->setIcon(COLUMN_ATTACHEMENTS, QIcon(":/images/attachment.png"));
    headerItem->setText(COLUMN_SUBJECT,      tr("Subject"));
    headerItem->setText(COLUMN_UNREAD,       "");
    headerItem->setIcon(COLUMN_UNREAD,       QIcon(":/images/message-state-header.png"));
    headerItem->setText(COLUMN_FROM,         tr("From"));
    headerItem->setText(COLUMN_SIGNATURE,    "");
    headerItem->setIcon(COLUMN_SIGNATURE,    QIcon(":/images/signature.png"));
    headerItem->setText(COLUMN_DATE,         tr("Date"));
    headerItem->setText(COLUMN_TAGS,         tr("Tags"));
    headerItem->setText(COLUMN_CONTENT,      tr("Content"));
    headerItem->setText(COLUMN_STAR,         "");
    headerItem->setIcon(COLUMN_STAR,         QIcon(IMAGE_STAR_ON));

    headerItem->setToolTip(COLUMN_ATTACHEMENTS, tr("Click to sort by attachments"));
    headerItem->setToolTip(COLUMN_SUBJECT,      tr("Click to sort by subject"));
    headerItem->setToolTip(COLUMN_UNREAD,       tr("Click to sort by read"));
    headerItem->setToolTip(COLUMN_FROM,         tr("Click to sort by from"));
    headerItem->setToolTip(COLUMN_SIGNATURE,    tr("Click to sort by signature"));
    headerItem->setToolTip(COLUMN_DATE,         tr("Click to sort by date"));
    headerItem->setToolTip(COLUMN_TAGS,         tr("Click to sort by tags"));
    headerItem->setToolTip(COLUMN_STAR,         tr("Click to sort by star"));

    mMessageCompareRole = new RSTreeWidgetItemCompareRole;
    mMessageCompareRole->setRole(COLUMN_SUBJECT, ROLE_SORT);
    mMessageCompareRole->setRole(COLUMN_UNREAD, ROLE_SORT);
    mMessageCompareRole->setRole(COLUMN_FROM, ROLE_SORT);
    mMessageCompareRole->setRole(COLUMN_DATE, ROLE_SORT);
    mMessageCompareRole->setRole(COLUMN_TAGS, ROLE_SORT);
    mMessageCompareRole->setRole(COLUMN_ATTACHEMENTS, ROLE_SORT);
    mMessageCompareRole->setRole(COLUMN_STAR, ROLE_SORT);

    RSItemDelegate *itemDelegate = new RSItemDelegate(this);
    itemDelegate->setSpacing(QSize(0, 2));
    ui.messageTreeWidget->setItemDelegate(itemDelegate);

    ui.messageTreeWidget->sortByColumn(COLUMN_DATA, Qt::DescendingOrder);

    // workaround for Qt bug, should be solved in next Qt release 4.7.0
    // http://bugreports.qt.nokia.com/browse/QTBUG-8270
    QShortcut *Shortcut = new QShortcut(QKeySequence (Qt::Key_Delete), ui.messageTreeWidget, 0, 0, Qt::WidgetShortcut);
    connect(Shortcut, SIGNAL(activated()), this, SLOT( removemessage ()));
    Shortcut = new QShortcut(QKeySequence (Qt::SHIFT | Qt::Key_Delete), ui.messageTreeWidget, 0, 0, Qt::WidgetShortcut);
    connect(Shortcut, SIGNAL(activated()), this, SLOT( removemessage ()));

    /* Set initial section sizes */
    QHeaderView * msgwheader = ui.messageTreeWidget->header () ;
    msgwheader->resizeSection (COLUMN_ATTACHEMENTS, 24);
    msgwheader->resizeSection (COLUMN_SUBJECT,      250);
    msgwheader->resizeSection (COLUMN_FROM,         140);
    msgwheader->resizeSection (COLUMN_SIGNATURE,    24);
    msgwheader->resizeSection (COLUMN_DATE,         140);

    QHeaderView_setSectionResizeMode(msgwheader, COLUMN_STAR, QHeaderView::Fixed);
    msgwheader->resizeSection (COLUMN_STAR, 24);

    QHeaderView_setSectionResizeMode(msgwheader, COLUMN_UNREAD, QHeaderView::Fixed);
    msgwheader->resizeSection (COLUMN_UNREAD, 24);

    ui.forwardmessageButton->setToolTip(tr("Forward selected Message"));
    ui.replyallmessageButton->setToolTip(tr("Reply to All"));

    QMenu *printmenu = new QMenu();
    printmenu->addAction(ui.actionPrint);
    printmenu->addAction(ui.actionPrintPreview);
    ui.printbutton->setMenu(printmenu);

    QMenu *viewmenu = new QMenu();
    viewmenu->addAction(ui.actionTextBesideIcon);
    viewmenu->addAction(ui.actionIconOnly);
    ui.viewtoolButton->setMenu(viewmenu);
    
    // Set initial size of the splitter
    ui.listSplitter->setStretchFactor(0, 0);
    ui.listSplitter->setStretchFactor(1, 1);
    

    /* add filter actions */
    ui.filterLineEdit->addFilter(QIcon(), tr("Subject"), COLUMN_SUBJECT, tr("Search Subject"));
    ui.filterLineEdit->addFilter(QIcon(), tr("From"), COLUMN_FROM, tr("Search From"));
    ui.filterLineEdit->addFilter(QIcon(), tr("Date"), COLUMN_DATE, tr("Search Date"));
    ui.filterLineEdit->addFilter(QIcon(), tr("Content"), COLUMN_CONTENT, tr("Search Content"));
    ui.filterLineEdit->addFilter(QIcon(), tr("Tags"), COLUMN_TAGS, tr("Search Tags"));
    ui.filterLineEdit->addFilter(QIcon(), tr("Attachments"), COLUMN_ATTACHEMENTS, tr("Search Attachments"));

    //setting default filter by column as subject
    ui.filterLineEdit->setCurrentFilter(COLUMN_SUBJECT);

    // load settings
    processSettings(true);

    /* Set header sizes for the fixed columns and resize modes, must be set after processSettings */
    QHeaderView_setSectionResizeMode(msgwheader, COLUMN_ATTACHEMENTS, QHeaderView::Fixed);
    QHeaderView_setSectionResizeMode(msgwheader, COLUMN_DATE, QHeaderView::Interactive);
    QHeaderView_setSectionResizeMode(msgwheader, COLUMN_UNREAD, QHeaderView::Fixed);
    QHeaderView_setSectionResizeMode(msgwheader, COLUMN_SIGNATURE, QHeaderView::Fixed);
    msgwheader->resizeSection (COLUMN_UNREAD, 24);
    msgwheader->resizeSection (COLUMN_SIGNATURE, 24);
    msgwheader->resizeSection (COLUMN_STAR, 24);
    QHeaderView_setSectionResizeMode(msgwheader, COLUMN_STAR, QHeaderView::Fixed);
    msgwheader->setStretchLastSection(false);

    // fill folder list
    updateMessageSummaryList();
    ui.listWidget->setCurrentRow(ROW_INBOX);

    // create tag menu
    TagsMenu *menu = new TagsMenu (tr("Tags"), this);
    connect(menu, SIGNAL(aboutToShow()), this, SLOT(tagAboutToShow()));
    connect(menu, SIGNAL(tagSet(int, bool)), this, SLOT(tagSet(int, bool)));
    connect(menu, SIGNAL(tagRemoveAll()), this, SLOT(tagRemoveAll()));

    ui.tagButton->setMenu(menu);

    // fill quick view
    fillQuickView();

    // create timer for navigation
    timer = new RsProtectedTimer(this);
    timer->setInterval(300);
    timer->setSingleShot(true);
    connect(timer, SIGNAL(timeout()), this, SLOT(updateCurrentMessage()));

    ui.messageTreeWidget->installEventFilter(this);

    // remove close button of the the first tab
    ui.tabWidget->hideCloseButton(0);

  /* Hide platform specific features */
#ifdef Q_WS_WIN

#endif

 QString help_str = tr(
 " <h1><img width=\"32\" src=\":/images/64px_help.png\">&nbsp;&nbsp;Messages</h1>                         \
 <p>Retroshare has its own internal email system. You can send/receive emails to/from connected friend nodes.</p> \
 <p>It is also possible to send messages to other people's Identities using the global routing system. These messages \
 	are always encrypted and are relayed by intermediate nodes until they reach their final destination. </p>\
	<p>It is recommended to cryptographically sign distant messages, as a proof of your identity, using \
	the <img width=\"16\" src=\":/images/stock_signature_ok.png\"/> button \
 	in the message composer window. Distant messages stay into your Outbox until an acknowledgement of receipt has been received.</p>\
 <p>Generally, you may use messages to recommend files to your friends by pasting file links, \
 or recommend friend nodes to other friends nodes, in order to strenghten your network, or send feedback \
 to a channel's owner.</p>                   \
 ") ;

	 registerHelpButton(ui.helpButton,help_str) ;
}

MessagesDialog::~MessagesDialog()
{
    // stop and delete timer
    timer->stop();
    delete(timer);

    // save settings
    processSettings(false);
}

UserNotify *MessagesDialog::getUserNotify(QObject *parent)
{
    return new MessageUserNotify(parent);
}

void MessagesDialog::processSettings(bool load)
{
    int messageTreeVersion = 2; // version number for the settings to solve problems when modifying the column count

    inProcessSettings = true;

    QHeaderView *msgwheader = ui.messageTreeWidget->header () ;

    Settings->beginGroup("MessageDialog");

    if (load) {
        // load settings

        // filterColumn
        ui.filterLineEdit->setCurrentFilter(Settings->value("filterColumn", COLUMN_SUBJECT).toInt());

        // state of message tree
        if (Settings->value("MessageTreeVersion").toInt() == messageTreeVersion) {
            msgwheader->restoreState(Settings->value("MessageTree").toByteArray());
        }

        // state of splitter
        ui.msgSplitter->restoreState(Settings->value("Splitter").toByteArray());
        ui.msgSplitter_2->restoreState(Settings->value("Splitter2").toByteArray());
        ui.listSplitter->restoreState(Settings->value("Splitter3").toByteArray());

        /* toolbar button style */
        Qt::ToolButtonStyle style = (Qt::ToolButtonStyle) Settings->value("ToolButon_Style", Qt::ToolButtonIconOnly).toInt();
        setToolbarButtonStyle(style);
    } else {
        // save settings

        // state of message tree
        Settings->setValue("MessageTree", msgwheader->saveState());
        Settings->setValue("MessageTreeVersion", messageTreeVersion);

        // state of splitter
        Settings->setValue("Splitter", ui.msgSplitter->saveState());
        Settings->setValue("Splitter2", ui.msgSplitter_2->saveState());
        Settings->setValue("Splitter3", ui.listSplitter->saveState());

        /* toolbar button style */
        Settings->setValue("ToolButon_Style", ui.newmessageButton->toolButtonStyle());
    }

    Settings->endGroup();

    if (msgWidget) {
        msgWidget->processSettings("MessageDialog", load);
    }

    inProcessSettings = false;
}

bool MessagesDialog::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui.messageTreeWidget) {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent && keyEvent->key() == Qt::Key_Space) {
                // Space pressed
                clicked(ui.messageTreeWidget->currentItem(), COLUMN_UNREAD);
                return true; // eat event
            }
        }
    }
    // pass the event on to the parent class
    return MainPage::eventFilter(obj, event);
}

void MessagesDialog::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::StyleChange:
        insertMessages();
        break;
    default:
        // remove compiler warnings
        break;
    }
}

void MessagesDialog::fillQuickView()
{
	MsgTagType tags;
	rsMsgs->getMessageTagTypes(tags);
	std::map<uint32_t, std::pair<std::string, uint32_t> >::iterator tag;

	// fill tags
	inChange = true;

	// save current selection
	QListWidgetItem *item = ui.quickViewWidget->currentItem();
	int selectedType = 0;
	uint32_t selectedId = 0;
	if (item) {
		selectedType = item->data(ROLE_QUICKVIEW_TYPE).toInt();
		selectedId = item->data(ROLE_QUICKVIEW_ID).toInt();
	}

	QListWidgetItem *itemToSelect = NULL;
	QString text;

	ui.quickViewWidget->clear();

	// add static items
	item = new QListWidgetItem(tr("Starred"), ui.quickViewWidget);
	item->setIcon(QIcon(IMAGE_STAR_ON));
	item->setData(ROLE_QUICKVIEW_TYPE, QUICKVIEW_TYPE_STATIC);
	item->setData(ROLE_QUICKVIEW_ID, QUICKVIEW_STATIC_ID_STARRED);
	item->setData(ROLE_QUICKVIEW_TEXT, item->text()); // for updateMessageSummaryList

	if (selectedType == QUICKVIEW_TYPE_STATIC && selectedId == QUICKVIEW_STATIC_ID_STARRED) {
		itemToSelect = item;
	}

	item = new QListWidgetItem(tr("System"), ui.quickViewWidget);
	item->setIcon(QIcon(IMAGE_SYSTEM));
	item->setData(ROLE_QUICKVIEW_TYPE, QUICKVIEW_TYPE_STATIC);
	item->setData(ROLE_QUICKVIEW_ID, QUICKVIEW_STATIC_ID_SYSTEM);
	item->setData(ROLE_QUICKVIEW_TEXT, item->text()); // for updateMessageSummaryList

	if (selectedType == QUICKVIEW_TYPE_STATIC && selectedId == QUICKVIEW_STATIC_ID_SYSTEM) {
		itemToSelect = item;
	}

	for (tag = tags.types.begin(); tag != tags.types.end(); ++tag) {
		text = TagDefs::name(tag->first, tag->second.first);

		item = new QListWidgetItem (text, ui.quickViewWidget);
		item->setForeground(QBrush(QColor(tag->second.second)));
		item->setIcon(QIcon(":/images/foldermail.png"));
		item->setData(ROLE_QUICKVIEW_TYPE, QUICKVIEW_TYPE_TAG);
		item->setData(ROLE_QUICKVIEW_ID, tag->first);
		item->setData(ROLE_QUICKVIEW_TEXT, text); // for updateMessageSummaryList

		if (selectedType == QUICKVIEW_TYPE_TAG && tag->first == selectedId) {
			itemToSelect = item;
		}
	}

	if (itemToSelect) {
		ui.quickViewWidget->setCurrentItem(itemToSelect);
	}

	inChange = false;

	updateMessageSummaryList();
}

// replaced by shortcut
//void MessagesDialog::keyPressEvent(QKeyEvent *e)
//{
//	if(e->key() == Qt::Key_Delete)
//	{
//		removemessage() ;
//		e->accept() ;
//	}
//	else
//		MainPage::keyPressEvent(e) ;
//}

int MessagesDialog::getSelectedMsgCount (QList<QTreeWidgetItem*> *items, QList<QTreeWidgetItem*> *itemsRead, QList<QTreeWidgetItem*> *itemsUnread, QList<QTreeWidgetItem*> *itemsStar)
{
    if (items) items->clear();
    if (itemsRead) itemsRead->clear();
    if (itemsUnread) itemsUnread->clear();
    if (itemsStar) itemsStar->clear();

    //To check if the selection has more than one row.
    QList<QTreeWidgetItem*> selectedItems = ui.messageTreeWidget->selectedItems();
    foreach (QTreeWidgetItem *item, selectedItems)
    {
        if (items || itemsRead || itemsUnread || itemsStar) {
            if (items) items->append(item);

            if (item->data(COLUMN_DATA, ROLE_UNREAD).toBool()) {
                if (itemsUnread) itemsUnread->append(item);
            } else {
                if (itemsRead) itemsRead->append(item);
            }

            if (itemsStar) {
                if (item->data(COLUMN_DATA, ROLE_MSGFLAGS).toInt() & RS_MSG_STAR) {
                    itemsStar->append(item);
                }
            }
        }
    }

    return selectedItems.size();
}

bool MessagesDialog::isMessageRead(QTreeWidgetItem *item)
{
    if (!item) {
        return true;
    }

    return !item->data(COLUMN_DATA, ROLE_UNREAD).toBool();
}

bool MessagesDialog::hasMessageStar(QTreeWidgetItem *item)
{
    if (!item) {
        return false;
    }

    return item->data(COLUMN_DATA, ROLE_MSGFLAGS).toInt() & RS_MSG_STAR;
}

void MessagesDialog::messageTreeWidgetCustomPopupMenu(QPoint /*point*/)
{
    std::string cid;
    std::string mid;

    MessageInfo msgInfo;
    if (getCurrentMsg(cid, mid)) {
        rsMsgs->getMessage(mid, msgInfo);
    }

    QList<QTreeWidgetItem*> itemsRead;
    QList<QTreeWidgetItem*> itemsUnread;
    QList<QTreeWidgetItem*> itemsStar;
    int nCount = getSelectedMsgCount (NULL, &itemsRead, &itemsUnread, &itemsStar);

    /** Defines the actions for the context menu */

    QMenu contextMnu( this );

    QAction *action = contextMnu.addAction(QIcon(":/images/view_split_top_bottom.png"), tr("Open in a new window"), this, SLOT(openAsWindow()));
    if (nCount != 1) {
        action->setDisabled(true);
    }

    action = contextMnu.addAction(QIcon(":/images/tab-dock.png"), tr("Open in a new tab"), this, SLOT(openAsTab()));
    if (nCount != 1) {
        action->setDisabled(true);
    }

    contextMnu.addSeparator();

    contextMnu.addAction(ui.actionReply);
    ui.actionReply->setEnabled(nCount == 1);

    contextMnu.addAction(ui.actionReplyAll);
    ui.actionReplyAll->setEnabled(nCount == 1);

    contextMnu.addAction(ui.actionForward);
    ui.actionForward->setEnabled(nCount == 1);

    contextMnu.addSeparator();

    action = contextMnu.addAction(QIcon(":/images/message-mail-read.png"), tr("Mark as read"), this, SLOT(markAsRead()));
    if (itemsUnread.isEmpty()) {
        action->setDisabled(true);
    }

    action = contextMnu.addAction(QIcon(":/images/message-mail.png"), tr("Mark as unread"), this, SLOT(markAsUnread()));
    if (itemsRead.isEmpty()) {
        action->setDisabled(true);
    }

    action = contextMnu.addAction(tr("Add Star"));
    action->setCheckable(true);
    action->setChecked(itemsStar.size());
    connect(action, SIGNAL(triggered(bool)), this, SLOT(markWithStar(bool)));

    contextMnu.addSeparator();

    // add tags
    contextMnu.addMenu(ui.tagButton->menu());

    contextMnu.addSeparator();

    QString text;
    if ((msgInfo.msgflags & RS_MSG_BOXMASK) == RS_MSG_DRAFTBOX) {
        text = tr("Edit");
    } else {
        text = tr("Edit as new");
    }
    action = contextMnu.addAction(text, this, SLOT(editmessage()));
    if (nCount != 1) {
        action->setDisabled(true);
    }

    action = contextMnu.addAction(QIcon(IMAGE_MESSAGEREMOVE), (nCount > 1) ? tr("Remove Messages") : tr("Remove Message"), this, SLOT(removemessage()));
    if (nCount == 0) {
        action->setDisabled(true);
    }

    int listrow = ui.listWidget->currentRow();
    if (listrow == ROW_TRASHBOX) {
        action = contextMnu.addAction(tr("Undelete"), this, SLOT(undeletemessage()));
        if (nCount == 0) {
            action->setDisabled(true);
        }
    }

    contextMnu.addAction(ui.actionSaveAs);
    contextMnu.addAction(ui.actionPrintPreview);
    contextMnu.addAction(ui.actionPrint);
    contextMnu.addSeparator();

    contextMnu.addAction(QIcon(IMAGE_MESSAGE), tr("New Message"), this, SLOT(newmessage()));

    contextMnu.exec(QCursor::pos());
}

void MessagesDialog::folderlistWidgetCustomPopupMenu(QPoint /*point*/)
{
    if (ui.listWidget->currentRow() != ROW_TRASHBOX) {
        /* Context menu only neede for trash box */
        return;
    }

    QMenu contextMnu(this);

    contextMnu.addAction(tr("Empty trash"), this, SLOT(emptyTrash()));

    contextMnu.exec(QCursor::pos());
}

void MessagesDialog::newmessage()
{
    MessageComposer *nMsgDialog = MessageComposer::newMsg();
    if (nMsgDialog == NULL) {
        return;
    }

    nMsgDialog->show();
    nMsgDialog->activateWindow();

    /* window will destroy itself! */
}

void MessagesDialog::openAsWindow()
{
    std::string cid;
    std::string mid;

    if(!getCurrentMsg(cid, mid))
        return ;

    MessageWidget *msgWidget = MessageWidget::openMsg(mid, true);
    if (msgWidget == NULL) {
        return;
    }

    msgWidget->activateWindow();

    /* window will destroy itself! */
}

void MessagesDialog::openAsTab()
{
    std::string cid;
    std::string mid;

    if(!getCurrentMsg(cid, mid))
        return ;

    MessageWidget *msgWidget = MessageWidget::openMsg(mid, false);
    if (msgWidget == NULL) {
        return;
    }

    ui.tabWidget->addTab(msgWidget, msgWidget->subject(true));
    ui.tabWidget->setCurrentWidget(msgWidget);

    /* window will destroy itself! */
}

void MessagesDialog::editmessage()
{
    std::string cid;
    std::string mid;

    if(!getCurrentMsg(cid, mid))
        return ;

    MessageComposer *msgComposer = MessageComposer::newMsg(mid);
    if (msgComposer == NULL) {
        return;
    }

    msgComposer->show();
    msgComposer->activateWindow();

    /* window will destroy itself! */
}

void MessagesDialog::changeBox(int)
{
    if (inChange) {
        // already in change method
        return;
    }

    inChange = true;

    ui.messageTreeWidget->clear();

    ui.quickViewWidget->setCurrentItem(NULL);
    listMode = LIST_BOX;

    insertMessages();
    insertMsgTxtAndFiles(ui.messageTreeWidget->currentItem());

    inChange = false;
}

void MessagesDialog::changeQuickView(int newrow)
{
    Q_UNUSED(newrow);

    if (inChange) {
        // already in change method
        return;
    }

    inChange = true;

    ui.messageTreeWidget->clear();

    ui.listWidget->setCurrentItem(NULL);
    listMode = LIST_QUICKVIEW;

    insertMessages();
    insertMsgTxtAndFiles(ui.messageTreeWidget->currentItem());

    inChange = false;
}

void MessagesDialog::messagesTagsChanged()
{
    if (lockUpdate) {
        return;
    }

    fillQuickView();
    insertMessages();
}

static void InitIconAndFont(QTreeWidgetItem *item)
{
    int msgFlags = item->data(COLUMN_DATA, ROLE_MSGFLAGS).toInt();

    // show the real "New" state
    if (msgFlags & RS_MSG_NEW) {
        item->setIcon(COLUMN_SUBJECT, QIcon(":/images/message-state-new.png"));
    } else {
        if (msgFlags & RS_MSG_USER_REQUEST) {
            item->setIcon(COLUMN_SUBJECT, QIcon(":/images/user/user_request16.png"));
        } else if (msgFlags & RS_MSG_FRIEND_RECOMMENDATION) {
            item->setIcon(COLUMN_SUBJECT, QIcon(":/images/user/friend_suggestion16.png"));
        } else if (msgFlags & RS_MSG_PUBLISH_KEY) {
            item->setIcon(COLUMN_SUBJECT, QIcon(":/images/share-icon-16.png"));
        } else if (msgFlags & RS_MSG_UNREAD_BY_USER) {
            if ((msgFlags & (RS_MSG_REPLIED | RS_MSG_FORWARDED)) == RS_MSG_REPLIED) {
                item->setIcon(COLUMN_SUBJECT, QIcon(":/images/message-mail-replied.png"));
            } else if ((msgFlags & (RS_MSG_REPLIED | RS_MSG_FORWARDED)) == RS_MSG_FORWARDED) {
                item->setIcon(COLUMN_SUBJECT, QIcon(":/images/message-mail-forwarded.png"));
            } else if ((msgFlags & (RS_MSG_REPLIED | RS_MSG_FORWARDED)) == (RS_MSG_REPLIED | RS_MSG_FORWARDED)) {
                item->setIcon(COLUMN_SUBJECT, QIcon(":/images/message-mail-replied-forw.png"));
            } else {
                item->setIcon(COLUMN_SUBJECT, QIcon(":/images/message-mail.png"));
            }
        } else {
            if ((msgFlags & (RS_MSG_REPLIED | RS_MSG_FORWARDED)) == RS_MSG_REPLIED) {
                item->setIcon(COLUMN_SUBJECT, QIcon(":/images/message-mail-replied-read.png"));
            } else if ((msgFlags & (RS_MSG_REPLIED | RS_MSG_FORWARDED)) == RS_MSG_FORWARDED) {
                item->setIcon(COLUMN_SUBJECT, QIcon(":/images/message-mail-forwarded-read.png"));
            } else if ((msgFlags & (RS_MSG_REPLIED | RS_MSG_FORWARDED)) == (RS_MSG_REPLIED | RS_MSG_FORWARDED)) {
                item->setIcon(COLUMN_SUBJECT, QIcon(":/images/message-mail-replied-forw-read.png"));
            } else {
                item->setIcon(COLUMN_SUBJECT, QIcon(":/images/message-mail-read.png"));
            }
        }
    }

    if (msgFlags & RS_MSG_STAR) {
        item->setIcon(COLUMN_STAR, QIcon(IMAGE_STAR_ON));
        item->setData(COLUMN_STAR, ROLE_SORT, 1);
    } else {
        item->setIcon(COLUMN_STAR, QIcon(IMAGE_STAR_OFF));
        item->setData(COLUMN_STAR, ROLE_SORT, 0);
    }

    bool isNew = msgFlags & (RS_MSG_NEW | RS_MSG_UNREAD_BY_USER);

    // set icon
    if (isNew) {
        item->setIcon(COLUMN_UNREAD, QIcon(":/images/message-state-unread.png"));
        item->setData(COLUMN_UNREAD, ROLE_SORT, 1);
    } else {
        item->setIcon(COLUMN_UNREAD, QIcon(":/images/message-state-read.png"));
        item->setData(COLUMN_UNREAD, ROLE_SORT, 0);
    }

    // set font
    for (int i = 0; i < COLUMN_COUNT; ++i) {
        QFont qf = item->font(i);
        qf.setBold(isNew);
        item->setFont(i, qf);
    }

    item->setData(COLUMN_DATA, ROLE_UNREAD, isNew);
}

void MessagesDialog::insertMessages()
{
    if (lockUpdate) {
        return;
    }

    std::cerr <<"MessagesDialog::insertMessages called";

    std::list<MsgInfoSummary> msgList;
    std::list<MsgInfoSummary>::const_iterator it;
    MessageInfo msgInfo;
    bool gotInfo;
    QString text;

    RsPeerId ownId = rsPeers->getOwnId();

    rsMsgs -> getMessageSummaries(msgList);

    std::cerr << "MessagesDialog::insertMessages()" << std::endl;

    int filterColumn = ui.filterLineEdit->currentFilter();

    /* check the mode we are in */
    unsigned int msgbox = 0;
    bool isTrash = false;
    bool doFill = true;
    int quickViewType = 0;
    uint32_t quickViewId = 0;
    QString boxText;
    QIcon boxIcon;
    QString placeholderText;

    switch (listMode) {
    case LIST_NOTHING:
        doFill = false;
        break;

    case LIST_BOX:
        {
            QListWidgetItem *listItem = ui.listWidget->currentItem();
            if (listItem) {
                boxIcon = listItem->icon();
            }

            int listrow = ui.listWidget->currentRow();

            switch (listrow) {
            case ROW_INBOX:
                msgbox = RS_MSG_INBOX;
                boxText = tr("Inbox");
                break;
            case ROW_OUTBOX:
                msgbox = RS_MSG_OUTBOX;
                boxText = tr("Outbox");
                break;
            case ROW_DRAFTBOX:
                msgbox = RS_MSG_DRAFTBOX;
                boxText = tr("Drafts");
                break;
            case ROW_SENTBOX:
                msgbox = RS_MSG_SENTBOX;
                boxText = tr("Sent");
                break;
            case ROW_TRASHBOX:
                isTrash = true;
                boxText = tr("Trash");
                break;
            default:
                doFill = false;
            }
        }
        break;

   case LIST_QUICKVIEW:
        {
            QListWidgetItem *listItem = ui.quickViewWidget->currentItem();
            if (listItem) {
                quickViewType = listItem->data(ROLE_QUICKVIEW_TYPE).toInt();
                quickViewId = listItem->data(ROLE_QUICKVIEW_ID).toInt();

                boxText = listItem->text();
                boxIcon = listItem->icon();

                switch (quickViewType) {
                case QUICKVIEW_TYPE_NOTHING:
                    doFill = false;
                    break;
                case QUICKVIEW_TYPE_STATIC:
                    switch (quickViewId) {
                    case QUICKVIEW_STATIC_ID_STARRED:
                        placeholderText = tr("No starred messages available. Stars let you give messages a special status to make them easier to find. To star a message, click on the light gray star beside any message.");
                        break;
                    case QUICKVIEW_STATIC_ID_SYSTEM:
                        placeholderText = tr("No system messages available.");
                        break;
                    }
                    break;
                case QUICKVIEW_TYPE_TAG:
                    break;
                }
            } else {
                doFill = false;
            }
        }
        break;

    default:
        doFill = false;
    }

    ui.tabWidget->setTabText (0, boxText);
    ui.tabWidget->setTabIcon (0, boxIcon);
    ui.messageTreeWidget->setPlaceholderText(placeholderText);

    QTreeWidgetItem *headerItem = ui.messageTreeWidget->headerItem();
    if (msgbox == RS_MSG_INBOX) {
        headerItem->setText(COLUMN_FROM, tr("From"));
        headerItem->setToolTip(COLUMN_FROM, tr("Click to sort by from"));
    } else {
        headerItem->setText(COLUMN_FROM, tr("To"));
        headerItem->setToolTip(COLUMN_FROM, tr("Click to sort by to"));
    }

    if (doFill) {
        MsgTagType Tags;
        rsMsgs->getMessageTagTypes(Tags);

        /* search messages */
        std::list<MsgInfoSummary> msgToShow;
        for(it = msgList.begin(); it != msgList.end(); ++it) {
            if (listMode == LIST_BOX) {
                if (isTrash) {
                    if ((it->msgflags & RS_MSG_TRASH) == 0) {
                        continue;
                    }
                } else {
                    if (it->msgflags & RS_MSG_TRASH) {
                        continue;
                    }
                    if ((it->msgflags & RS_MSG_BOXMASK) != msgbox) {
                        continue;
                    }
                }
            } else if (listMode == LIST_QUICKVIEW && quickViewType == QUICKVIEW_TYPE_TAG) {
                MsgTagInfo tagInfo;
                rsMsgs->getMessageTag(it->msgId, tagInfo);
                if (std::find(tagInfo.tagIds.begin(), tagInfo.tagIds.end(), quickViewId) == tagInfo.tagIds.end()) {
                    continue;
                }
            } else if (listMode == LIST_QUICKVIEW && quickViewType == QUICKVIEW_TYPE_STATIC) {
                if (quickViewId == QUICKVIEW_STATIC_ID_STARRED && (it->msgflags & RS_MSG_STAR) == 0) {
                    continue;
                }
                if (quickViewId == QUICKVIEW_STATIC_ID_SYSTEM && (it->msgflags & RS_MSG_SYSTEM) == 0) {
                    continue;
                }
            } else {
                continue;
            }

            msgToShow.push_back(*it);
        }

        /* remove old items */
        QTreeWidgetItemIterator itemIterator(ui.messageTreeWidget);
        QTreeWidgetItem *treeItem;
        while ((treeItem = *itemIterator) != NULL) {
            ++itemIterator;
            std::string msgIdFromRow = treeItem->data(COLUMN_DATA, ROLE_MSGID).toString().toStdString();
            for(it = msgToShow.begin(); it != msgToShow.end(); ++it) {
                if (it->msgId == msgIdFromRow) {
                    break;
                }
            }

            if (it == msgToShow.end ()) {
                delete(treeItem);
            }
        }

        for(it = msgToShow.begin(); it != msgToShow.end(); ++it)
        {
            /* check the message flags, to decide which
             * group it should go in...
             *
             * InBox
             * OutBox
             * Drafts
             * Sent
             *
             * FLAGS = OUTGOING.
             * 	-> Outbox/Drafts/Sent
             * 	  + SENT -> Sent
             *	  + IN_PROGRESS -> Draft.
             *	  + nuffing -> Outbox.
             * FLAGS = INCOMING = (!OUTGOING)
             * 	-> + NEW -> Bold.
             *
             */

            gotInfo = false;
            msgInfo = MessageInfo(); // clear

            // search exisisting items
            QTreeWidgetItemIterator existingItemIterator(ui.messageTreeWidget);
            while ((treeItem = *existingItemIterator) != NULL) {
                ++existingItemIterator;
                if (it->msgId == treeItem->data(COLUMN_DATA, ROLE_MSGID).toString().toStdString()) {
                    break;
                }
            }

            /* make a widget per friend */

            bool insertItem = false;

            GxsIdRSTreeWidgetItem *item;
            if (treeItem) {
                item = dynamic_cast<GxsIdRSTreeWidgetItem*>(treeItem);
                if (!item) {
                    std::cerr << "MessagesDialog::insertMessages() Item is no GxsIdRSTreeWidgetItem" << std::endl;
                    continue;
                }
            } else {
                item = new GxsIdRSTreeWidgetItem(mMessageCompareRole);
                insertItem = true;
            }

            /* So Text should be:
             * (1) Msg / Broadcast
             * (1b) Person / Channel Name
             * (2) Rank
             * (3) Date
             * (4) Title
             * (5) Msg
             * (6) File Count
             * (7) File Total
             */

            QString dateString;
            // Date First.... (for sorting)
            {
                QDateTime qdatetime;
                qdatetime.setTime_t(it->ts);

                // add string to all data
                dateString = qdatetime.toString("_yyyyMMdd_hhmmss");

                //if the mail is on same date show only time.
                if (qdatetime.daysTo(QDateTime::currentDateTime()) == 0)
                {
                    item->setText(COLUMN_DATE, DateTime::formatTime(qdatetime.time()));
                }
                else
                {
                    item->setText(COLUMN_DATE, DateTime::formatDateTime(qdatetime));
                }
                // for sorting
                item->setData(COLUMN_DATE, ROLE_SORT, qdatetime);
            }

            //  From ....
            {
                if (msgbox == RS_MSG_INBOX || msgbox == RS_MSG_OUTBOX) {
                    if ((it->msgflags & RS_MSG_SYSTEM) && it->srcId == ownId) {
                        text = "RetroShare";
                    } else {
                        if (it->msgflags & RS_MSG_DISTANT)
            {
                            // distant message
                            if (gotInfo || rsMsgs->getMessage(it->msgId, msgInfo)) {
                                gotInfo = true;
                                item->setId(RsGxsId(msgInfo.rsgxsid_srcId), COLUMN_FROM);
                            } else {
                                std::cerr << "MessagesDialog::insertMsgTxtAndFiles() Couldn't find Msg" << std::endl;
                            }
                        } else {
                            text = QString::fromUtf8(rsPeers->getPeerName(it->srcId).c_str());
                        }
                    }
                } else {
                    if (gotInfo || rsMsgs->getMessage(it->msgId, msgInfo)) {
                        gotInfo = true;

                        text.clear();

                        for(std::list<RsPeerId>::const_iterator pit = msgInfo.rspeerid_msgto.begin(); pit != msgInfo.rspeerid_msgto.end(); ++pit)
                        {
                            if (!text.isEmpty())
                                text += ", ";

                            std::string peerName = rsPeers->getPeerName(*pit);
                            if (peerName.empty())
                                text += PeerDefs::rsid("", *pit);
                             else
                                text += QString::fromUtf8(peerName.c_str());
                        }
                        for(std::list<RsGxsId>::const_iterator pit = msgInfo.rsgxsid_msgto.begin(); pit != msgInfo.rsgxsid_msgto.end(); ++pit)
                        {
                            if (!text.isEmpty())
                                text += ", ";

                            RsIdentityDetails details;
                            if (rsIdentity->getIdDetails(*pit, details) && !details.mNickname.empty())
                                text += QString::fromUtf8(details.mNickname.c_str()) ;
                            else
                                text += PeerDefs::rsid("", *pit);
                        }
                    } else {
                        std::cerr << "MessagesDialog::insertMsgTxtAndFiles() Couldn't find Msg" << std::endl;
                    }
                }
                if(it->msgflags & RS_MSG_DISTANT)
        {
                    item->setText(COLUMN_FROM, text);
                    item->setData(COLUMN_FROM, ROLE_SORT, text + dateString);
                }
            }

            // Subject
        text = QString::fromUtf8(it->title.c_str());

            item->setText(COLUMN_SUBJECT, text);
            item->setData(COLUMN_SUBJECT, ROLE_SORT, text + dateString);

            // internal data
            QString msgId = QString::fromStdString(it->msgId);
            item->setData(COLUMN_DATA, ROLE_SRCID, QString::fromStdString(it->srcId.toStdString()));
            item->setData(COLUMN_DATA, ROLE_MSGID, msgId);
            item->setData(COLUMN_DATA, ROLE_MSGFLAGS, it->msgflags);

            // Init icon and font
            InitIconAndFont(item);

            // Tags
            MsgTagInfo tagInfo;
            rsMsgs->getMessageTag(it->msgId, tagInfo);

            text.clear();

            // build tag names
            std::map<uint32_t, std::pair<std::string, uint32_t> >::iterator Tag;
            for (std::list<uint32_t>::iterator tagId = tagInfo.tagIds.begin(); tagId != tagInfo.tagIds.end(); ++tagId) {
                if (text.isEmpty() == false) {
                    text += ",";
                }
                Tag = Tags.types.find(*tagId);
                if (Tag != Tags.types.end()) {
                    text += TagDefs::name(Tag->first, Tag->second.first);
                } else {
                    // clean tagId
                    rsMsgs->setMessageTag(it->msgId, *tagId, false);
                }
            }
            item->setText(COLUMN_TAGS, text);
            item->setData(COLUMN_TAGS, ROLE_SORT, text);

            // set color
            QColor color;
            if (tagInfo.tagIds.size()) {
                Tag = Tags.types.find(tagInfo.tagIds.front());
                if (Tag != Tags.types.end()) {
                    color = Tag->second.second;
                } else {
                    // clean tagId
                    rsMsgs->setMessageTag(it->msgId, tagInfo.tagIds.front(), false);
                }
            }
            if (!color.isValid()) {
                color = ui.messageTreeWidget->palette().color(QPalette::Text);
            }
            QBrush brush = QBrush(color);
            for (int i = 0; i < COLUMN_COUNT; ++i) {
                item->setForeground(i, brush);
            }

            // No of Files.
            {
                item->setText(COLUMN_ATTACHEMENTS, QString::number(it->count));
                item->setData(COLUMN_ATTACHEMENTS, ROLE_SORT, item->text(COLUMN_ATTACHEMENTS) + dateString);
                item->setTextAlignment(COLUMN_ATTACHEMENTS, Qt::AlignHCenter);
            }

            if (filterColumn == COLUMN_CONTENT) {
                // need content for filter
                if (gotInfo || rsMsgs->getMessage(it->msgId, msgInfo)) {
                    gotInfo = true;
                    QTextDocument doc;
                    doc.setHtml(QString::fromUtf8(msgInfo.msg.c_str()));
                    item->setText(COLUMN_CONTENT, doc.toPlainText().replace(QString("\n"), QString(" ")));
                } else {
                    std::cerr << "MessagesDialog::insertMsgTxtAndFiles() Couldn't find Msg" << std::endl;
                    item->setText(COLUMN_CONTENT, "");
                }
            }

            else if(it->msgflags & RS_MSG_DISTANT)
            {
                item->setIcon(COLUMN_SIGNATURE, QIcon(":/images/blue_lock_open.png")) ;
                item->setToolTip(COLUMN_SIGNATURE, tr("This message comes from a distant person.")) ;
                item->setIcon(COLUMN_SUBJECT, QIcon(":/images/message-mail-read.png")) ;

                if(it->msgflags & RS_MSG_SIGNED)
                {
                    if(it->msgflags & RS_MSG_SIGNATURE_CHECKS)
                    {
                        item->setIcon(COLUMN_SIGNATURE, QIcon(":/images/stock_signature_ok.png")) ;
                        item->setToolTip(COLUMN_SIGNATURE, tr("This message was signed and the signature checks")) ;
                    }
                    else
                    {
                        item->setIcon(COLUMN_SIGNATURE, QIcon(":/images/stock_signature_bad.png")) ;
                        item->setToolTip(COLUMN_SIGNATURE, tr("This message was signed but the signature doesn't check")) ;
                    }
                }
            }
            else
                item->setIcon(COLUMN_SIGNATURE, QIcon()) ;

            if (insertItem) {
                /* add to the list */
                ui.messageTreeWidget->addTopLevelItem(item);
            }
        }
    } else {
        ui.messageTreeWidget->clear();
    }

    ui.messageTreeWidget->showColumn(COLUMN_ATTACHEMENTS);
    ui.messageTreeWidget->showColumn(COLUMN_SUBJECT);
    ui.messageTreeWidget->showColumn(COLUMN_UNREAD);
    ui.messageTreeWidget->showColumn(COLUMN_FROM);
    ui.messageTreeWidget->showColumn(COLUMN_DATE);
    ui.messageTreeWidget->showColumn(COLUMN_TAGS);
    ui.messageTreeWidget->hideColumn(COLUMN_CONTENT);

    if (!ui.filterLineEdit->text().isEmpty()) {
        ui.messageTreeWidget->filterItems(ui.filterLineEdit->currentFilter(), ui.filterLineEdit->text());
    }

    updateMessageSummaryList();
}

// current row in messageTreeWidget has changed
void MessagesDialog::currentItemChanged(QTreeWidgetItem *item)
{
    timer->stop();

    if (item) {
        timerIndex = ui.messageTreeWidget->indexOfTopLevelItem(item);
        timer->start();
    } else {
        timerIndex = -1;
    }

    updateInterface();
}

// click in messageTreeWidget
void MessagesDialog::clicked(QTreeWidgetItem *item, int column)
{
    if (!item) {
        return;
    }

    switch (column) {
    case COLUMN_UNREAD:
        {
            QList<QTreeWidgetItem*> items;
            items.append(item);
            setMsgAsReadUnread(items, !isMessageRead(item));
            insertMsgTxtAndFiles(item, false);
            updateMessageSummaryList();
            return;
        }
    case COLUMN_STAR:
        {
            QList<QTreeWidgetItem*> items;
            items.append(item);
            setMsgStar(items, !hasMessageStar(item));
            return;
        }
    }

    timer->stop();
    timerIndex = ui.messageTreeWidget->indexOfTopLevelItem(item);

    // show current message directly
    updateCurrentMessage();
}

// double click in messageTreeWidget
void MessagesDialog::doubleClicked(QTreeWidgetItem *item, int column)
{
    /* activate row */
    clicked (item, column);

    std::string cid;
    std::string mid;

    if(!getCurrentMsg(cid, mid))
        return ;

    MessageInfo msgInfo;
    if (!rsMsgs->getMessage(mid, msgInfo)) {
        return;
    }

    if ((msgInfo.msgflags & RS_MSG_BOXMASK) == RS_MSG_DRAFTBOX) {
        editmessage();
        return;
    }

    /* edit message */
    switch (Settings->getMsgOpen()) {
    case RshareSettings::MSG_OPEN_TAB:
        openAsTab();
        break;
    case RshareSettings::MSG_OPEN_WINDOW:
        openAsWindow();
        break;
    }
}

// show current message directly
void MessagesDialog::updateCurrentMessage()
{
    timer->stop();
    insertMsgTxtAndFiles(ui.messageTreeWidget->topLevelItem(timerIndex));
}

void MessagesDialog::setMsgAsReadUnread(const QList<QTreeWidgetItem*> &items, bool read)
{
    LockUpdate Lock (this, false);

    foreach (QTreeWidgetItem *item, items) {
        std::string mid = item->data(COLUMN_DATA, ROLE_MSGID).toString().toStdString();

        if (rsMsgs->MessageRead(mid, !read)) {
            int msgFlag = item->data(COLUMN_DATA, ROLE_MSGFLAGS).toInt();
            msgFlag &= ~RS_MSG_NEW;

            if (read) {
                msgFlag &= ~RS_MSG_UNREAD_BY_USER;
            } else {
                msgFlag |= RS_MSG_UNREAD_BY_USER;
            }

            item->setData(COLUMN_DATA, ROLE_MSGFLAGS, msgFlag);

            InitIconAndFont(item);
        }
    }

    // LockUpdate
}

void MessagesDialog::markAsRead()
{
    QList<QTreeWidgetItem*> itemsUnread;
    getSelectedMsgCount (NULL, NULL, &itemsUnread, NULL);

    setMsgAsReadUnread (itemsUnread, true);
    updateMessageSummaryList();
}

void MessagesDialog::markAsUnread()
{
    QList<QTreeWidgetItem*> itemsRead;
    getSelectedMsgCount (NULL, &itemsRead, NULL, NULL);

    setMsgAsReadUnread (itemsRead, false);
    updateMessageSummaryList();
}

void MessagesDialog::markWithStar(bool checked)
{
    QList<QTreeWidgetItem*> items;
    getSelectedMsgCount (&items, NULL, NULL, NULL);

    setMsgStar(items, checked);
}

void MessagesDialog::setMsgStar(const QList<QTreeWidgetItem*> &items, bool star)
{
    LockUpdate Lock (this, false);

    foreach (QTreeWidgetItem *item, items) {
        std::string mid = item->data(COLUMN_DATA, ROLE_MSGID).toString().toStdString();

        if (rsMsgs->MessageStar(mid, star)) {
            int msgFlag = item->data(COLUMN_DATA, ROLE_MSGFLAGS).toInt();
            msgFlag &= ~RS_MSG_STAR;

            if (star) {
                msgFlag |= RS_MSG_STAR;
            } else {
                msgFlag &= ~RS_MSG_STAR;
            }

            item->setData(COLUMN_DATA, ROLE_MSGFLAGS, msgFlag);

            InitIconAndFont(item);

            Lock.setUpdate(true);
        }
    }

    // LockUpdate
}

void MessagesDialog::insertMsgTxtAndFiles(QTreeWidgetItem *item, bool bSetToRead)
{
    std::cerr << "MessagesDialog::insertMsgTxtAndFiles()" << std::endl;

    /* get its Ids */
    std::string cid;
    std::string mid;

    if (item == NULL) {
        mCurrMsgId.clear();
        msgWidget->fill(mCurrMsgId);
        updateInterface();
        return;
    }
    mid = item->data(COLUMN_DATA, ROLE_MSGID).toString().toStdString();

    int nCount = getSelectedMsgCount (NULL, NULL, NULL, NULL);
    if (nCount == 1) {
        ui.actionSaveAs->setEnabled(true);
        ui.actionPrintPreview->setEnabled(true);
        ui.actionPrint->setEnabled(true);
    } else {
        ui.actionSaveAs->setDisabled(true);
        ui.actionPrintPreview->setDisabled(true);
        ui.actionPrint->setDisabled(true);
    }

    if (mCurrMsgId == mid) {
        // message doesn't changed
        return;
    }

    /* Save the Data.... for later */

    mCurrMsgId = mid;

    MessageInfo msgInfo;
    if (!rsMsgs -> getMessage(mid, msgInfo)) {
        std::cerr << "MessagesDialog::insertMsgTxtAndFiles() Couldn't find Msg" << std::endl;
        return;
    }

    QList<QTreeWidgetItem*> items;
    items.append(item);

    bool bSetToReadOnActive = Settings->getMsgSetToReadOnActivate();

    if (msgInfo.msgflags & RS_MSG_NEW) {
        // set always to read or unread
        if (bSetToReadOnActive == false || bSetToRead == false) {
            // set locally to unread
            setMsgAsReadUnread(items, false);
        } else {
            setMsgAsReadUnread(items, true);
        }
        updateMessageSummaryList();
    } else {
        if ((msgInfo.msgflags & RS_MSG_UNREAD_BY_USER) && bSetToRead && bSetToReadOnActive) {
            // set to read
            setMsgAsReadUnread(items, true);
            updateMessageSummaryList();
        }
    }

    msgWidget->fill(mCurrMsgId);
    updateInterface();
}

bool MessagesDialog::getCurrentMsg(std::string &cid, std::string &mid)
{
    QTreeWidgetItem *item = ui.messageTreeWidget->currentItem();

    /* get its Ids */
    if (!item)
    {
        //If no message is selected. assume first message is selected.
        if (ui.messageTreeWidget->topLevelItemCount() == 0)
        {
            item = ui.messageTreeWidget->topLevelItem(0);
        }
    }

    if (!item) {
        return false;
    }
    cid = item->data(COLUMN_DATA, ROLE_SRCID).toString().toStdString();
    mid = item->data(COLUMN_DATA, ROLE_MSGID).toString().toStdString();
    return true;
}

void MessagesDialog::removemessage()
{
    LockUpdate Lock (this, true);

    QList<QTreeWidgetItem*> selectedItems = ui.messageTreeWidget->selectedItems();

    bool doDelete = false;
    int listrow = ui.listWidget->currentRow();
    if (listrow == ROW_TRASHBOX) {
        doDelete = true;
    } else {
        if (QApplication::keyboardModifiers() & Qt::ShiftModifier) {
            doDelete = true;
        }
    }

    foreach (QTreeWidgetItem *item, selectedItems) {
        QString mid = item->data(COLUMN_DATA, ROLE_MSGID).toString();

        // close tab showing this message
//      closeTab(mid.toStdString());

        if (doDelete) {
            rsMsgs->MessageDelete(mid.toStdString());
        } else {
            rsMsgs->MessageToTrash(mid.toStdString(), true);
        }
    }

    // LockUpdate -> insertMessages();
}

void MessagesDialog::undeletemessage()
{
    LockUpdate Lock (this, true);

    QList<QTreeWidgetItem*> items;
    getSelectedMsgCount (&items, NULL, NULL, NULL);
    foreach (QTreeWidgetItem *item, items) {
        QString mid = item->data(COLUMN_DATA, ROLE_MSGID).toString();
        rsMsgs->MessageToTrash(mid.toStdString(), false);
    }

    // LockUpdate -> insertMessages();
}

void MessagesDialog::setToolbarButtonStyle(Qt::ToolButtonStyle style)
{
    ui.newmessageButton->setToolButtonStyle(style);
    ui.removemessageButton->setToolButtonStyle(style);
    ui.replymessageButton->setToolButtonStyle(style);
    ui.replyallmessageButton->setToolButtonStyle(style);
    ui.forwardmessageButton->setToolButtonStyle(style);
    ui.tagButton->setToolButtonStyle(style);
    ui.printbutton->setToolButtonStyle(style);
    ui.viewtoolButton->setToolButtonStyle(style);
}

void MessagesDialog::buttonStyle()
{
    setToolbarButtonStyle((Qt::ToolButtonStyle) dynamic_cast<QAction*>(sender())->data().toInt());
}

void MessagesDialog::filterChanged(const QString& text)
{
    ui.messageTreeWidget->filterItems(ui.filterLineEdit->currentFilter(), text);
}

void MessagesDialog::filterColumnChanged(int column)
{
    if (inProcessSettings) {
        return;
    }

    if (column == COLUMN_CONTENT) {
        // need content ... refill
        insertMessages();
    }
    ui.messageTreeWidget->filterItems(column, ui.filterLineEdit->text());

    // save index
    Settings->setValueToGroup("MessageDialog", "filterColumn", column);
}

void MessagesDialog::updateMessageSummaryList()
{
    unsigned int newInboxCount = 0;
    unsigned int newOutboxCount = 0;
    unsigned int newDraftCount = 0;
    unsigned int newSentboxCount = 0;
    unsigned int inboxCount = 0;
    unsigned int trashboxCount = 0;
    unsigned int starredCount = 0;
    unsigned int systemCount = 0;

    /* calculating the new messages */
//    rsMsgs->getMessageCount (&inboxCount, &newInboxCount, &newOutboxCount, &newDraftCount, &newSentboxCount);

    std::list<MsgInfoSummary> msgList;
    std::list<MsgInfoSummary>::const_iterator it;

    rsMsgs->getMessageSummaries(msgList);

    QMap<int, int> tagCount;

    /* calculating the new messages */
    for (it = msgList.begin(); it != msgList.end(); ++it) {
        /* calcluate tag count */
        MsgTagInfo tagInfo;
        rsMsgs->getMessageTag(it->msgId, tagInfo);
        for (std::list<uint32_t>::iterator tagId = tagInfo.tagIds.begin(); tagId != tagInfo.tagIds.end(); ++tagId) {
            int nCount = tagCount [*tagId];
            ++nCount;
            tagCount [*tagId] = nCount;
        }

        if (it->msgflags & RS_MSG_STAR) {
            ++starredCount;
        }

        if (it->msgflags & RS_MSG_SYSTEM) {
            ++systemCount;
        }

        /* calculate box */
        if (it->msgflags & RS_MSG_TRASH) {
            ++trashboxCount;
            continue;
        }

        switch (it->msgflags & RS_MSG_BOXMASK) {
        case RS_MSG_INBOX:
                ++inboxCount;
                if (it->msgflags & (RS_MSG_NEW | RS_MSG_UNREAD_BY_USER)) {
                    ++newInboxCount;
                }
                break;
        case RS_MSG_OUTBOX:
                ++newOutboxCount;
                break;
        case RS_MSG_DRAFTBOX:
                ++newDraftCount;
                break;
        case RS_MSG_SENTBOX:
                ++newSentboxCount;
                break;
        }
    }
    

    int listrow = ui.listWidget->currentRow();
    QString textTotal;

    switch (listrow) 
    {
        case ROW_INBOX:
            textTotal = tr("Total:") + " "  + QString::number(inboxCount);
            ui.total_label->setText(textTotal);
            break;
        case ROW_OUTBOX:
            textTotal = tr("Total:") + " "  + QString::number(newOutboxCount);
            ui.total_label->setText(textTotal);
            break;
        case ROW_DRAFTBOX:
            textTotal = tr("Total:") + " "  + QString::number(newDraftCount);
            ui.total_label->setText(textTotal);
            break;
        case ROW_SENTBOX:
            textTotal = tr("Total:") + " "  + QString::number(newSentboxCount);
            ui.total_label->setText(textTotal);
            break;
        case ROW_TRASHBOX:
            textTotal = tr("Total:") + " "  + QString::number(trashboxCount);
            ui.total_label->setText(textTotal);
            break;
    }


    QString textItem;
    /*updating the labels in leftcolumn*/

    //QList<QListWidgetItem *> QListWidget::findItems ( const QString & text, Qt::MatchFlags flags ) const
    QListWidgetItem* item = ui.listWidget->item(ROW_INBOX);
    if (newInboxCount != 0)
    {
        textItem = tr("Inbox") + " (" + QString::number(newInboxCount)+")";
        item->setText(textItem);
        QFont qf = item->font();
        qf.setBold(true);
        item->setFont(qf);
        item->setIcon(QIcon(":/images/folder-inbox-new.png"));
        item->setForeground(QBrush(mTextColorInbox));
    }
    else
    {
        textItem = tr("Inbox");
        item->setText(textItem);
        QFont qf = item->font();
        qf.setBold(false);
        item->setFont(qf);
        item->setIcon(QIcon(":/images/folder-inbox.png"));
        item->setForeground(QBrush(ui.messageTreeWidget->palette().color(QPalette::Text)));
    }

    //QList<QListWidgetItem *> QListWidget::findItems ( const QString & text, Qt::MatchFlags flags ) const
    item = ui.listWidget->item(ROW_OUTBOX);
    if (newOutboxCount != 0)
    {
        textItem = tr("Outbox") + " (" + QString::number(newOutboxCount)+")";
        item->setText(textItem);
        QFont qf = item->font();
        qf.setBold(true);
        item->setFont(qf);
    }
    else
    {
        textItem = tr("Outbox");
        item->setText(textItem);
        QFont qf = item->font();
        qf.setBold(false);
        item->setFont(qf);

    }

    //QList<QListWidgetItem *> QListWidget::findItems ( const QString & text, Qt::MatchFlags flags ) const
    item = ui.listWidget->item(ROW_DRAFTBOX);
    if (newDraftCount != 0)
    {
        textItem = tr("Drafts") + " (" + QString::number(newDraftCount)+")";
        item->setText(textItem);
        QFont qf = item->font();
        qf.setBold(true);
        item->setFont(qf);
    }
    else
    {
        textItem = tr("Drafts");
        item->setText(textItem);
        QFont qf = item->font();
        qf.setBold(false);
        item->setFont(qf);

    }

    item = ui.listWidget->item(ROW_TRASHBOX);
    if (trashboxCount != 0)
    {
        textItem = tr("Trash") + " (" + QString::number(trashboxCount)+")";
        item->setText(textItem);
    }
    else
    {
        textItem = tr("Trash");
        item->setText(textItem);
    }

    /* set tag counts */
    int rowCount = ui.quickViewWidget->count();
    for (int row = 0; row < rowCount; ++row) {
        QListWidgetItem *item = ui.quickViewWidget->item(row);
        switch (item->data(ROLE_QUICKVIEW_TYPE).toInt()) {
        case QUICKVIEW_TYPE_TAG:
            {
                int count = tagCount[item->data(ROLE_QUICKVIEW_ID).toInt()];

                QString text = item->data(ROLE_QUICKVIEW_TEXT).toString();
                if (count) {
                    text += " (" + QString::number(count) + ")";
                }

                item->setText(text);
            }
            break;
        case QUICKVIEW_TYPE_STATIC:
            {
                QString text = item->data(ROLE_QUICKVIEW_TEXT).toString();
                switch (item->data(ROLE_QUICKVIEW_ID).toInt()) {
                case QUICKVIEW_STATIC_ID_STARRED:
                    text += " (" + QString::number(starredCount) + ")";
                    break;
                case QUICKVIEW_STATIC_ID_SYSTEM:
                    text += " (" + QString::number(systemCount) + ")";
                    break;
                }

                item->setText(text);
            }
            break;
        }
    }
}

void MessagesDialog::tagAboutToShow()
{
	TagsMenu *menu = dynamic_cast<TagsMenu*>(ui.tagButton->menu());
	if (menu == NULL) {
		return;
	}

	// activate actions from the first selected row
	MsgTagInfo tagInfo;

    QList<QTreeWidgetItem*> items;
    getSelectedMsgCount (&items, NULL, NULL, NULL);

    if (items.size()) {
        std::string msgId = items.front()->data(COLUMN_DATA, ROLE_MSGID).toString().toStdString();

		rsMsgs->getMessageTag(msgId, tagInfo);
	}

	menu->activateActions(tagInfo.tagIds);
}

void MessagesDialog::tagRemoveAll()
{
	LockUpdate Lock (this, false);

    QList<QTreeWidgetItem*> items;
    getSelectedMsgCount (&items, NULL, NULL, NULL);
    foreach (QTreeWidgetItem *item, items) {
        std::string msgId = item->data(COLUMN_DATA, ROLE_MSGID).toString().toStdString();

		rsMsgs->setMessageTag(msgId, 0, false);
		Lock.setUpdate(true);
	}

	// LockUpdate -> insertMessages();
}

void MessagesDialog::tagSet(int tagId, bool set)
{
	if (tagId == 0) {
		return;
	}

	LockUpdate Lock (this, false);

    QList<QTreeWidgetItem*> items;
    getSelectedMsgCount (&items, NULL, NULL, NULL);
    foreach (QTreeWidgetItem *item, items) {
        std::string msgId = item->data(COLUMN_DATA, ROLE_MSGID).toString().toStdString();

		if (rsMsgs->setMessageTag(msgId, tagId, set)) {
			Lock.setUpdate(true);
		}
	}

	// LockUpdate -> insertMessages();
}

void MessagesDialog::emptyTrash()
{
    LockUpdate Lock (this, true);

    std::list<MsgInfoSummary> msgList;
    rsMsgs->getMessageSummaries(msgList);

    std::list<MsgInfoSummary>::const_iterator it;
    for (it = msgList.begin(); it != msgList.end(); ++it) {
        if (it->msgflags & RS_MSG_TRASH) {
            rsMsgs->MessageDelete(it->msgId);
        }
    }

    // LockUpdate -> insertMessages();
}

void MessagesDialog::tabChanged(int /*tab*/)
{
	connectActions();
	updateInterface();
}

void MessagesDialog::tabCloseRequested(int tab)
{
	if (tab == 0) {
		return;
	}

	QWidget *widget = ui.tabWidget->widget(tab);

	if (widget) {
		widget->deleteLater();
	}
}

void MessagesDialog::closeTab(const std::string &msgId)
{
    QList<MessageWidget*> msgWidgets;

    for (int tab = 1; tab < ui.tabWidget->count(); ++tab) {
        MessageWidget *msgWidget = dynamic_cast<MessageWidget*>(ui.tabWidget->widget(tab));
        if (msgWidget && msgWidget->msgId() == msgId) {
            msgWidgets.append(msgWidget);
        }
    }
    qDeleteAll(msgWidgets);
}

void MessagesDialog::connectActions()
{
	int tab = ui.tabWidget->currentIndex();

	MessageWidget *msg;
	if (tab == 0) {
		msg = msgWidget;
	} else {
		msg = dynamic_cast<MessageWidget*>(ui.tabWidget->widget(tab));
	}

	ui.replymessageButton->disconnect();
	ui.replyallmessageButton->disconnect();
	ui.forwardmessageButton->disconnect();
	ui.printbutton->disconnect();
	ui.actionPrint->disconnect();
	ui.actionPrintPreview->disconnect();
	ui.actionSaveAs->disconnect();
	ui.removemessageButton->disconnect();

	ui.actionReply->disconnect();
	ui.actionReplyAll->disconnect();
	ui.actionForward->disconnect();

	if (msgWidget) {
		// connect actions
		msg->connectAction(MessageWidget::ACTION_REPLY, ui.actionReply);
		msg->connectAction(MessageWidget::ACTION_REPLY_ALL, ui.actionReplyAll);
		msg->connectAction(MessageWidget::ACTION_FORWARD, ui.actionForward);
	}

	if (msg) {
		if (tab == 0) {
			// connect with own slot to remove multiple messages
			connect(ui.removemessageButton, SIGNAL(clicked()), this, SLOT(removemessage()));
		} else {
			msg->connectAction(MessageWidget::ACTION_REMOVE, ui.removemessageButton);
		}
		msg->connectAction(MessageWidget::ACTION_REPLY, ui.replymessageButton);
		msg->connectAction(MessageWidget::ACTION_REPLY_ALL, ui.replyallmessageButton);
		msg->connectAction(MessageWidget::ACTION_FORWARD, ui.forwardmessageButton);
		msg->connectAction(MessageWidget::ACTION_PRINT, ui.printbutton);
		msg->connectAction(MessageWidget::ACTION_PRINT, ui.actionPrint);
		msg->connectAction(MessageWidget::ACTION_PRINT_PREVIEW, ui.actionPrintPreview);
		msg->connectAction(MessageWidget::ACTION_SAVE_AS, ui.actionSaveAs);
	}
}

void MessagesDialog::updateInterface()
{
	int count = 0;

	int tab = ui.tabWidget->currentIndex();

	if (tab == 0) {
		count = getSelectedMsgCount(NULL, NULL, NULL, NULL);
	} else {
		MessageWidget *msg = dynamic_cast<MessageWidget*>(ui.tabWidget->widget(tab));
		if (msg && msg->msgId().empty() == false) {
			count = 1;
		}
	}

	ui.replymessageButton->setEnabled(count == 1);
	ui.replyallmessageButton->setEnabled(count == 1);
	ui.forwardmessageButton->setEnabled(count == 1);
	ui.printbutton->setEnabled(count == 1);
	ui.actionPrint->setEnabled(count == 1);
	ui.actionPrintPreview->setEnabled(count == 1);
	ui.actionSaveAs->setEnabled(count == 1);
	ui.removemessageButton->setEnabled(count >= 1);
	ui.tagButton->setEnabled(count >= 1);
}
