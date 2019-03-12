/*******************************************************************************
 * gui/MessagesDialog.cpp                                                      *
 *                                                                             *
 * Copyright (c) 2006 Crypton          <retroshare.project@gmail.com>          *
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

#include <QDateTime>
#include <QKeyEvent>
#include <QMessageBox>
#include <QStandardItemModel>
#include <QShortcut>
#include <QTimer>

#include "MessagesDialog.h"

#include "notifyqt.h"
#include "common/TagDefs.h"
#include "common/PeerDefs.h"
#include "common/RSElidedItemDelegate.h"
#include "gxs/GxsIdTreeWidgetItem.h"
#include "gxs/GxsIdDetails.h"
#include "gui/Identity/IdDialog.h"
#include "gui/MainWindow.h"
#include "msgs/MessageComposer.h"
#include "msgs/MessageInterface.h"
#include "msgs/MessageUserNotify.h"
#include "msgs/MessageWidget.h"
#include "msgs/TagsMenu.h"
#include "msgs/MessageModel.h"
#include "settings/rsharesettings.h"

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
#define IMAGE_DECRYPTMESSAGE   ":/images/decrypt-mail.png"
#define IMAGE_AUTHOR_INFO      ":/images/info16.png"

#define COLUMN_STAR          0
#define COLUMN_ATTACHEMENTS  1
#define COLUMN_SUBJECT       2
#define COLUMN_UNREAD        3
#define COLUMN_FROM          4
//#define COLUMN_SIGNATURE     5
#define COLUMN_DATE          5
#define COLUMN_CONTENT       6
#define COLUMN_TAGS          7
#define COLUMN_COUNT         8

#define COLUMN_DATA          0 // column for storing the userdata like msgid and srcid

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


class MessageSortFilterProxyModel: public QSortFilterProxyModel
{
public:
    MessageSortFilterProxyModel(const QHeaderView *header,QObject *parent = NULL): QSortFilterProxyModel(parent),m_header(header) {}

    bool lessThan(const QModelIndex& left, const QModelIndex& right) const override
    {
		return left.data(RsMessageModel::SortRole) < right.data(RsMessageModel::SortRole) ;
    }

    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override
    {
        return sourceModel()->index(source_row,0,source_parent).data(RsMessageModel::FilterRole).toString() == RsMessageModel::FilterString ;
    }

private:
    const QHeaderView *m_header ;
};

/** Constructor */
MessagesDialog::MessagesDialog(QWidget *parent)
: MainPage(parent)
{
    /* Invoke the Qt Designer generated object setup routine */
    ui.setupUi(this);

    inProcessSettings = false;
    inChange = false;
    lockUpdate = 0;

    ui.actionTextBesideIcon->setData(Qt::ToolButtonTextBesideIcon);
    ui.actionIconOnly->setData(Qt::ToolButtonIconOnly);
    ui.actionTextUnderIcon->setData(Qt::ToolButtonTextUnderIcon);

    connect(ui.filterLineEdit, SIGNAL(textChanged(QString)), this, SLOT(filterChanged(QString)));
    connect(ui.filterLineEdit, SIGNAL(filterChanged(int)), this, SLOT(filterColumnChanged(int)));

    msgWidget = new MessageWidget(true, this);
	ui.msgLayout->addWidget(msgWidget);

    connectActions();

    listMode = LIST_NOTHING;

    mMessageModel = new RsMessageModel(this);
    mMessageProxyModel = new MessageSortFilterProxyModel(ui.messageTreeWidget->header(),this);
    mMessageProxyModel->setSourceModel(mMessageModel);
    mMessageProxyModel->setSortRole(RsMessageModel::SortRole);
    ui.messageTreeWidget->setModel(mMessageProxyModel);

	changeBox(RsMessageModel::BOX_INBOX);

	mMessageProxyModel->setFilterRole(RsMessageModel::FilterRole);
	//mMessageProxyModel->setFilterRegExp(QRegExp(QString(RsMessageModel::FilterString))) ;

	ui.messageTreeWidget->setSortingEnabled(true);

    //ui.messageTreeWidget->setItemDelegateForColumn(RsMessageModel::COLUMN_THREAD_DISTRIBUTION,new DistributionItemDelegate()) ;
    ui.messageTreeWidget->setItemDelegateForColumn(RsMessageModel::COLUMN_THREAD_AUTHOR,new GxsIdTreeItemDelegate()) ;
    //ui.messageTreeWidget->setItemDelegateForColumn(RsGxsForumModel::COLUMN_THREAD_READ,new ReadStatusItemDelegate()) ;

#ifdef TO_REMOVE
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
#endif

    mMessageCompareRole = new RSTreeWidgetItemCompareRole;
    mMessageCompareRole->setRole(COLUMN_SUBJECT,      RsMessageModel::SortRole);
    mMessageCompareRole->setRole(COLUMN_UNREAD,       RsMessageModel::SortRole);
    mMessageCompareRole->setRole(COLUMN_FROM,         RsMessageModel::SortRole);
    mMessageCompareRole->setRole(COLUMN_DATE,         RsMessageModel::SortRole);
    mMessageCompareRole->setRole(COLUMN_TAGS,         RsMessageModel::SortRole);
    mMessageCompareRole->setRole(COLUMN_ATTACHEMENTS, RsMessageModel::SortRole);
    mMessageCompareRole->setRole(COLUMN_STAR,         RsMessageModel::SortRole);

    RSElidedItemDelegate *itemDelegate = new RSElidedItemDelegate(this);
    itemDelegate->setSpacing(QSize(0, 2));
    ui.messageTreeWidget->setItemDelegate(itemDelegate);

    ui.messageTreeWidget->sortByColumn(COLUMN_DATA, Qt::DescendingOrder);

    // workaround for Qt bug, should be solved in next Qt release 4.7.0
    // http://bugreports.qt.nokia.com/browse/QTBUG-8270
    QShortcut *Shortcut = new QShortcut(QKeySequence (Qt::Key_Delete), ui.messageTreeWidget, 0, 0, Qt::WidgetShortcut);
    connect(Shortcut, SIGNAL(activated()), this, SLOT( removemessage ()));
    Shortcut = new QShortcut(QKeySequence (Qt::SHIFT | Qt::Key_Delete), ui.messageTreeWidget, 0, 0, Qt::WidgetShortcut);
    connect(Shortcut, SIGNAL(activated()), this, SLOT( removemessage ()));

    QFontMetricsF fm(font());

    /* Set initial section sizes */
    QHeaderView * msgwheader = ui.messageTreeWidget->header () ;
    msgwheader->resizeSection (COLUMN_ATTACHEMENTS, fm.width('0')*1.2f);
    msgwheader->resizeSection (COLUMN_SUBJECT,      250);
    msgwheader->resizeSection (COLUMN_FROM,         140);
    msgwheader->resizeSection (COLUMN_DATE,         140);

    QHeaderView_setSectionResizeModeColumn(msgwheader, COLUMN_STAR, QHeaderView::Fixed);
    msgwheader->resizeSection (COLUMN_STAR, 24);

    QHeaderView_setSectionResizeModeColumn(msgwheader, COLUMN_UNREAD, QHeaderView::Fixed);
    msgwheader->resizeSection (COLUMN_UNREAD, 24);

    ui.forwardmessageButton->setToolTip(tr("Forward selected Message"));
    ui.replyallmessageButton->setToolTip(tr("Reply to All"));

    QMenu *printmenu = new QMenu();
    printmenu->addAction(ui.actionPrint);
    printmenu->addAction(ui.actionPrintPreview);
    ui.printButton->setMenu(printmenu);

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
    QHeaderView_setSectionResizeModeColumn(msgwheader, COLUMN_ATTACHEMENTS, QHeaderView::Fixed);
    QHeaderView_setSectionResizeModeColumn(msgwheader, COLUMN_DATE, QHeaderView::Interactive);
    QHeaderView_setSectionResizeModeColumn(msgwheader, COLUMN_UNREAD, QHeaderView::Fixed);
    //QHeaderView_setSectionResizeModeColumn(msgwheader, COLUMN_SIGNATURE, QHeaderView::Fixed);
    msgwheader->resizeSection (COLUMN_UNREAD, 24);
    //msgwheader->resizeSection (COLUMN_SIGNATURE, 24);
    msgwheader->resizeSection (COLUMN_STAR, 24);
    QHeaderView_setSectionResizeModeColumn(msgwheader, COLUMN_STAR, QHeaderView::Fixed);
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

    //ui.messageTreeWidget->installEventFilter(this);

    // remove close button of the the first tab
    ui.tabWidget->hideCloseButton(0);

    int S = QFontMetricsF(font()).height();
 QString help_str = tr(
 " <h1><img width=\"%1\" src=\":/icons/help_64.png\">&nbsp;&nbsp;Messages</h1>                         \
 <p>Retroshare has its own internal email system. You can send/receive emails to/from connected friend nodes.</p> \
 <p>It is also possible to send messages to other people's Identities using the global routing system. These messages \
    are always encrypted and signed, and are relayed by intermediate nodes until they reach their final destination. </p>\
    <p>Distant messages stay into your Outbox until an acknowledgement of receipt has been received.</p>\
 <p>Generally, you may use messages to recommend files to your friends by pasting file links, \
 or recommend friend nodes to other friend nodes, in order to strenghten your network, or send feedback \
 to a channel's owner.</p>                   \
 ").arg(QString::number(2*S)).arg(QString::number(S)) ;

	 registerHelpButton(ui.helpButton,help_str,"MessagesDialog") ;

    connect(NotifyQt::getInstance(), SIGNAL(messagesChanged()), mMessageModel, SLOT(updateMessages()));
    connect(NotifyQt::getInstance(), SIGNAL(messagesTagsChanged()), this, SLOT(messagesTagsChanged()));
    connect(ui.messageTreeWidget, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(messageTreeWidgetCustomPopupMenu(const QPoint&)));
    connect(ui.listWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(folderlistWidgetCustomPopupMenu(QPoint)));
    connect(ui.messageTreeWidget, SIGNAL(clicked(const QModelIndex&)) , this, SLOT(clicked(const QModelIndex&)));
    connect(ui.messageTreeWidget, SIGNAL(doubleClicked(const QModelIndex&)) , this, SLOT(doubleClicked(const QModelIndex&)));
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


}

MessagesDialog::~MessagesDialog()
{
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
        ui.msgSplitter->restoreState(Settings->value("SplitterMsg").toByteArray());
        ui.listSplitter->restoreState(Settings->value("SplitterList").toByteArray());

        /* toolbar button style */
        Qt::ToolButtonStyle style = (Qt::ToolButtonStyle) Settings->value("ToolButon_Style", Qt::ToolButtonIconOnly).toInt();
        setToolbarButtonStyle(style);
    } else {
        // save settings

        // state of message tree
        Settings->setValue("MessageTree", msgwheader->saveState());
        Settings->setValue("MessageTreeVersion", messageTreeVersion);

        // state of splitter
        Settings->setValue("SplitterMsg", ui.msgSplitter->saveState());
        Settings->setValue("SplitterList", ui.listSplitter->saveState());

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
    if (obj == ui.messageTreeWidget && event->type() == QEvent::KeyPress)
	{
		QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

		if (keyEvent && keyEvent->key() == Qt::Key_Space)
        {
			// Space pressed
			clicked(ui.messageTreeWidget->currentIndex());
			return true; // eat event
		}
	}

    // pass the event on to the parent class
    return MainPage::eventFilter(obj, event);
}

// void MessagesDialog::changeEvent(QEvent *e)
// {
//     QWidget::changeEvent(e);
//
//     switch (e->type()) {
//     case QEvent::StyleChange:
//         insertMessages();
//         break;
//     default:
//         // remove compiler warnings
//         break;
//     }
// }

void MessagesDialog::fillQuickView()
{
	MsgTagType tags;
	rsMail->getMessageTagTypes(tags);
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
int MessagesDialog::getSelectedMessages(QList<QString>& mid)
{
    //To check if the selection has more than one row.

    mid.clear();
    QModelIndexList qmil = ui.messageTreeWidget->selectionModel()->selectedRows();

    foreach(const QModelIndex& m, qmil)
		mid.push_back(m.sibling(m.row(),COLUMN_DATA).data(RsMessageModel::MsgIdRole).toString()) ;

    return mid.size();
}

int MessagesDialog::getSelectedMsgCount (QList<QModelIndex> *items, QList<QModelIndex> *itemsRead, QList<QModelIndex> *itemsUnread, QList<QModelIndex> *itemsStar)
{
	QModelIndexList qmil = ui.messageTreeWidget->selectionModel()->selectedRows();

    if (items) items->clear();
    if (itemsRead) itemsRead->clear();
    if (itemsUnread) itemsUnread->clear();
    if (itemsStar) itemsStar->clear();

    foreach(const QModelIndex& m, qmil)
    {
		if (items)
			items->append(m);

		if (m.data(RsMessageModel::UnreadRole).toBool())
			if (itemsUnread)
                itemsUnread->append(m);
            else if(itemsRead)
                itemsRead->append(m);

		if (itemsStar && m.data(RsMessageModel::MsgFlagsRole).toInt() & RS_MSG_STAR)
			itemsStar->append(m);
    }

    return qmil.size();
}

bool MessagesDialog::isMessageRead(const QModelIndex& index)
{
    if (!index.isValid())
        return true;

    return !index.data(RsMessageModel::UnreadRole).toBool();
}

bool MessagesDialog::hasMessageStar(const QModelIndex& index)
{
    if (!index.isValid())
        return false;

    return index.data(RsMessageModel::MsgFlagsRole).toInt() & RS_MSG_STAR;
}

void MessagesDialog::messageTreeWidgetCustomPopupMenu(QPoint /*point*/)
{
    std::string cid;
    std::string mid;

    MessageInfo msgInfo;
    if (!getCurrentMsg(cid, mid))
    {
        std::cerr << "No current message!" << std::endl;
        return ;
    }

    if(!rsMail->getMessage(mid, msgInfo))
        return ;

    QList<QModelIndex> itemsRead;
    QList<QModelIndex> itemsUnread;
    QList<QModelIndex> itemsStar;

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

    RsIdentityDetails details;

    // test if identity is known. If not, no need to call the people tab. Also some mails come from nodes and we wont show that node in the people tab either.
    // The problem here is that the field rsgxsid_srcId is always populated with either the GxsId or the node of the source, which is inconsistent.

    if(nCount==1 && rsIdentity->getIdDetails(msgInfo.rsgxsid_srcId,details))
	{
        std::cerr << "Src ID = " << msgInfo.rsgxsid_srcId << std::endl;

		contextMnu.addAction(QIcon(IMAGE_AUTHOR_INFO),tr("Show author in People"),this,SLOT(showAuthorInPeopleTab()));
		contextMnu.addSeparator();
	}

    contextMnu.addAction(QIcon(IMAGE_MESSAGE), tr("New Message"), this, SLOT(newmessage()));

    contextMnu.exec(QCursor::pos());
}

void MessagesDialog::showAuthorInPeopleTab()
{
	std::string cid;
    std::string mid;

    if(!getCurrentMsg(cid, mid))
        return ;

    MessageInfo msgInfo;
    if (!rsMail->getMessage(mid, msgInfo))
        return;

	if(msgInfo.rsgxsid_srcId.isNull())
        return ;

	/* window will destroy itself! */
	IdDialog *idDialog = dynamic_cast<IdDialog*>(MainWindow::getPage(MainWindow::People));

	if (!idDialog)
		return ;

	MainWindow::showWindow(MainWindow::People);
	idDialog->navigate(RsGxsId(msgInfo.rsgxsid_srcId)) ;
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

void MessagesDialog::changeBox(int box_row)
{
    if (inChange) {
        // already in change method
        return;
    }

    inChange = true;

//    ui.messageTreeWidget->clear();

    ui.quickViewWidget->setCurrentItem(NULL);
    listMode = LIST_BOX;

//    insertMessages();
//    insertMsgTxtAndFiles(ui.messageTreeWidget->currentItem());

    switch(box_row)
    {
    	case 0: mMessageModel->setCurrentBox(RsMessageModel::BOX_INBOX ); break;
    	case 1: mMessageModel->setCurrentBox(RsMessageModel::BOX_OUTBOX); break;
    	case 2: mMessageModel->setCurrentBox(RsMessageModel::BOX_DRAFTS); break;
    	case 3: mMessageModel->setCurrentBox(RsMessageModel::BOX_SENT  ); break;
    	case 4: mMessageModel->setCurrentBox(RsMessageModel::BOX_TRASH ); break;
    default:
        mMessageModel->setCurrentBox(RsMessageModel::BOX_NONE); break;
    }
    inChange = false;
}

void MessagesDialog::changeQuickView(int newrow)
{
    Q_UNUSED(newrow);

    ui.listWidget->setCurrentItem(NULL);
    listMode = LIST_QUICKVIEW;

    RsMessageModel::QuickViewFilter f = RsMessageModel::QUICK_VIEW_ALL ;

    if(newrow >= 0)   // -1 means that nothing is selected
		switch(newrow)
		{
		case   0x00:      f = RsMessageModel::QUICK_VIEW_STARRED   ; break;
		case   0x01:      f = RsMessageModel::QUICK_VIEW_SYSTEM   ; break;
		case   0x02:      f = RsMessageModel::QUICK_VIEW_IMPORTANT; break;
		case   0x03:      f = RsMessageModel::QUICK_VIEW_WORK     ; break;
		case   0x04:      f = RsMessageModel::QUICK_VIEW_PERSONAL ; break;
		case   0x05:      f = RsMessageModel::QUICK_VIEW_TODO     ; break;
		case   0x06:      f = RsMessageModel::QUICK_VIEW_LATER    ; break;
		default:
			f = RsMessageModel::QuickViewFilter( (int)RsMessageModel::QUICK_VIEW_USER + newrow - 0x06);
		}
    mMessageModel->setQuickViewFilter(f);
    insertMsgTxtAndFiles(ui.messageTreeWidget->currentIndex());
}

void MessagesDialog::messagesTagsChanged()
{
    fillQuickView();
    mMessageModel->updateMessages();
}

static void InitIconAndFont(QTreeWidgetItem *item)
{
}

#ifdef TO_REMOVE
void MessagesDialog::insertMessages()
{
    if (lockUpdate) {
        return;
    }

    std::list<MsgInfoSummary> msgList;
    std::list<MsgInfoSummary>::const_iterator it;
    MessageInfo msgInfo;
    bool gotInfo;
    QString text;

    RsPeerId ownId = rsPeers->getOwnId();

    rsMail -> getMessageSummaries(msgList);

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
        rsMail->getMessageTagTypes(Tags);

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
                rsMail->getMessageTag(it->msgId, tagInfo);
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
                item = new GxsIdRSTreeWidgetItem(mMessageCompareRole,GxsIdDetails::ICON_TYPE_AVATAR);
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
                bool setText = true;
                if (msgbox == RS_MSG_INBOX || msgbox == RS_MSG_OUTBOX) 
                {
                    if ((it->msgflags & RS_MSG_SYSTEM) && it->srcId == ownId) {
                        text = "RetroShare";
                    } 
                    else 
                    {
                        if (it->msgflags & RS_MSG_DISTANT)
                        {
                            // distant message
                            setText = false;
                            if (gotInfo || rsMail->getMessage(it->msgId, msgInfo)) {
                                gotInfo = true;
                                
                                if(msgbox != RS_MSG_INBOX && !msgInfo.rsgxsid_msgto.empty())
					item->setId(RsGxsId(*msgInfo.rsgxsid_msgto.begin()), COLUMN_FROM, false);
                                else
					item->setId(RsGxsId(msgInfo.rsgxsid_srcId), COLUMN_FROM, false);
                            } 
                            else 
                                std::cerr << "MessagesDialog::insertMsgTxtAndFiles() Couldn't find Msg" << std::endl;
                        } 
                        else 
                            text = QString::fromUtf8(rsPeers->getPeerName(it->srcId).c_str());
                    }
                } 
                else 
                {
                    if (gotInfo || rsMail->getMessage(it->msgId, msgInfo)) {
                        gotInfo = true;

                        text.clear();

                        for(std::set<RsPeerId>::const_iterator pit = msgInfo.rspeerid_msgto.begin(); pit != msgInfo.rspeerid_msgto.end(); ++pit)
                        {
                            if (!text.isEmpty())
                                text += ", ";

                            std::string peerName = rsPeers->getPeerName(*pit);
                            if (peerName.empty())
                                text += PeerDefs::rsid("", *pit);
                             else
                                text += QString::fromUtf8(peerName.c_str());
                        }
                        for(std::set<RsGxsId>::const_iterator pit = msgInfo.rsgxsid_msgto.begin(); pit != msgInfo.rsgxsid_msgto.end(); ++pit)
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
                if (setText)
                {
                    item->setText(COLUMN_FROM, text);
                    item->setData(COLUMN_FROM, ROLE_SORT, text + dateString);
                } else {
                    item->setData(COLUMN_FROM, ROLE_SORT, item->text(COLUMN_FROM) + dateString);
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
            rsMail->getMessageTag(it->msgId, tagInfo);

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
                    rsMail->setMessageTag(it->msgId, *tagId, false);
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
                    rsMail->setMessageTag(it->msgId, tagInfo.tagIds.front(), false);
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
                if (gotInfo || rsMail->getMessage(it->msgId, msgInfo)) {
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
                item->setIcon(COLUMN_SUBJECT, QIcon(":/images/message-mail-read.png")) ;
                
                if (msgbox == RS_MSG_INBOX )
                {
                    item->setToolTip(COLUMN_SIGNATURE, tr("This message comes from a distant person.")) ;
                }
                else if  (msgbox == RS_MSG_OUTBOX)
                {
                    item->setToolTip(COLUMN_SIGNATURE, tr("This message goes to a distant person.")) ;
                }

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
#endif

// click in messageTreeWidget
void MessagesDialog::clicked(const QModelIndex& index)
{
    if(!index.isValid())
        return;

    switch (index.column())
    {
    case COLUMN_UNREAD:
        {
            mMessageModel->setMsgReadStatus(index, !isMessageRead(index));
            insertMsgTxtAndFiles(index);
            updateMessageSummaryList();
            return;
        }
    case COLUMN_STAR:
        {
            mMessageModel->setMsgStar(index, !hasMessageStar(index));
            return;
        }
    }

    // show current message directly
	insertMsgTxtAndFiles(index);
}

// double click in messageTreeWidget
void MessagesDialog::doubleClicked(const QModelIndex& index)
{
    /* activate row */
    clicked(index);

    std::string cid;
    std::string mid;

    if(!getCurrentMsg(cid, mid))
        return ;

    MessageInfo msgInfo;
    if (!rsMail->getMessage(mid, msgInfo)) {
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
}

// void MessagesDialog::setMsgAsReadUnread(const QList<QTreeWidgetItem*> &items, bool read)
// {
//     LockUpdate Lock (this, false);
//
//     foreach (QTreeWidgetItem *item, items) {
//         std::string mid = item->data(COLUMN_DATA, RsMessageModel::MsgIdRole).toString().toStdString();
//
//         if (rsMail->MessageRead(mid, !read)) {
//             int msgFlag = item->data(COLUMN_DATA, RsMessageModel::MsgFlagsRole).toInt();
//             msgFlag &= ~RS_MSG_NEW;
//
//             if (read) {
//                 msgFlag &= ~RS_MSG_UNREAD_BY_USER;
//             } else {
//                 msgFlag |= RS_MSG_UNREAD_BY_USER;
//             }
//
//             item->setData(COLUMN_DATA, RsMessageModel::MsgFlagsRole, msgFlag);
//
//             InitIconAndFont(item);
//         }
//     }
//
//     // LockUpdate
// }

void MessagesDialog::markAsRead()
{
    QList<QModelIndex> itemsUnread;
    getSelectedMsgCount (NULL, NULL, &itemsUnread, NULL);

    foreach(const QModelIndex& index,itemsUnread)
        mMessageModel->setMsgReadStatus(index,true);

    updateMessageSummaryList();
}

void MessagesDialog::markAsUnread()
{
    QList<QModelIndex> itemsRead;
    getSelectedMsgCount (NULL, &itemsRead, NULL, NULL);

    foreach(const QModelIndex& index,itemsRead)
        mMessageModel->setMsgReadStatus(index,false);

    updateMessageSummaryList();
}

void MessagesDialog::markWithStar(bool checked)
{
    QModelIndexList lst = ui.messageTreeWidget->selectionModel()->selectedRows();

    foreach(const QModelIndex& index,lst)
		mMessageModel->setMsgStar(index, checked);
}



void MessagesDialog::insertMsgTxtAndFiles(const QModelIndex& index)
{
    /* get its Ids */
    std::string cid;
    std::string mid;

    if(!index.isValid())
	{
		mCurrMsgId.clear();
		msgWidget->fill(mCurrMsgId);
		updateInterface();
		return;
	}
    mid = index.data(RsMessageModel::MsgIdRole).toString().toStdString();

//    int nCount = getSelectedMsgCount (NULL, NULL, NULL, NULL);
//
//    if (nCount == 1) {
//        ui.actionSaveAs->setEnabled(true);
//        ui.actionPrintPreview->setEnabled(true);
//        ui.actionPrint->setEnabled(true);
//    } else {
//        ui.actionSaveAs->setDisabled(true);
//        ui.actionPrintPreview->setDisabled(true);
//        ui.actionPrint->setDisabled(true);
//    }

    /* Save the Data.... for later */

    MessageInfo msgInfo;
    if (!rsMail -> getMessage(mid, msgInfo)) {
        std::cerr << "MessagesDialog::insertMsgTxtAndFiles() Couldn't find Msg" << std::endl;
        return;
    }

//    QList<QTreeWidgetItem*> items;
//    items.append(item);
//
//    bool bSetToReadOnActive = Settings->getMsgSetToReadOnActivate();
//
//    if (msgInfo.msgflags & RS_MSG_NEW) {
//        // set always to read or unread
//        if (bSetToReadOnActive == false || bSetToRead == false) {
//            // set locally to unread
//            setMsgAsReadUnread(items, false);
//        } else {
//            setMsgAsReadUnread(items, true);
//        }
//        updateMessageSummaryList();
//    } else {
//        if ((msgInfo.msgflags & RS_MSG_UNREAD_BY_USER) && bSetToRead && bSetToReadOnActive) {
//            // set to read
//            setMsgAsReadUnread(items, true);
//            updateMessageSummaryList();
//        }
//    }

    updateInterface();
    msgWidget->fill(mid);
}

bool MessagesDialog::getCurrentMsg(std::string &cid, std::string &mid)
{
    QModelIndex indx = ui.messageTreeWidget->currentIndex();

#ifdef TODO
    /* get its Ids */
    if (!indx.isValid())
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
#endif
    if(!indx.isValid())
        return false ;

    cid = indx.sibling(indx.row(),COLUMN_DATA).data(RsMessageModel::SrcIdRole).toString().toStdString();
    mid = indx.sibling(indx.row(),COLUMN_DATA).data(RsMessageModel::MsgIdRole).toString().toStdString();

    return true;
}

void MessagesDialog::removemessage()
{
    QList<QString> selectedMessages ;
	getSelectedMessages(selectedMessages);

    bool doDelete = false;
    int listrow = ui.listWidget->currentRow();
    if (listrow == ROW_TRASHBOX) {
        doDelete = true;
    } else {
        if (QApplication::keyboardModifiers() & Qt::ShiftModifier) {
            doDelete = true;
        }
    }

    foreach (const QString& m, selectedMessages) {
        if (doDelete) {
            rsMail->MessageDelete(m.toStdString());
        } else {
            rsMail->MessageToTrash(m.toStdString(), true);
        }
    }
}

void MessagesDialog::undeletemessage()
{
    QList<QString> msgids;
    getSelectedMessages(msgids);

    foreach (const QString& s, msgids)
        rsMail->MessageToTrash(s.toStdString(), false);
}

void MessagesDialog::setToolbarButtonStyle(Qt::ToolButtonStyle style)
{
    ui.newmessageButton->setToolButtonStyle(style);
    ui.removemessageButton->setToolButtonStyle(style);
    ui.replymessageButton->setToolButtonStyle(style);
    ui.replyallmessageButton->setToolButtonStyle(style);
    ui.forwardmessageButton->setToolButtonStyle(style);
    ui.tagButton->setToolButtonStyle(style);
    ui.printButton->setToolButtonStyle(style);
    ui.viewtoolButton->setToolButtonStyle(style);
}

void MessagesDialog::buttonStyle()
{
    setToolbarButtonStyle((Qt::ToolButtonStyle) dynamic_cast<QAction*>(sender())->data().toInt());
}

void MessagesDialog::filterChanged(const QString& text)
{
    QStringList items = text.split(' ',QString::SkipEmptyParts);
    mMessageModel->setFilter(ui.filterLineEdit->currentFilter(),items);
}

void MessagesDialog::filterColumnChanged(int column)
{
    if (inProcessSettings)
        return;

    QStringList items = ui.filterLineEdit->text().split(' ',QString::SkipEmptyParts);
    mMessageModel->setFilter(column,items);

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

    std::list<MsgInfoSummary> msgList;
    rsMail->getMessageSummaries(msgList);

    QMap<int, int> tagCount;

    /* calculating the new messages */
    for (auto it = msgList.begin(); it != msgList.end(); ++it)
    {
        /* calcluate tag count */

        for (auto tagId = (*it).msgtags.begin(); tagId != (*it).msgtags.end(); ++tagId)
        {
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
            ui.totalLabel->setText(textTotal);
            break;
        case ROW_OUTBOX:
            textTotal = tr("Total:") + " "  + QString::number(newOutboxCount);
            ui.totalLabel->setText(textTotal);
            break;
        case ROW_DRAFTBOX:
            textTotal = tr("Total:") + " "  + QString::number(newDraftCount);
            ui.totalLabel->setText(textTotal);
            break;
        case ROW_SENTBOX:
            textTotal = tr("Total:") + " "  + QString::number(newSentboxCount);
            ui.totalLabel->setText(textTotal);
            break;
        case ROW_TRASHBOX:
            textTotal = tr("Total:") + " "  + QString::number(trashboxCount);
            ui.totalLabel->setText(textTotal);
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
#ifdef TODO
	TagsMenu *menu = dynamic_cast<TagsMenu*>(ui.tagButton->menu());
	if (menu == NULL) {
		return;
	}

	// activate actions from the first selected row
	MsgTagInfo tagInfo;

    QList<QString> msgids;
    getSelectedMessages(msgids);

    if(!msgids.empty())
		rsMail->getMessageTag(msgids.front().toStdString(), tagInfo);

	menu->activateActions(tagInfo.tagIds);
#endif
}

void MessagesDialog::tagRemoveAll()
{
    QList<QString> msgids;
    getSelectedMessages(msgids);

    foreach(const QString& s, msgids)
		rsMail->setMessageTag(s.toStdString(), 0, false);
}

void MessagesDialog::tagSet(int tagId, bool set)
{
	if (tagId == 0) {
		return;
	}

    QList<QString> msgids;
    getSelectedMessages(msgids);

    foreach (const QString& s, msgids)
		rsMail->setMessageTag(s.toStdString(), tagId, set);
}

void MessagesDialog::emptyTrash()
{
    std::list<Rs::Msgs::MsgInfoSummary> msgs ;
    mMessageModel->getMessageSummaries(RsMessageModel::BOX_TRASH,msgs);

    for(auto it(msgs.begin());it!=msgs.end();++it)
		rsMail->MessageDelete(it->msgId);
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
	ui.printButton->disconnect();
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
		msg->connectAction(MessageWidget::ACTION_PRINT, ui.printButton);
		msg->connectAction(MessageWidget::ACTION_PRINT, ui.actionPrint);
		msg->connectAction(MessageWidget::ACTION_PRINT_PREVIEW, ui.actionPrintPreview);
		msg->connectAction(MessageWidget::ACTION_SAVE_AS, ui.actionSaveAs);
	}
}

void MessagesDialog::updateInterface()
{
	int count = 0;

	int tab = ui.tabWidget->currentIndex();

	if (tab == 0)
    {
        QList<QString> msgs;
		count = getSelectedMessages(msgs);
	}
    else
    {
		MessageWidget *msg = dynamic_cast<MessageWidget*>(ui.tabWidget->widget(tab));
		if (msg && msg->msgId().empty() == false)
			count = 1;
	}

	ui.replymessageButton->setEnabled(count == 1);
	ui.replyallmessageButton->setEnabled(count == 1);
	ui.forwardmessageButton->setEnabled(count == 1);
	ui.printButton->setEnabled(count == 1);
	ui.actionPrint->setEnabled(count == 1);
	ui.actionPrintPreview->setEnabled(count == 1);
	ui.actionSaveAs->setEnabled(count == 1);
	ui.removemessageButton->setEnabled(count >= 1);
	ui.tagButton->setEnabled(count >= 1);
}
