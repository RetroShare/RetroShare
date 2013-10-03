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
#include "util/DateTime.h"
#include "util/RsProtectedTimer.h"

#include <retroshare/rspeers.h>
#include <retroshare/rsmsgs.h>

#include <algorithm>

/* Images for context menu icons */
#define IMAGE_MESSAGE		   ":/images/folder-draft.png"
#define IMAGE_MESSAGEREMOVE    ":/images/message-mail-imapdelete.png"
#define IMAGE_STAR_ON          ":/images/star-on-16.png"
#define IMAGE_STAR_OFF         ":/images/star-off-16.png"
#define IMAGE_SYSTEM           ":/images/user/user_request16.png"

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

    m_pDialog->lockUpdate++;
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

    connect(ui.messagestreeView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(messageslistWidgetCustomPopupMenu(QPoint)));
    connect(ui.listWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(folderlistWidgetCustomPopupMenu(QPoint)));
    connect(ui.messagestreeView, SIGNAL(clicked(const QModelIndex&)) , this, SLOT(clicked(const QModelIndex&)));
    connect(ui.messagestreeView, SIGNAL(doubleClicked(const QModelIndex&)) , this, SLOT(doubleClicked(const QModelIndex&)));
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
    MessagesModel = new QStandardItemModel(0, COLUMN_COUNT);
    MessagesModel->setHeaderData(COLUMN_ATTACHEMENTS,  Qt::Horizontal, QIcon(":/images/attachment.png"), Qt::DecorationRole);
    MessagesModel->setHeaderData(COLUMN_SUBJECT,       Qt::Horizontal, tr("Subject"));
    MessagesModel->setHeaderData(COLUMN_UNREAD,        Qt::Horizontal, QIcon(":/images/message-state-header.png"), Qt::DecorationRole);
    MessagesModel->setHeaderData(COLUMN_FROM,          Qt::Horizontal, tr("From"));
    MessagesModel->setHeaderData(COLUMN_SIGNATURE,     Qt::Horizontal, QIcon(":/images/signature.png"),Qt::DecorationRole);
    MessagesModel->setHeaderData(COLUMN_DATE,          Qt::Horizontal, tr("Date"));
    MessagesModel->setHeaderData(COLUMN_TAGS,          Qt::Horizontal, tr("Tags"));
    MessagesModel->setHeaderData(COLUMN_CONTENT,       Qt::Horizontal, tr("Content"));
    MessagesModel->setHeaderData(COLUMN_STAR,          Qt::Horizontal, QIcon(IMAGE_STAR_ON), Qt::DecorationRole);

    MessagesModel->setHeaderData(COLUMN_ATTACHEMENTS,  Qt::Horizontal, tr("Click to sort by attachments"), Qt::ToolTipRole);
    MessagesModel->setHeaderData(COLUMN_SUBJECT,       Qt::Horizontal, tr("Click to sort by subject"), Qt::ToolTipRole);
    MessagesModel->setHeaderData(COLUMN_UNREAD,        Qt::Horizontal, tr("Click to sort by read"), Qt::ToolTipRole);
    MessagesModel->setHeaderData(COLUMN_FROM,          Qt::Horizontal, tr("Click to sort by from"), Qt::ToolTipRole);
    MessagesModel->setHeaderData(COLUMN_SIGNATURE,     Qt::Horizontal, tr("Click to sort by signature"), Qt::ToolTipRole);
    MessagesModel->setHeaderData(COLUMN_DATE,          Qt::Horizontal, tr("Click to sort by date"), Qt::ToolTipRole);
    MessagesModel->setHeaderData(COLUMN_TAGS,          Qt::Horizontal, tr("Click to sort by tags"), Qt::ToolTipRole);
    MessagesModel->setHeaderData(COLUMN_STAR,          Qt::Horizontal, tr("Click to sort by star"), Qt::ToolTipRole);

    proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setDynamicSortFilter(true);
    proxyModel->setSourceModel(MessagesModel);
    proxyModel->setSortRole(ROLE_SORT);
    proxyModel->sort (COLUMN_DATE, Qt::DescendingOrder);
    ui.messagestreeView->setModel(proxyModel);
    ui.messagestreeView->setSelectionBehavior(QTreeView::SelectRows);
    connect(ui.messagestreeView->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(updateInterface()));

    RSItemDelegate *itemDelegate = new RSItemDelegate(this);
    itemDelegate->setSpacing(QSize(0, 2));
    ui.messagestreeView->setItemDelegate(itemDelegate);

    ui.messagestreeView->setRootIsDecorated(false);
    ui.messagestreeView->setSortingEnabled(true);
    ui.messagestreeView->sortByColumn(COLUMN_DATE, Qt::DescendingOrder);

    // connect after setting model
    connect( ui.messagestreeView->selectionModel(), SIGNAL(currentChanged ( QModelIndex, QModelIndex ) ) , this, SLOT( currentChanged( const QModelIndex & ) ) );

    // workaround for Qt bug, should be solved in next Qt release 4.7.0
    // http://bugreports.qt.nokia.com/browse/QTBUG-8270
    QShortcut *Shortcut = new QShortcut(QKeySequence (Qt::Key_Delete), ui.messagestreeView, 0, 0, Qt::WidgetShortcut);
    connect(Shortcut, SIGNAL(activated()), this, SLOT( removemessage ()));
    Shortcut = new QShortcut(QKeySequence (Qt::SHIFT | Qt::Key_Delete), ui.messagestreeView, 0, 0, Qt::WidgetShortcut);
    connect(Shortcut, SIGNAL(activated()), this, SLOT( removemessage ()));

    /* Set header initial section sizes */
    QHeaderView * msgwheader = ui.messagestreeView->header () ;
    msgwheader->resizeSection (COLUMN_ATTACHEMENTS, 24);
    msgwheader->resizeSection (COLUMN_SUBJECT,      250);
    msgwheader->resizeSection (COLUMN_UNREAD,       16);
    msgwheader->resizeSection (COLUMN_FROM,         140);
    msgwheader->resizeSection (COLUMN_SIGNATURE,    24);
    msgwheader->resizeSection (COLUMN_DATE,         140);
    msgwheader->resizeSection (COLUMN_STAR,         16);

    msgwheader->setResizeMode (COLUMN_STAR, QHeaderView::Fixed);
    msgwheader->resizeSection (COLUMN_STAR, 24);

    ui.forwardmessageButton->setToolTip(tr("Forward selected Message"));
    ui.replyallmessageButton->setToolTip(tr("Reply to All"));

    QMenu *printmenu = new QMenu();
    printmenu->addAction(ui.actionPrint);
    printmenu->addAction(ui.actionPrintPreview);
    ui.printbutton->setMenu(printmenu);

    QMenu *viewmenu = new QMenu();
    viewmenu->addAction(ui.actionTextBesideIcon);
    viewmenu->addAction(ui.actionIconOnly);
    //viewmenu->addAction(ui.actionTextUnderIcon);
    ui.viewtoolButton->setMenu(viewmenu);

    /* add filter actions */
    ui.filterLineEdit->addFilter(QIcon(), tr("Subject"), COLUMN_SUBJECT, tr("Search Subject"));
    ui.filterLineEdit->addFilter(QIcon(), tr("From"), COLUMN_FROM, tr("Search From"));
    ui.filterLineEdit->addFilter(QIcon(), tr("Date"), COLUMN_DATE, tr("Search Date"));
    ui.filterLineEdit->addFilter(QIcon(), tr("Content"), COLUMN_CONTENT, tr("Search Content"));
    ui.filterLineEdit->addFilter(QIcon(), tr("Tags"), COLUMN_TAGS, tr("Search Tags"));
    ui.filterLineEdit->addFilter(QIcon(), tr("Attachments"), COLUMN_ATTACHEMENTS, tr("Search Attachments"));

    //setting default filter by column as subject
    ui.filterLineEdit->setCurrentFilter(COLUMN_SUBJECT);
    proxyModel->setFilterKeyColumn(COLUMN_SUBJECT);

    // load settings
    processSettings(true);

    /* Set header sizes for the fixed columns and resize modes, must be set after processSettings */
    msgwheader->setResizeMode (COLUMN_ATTACHEMENTS, QHeaderView::Fixed);
    msgwheader->setResizeMode (COLUMN_DATE, QHeaderView::Interactive);
    msgwheader->setResizeMode (COLUMN_UNREAD, QHeaderView::Fixed);
    msgwheader->setResizeMode (COLUMN_SIGNATURE, QHeaderView::Fixed);
    msgwheader->resizeSection (COLUMN_UNREAD, 24);
    msgwheader->resizeSection (COLUMN_SIGNATURE, 24);
    msgwheader->resizeSection (COLUMN_STAR, 24);
    msgwheader->setResizeMode (COLUMN_STAR, QHeaderView::Fixed);
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

    ui.messagestreeView->installEventFilter(this);

    // remove close button of the the first tab
    ui.tabWidget->hideCloseButton(0);

  /* Hide platform specific features */
#ifdef Q_WS_WIN

#endif

	 QString help_str = tr(
			 " <h1><img width=\"32\" src=\":/images/64px_help.png\">&nbsp;&nbsp;Messages</h1>                         \
			 <p>Messages are like <b>e-mail</b>: you send/receive them from your friends when both of you	are connected.</p> \
			 <p>It is also possible to send messages to non friends, using tunnels. Such messages are always encrypted. It is \
			 recommended to cryptographically sign distant messages, as a proof of your identity, using the <img width=\"16\" src=\":/images/stock_signature_ok.png\"/> button \
			 in the message composer window. Distant messages are not guarrantied to arrive, since this requires the distant peer to accept them (You need yourself to switch this on in Config-Messages).</p>\
			 <p>Some additional features allow you to exchange data in messages: you may recommend files to your friends by pasting file links, \
			 or recommend friends-to-be to other friends, in order to streathen your network.</p>	                   \
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

    QHeaderView *msgwheader = ui.messagestreeView->header () ;

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
        Qt::ToolButtonStyle style = (Qt::ToolButtonStyle) Settings->value("ToolButon_Stlye", Qt::ToolButtonIconOnly).toInt();
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
        Settings->setValue("ToolButon_Stlye", ui.newmessageButton->toolButtonStyle());
    }

    Settings->endGroup();

    if (msgWidget) {
        msgWidget->processSettings("MessageDialog", load);
    }

    inProcessSettings = false;
}

bool MessagesDialog::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui.messagestreeView) {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent && keyEvent->key() == Qt::Key_Space) {
                // Space pressed
                QModelIndex currentIndex = ui.messagestreeView->currentIndex();
                QModelIndex index = ui.messagestreeView->model()->index(currentIndex.row(), COLUMN_UNREAD, currentIndex.parent());
                clicked(index);
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

	for (tag = tags.types.begin(); tag != tags.types.end(); tag++) {
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

int MessagesDialog::getSelectedMsgCount (QList<int> *pRows, QList<int> *pRowsRead, QList<int> *pRowsUnread, QList<int> *pRowsStar)
{
    if (pRowsRead) pRowsRead->clear();
    if (pRowsUnread) pRowsUnread->clear();
    if (pRowsStar) pRowsStar->clear();

    //To check if the selection has more than one row.
    QList<QModelIndex> selectedIndexList = ui.messagestreeView->selectionModel() -> selectedIndexes ();
    QList<int> rowList;
    for(QList<QModelIndex>::iterator it = selectedIndexList.begin(); it != selectedIndexList.end(); it++)
    {
        int row = it->row();
        if (rowList.contains(row) == false)
        {
            rowList.append(row);

            if (pRows || pRowsRead || pRowsUnread || pRowsStar) {
                int mappedRow = proxyModel->mapToSource(*it).row();

                if (pRows) pRows->append(mappedRow);

                if (MessagesModel->item(mappedRow, COLUMN_DATA)->data(ROLE_UNREAD).toBool()) {
                    if (pRowsUnread) pRowsUnread->append(mappedRow);
                } else {
                    if (pRowsRead) pRowsRead->append(mappedRow);
                }

                if (pRowsStar) {
                    if (MessagesModel->item(mappedRow, COLUMN_DATA)->data(ROLE_MSGFLAGS).toInt() & RS_MSG_STAR) {
                        pRowsStar->append(mappedRow);
                    }
                }
            }
        }
    }

    return rowList.size();
}

bool MessagesDialog::isMessageRead(int nRow)
{
    QStandardItem *item = MessagesModel->item(nRow,COLUMN_DATA);
    return !item->data(ROLE_UNREAD).toBool();
}

bool MessagesDialog::hasMessageStar(int nRow)
{
    QStandardItem *item = MessagesModel->item(nRow,COLUMN_DATA);
    return item->data(ROLE_MSGFLAGS).toInt() & RS_MSG_STAR;
}

void MessagesDialog::messageslistWidgetCustomPopupMenu( QPoint /*point*/ )
{
    std::string cid;
    std::string mid;

    MessageInfo msgInfo;
    if (getCurrentMsg(cid, mid)) {
        rsMsgs->getMessage(mid, msgInfo);
    }

    QList<int> RowsRead;
    QList<int> RowsUnread;
    QList<int> RowsStar;
    int nCount = getSelectedMsgCount (NULL, &RowsRead, &RowsUnread, &RowsStar);

    /** Defines the actions for the context menu */

    QMenu contextMnu( this );

    QAction *action = contextMnu.addAction(tr("Open in a new window"), this, SLOT(openAsWindow()));
    if (nCount != 1) {
        action->setDisabled(true);
    }

    action = contextMnu.addAction(tr("Open in a new tab"), this, SLOT(openAsTab()));
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
    if (RowsUnread.size() == 0) {
        action->setDisabled(true);
    }

    action = contextMnu.addAction(QIcon(":/images/message-mail.png"), tr("Mark as unread"), this, SLOT(markAsUnread()));
    if (RowsRead.size() == 0) {
        action->setDisabled(true);
    }

    action = contextMnu.addAction(tr("Add Star"));
    action->setCheckable(true);
    action->setChecked(RowsStar.size());
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

	 if(nCount==1 && (msgInfo.msgflags & RS_MSG_ENCRYPTED))
		 action = contextMnu.addAction(QIcon(IMAGE_SYSTEM), tr("Decrypt Message"), this, SLOT(decryptSelectedMsg()));

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

    MessagesModel->removeRows (0, MessagesModel->rowCount());

    ui.quickViewWidget->setCurrentItem(NULL);
    listMode = LIST_BOX;

    insertMessages();
    insertMsgTxtAndFiles();

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

    MessagesModel->removeRows (0, MessagesModel->rowCount());

    ui.listWidget->setCurrentItem(NULL);
    listMode = LIST_QUICKVIEW;

    insertMessages();
    insertMsgTxtAndFiles();

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

static void InitIconAndFont(QStandardItem *item[COLUMN_COUNT])
{
    int msgFlags = item[COLUMN_DATA]->data(ROLE_MSGFLAGS).toInt();

    // show the real "New" state
    if (msgFlags & RS_MSG_NEW) {
        item[COLUMN_SUBJECT]->setIcon(QIcon(":/images/message-state-new.png"));
    } else {
        if (msgFlags & RS_MSG_USER_REQUEST) {
            item[COLUMN_SUBJECT]->setIcon(QIcon(":/images/user/user_request16.png"));
        } else if (msgFlags & RS_MSG_FRIEND_RECOMMENDATION) {
            item[COLUMN_SUBJECT]->setIcon(QIcon(":/images/user/friend_suggestion16.png"));
        } else if (msgFlags & RS_MSG_UNREAD_BY_USER) {
            if ((msgFlags & (RS_MSG_REPLIED | RS_MSG_FORWARDED)) == RS_MSG_REPLIED) {
                item[COLUMN_SUBJECT]->setIcon(QIcon(":/images/message-mail-replied.png"));
            } else if ((msgFlags & (RS_MSG_REPLIED | RS_MSG_FORWARDED)) == RS_MSG_FORWARDED) {
                item[COLUMN_SUBJECT]->setIcon(QIcon(":/images/message-mail-forwarded.png"));
            } else if ((msgFlags & (RS_MSG_REPLIED | RS_MSG_FORWARDED)) == (RS_MSG_REPLIED | RS_MSG_FORWARDED)) {
                item[COLUMN_SUBJECT]->setIcon(QIcon(":/images/message-mail-replied-forw.png"));
            } else {
                item[COLUMN_SUBJECT]->setIcon(QIcon(":/images/message-mail.png"));
            }
        } else {
            if ((msgFlags & (RS_MSG_REPLIED | RS_MSG_FORWARDED)) == RS_MSG_REPLIED) {
                item[COLUMN_SUBJECT]->setIcon(QIcon(":/images/message-mail-replied-read.png"));
            } else if ((msgFlags & (RS_MSG_REPLIED | RS_MSG_FORWARDED)) == RS_MSG_FORWARDED) {
                item[COLUMN_SUBJECT]->setIcon(QIcon(":/images/message-mail-forwarded-read.png"));
            } else if ((msgFlags & (RS_MSG_REPLIED | RS_MSG_FORWARDED)) == (RS_MSG_REPLIED | RS_MSG_FORWARDED)) {
                item[COLUMN_SUBJECT]->setIcon(QIcon(":/images/message-mail-replied-forw-read.png"));
            } else {
                item[COLUMN_SUBJECT]->setIcon(QIcon(":/images/message-mail-read.png"));
            }
        }
    }

    if (msgFlags & RS_MSG_STAR) {
        item[COLUMN_STAR]->setIcon(QIcon(IMAGE_STAR_ON));
        item[COLUMN_STAR]->setText("1");
        item[COLUMN_STAR]->setData(1, ROLE_SORT);
    } else {
        item[COLUMN_STAR]->setIcon(QIcon(IMAGE_STAR_OFF));
        item[COLUMN_STAR]->setText("0");
        item[COLUMN_STAR]->setData(0, ROLE_SORT);
    }

    bool isNew = msgFlags & (RS_MSG_NEW | RS_MSG_UNREAD_BY_USER);

    // set icon
    if (isNew) {
        item[COLUMN_UNREAD]->setIcon(QIcon(":/images/message-state-unread.png"));
        item[COLUMN_UNREAD]->setText("1");
        item[COLUMN_UNREAD]->setData(1, ROLE_SORT);
    } else {
        item[COLUMN_UNREAD]->setIcon(QIcon(":/images/message-state-read.png"));
        item[COLUMN_UNREAD]->setText("0");
        item[COLUMN_UNREAD]->setData(0, ROLE_SORT);
    }

    // set font
    for (int i = 0; i < COLUMN_COUNT; i++) {
        QFont qf = item[i]->font();
        qf.setBold(isNew);
        item[i]->setFont(qf);
    }

    item[COLUMN_DATA]->setData(isNew, ROLE_UNREAD);
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

    std::string ownId = rsPeers->getOwnId();

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
            QListWidgetItem *item = ui.listWidget->currentItem();
            if (item) {
                boxIcon = item->icon();
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
            QListWidgetItem *item = ui.quickViewWidget->currentItem();
            if (item) {
                quickViewType = item->data(ROLE_QUICKVIEW_TYPE).toInt();
                quickViewId = item->data(ROLE_QUICKVIEW_ID).toInt();

                boxText = item->text();
                boxIcon = item->icon();

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
    ui.messagestreeView->setPlaceholderText(placeholderText);

    if (msgbox == RS_MSG_INBOX) {
        MessagesModel->setHeaderData(COLUMN_FROM, Qt::Horizontal, tr("From"));
        MessagesModel->setHeaderData(COLUMN_FROM, Qt::Horizontal, tr("Click to sort by from"), Qt::ToolTipRole);
    } else {
        MessagesModel->setHeaderData(COLUMN_FROM, Qt::Horizontal, tr("To"));
		MessagesModel->setHeaderData(COLUMN_FROM, Qt::Horizontal, tr("Click to sort by to"), Qt::ToolTipRole);
    }

    if (doFill) {
        MsgTagType Tags;
        rsMsgs->getMessageTagTypes(Tags);

        /* search messages */
        std::list<MsgInfoSummary> msgToShow;
        for(it = msgList.begin(); it != msgList.end(); it++) {
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
        int nRowCount = MessagesModel->rowCount();
        int nRow = 0;
        for (nRow = 0; nRow < nRowCount; ) {
            std::string msgIdFromRow = MessagesModel->item(nRow, COLUMN_DATA)->data(ROLE_MSGID).toString().toStdString();
            for(it = msgToShow.begin(); it != msgToShow.end(); it++) {
                if (it->msgId == msgIdFromRow) {
                    break;
                }
            }

            if (it == msgToShow.end ()) {
                MessagesModel->removeRow (nRow);
                nRowCount = MessagesModel->rowCount();
            } else {
                nRow++;
            }
        }

        for(it = msgToShow.begin(); it != msgToShow.end(); it++)
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
            nRowCount = MessagesModel->rowCount();
            for (nRow = 0; nRow < nRowCount; nRow++) {
                if (it->msgId == MessagesModel->item(nRow, COLUMN_DATA)->data(ROLE_MSGID).toString().toStdString()) {
                    break;
                }
            }

            /* make a widget per friend */

            QStandardItem *item [COLUMN_COUNT];

            bool bInsert = false;

            if (nRow < nRowCount) {
                for (int i = 0; i < COLUMN_COUNT; i++) {
                    item[i] = MessagesModel->item(nRow, i);
                }
            } else {
                for (int i = 0; i < COLUMN_COUNT; i++) {
                    item[i] = new QStandardItem();
                }
                bInsert = true;
            }

            //set this false if you want to expand on double click
            for (int i = 0; i < COLUMN_COUNT; i++) {
                item[i]->setEditable(false);
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
                    item[COLUMN_DATE]->setData(DateTime::formatTime(qdatetime.time()), Qt::DisplayRole);
                }
                else
                {
                    item[COLUMN_DATE]->setData(DateTime::formatDateTime(qdatetime), Qt::DisplayRole);
                }
                // for sorting
                item[COLUMN_DATE]->setData(qdatetime, ROLE_SORT);
            }

            //  From ....
            {
                if (msgbox == RS_MSG_INBOX || msgbox == RS_MSG_OUTBOX) {
                    if ((it->msgflags & RS_MSG_SYSTEM) && it->srcId == ownId) {
                        text = "RetroShare";
                    } else {
                        text = QString::fromUtf8(rsPeers->getPeerName(it->srcId).c_str());
                    }
                } else {
                    if (gotInfo || rsMsgs->getMessage(it->msgId, msgInfo)) {
                        gotInfo = true;

                        text.clear();

                        std::list<std::string>::const_iterator pit;
                        for (pit = msgInfo.msgto.begin(); pit != msgInfo.msgto.end(); pit++)
                        {
                            if (text.isEmpty() == false) {
                                text += ", ";
                            }

                            std::string peerName = rsPeers->getPeerName(*pit);
                            if (peerName.empty()) {
                                text += PeerDefs::rsid("", *pit);
                            } else {
                                text += QString::fromUtf8(peerName.c_str());
                            }
                        }
                    } else {
                        std::cerr << "MessagesDialog::insertMsgTxtAndFiles() Couldn't find Msg" << std::endl;
                    }
                }
                item[COLUMN_FROM]->setText(text);
                item[COLUMN_FROM]->setData(text + dateString, ROLE_SORT);
            }

            // Subject
				if(it->msgflags & RS_MSG_ENCRYPTED)
					text = tr("Encrypted message. Right-click to decrypt it.") ;
				else
					text = QString::fromStdString(it->title);

            item[COLUMN_SUBJECT]->setText(text);
            item[COLUMN_SUBJECT]->setData(text + dateString, ROLE_SORT);

            // internal data
            QString msgId = QString::fromStdString(it->msgId);
            item[COLUMN_DATA]->setData(QString::fromStdString(it->srcId), ROLE_SRCID);
            item[COLUMN_DATA]->setData(msgId, ROLE_MSGID);
            item[COLUMN_DATA]->setData(it->msgflags, ROLE_MSGFLAGS);

            // Init icon and font
            InitIconAndFont(item);

            // Tags
            MsgTagInfo tagInfo;
            rsMsgs->getMessageTag(it->msgId, tagInfo);

            text.clear();

            // build tag names
            std::map<uint32_t, std::pair<std::string, uint32_t> >::iterator Tag;
            for (std::list<uint32_t>::iterator tagId = tagInfo.tagIds.begin(); tagId != tagInfo.tagIds.end(); tagId++) {
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
            item[COLUMN_TAGS]->setText(text);
            item[COLUMN_TAGS]->setData(text, ROLE_SORT);

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
                color = ui.messagestreeView->palette().color(QPalette::Text);
            }
            QBrush brush = QBrush(color);
            for (int i = 0; i < COLUMN_COUNT; i++) {
                item[i]->setForeground(brush);
            }

            // No of Files.
            {
                item[COLUMN_ATTACHEMENTS] -> setText(QString::number(it -> count));
                item[COLUMN_ATTACHEMENTS] -> setData(item[COLUMN_ATTACHEMENTS]->text() + dateString, ROLE_SORT);
                item[COLUMN_ATTACHEMENTS] -> setTextAlignment(Qt::AlignHCenter);
            }

            if (filterColumn == COLUMN_CONTENT) {
                // need content for filter
                if (gotInfo || rsMsgs->getMessage(it->msgId, msgInfo)) {
                    gotInfo = true;
                    QTextDocument doc;
                    doc.setHtml(QString::fromStdString(msgInfo.msg));
                    item[COLUMN_CONTENT]->setText(doc.toPlainText().replace(QString("\n"), QString(" ")));
                } else {
                    std::cerr << "MessagesDialog::insertMsgTxtAndFiles() Couldn't find Msg" << std::endl;
                    item[COLUMN_CONTENT]->setText("");
                }
            }

				if(it->msgflags & RS_MSG_ENCRYPTED)
				{
					item[COLUMN_SIGNATURE]->setIcon(QIcon(":/images/blue_lock.png")) ;
					item[COLUMN_SIGNATURE]->setToolTip(tr("This message is encrypted. Right click to decrypt it.")) ;
				}
				else 
					if(it->msgflags & RS_MSG_SIGNED) 
						if(it->msgflags & RS_MSG_SIGNATURE_CHECKS)
						{
							item[COLUMN_SIGNATURE]->setIcon(QIcon(":/images/stock_signature_ok.png")) ;
							item[COLUMN_SIGNATURE]->setToolTip(tr("This message was signed and the signature checks")) ;
						}
						else
						{
							item[COLUMN_SIGNATURE]->setIcon(QIcon(":/images/stock_signature_bad.png")) ;
							item[COLUMN_SIGNATURE]->setToolTip(tr("This message was signed but the signature doesn't check")) ;
						}
					else
						item[COLUMN_SIGNATURE]->setIcon(QIcon()) ;

				if (bInsert) {
                /* add to the list */
                QList<QStandardItem *> itemList;
                for (int i = 0; i < COLUMN_COUNT; i++) {
                    itemList.append(item[i]);
                }
                MessagesModel->appendRow(itemList);
            }
        }
    } else {
        MessagesModel->removeRows (0, MessagesModel->rowCount());
    }

    ui.messagestreeView->showColumn(COLUMN_ATTACHEMENTS);
    ui.messagestreeView->showColumn(COLUMN_SUBJECT);
    ui.messagestreeView->showColumn(COLUMN_UNREAD);
    ui.messagestreeView->showColumn(COLUMN_FROM);
    ui.messagestreeView->showColumn(COLUMN_DATE);
    ui.messagestreeView->showColumn(COLUMN_TAGS);
    ui.messagestreeView->hideColumn(COLUMN_CONTENT);

    updateMessageSummaryList();
}

// current row in messagestreeView has changed
void MessagesDialog::currentChanged(const QModelIndex &index )
{
    timer->stop();
    timerIndex = index;
    timer->start();
}

// click in messagestreeView
void MessagesDialog::clicked(const QModelIndex &index )
{
    if (index.isValid() == false) {
        return;
    }

    switch (index.column()) {
    case COLUMN_UNREAD:
        {
            int mappedRow = proxyModel->mapToSource(index).row();

            QList<int> Rows;
            Rows.append(mappedRow);
            setMsgAsReadUnread(Rows, !isMessageRead(mappedRow));
            insertMsgTxtAndFiles(index, false);
            updateMessageSummaryList();
            return;
        }
    case COLUMN_STAR:
        {
            int mappedRow = proxyModel->mapToSource(index).row();

            QList<int> Rows;
            Rows.append(mappedRow);
            setMsgStar(Rows, !hasMessageStar(mappedRow));
            return;
        }
    }

    timer->stop();
    timerIndex = index;
    // show current message directly
    updateCurrentMessage();
}

// double click in messagestreeView
void MessagesDialog::doubleClicked(const QModelIndex &index)
{
    /* activate row */
    clicked (index);

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
    insertMsgTxtAndFiles(timerIndex);
}

void MessagesDialog::setMsgAsReadUnread(const QList<int> &Rows, bool read)
{
    LockUpdate Lock (this, false);

    for (int nRow = 0; nRow < Rows.size(); nRow++) {
        QStandardItem* item[COLUMN_COUNT];
        for(int nCol = 0; nCol < COLUMN_COUNT; nCol++)
        {
            item[nCol] = MessagesModel->item(Rows [nRow], nCol);
        }

        std::string mid = item[COLUMN_DATA]->data(ROLE_MSGID).toString().toStdString();

        if (rsMsgs->MessageRead(mid, !read)) {
            int msgFlag = item[COLUMN_DATA]->data(ROLE_MSGFLAGS).toInt();
            msgFlag &= ~RS_MSG_NEW;

            if (read) {
                msgFlag &= ~RS_MSG_UNREAD_BY_USER;
            } else {
                msgFlag |= RS_MSG_UNREAD_BY_USER;
            }

            item[COLUMN_DATA]->setData(msgFlag, ROLE_MSGFLAGS);

            InitIconAndFont(item);
        }
    }

    // LockUpdate
}

void MessagesDialog::markAsRead()
{
    QList<int> RowsUnread;
    getSelectedMsgCount (NULL, NULL, &RowsUnread, NULL);

    setMsgAsReadUnread (RowsUnread, true);
    updateMessageSummaryList();
}

void MessagesDialog::markAsUnread()
{
    QList<int> RowsRead;
    getSelectedMsgCount (NULL, &RowsRead, NULL, NULL);

    setMsgAsReadUnread (RowsRead, false);
    updateMessageSummaryList();
}

void MessagesDialog::markWithStar(bool checked)
{
    QList<int> Rows;
    getSelectedMsgCount (&Rows, NULL, NULL, NULL);

    setMsgStar(Rows, checked);
}

void MessagesDialog::setMsgStar(const QList<int> &Rows, bool star)
{
    LockUpdate Lock (this, false);

    for (int nRow = 0; nRow < Rows.size(); nRow++) {
        QStandardItem* item[COLUMN_COUNT];
        for(int nCol = 0; nCol < COLUMN_COUNT; nCol++)
        {
            item[nCol] = MessagesModel->item(Rows [nRow], nCol);
        }

        std::string mid = item[COLUMN_DATA]->data(ROLE_MSGID).toString().toStdString();

        if (rsMsgs->MessageStar(mid, star)) {
            int msgFlag = item[COLUMN_DATA]->data(ROLE_MSGFLAGS).toInt();
            msgFlag &= ~RS_MSG_STAR;

            if (star) {
                msgFlag |= RS_MSG_STAR;
            } else {
                msgFlag &= ~RS_MSG_STAR;
            }

            item[COLUMN_DATA]->setData(msgFlag, ROLE_MSGFLAGS);

            InitIconAndFont(item);

            Lock.setUpdate(true);
        }
    }

    // LockUpdate
}

void MessagesDialog::insertMsgTxtAndFiles(QModelIndex Index, bool bSetToRead)
{
    std::cerr << "MessagesDialog::insertMsgTxtAndFiles()" << std::endl;

    /* get its Ids */
    std::string cid;
    std::string mid;

    QModelIndex currentIndex = proxyModel->mapToSource(Index);
    if (currentIndex.isValid() == false) {
        mCurrMsgId.clear();
        msgWidget->fill(mCurrMsgId);
        updateInterface();
        return;
    }

    QStandardItem *item = MessagesModel->item(currentIndex.row(),COLUMN_DATA);
    if (item == NULL) {
        mCurrMsgId.clear();
        msgWidget->fill(mCurrMsgId);
        updateInterface();
        return;
    }
    mid = item->data(ROLE_MSGID).toString().toStdString();

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

    QList<int> Rows;
    Rows.append(currentIndex.row());

    bool bSetToReadOnActive = Settings->getMsgSetToReadOnActivate();

    if (msgInfo.msgflags & RS_MSG_NEW) {
        // set always to read or unread
        if (bSetToReadOnActive == false || bSetToRead == false) {
            // set locally to unread
            setMsgAsReadUnread(Rows, false);
        } else {
            setMsgAsReadUnread(Rows, true);
        }
        updateMessageSummaryList();
    } else {
        if ((msgInfo.msgflags & RS_MSG_UNREAD_BY_USER) && bSetToRead && bSetToReadOnActive) {
            // set to read
            setMsgAsReadUnread(Rows, true);
            updateMessageSummaryList();
        }
    }

    msgWidget->fill(mCurrMsgId);
    updateInterface();
}

void MessagesDialog::decryptSelectedMsg()
{
    MessageInfo msgInfo;

    if (!rsMsgs->getMessage(mCurrMsgId, msgInfo)) 
		 return ;

	 if(!msgInfo.msgflags & RS_MSG_ENCRYPTED)
	 {
		 QMessageBox::warning(NULL,tr("Decryption failed!"),tr("This message is not encrypted. Cannot decrypt!")) ;
		 return ;
	 }

	 if(!rsMsgs->decryptMessage(mCurrMsgId) )
		 QMessageBox::warning(NULL,tr("Decryption failed!"),tr("This message could not be decrypted.")) ;

	 //setMsgAsReadUnread(currentIndex.row(), true);
    timer->start();

	 updateMessageSummaryList();

	 //QModelIndex currentIndex = ui.messagestreeView->currentIndex();
	 //QModelIndex index = ui.messagestreeView->model()->index(currentIndex.row(), COLUMN_UNREAD, currentIndex.parent());
	 //currentChanged(index);

    MessagesModel->removeRows (0, MessagesModel->rowCount());

    insertMessages();
    insertMsgTxtAndFiles();
}

bool MessagesDialog::getCurrentMsg(std::string &cid, std::string &mid)
{
    QModelIndex currentIndex = ui.messagestreeView->currentIndex();
    currentIndex = proxyModel->mapToSource(currentIndex);
    int rowSelected = -1;

    /* get its Ids */
    if (currentIndex.isValid() == false)
    {
        //If no message is selected. assume first message is selected.
        if(MessagesModel->rowCount() == 0)
        {
            return false;
        }
        else
        {
            rowSelected = 0;
        }
    }
    else
    {
        rowSelected = currentIndex.row();
    }

    QStandardItem *item = MessagesModel->item(rowSelected,COLUMN_DATA);
    if (item == NULL) {
        return false;
    }
    cid = item->data(ROLE_SRCID).toString().toStdString();
    mid = item->data(ROLE_MSGID).toString().toStdString();
    return true;
}

void MessagesDialog::removemessage()
{
    LockUpdate Lock (this, true);

    QList<QModelIndex> selectedIndexList= ui.messagestreeView->selectionModel() -> selectedIndexes ();
    QList<int> rowList;
    QModelIndex selectedIndex;

    for(QList<QModelIndex>::iterator it = selectedIndexList.begin(); it != selectedIndexList.end(); it++) {
        selectedIndex = proxyModel->mapToSource(*it);
        int row = selectedIndex.row();
        if (rowList.contains(row) == false) {
            rowList.append(row);
        }
    }

    bool bDelete = false;
    int listrow = ui.listWidget->currentRow();
    if (listrow == ROW_TRASHBOX) {
        bDelete = true;
    } else {
        if (QApplication::keyboardModifiers() & Qt::ShiftModifier) {
            bDelete = true;
        }
    }

    for(QList<int>::const_iterator it1 = rowList.begin(); it1 != rowList.end(); it1++) {
        QStandardItem *pItem = MessagesModel->item((*it1), COLUMN_DATA);
        if (pItem) {
            QString mid = pItem->data(ROLE_MSGID).toString();

            // close tab showing this message
//            closeTab(mid.toStdString());

            if (bDelete) {
                rsMsgs->MessageDelete(mid.toStdString());
            } else {
                rsMsgs->MessageToTrash(mid.toStdString(), true);
            }
        }
    }

    // LockUpdate -> insertMessages();
}

void MessagesDialog::undeletemessage()
{
    LockUpdate Lock (this, true);

    QList<int> Rows;
    getSelectedMsgCount (&Rows, NULL, NULL, NULL);
    for (int nRow = 0; nRow < Rows.size(); nRow++) {
        QString mid = MessagesModel->item (Rows [nRow], COLUMN_DATA)->data(ROLE_MSGID).toString();
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
    QRegExp regExp(text, Qt::CaseInsensitive, QRegExp::FixedString);
    proxyModel->setFilterRegExp(regExp);
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
    proxyModel->setFilterKeyColumn(column);

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
    for (it = msgList.begin(); it != msgList.end(); it++) {
        /* calcluate tag count */
        MsgTagInfo tagInfo;
        rsMsgs->getMessageTag(it->msgId, tagInfo);
        for (std::list<uint32_t>::iterator tagId = tagInfo.tagIds.begin(); tagId != tagInfo.tagIds.end(); tagId++) {
            int nCount = tagCount [*tagId];
            nCount++;
            tagCount [*tagId] = nCount;
        }

        if (it->msgflags & RS_MSG_STAR) {
            starredCount++;
        }

        if (it->msgflags & RS_MSG_SYSTEM) {
            systemCount++;
        }

        /* calculate box */
        if (it->msgflags & RS_MSG_TRASH) {
            trashboxCount++;
            continue;
        }

        switch (it->msgflags & RS_MSG_BOXMASK) {
        case RS_MSG_INBOX:
                inboxCount++;
                if (it->msgflags & (RS_MSG_NEW | RS_MSG_UNREAD_BY_USER)) {
                    newInboxCount++;
                }
                break;
        case RS_MSG_OUTBOX:
                newOutboxCount++;
                break;
        case RS_MSG_DRAFTBOX:
                newDraftCount++;
                break;
        case RS_MSG_SENTBOX:
                newSentboxCount++;
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
        item->setForeground(QBrush(ui.messagestreeView->palette().color(QPalette::Text)));
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
    for (int row = 0; row < rowCount; row++) {
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

	QList<int> rows;
	getSelectedMsgCount (&rows, NULL, NULL, NULL);

	if (rows.size()) {
		QStandardItem* item = MessagesModel->item(rows [0], COLUMN_DATA);
		std::string msgId = item->data(ROLE_MSGID).toString().toStdString();

		rsMsgs->getMessageTag(msgId, tagInfo);
	}

	menu->activateActions(tagInfo.tagIds);
}

void MessagesDialog::tagRemoveAll()
{
	LockUpdate Lock (this, false);

	QList<int> rows;
	getSelectedMsgCount (&rows, NULL, NULL, NULL);
	for (int row = 0; row < rows.size(); row++) {
		QStandardItem* item = MessagesModel->item(rows [row], COLUMN_DATA);
		std::string msgId = item->data(ROLE_MSGID).toString().toStdString();

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

	QList<int> rows;
	getSelectedMsgCount (&rows, NULL, NULL, NULL);
	for (int row = 0; row < rows.size(); row++) {
		QStandardItem* item = MessagesModel->item(rows [row], COLUMN_DATA);
		std::string msgId = item->data(ROLE_MSGID).toString().toStdString();

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
    for (it = msgList.begin(); it != msgList.end(); it++) {
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

    for (int tab = 1; tab < ui.tabWidget->count(); tab++) {
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
