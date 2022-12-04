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

#include "gui/notifyqt.h"
#include "gui/common/TagDefs.h"
#include "gui/common/PeerDefs.h"
#include "gui/common/RSElidedItemDelegate.h"
#include "gui/gxs/GxsIdTreeWidgetItem.h"
#include "gui/gxs/GxsIdDetails.h"
#include "gui/Identity/IdDialog.h"
#include "gui/MainWindow.h"
#include "gui/msgs/MessageComposer.h"
#include "gui/msgs/MessageInterface.h"
#include "gui/msgs/MessageUserNotify.h"
#include "gui/msgs/MessageWidget.h"
#include "gui/msgs/TagsMenu.h"
#include "gui/msgs/MessageModel.h"
#include "gui/settings/rsharesettings.h"

#include "util/DateTime.h"
#include "util/misc.h"
#include "util/QtVersion.h"
#include "util/qtthreadsutils.h"
#include "util/RsProtectedTimer.h"

#include <retroshare/rspeers.h>
#include <retroshare/rsmsgs.h>

#include <algorithm>

/* Images for context menu icons */
#define IMAGE_MAIL             ":/icons/png/message.png"
#define IMAGE_MESSAGE          ":/icons/mail/compose.png"
#define IMAGE_MESSAGEREMOVE    ":/icons/mail/delete.png"
#define IMAGE_STAR_ON          ":/images/star-on-16.png"
#define IMAGE_STAR_OFF         ":/images/star-off-16.png"
#define IMAGE_SYSTEM           ":/icons/notification.png"
#define IMAGE_DECRYPTMESSAGE   ":/images/decrypt-mail.png"
#define IMAGE_AUTHOR_INFO      ":/images/info16.png"
#define IMAGE_NOTFICATION      ":/icons/notification.png"
#define IMAGE_SPAM_ON          ":/images/junk_on.png"
#define IMAGE_SPAM_OFF         ":/images/junk_off.png"
#define IMAGE_ATTACHMENTS      ":/icons/mail/attach24.png"
#define IMAGE_ATTACHMENT      ":/icons/mail/attach16.png"

#define IMAGE_INBOX             ":/images/folder-inbox.png"
#define IMAGE_OUTBOX            ":/images/folder-outbox.png"
#define IMAGE_SENT              ":/images/folder-sent.png"
#define IMAGE_DRAFTS            ":/images/folder-draft.png"
#define IMAGE_TRASH             ":/images/folder-trash.png"
#define IMAGE_FOLDER            ":/images/foldermail.png"

#define ROLE_QUICKVIEW_TYPE Qt::UserRole
#define ROLE_QUICKVIEW_ID   Qt::UserRole + 1
#define ROLE_QUICKVIEW_TEXT Qt::UserRole + 2

#define QUICKVIEW_TYPE_NOTHING 0
#define QUICKVIEW_TYPE_STATIC  1
#define QUICKVIEW_TYPE_TAG     2

#define QUICKVIEW_STATIC_ID_STARRED 1
#define QUICKVIEW_STATIC_ID_SYSTEM  2
#define QUICKVIEW_STATIC_ID_SPAM  3
#define QUICKVIEW_STATIC_ID_ATTACHMENT  4

#define ROW_INBOX         0
#define ROW_OUTBOX        1
#define ROW_DRAFTBOX      2
#define ROW_SENTBOX       3
#define ROW_TRASHBOX      4


class MessageSortFilterProxyModel: public QSortFilterProxyModel
{
public:
	MessageSortFilterProxyModel(QObject *parent = NULL)
	    : QSortFilterProxyModel(parent), m_sortingEnabled(false)
	{
		setDynamicSortFilter(false); // causes crashes when true
	}

	void sort( int column, Qt::SortOrder order = Qt::AscendingOrder ) override
	{
		if(m_sortingEnabled)
			return QSortFilterProxyModel::sort(column,order) ;
	}

	void setSortingEnabled(bool b) { m_sortingEnabled = b ; }

protected:
	bool lessThan(const QModelIndex& left, const QModelIndex& right) const override
	{
		return sourceModel()->data(left, RsMessageModel::SortRole) < sourceModel()->data(right, RsMessageModel::SortRole) ;
	}

	bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override
	{
		//No need Column as Model filter check with filter selection
		return sourceModel()->index(source_row,0,source_parent).data(RsMessageModel::FilterRole).toString() == RsMessageModel::FilterString ;
	}

private:
	bool m_sortingEnabled;
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
    lastSelectedIndex = QModelIndex();
    mLastCurrentQuickViewRow = -1;

    msgWidget = new MessageWidget(true, this);
	ui.msgLayout->addWidget(msgWidget);
	connect(msgWidget, SIGNAL(messageRemoved()), this, SLOT(messageRemoved()));

    connectActions();

    //listMode = LIST_NOTHING;

    mMessageModel = new RsMessageModel(this);
    mMessageProxyModel = new MessageSortFilterProxyModel(this);
    mMessageProxyModel->setSourceModel(mMessageModel);
    mMessageProxyModel->setSortRole(RsMessageModel::SortRole);
    mMessageProxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
	mMessageProxyModel->setFilterRole(RsMessageModel::FilterRole);
	mMessageProxyModel->setFilterRegExp(QRegExp(RsMessageModel::FilterString));

    ui.messageTreeWidget->setModel(mMessageProxyModel);

	changeBox(0);	// set to inbox

    RSElidedItemDelegate *itemDelegate = new RSElidedItemDelegate(this);
    itemDelegate->setSpacing(QSize(0, 2));
    ui.messageTreeWidget->setItemDelegateForColumn(RsMessageModel::COLUMN_THREAD_SUBJECT,itemDelegate);

    ui.messageTreeWidget->setItemDelegateForColumn(RsMessageModel::COLUMN_THREAD_AUTHOR,new GxsIdTreeItemDelegate()) ;
    ui.messageTreeWidget->setItemDelegateForColumn(RsMessageModel::COLUMN_THREAD_TO,new GxsIdTreeItemDelegate()) ;

    // workaround for Qt bug, should be solved in next Qt release 4.7.0
    // http://bugreports.qt.nokia.com/browse/QTBUG-8270
    QShortcut *Shortcut = new QShortcut(QKeySequence (Qt::Key_Delete), ui.messageTreeWidget, 0, 0, Qt::WidgetShortcut);
    connect(Shortcut, SIGNAL(activated()), this, SLOT( removemessage ()));
    Shortcut = new QShortcut(QKeySequence (Qt::SHIFT | Qt::Key_Delete), ui.messageTreeWidget, 0, 0, Qt::WidgetShortcut);
    connect(Shortcut, SIGNAL(activated()), this, SLOT( removemessage ()));

    QFontMetricsF fm(font());

    /* Set initial section sizes */
    QHeaderView * msgwheader = ui.messageTreeWidget->header () ;

    // Set initial size of the splitter
    ui.listSplitter->setStretchFactor(0, 0);
    ui.listSplitter->setStretchFactor(1, 1);
	
    // Set initial size of the splitter
    ui.boxSplitter->setStretchFactor(0, 0);
    ui.boxSplitter->setStretchFactor(1, 1);

    /* add filter actions */
    ui.filterLineEdit->addFilter(QIcon(), tr("Subject"),     RsMessageModel::COLUMN_THREAD_SUBJECT,   tr("Search Subject"));
    ui.filterLineEdit->addFilter(QIcon(), tr("From"),        RsMessageModel::COLUMN_THREAD_AUTHOR,    tr("Search From"));
    ui.filterLineEdit->addFilter(QIcon(), tr("To"),          RsMessageModel::COLUMN_THREAD_TO,        tr("Search To"));
    ui.filterLineEdit->addFilter(QIcon(), tr("Date"),        RsMessageModel::COLUMN_THREAD_DATE,      tr("Search Date"));
    ui.filterLineEdit->addFilter(QIcon(), tr("Content"),     RsMessageModel::COLUMN_THREAD_CONTENT,   tr("Search Content"));
    ui.filterLineEdit->addFilter(QIcon(), tr("Tags"),        RsMessageModel::COLUMN_THREAD_TAGS,      tr("Search Tags"));
    ui.filterLineEdit->addFilter(QIcon(), tr("Attachments"), RsMessageModel::COLUMN_THREAD_ATTACHMENT,tr("Search Attachments"));

    //setting default filter by column as subject
    ui.filterLineEdit->setCurrentFilter(RsMessageModel::COLUMN_THREAD_SUBJECT);

    ///////////////////////////////////////////////////////////////////////////////////////
    // Post "load settings" actions (which makes sure they are not affected by settings) //
    ///////////////////////////////////////////////////////////////////////////////////////

    ui.messageTreeWidget->setColumnHidden(RsMessageModel::COLUMN_THREAD_CONTENT,true);
    ui.messageTreeWidget->setColumnHidden(RsMessageModel::COLUMN_THREAD_MSGID,true);

    msgwheader->resizeSection (RsMessageModel::COLUMN_THREAD_STAR,       fm.width('0')*1.5);
    msgwheader->resizeSection (RsMessageModel::COLUMN_THREAD_ATTACHMENT, fm.width('0')*1.5);
    msgwheader->resizeSection (RsMessageModel::COLUMN_THREAD_READ,       fm.width('0')*1.5);
    msgwheader->resizeSection (RsMessageModel::COLUMN_THREAD_SPAM,       fm.width('0')*1.5);

    msgwheader->resizeSection (RsMessageModel::COLUMN_THREAD_SUBJECT,    fm.width("You have a message")*3.0);
    msgwheader->resizeSection (RsMessageModel::COLUMN_THREAD_AUTHOR,     fm.width("[Retroshare]")*1.1);
    msgwheader->resizeSection (RsMessageModel::COLUMN_THREAD_TO,         fm.width("[Retroshare]")*1.1);
    msgwheader->resizeSection (RsMessageModel::COLUMN_THREAD_DATE,       fm.width("01/01/1970")*1.1);

    msgwheader->setSectionResizeMode(RsMessageModel::COLUMN_THREAD_SUBJECT,    QHeaderView::Interactive);
    msgwheader->setSectionResizeMode(RsMessageModel::COLUMN_THREAD_AUTHOR,     QHeaderView::Interactive);
    msgwheader->setSectionResizeMode(RsMessageModel::COLUMN_THREAD_TO,         QHeaderView::Interactive);
    msgwheader->setSectionResizeMode(RsMessageModel::COLUMN_THREAD_DATE,       QHeaderView::Interactive);

    msgwheader->setSectionResizeMode(RsMessageModel::COLUMN_THREAD_STAR,       QHeaderView::Fixed);
    msgwheader->setSectionResizeMode(RsMessageModel::COLUMN_THREAD_ATTACHMENT, QHeaderView::Fixed);
    msgwheader->setSectionResizeMode(RsMessageModel::COLUMN_THREAD_READ,       QHeaderView::Fixed);
    msgwheader->setSectionResizeMode(RsMessageModel::COLUMN_THREAD_SPAM,       QHeaderView::Fixed);

	ui.messageTreeWidget->setSortingEnabled(true);

    /* Set header sizes for the fixed columns and resize modes, must be set after processSettings */
    msgwheader->setStretchLastSection(true);

	QFontMetricsF fontMetrics(ui.messageTreeWidget->font());
	int iconHeight = fontMetrics.height() * 1.4;
	ui.messageTreeWidget->setIconSize(QSize(iconHeight, iconHeight));

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

	sortColumn(RsMessageModel::COLUMN_THREAD_DATE,Qt::DescendingOrder);

    //ui.messageTreeWidget->installEventFilter(this);

	// remove close button of the the first tab
	ui.tabWidget->hideCloseButton(0);
	ui.tabWidget->setHideTabBarWithOneTab(true);

	int H = misc::getFontSizeFactor("HelpButton").height();
	QString help_str = tr(
	    "<h1><img width=\"%1\" src=\":/icons/help_64.png\">&nbsp;&nbsp;Messages</h1>"
	    "<p>Retroshare has its own internal email system. You can send/receive emails to/from connected friend nodes.</p>"
	    "<p>It is also possible to send messages to other people's Identities using the global routing system. These messages"
	    "   are always encrypted and signed, and are relayed by intermediate nodes until they reach their final destination. </p>"
	    "<p>Distant messages stay into your Outbox until an acknowledgement of receipt has been received.</p>"
	    "<p>Generally, you may use messages to recommend files to your friends by pasting file links,"
	    "   or recommend friend nodes to other friend nodes, in order to strengthen your network, or send feedback"
	    "   to a channel's owner.</p>"
	                     ).arg(QString::number(2*H)) ;

	 registerHelpButton(ui.helpButton,help_str,"MessagesDialog") ;

    connect(ui.filterLineEdit, SIGNAL(textChanged(QString)), this, SLOT(filterChanged(QString)));
    connect(ui.filterLineEdit, SIGNAL(filterChanged(int)), this, SLOT(filterColumnChanged(int)));

    connect(mMessageModel,SIGNAL(messagesAboutToLoad()),this,SLOT(preModelUpdate()));
    connect(mMessageModel,SIGNAL(messagesLoaded()),this,SLOT(postModelUpdate()));

    connect(ui.listWidget,           SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(folderlistWidgetCustomPopupMenu(QPoint)));
    connect(ui.listWidget,           SIGNAL(currentRowChanged(int)), this, SLOT(changeBox(int)));
    //connect(ui.quickViewWidget,      SIGNAL(currentRowChanged(int)), this, SLOT(changeQuickView(int)));
    connect(ui.tabWidget,            SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));
    connect(ui.tabWidget,            SIGNAL(tabCloseRequested(int)), this, SLOT(tabCloseRequested(int)));
    connect(ui.newmessageButton,     SIGNAL(clicked()), this, SLOT(newmessage()));
    connect(ui.quickViewWidget,      SIGNAL(clicked(const QModelIndex&)), this, SLOT(resetQuickView(const QModelIndex&)));

    connect(ui.messageTreeWidget,    SIGNAL(clicked(const QModelIndex&)) , this, SLOT(clicked(const QModelIndex&)));
    connect(ui.messageTreeWidget,    SIGNAL(doubleClicked(const QModelIndex&)) , this, SLOT(doubleClicked(const QModelIndex&)));
    connect(ui.messageTreeWidget,    SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(messageTreeWidgetCustomPopupMenu(const QPoint&)));

    connect(ui.messageTreeWidget->header(),SIGNAL(sortIndicatorChanged(int,Qt::SortOrder)), this, SLOT(sortColumn(int,Qt::SortOrder)));

    connect(ui.messageTreeWidget->selectionModel(), SIGNAL(currentChanged(const QModelIndex&,const QModelIndex&)), this, SLOT(currentChanged(const QModelIndex&,const QModelIndex&)));

    // load settings
    processSettings(true);

    ui.listWidget->setCurrentRow(0); // always starts with inbox => allows to setup the proper number of columns

    mEventHandlerId=0;
    rsEvents->registerEventsHandler( [this](std::shared_ptr<const RsEvent> event) { RsQThreadUtils::postToObject( [this,event]() { handleEvent_main_thread(event); }); }, mEventHandlerId, RsEventType::MAIL_STATUS );

    mTagEventHandlerId = 0;
    rsEvents->registerEventsHandler( [this](std::shared_ptr<const RsEvent> event) { RsQThreadUtils::postToObject( [this,event]() { handleTagEvent_main_thread(event); }); }, mEventHandlerId, RsEventType::MAIL_TAG );
}

void MessagesDialog::handleEvent_main_thread(std::shared_ptr<const RsEvent> event)
{
    if(event->mType != RsEventType::MAIL_STATUS)
        return;

    const RsMailStatusEvent *fe = dynamic_cast<const RsMailStatusEvent*>(event.get());
    if(!fe)
        return;

    switch (fe->mMailStatusEventCode)
    {
    case RsMailStatusEventCode::MESSAGE_SENT:
    case RsMailStatusEventCode::MESSAGE_REMOVED:
    case RsMailStatusEventCode::NEW_MESSAGE:
    case RsMailStatusEventCode::MESSAGE_CHANGED:
    case RsMailStatusEventCode::TAG_CHANGED:
        mMessageModel->updateMessages();
        updateMessageSummaryList();
        break;
    case RsMailStatusEventCode::MESSAGE_RECEIVED_ACK:
    case RsMailStatusEventCode::SIGNATURE_FAILED:
        break;
    }
}

void MessagesDialog::handleTagEvent_main_thread(std::shared_ptr<const RsEvent> event)
{
    if (event->mType != RsEventType::MAIL_TAG) {
        return;
    }

    const RsMailTagEvent *fe = dynamic_cast<const RsMailTagEvent*>(event.get());
    if (!fe) {
        return;
    }

    switch (fe->mMailTagEventCode) {
    case RsMailTagEventCode::TAG_ADDED:
    case RsMailTagEventCode::TAG_CHANGED:
    case RsMailTagEventCode::TAG_REMOVED:
        fillQuickView();
        mMessageModel->updateMessages();
        break;
    }
}

void MessagesDialog::preModelUpdate()
{
    // save current selection

    mTmpSavedSelectedIds.clear();
    getSelectedMessages(mTmpSavedSelectedIds);

    mTmpSavedCurrentId.clear();
    const QModelIndex& m = ui.messageTreeWidget->currentIndex();
    if (m.isValid()) {
        mTmpSavedCurrentId = m.sibling(m.row(), RsMessageModel::COLUMN_THREAD_MSGID).data(RsMessageModel::MsgIdRole).toString();
    }

    std::cerr << "Pre-change: saving selection for " << mTmpSavedSelectedIds.size() << " indexes" << std::endl;
}

void MessagesDialog::postModelUpdate()
{
    // restore selection

    std::cerr << "Post-change: restoring selection for " << mTmpSavedSelectedIds.size() << " indexes" << std::endl;
    QItemSelection sel;

    foreach(const QString& s,mTmpSavedSelectedIds)
    {
        QModelIndex i = mMessageProxyModel->mapFromSource(mMessageModel->getIndexOfMessage(s.toStdString()));
        sel.select(i.sibling(i.row(),0),i.sibling(i.row(),RsMessageModel::COLUMN_THREAD_NB_COLUMNS-1));
    }

    // Restoring selection should not trigger anything, especially the re-display of the msg, which
    // in turn will change the read status.

    whileBlocking(ui.messageTreeWidget->selectionModel())->select(sel,QItemSelectionModel::SelectCurrent);

    if (!mTmpSavedCurrentId.isEmpty()) {
        QModelIndex index = mMessageProxyModel->mapFromSource(mMessageModel->getIndexOfMessage(mTmpSavedCurrentId.toStdString()));
        if (index.isValid()) {
            whileBlocking(ui.messageTreeWidget->selectionModel())->setCurrentIndex(index, QItemSelectionModel::Select);
        }
    }
}

void MessagesDialog::sortColumn(int col,Qt::SortOrder so)
{
    mMessageProxyModel->setSortingEnabled(true);
	//ui.messageTreeWidget->setSortingEnabled(true);
    mMessageProxyModel->sort(col,so);
	//ui.messageTreeWidget->setSortingEnabled(false);
    mMessageProxyModel->setSortingEnabled(false);
}

MessagesDialog::~MessagesDialog()
{
    // save settings
    processSettings(false);

    rsEvents->unregisterEventsHandler(mEventHandlerId);
    rsEvents->unregisterEventsHandler(mTagEventHandlerId);
}

UserNotify *MessagesDialog::createUserNotify(QObject *parent)
{
    return new MessageUserNotify(parent);
}

void MessagesDialog::processSettings(bool load)
{
    int messageTreeVersion = 3; // version number for the settings to solve problems when modifying the column count

    inProcessSettings = true;

    QHeaderView *msgwheader = ui.messageTreeWidget->header () ;

    Settings->beginGroup("MessageDialog");

    if (load) {
        // load settings

        // filterColumn
        ui.filterLineEdit->setCurrentFilter(Settings->value("filterColumn", RsMessageModel::COLUMN_THREAD_SUBJECT).toInt());

        // state of message tree
        if (Settings->value("MessageTreeVersion").toInt() == messageTreeVersion)
            msgwheader->restoreState(Settings->value("MessageTree").toByteArray());

        // state of splitter
        ui.msgSplitter->restoreState(Settings->value("SplitterMsg").toByteArray());
        ui.listSplitter->restoreState(Settings->value("SplitterList").toByteArray());
        ui.boxSplitter->restoreState(Settings->value("SplitterBox").toByteArray());

    } else {
        // save settings

        // state of message tree
        Settings->setValue("MessageTree", msgwheader->saveState());
        Settings->setValue("MessageTreeVersion", messageTreeVersion);

        // state of splitter
        Settings->setValue("SplitterMsg", ui.msgSplitter->saveState());
        Settings->setValue("SplitterList", ui.listSplitter->saveState());
        Settings->setValue("SplitterBox", ui.boxSplitter->saveState());
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
    item = new QListWidgetItem(tr("Stared"), ui.quickViewWidget);
    item->setIcon(FilesDefs::getIconFromQtResourcePath(IMAGE_STAR_ON));
	item->setData(ROLE_QUICKVIEW_TYPE, QUICKVIEW_TYPE_STATIC);
	item->setData(ROLE_QUICKVIEW_ID, QUICKVIEW_STATIC_ID_STARRED);
	item->setData(ROLE_QUICKVIEW_TEXT, item->text()); // for updateMessageSummaryList

	if (selectedType == QUICKVIEW_TYPE_STATIC && selectedId == QUICKVIEW_STATIC_ID_STARRED) {
		itemToSelect = item;
	}

	item = new QListWidgetItem(tr("System"), ui.quickViewWidget);
    item->setIcon(FilesDefs::getIconFromQtResourcePath(IMAGE_NOTFICATION));
	item->setData(ROLE_QUICKVIEW_TYPE, QUICKVIEW_TYPE_STATIC);
	item->setData(ROLE_QUICKVIEW_ID, QUICKVIEW_STATIC_ID_SYSTEM);
	item->setData(ROLE_QUICKVIEW_TEXT, item->text()); // for updateMessageSummaryList

	if (selectedType == QUICKVIEW_TYPE_STATIC && selectedId == QUICKVIEW_STATIC_ID_SYSTEM) {
		itemToSelect = item;
	}

	item = new QListWidgetItem(tr("Spam"), ui.quickViewWidget);
    item->setIcon(FilesDefs::getIconFromQtResourcePath(IMAGE_SPAM_ON));
	item->setData(ROLE_QUICKVIEW_TYPE, QUICKVIEW_TYPE_STATIC);
	item->setData(ROLE_QUICKVIEW_ID, QUICKVIEW_STATIC_ID_SPAM);
	item->setData(ROLE_QUICKVIEW_TEXT, item->text()); // for updateMessageSummaryList

	if (selectedType == QUICKVIEW_TYPE_STATIC && selectedId == QUICKVIEW_STATIC_ID_SPAM) {
		itemToSelect = item;
	}

	item = new QListWidgetItem(tr("Attachment"), ui.quickViewWidget);
    item->setIcon(FilesDefs::getIconFromQtResourcePath(IMAGE_ATTACHMENT));
	item->setData(ROLE_QUICKVIEW_TYPE, QUICKVIEW_TYPE_STATIC);
	item->setData(ROLE_QUICKVIEW_ID, QUICKVIEW_STATIC_ID_ATTACHMENT);
	item->setData(ROLE_QUICKVIEW_TEXT, item->text()); // for updateMessageSummaryList

	if (selectedType == QUICKVIEW_TYPE_STATIC && selectedId == QUICKVIEW_STATIC_ID_ATTACHMENT) {
		itemToSelect = item;
	}

	for (tag = tags.types.begin(); tag != tags.types.end(); ++tag) {
		text = TagDefs::name(tag->first, tag->second.first);
		QPixmap tagpixmap(16,16);
		tagpixmap.fill(QColor(tag->second.second));

		item = new QListWidgetItem (text, ui.quickViewWidget);
		item->setData(Qt::ForegroundRole, QColor(tag->second.second));
		item->setIcon(tagpixmap);
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

int MessagesDialog::getSelectedMessages(QList<QString>& mid)
{
	//To check if the selection has more than one row.

	mid.clear();
	QModelIndexList qmil = ui.messageTreeWidget->selectionModel()->selectedRows();

	foreach(const QModelIndex& m, qmil)
		mid.push_back(m.sibling(m.row(),RsMessageModel::COLUMN_THREAD_MSGID).data(RsMessageModel::MsgIdRole).toString()) ;

	if (mid.isEmpty())
	{
		const QModelIndex& m = ui.messageTreeWidget->currentIndex();
		if (m.isValid())
			mid.push_back(m.sibling(m.row(),RsMessageModel::COLUMN_THREAD_MSGID).data(RsMessageModel::MsgIdRole).toString()) ;
	}

	return mid.size();
}

int MessagesDialog::getSelectedMsgCount (QList<QModelIndex> *items, QList<QModelIndex> *itemsRead, QList<QModelIndex> *itemsUnread, QList<QModelIndex> *itemsStar, QList<QModelIndex> *itemsJunk)
{
	QModelIndexList qmil = ui.messageTreeWidget->selectionModel()->selectedRows();

	if (items) items->clear();
	if (itemsRead) itemsRead->clear();
	if (itemsUnread) itemsUnread->clear();
	if (itemsStar) itemsStar->clear();
	if (itemsJunk) itemsJunk->clear();

	foreach(const QModelIndex& m, qmil)
	{
		if (items)
			items->append(m);

		if (m.data(RsMessageModel::UnreadRole).toBool())
		{ if (itemsUnread) itemsUnread->append(m); }
		else
		{ if (itemsRead) itemsRead->append(m); }

		if (itemsStar && m.data(RsMessageModel::MsgFlagsRole).toInt() & RS_MSG_STAR)
			itemsStar->append(m);

 		if (itemsJunk && m.data(RsMessageModel::MsgFlagsRole).toInt() & RS_MSG_SPAM)
			itemsJunk->append(m);
	}

	return qmil.size();
}

bool MessagesDialog::isMessageRead(const QModelIndex& real_index)
{
    if (!real_index.isValid())
        return true;

    return !real_index.data(RsMessageModel::UnreadRole).toBool();
}

bool MessagesDialog::hasMessageStar(const QModelIndex& real_index)
{
    if (!real_index.isValid())
        return false;

    return real_index.data(RsMessageModel::MsgFlagsRole).toInt() & RS_MSG_STAR;
}

bool MessagesDialog::hasMessageSpam(const QModelIndex& real_index)
{
    if (!real_index.isValid())
        return false;

    return real_index.data(RsMessageModel::MsgFlagsRole).toInt() & RS_MSG_SPAM;
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
    QList<QModelIndex> itemsJunk;

    int nCount = getSelectedMsgCount (NULL, &itemsRead, &itemsUnread, &itemsStar, &itemsJunk);

    /** Defines the actions for the context menu */

    QMenu contextMnu( this );

    QAction *action = contextMnu.addAction(FilesDefs::getIconFromQtResourcePath(":/icons/newwindow.svg"), tr("Open in a new window"), this, SLOT(openAsWindow()));

    if (nCount != 1) {
        action->setDisabled(true);
    }

    action = contextMnu.addAction(FilesDefs::getIconFromQtResourcePath(":/icons/newtab.svg"), tr("Open in a new tab"), this, SLOT(openAsTab()));
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

    action = contextMnu.addAction(FilesDefs::getIconFromQtResourcePath(":/images/message-mail-read.png"), tr("Mark as read"), this, SLOT(markAsRead()));
    if (itemsUnread.isEmpty()) {
        action->setDisabled(true);
    }

    action = contextMnu.addAction(FilesDefs::getIconFromQtResourcePath(":/images/message-mail.png"), tr("Mark as unread"), this, SLOT(markAsUnread()));

    if (itemsRead.isEmpty()) {
        action->setDisabled(true);
    }

    action = contextMnu.addAction(tr("Add Star"));
    action->setCheckable(true);
    action->setChecked(itemsStar.size());
    connect(action, SIGNAL(triggered(bool)), this, SLOT(markWithStar(bool)));

    action = contextMnu.addAction(tr("Mark as Junk"));
    action->setCheckable(true);
    action->setChecked(itemsJunk.size());
    connect(action, SIGNAL(triggered(bool)), this, SLOT(markWithJunk(bool)));

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

    action = contextMnu.addAction(FilesDefs::getIconFromQtResourcePath(IMAGE_MESSAGEREMOVE), (nCount > 1) ? tr("Remove Messages") : tr("Remove Message"), this, SLOT(removemessage()));
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

    if(nCount==1 && msgInfo.from.type() == MsgAddress::MSG_ADDRESS_TYPE_RSGXSID)
	{
        std::cerr << "Src ID = " << msgInfo.from.toGxsId() << std::endl;

        contextMnu.addAction(FilesDefs::getIconFromQtResourcePath(IMAGE_AUTHOR_INFO),tr("Show in People"),this,SLOT(showAuthorInPeopleTab()));
        contextMnu.addSeparator();
	}

    contextMnu.addAction(FilesDefs::getIconFromQtResourcePath(IMAGE_MESSAGE), tr("New Message"), this, SLOT(newmessage()));

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

    if(msgInfo.from.type() != MsgAddress::MSG_ADDRESS_TYPE_RSGXSID)
		return ;

	/* window will destroy itself! */
	IdDialog *idDialog = dynamic_cast<IdDialog*>(MainWindow::getPage(MainWindow::People));

	if (!idDialog)
		return ;

	MainWindow::showWindow(MainWindow::People);
    idDialog->navigate(RsGxsId(msgInfo.from.toGxsId())) ;
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
	connect(msgWidget, SIGNAL(messageRemoved()), this, SLOT(messageRemoved()));

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

    ui.tabWidget->addTab(msgWidget,FilesDefs::getIconFromQtResourcePath(IMAGE_MAIL), msgWidget->subject(true));
    ui.tabWidget->setCurrentWidget(msgWidget);
	connect(msgWidget, SIGNAL(messageRemoved()), this, SLOT(messageRemoved()));

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

	QListWidgetItem* item = ui.listWidget->item(box_row);

	if (item)
	{
		ui.quickViewWidget->setCurrentItem(NULL);

        changeQuickView(-1);
        //listMode = LIST_BOX;

		QString placeholderText = tr("No message available in your %1.").arg(item->text());
		switch(box_row)
		{
            case ROW_INBOX: mMessageModel->setCurrentBox(Rs::Msgs::BoxName::BOX_INBOX );
            break;
            case ROW_OUTBOX: mMessageModel->setCurrentBox(Rs::Msgs::BoxName::BOX_OUTBOX);
			break;
            case ROW_DRAFTBOX: mMessageModel->setCurrentBox(Rs::Msgs::BoxName::BOX_DRAFTS);
			break;
            case ROW_SENTBOX: mMessageModel->setCurrentBox(Rs::Msgs::BoxName::BOX_SENT  );
			break;
            case ROW_TRASHBOX: mMessageModel->setCurrentBox(Rs::Msgs::BoxName::BOX_TRASH );
			break;
			default:
                mMessageModel->setCurrentBox(Rs::Msgs::BoxName::BOX_NONE);
		}

		insertMsgTxtAndFiles(ui.messageTreeWidget->currentIndex());

		ui.messageTreeWidget->setPlaceholderText(placeholderText);
        ui.messageTreeWidget->setColumnHidden(RsMessageModel::COLUMN_THREAD_READ,box_row!=ROW_INBOX);
        ui.messageTreeWidget->setColumnHidden(RsMessageModel::COLUMN_THREAD_STAR,box_row==ROW_OUTBOX);
        ui.messageTreeWidget->setColumnHidden(RsMessageModel::COLUMN_THREAD_SPAM,box_row==ROW_OUTBOX);
        ui.messageTreeWidget->setColumnHidden(RsMessageModel::COLUMN_THREAD_TAGS,box_row==ROW_OUTBOX);
        ui.messageTreeWidget->setColumnHidden(RsMessageModel::COLUMN_THREAD_MSGID,true);
        ui.messageTreeWidget->setColumnHidden(RsMessageModel::COLUMN_THREAD_CONTENT,true);
    }
	else
	{
        mMessageModel->setCurrentBox(Rs::Msgs::BoxName::BOX_NONE);
	}
	inChange = false;

    updateMessageSummaryList();
}

void MessagesDialog::resetQuickView(const QModelIndex& i)
{
    if(!i.isValid())
        return;

    int n = ui.quickViewWidget->currentRow();

    if(mLastCurrentQuickViewRow == n)
    {
        changeQuickView(-1);
        ui.quickViewWidget->setCurrentRow(-1);
    }
    else
        changeQuickView(n);
}
void MessagesDialog::changeQuickView(int newrow)
{
    mLastCurrentQuickViewRow = newrow;
	RsMessageModel::QuickViewFilter f = RsMessageModel::QUICK_VIEW_ALL ;
	QListWidgetItem* item = ui.quickViewWidget->item(newrow);

	if(item )
	{
        //ui.listWidget->setCurrentItem(NULL);
        //changeBox(-1);
        //listMode = LIST_QUICKVIEW;

		QString placeholderText;
        switch (item->data(ROLE_QUICKVIEW_TYPE).toInt())
        {
			case QUICKVIEW_TYPE_TAG:
			{
				placeholderText = tr("No message using %1 tag available.").arg(item->data(ROLE_QUICKVIEW_TEXT).toString());
				f = RsMessageModel::QuickViewFilter( item->data(ROLE_QUICKVIEW_ID).toUInt());
			}
			break;
			case QUICKVIEW_TYPE_STATIC:
			{
				placeholderText = tr("No %1 message available.").arg(item->data(ROLE_QUICKVIEW_TEXT).toString());
				switch (item->data(ROLE_QUICKVIEW_ID).toInt()) {
					case QUICKVIEW_STATIC_ID_STARRED:{
						f = RsMessageModel::QUICK_VIEW_STARRED;
						placeholderText = tr("No starred message available. Stars let you give messages a special status to make them easier to find. To star a message, click on the light gray star beside any message.");
					}
					break;
					case QUICKVIEW_STATIC_ID_SYSTEM: f = RsMessageModel::QUICK_VIEW_SYSTEM;
					break;
					case QUICKVIEW_STATIC_ID_SPAM: 	f = RsMessageModel::QUICK_VIEW_SPAM;
					break;
					case QUICKVIEW_STATIC_ID_ATTACHMENT: f = RsMessageModel::QUICK_VIEW_ATTACHMENT;
				}
			}
		}

        //insertMsgTxtAndFiles(ui.messageTreeWidget->currentIndex());
		ui.messageTreeWidget->setPlaceholderText(placeholderText);
	}

	mMessageModel->setQuickViewFilter(f);
    mMessageProxyModel->setFilterRegExp(QRegExp(RsMessageModel::FilterString));	// this triggers the update of the proxy model
}

// click in messageTreeWidget
void MessagesDialog::currentChanged(const QModelIndex& new_proxy_index,const QModelIndex& /*old_proxy_index*/)
{
	if(!new_proxy_index.isValid())
		return;

	// show current message directly
	insertMsgTxtAndFiles(new_proxy_index);
}

// click in messageTreeWidget
void MessagesDialog::clicked(const QModelIndex& proxy_index)
{
	if(!proxy_index.isValid())
		return;

	QModelIndex real_index = mMessageProxyModel->mapToSource(proxy_index);

	switch (proxy_index.column())
	{
		case RsMessageModel::COLUMN_THREAD_READ:
		{
			mMessageModel->setMsgReadStatus(real_index, !isMessageRead(proxy_index));
			return;
		}
		case RsMessageModel::COLUMN_THREAD_STAR:
		{
			mMessageModel->setMsgStar(real_index, !hasMessageStar(proxy_index));
			return;
		}
		case RsMessageModel::COLUMN_THREAD_SPAM:
		{
			mMessageModel->setMsgJunk(real_index, !hasMessageSpam(proxy_index));
			return;
		}
	}

	// show current message directly
	//Already updated by currentChanged
	//insertMsgTxtAndFiles(proxy_index);
}

// double click in messageTreeWidget
void MessagesDialog::doubleClicked(const QModelIndex& proxy_index)
{
    /* activate row */
    clicked(proxy_index);

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

void MessagesDialog::markAsRead()
{
	QModelIndexList lst = ui.messageTreeWidget->selectionModel()->selectedRows();

	mMessageModel->setMsgsReadStatus(lst,true);

	updateMessageSummaryList();
}

void MessagesDialog::markAsUnread()
{
	QModelIndexList lst = ui.messageTreeWidget->selectionModel()->selectedRows();

	mMessageModel->setMsgsReadStatus(lst,false);

	updateMessageSummaryList();
}

void MessagesDialog::markWithStar(bool checked)
{
	QModelIndexList lst = ui.messageTreeWidget->selectionModel()->selectedRows();

	mMessageModel->setMsgsStar(lst, checked);
}

void MessagesDialog::markWithJunk(bool checked)
{
	QModelIndexList lst = ui.messageTreeWidget->selectionModel()->selectedRows();

	mMessageModel->setMsgsJunk(lst, checked);
}

void MessagesDialog::insertMsgTxtAndFiles(const QModelIndex& proxy_index)
{
    /* get its Ids */
    std::string mid;

    QModelIndex real_index = mMessageProxyModel->mapToSource(proxy_index);

	if(!real_index.isValid())
	{
		mCurrMsgId.clear();
		msgWidget->fill(mCurrMsgId);
		updateInterface();
		lastSelectedIndex = QModelIndex();
		return;
	}

	lastSelectedIndex = proxy_index;
	if (!ui.messageTreeWidget->indexBelow(proxy_index).isValid())
		lastSelectedIndex = ui.messageTreeWidget->indexAbove(proxy_index);

	mid = real_index.data(RsMessageModel::MsgIdRole).toString().toStdString();

    /* Save the Data.... for later */

    MessageInfo msgInfo;
    if (!rsMail -> getMessage(mid, msgInfo)) {
        std::cerr << "MessagesDialog::insertMsgTxtAndFiles() Couldn't find Msg" << std::endl;
        return;
    }

	bool bSetToReadOnActive = Settings->getMsgSetToReadOnActivate();

	if (msgInfo.msgflags & RS_MSG_NEW) // set always to read or unread
	{
		if (!bSetToReadOnActive)  // set locally to unread
			mMessageModel->setMsgReadStatus(real_index, false);
		else
			mMessageModel->setMsgReadStatus(real_index, true);
	}
	else if ((msgInfo.msgflags & RS_MSG_UNREAD_BY_USER) && bSetToReadOnActive)  // set to read
		mMessageModel->setMsgReadStatus(real_index, true);

	msgWidget->fill(mid);
	updateMessageSummaryList();
}

bool MessagesDialog::getCurrentMsg(std::string &cid, std::string &mid)
{
    QModelIndex indx = ui.messageTreeWidget->currentIndex();

    if(!indx.isValid())
        return false ;

    cid = indx.sibling(indx.row(),RsMessageModel::COLUMN_THREAD_MSGID).data(RsMessageModel::SrcIdRole).toString().toStdString();
    mid = indx.sibling(indx.row(),RsMessageModel::COLUMN_THREAD_MSGID).data(RsMessageModel::MsgIdRole).toString().toStdString();

    return true;
}

void MessagesDialog::removemessage()
{
    QList<QString> selectedMessages ;
	getSelectedMessages(selectedMessages);

    bool doDelete = false;
    int listrow = ui.listWidget->currentRow();
    if (listrow == ROW_TRASHBOX)
        doDelete = true;

    if(listrow == ROW_OUTBOX)
    {
        QMessageBox::warning(nullptr,tr("Deletion impossible"),tr("Messages in this box are automatically deleted when received."));
        return ;
    }

    if (QApplication::keyboardModifiers() & Qt::ShiftModifier)
        doDelete = true;

    foreach (const QString& m, selectedMessages) {
        if (doDelete) {
            rsMail->MessageDelete(m.toStdString());
        } else {
            rsMail->MessageToTrash(m.toStdString(), true);
        }
    }

    mMessageModel->updateMessages();
    updateMessageSummaryList();
	lastSelectedIndex = QModelIndex();
	messageRemoved();
}

void MessagesDialog::messageRemoved()
{
	if (lastSelectedIndex.isValid())
		ui.messageTreeWidget->setCurrentIndex(lastSelectedIndex);
	else
		insertMsgTxtAndFiles(QModelIndex());
}

void MessagesDialog::undeletemessage()
{
    QList<QString> msgids;
    getSelectedMessages(msgids);

    foreach (const QString& s, msgids)
        rsMail->MessageToTrash(s.toStdString(), false);

    mMessageModel->updateMessages();
    updateMessageSummaryList();
}

void MessagesDialog::filterChanged(const QString& text)
{
    QStringList items = text.split(' ',QString::SkipEmptyParts);

    RsMessageModel::FilterType f = RsMessageModel::FILTER_TYPE_NONE;

    switch(ui.filterLineEdit->currentFilter())
    {
    case RsMessageModel::COLUMN_THREAD_SUBJECT:      f = RsMessageModel::FILTER_TYPE_SUBJECT ; break;
    case RsMessageModel::COLUMN_THREAD_AUTHOR:       f = RsMessageModel::FILTER_TYPE_FROM ; break;
    case RsMessageModel::COLUMN_THREAD_TO:           f = RsMessageModel::FILTER_TYPE_TO ; break;
    case RsMessageModel::COLUMN_THREAD_DATE:         f = RsMessageModel::FILTER_TYPE_DATE ; break;
    case RsMessageModel::COLUMN_THREAD_CONTENT:      f = RsMessageModel::FILTER_TYPE_CONTENT ; break;
    case RsMessageModel::COLUMN_THREAD_TAGS:         f = RsMessageModel::FILTER_TYPE_TAGS ; break;
    case RsMessageModel::COLUMN_THREAD_ATTACHMENT:   f = RsMessageModel::FILTER_TYPE_ATTACHMENTS ; break;
    default:break;
    }

    mMessageModel->setFilter(f,items);
	mMessageProxyModel->setFilterRegExp(QRegExp(RsMessageModel::FilterString));	// this triggers the update of the proxy model

    QCoreApplication::processEvents();
}

void MessagesDialog::filterColumnChanged(int column)
{
    if (inProcessSettings)
        return;

    RsMessageModel::FilterType f = RsMessageModel::FILTER_TYPE_NONE;

	switch(column)
    {
    case RsMessageModel::COLUMN_THREAD_SUBJECT:      f = RsMessageModel::FILTER_TYPE_SUBJECT ; break;
    case RsMessageModel::COLUMN_THREAD_AUTHOR:       f = RsMessageModel::FILTER_TYPE_FROM ; break;
    case RsMessageModel::COLUMN_THREAD_TO:           f = RsMessageModel::FILTER_TYPE_TO ; break;
    case RsMessageModel::COLUMN_THREAD_DATE:         f = RsMessageModel::FILTER_TYPE_DATE ; break;
    case RsMessageModel::COLUMN_THREAD_CONTENT:      f = RsMessageModel::FILTER_TYPE_CONTENT ; break;
    case RsMessageModel::COLUMN_THREAD_TAGS:         f = RsMessageModel::FILTER_TYPE_TAGS ; break;
    case RsMessageModel::COLUMN_THREAD_ATTACHMENT:   f = RsMessageModel::FILTER_TYPE_ATTACHMENTS ; break;
    default:break;
    }

    QStringList items = ui.filterLineEdit->text().split(' ',QString::SkipEmptyParts);
    mMessageModel->setFilter(f,items);
	mMessageProxyModel->setFilterRegExp(QRegExp(RsMessageModel::FilterString));	// this triggers the update of the proxy model

    // save index
    Settings->setValueToGroup("MessageDialog", "filterColumn", column);

    QCoreApplication::processEvents();
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
    unsigned int spamCount = 0;
    unsigned int attachmentCount = 0;

    /* calculating the new messages */

    Rs::Msgs::BoxName box;
    int box_row = ui.listWidget->currentRow();

    switch(box_row)
    {
    case ROW_INBOX:    box = Rs::Msgs::BoxName::BOX_INBOX ; break;
    case ROW_OUTBOX:   box = Rs::Msgs::BoxName::BOX_OUTBOX; break;
    case ROW_DRAFTBOX: box = Rs::Msgs::BoxName::BOX_DRAFTS; break;
    case ROW_SENTBOX:  box = Rs::Msgs::BoxName::BOX_SENT  ; break;
    case ROW_TRASHBOX: box = Rs::Msgs::BoxName::BOX_TRASH ; break;
    default:
        box = Rs::Msgs::BoxName::BOX_NONE;
    }

    std::list<MsgInfoSummary> msgList;
    rsMail->getMessageSummaries(box ,msgList);
    // rsMail->getMessageSummaries(Rs::Msgs::BoxName::BOX_INBOX ,tmplist); msgList.splice(msgList.end(),tmplist);
    // rsMail->getMessageSummaries(Rs::Msgs::BoxName::BOX_OUTBOX,tmplist); msgList.splice(msgList.end(),tmplist);
    // rsMail->getMessageSummaries(Rs::Msgs::BoxName::BOX_DRAFTS,tmplist); msgList.splice(msgList.end(),tmplist);
    // rsMail->getMessageSummaries(Rs::Msgs::BoxName::BOX_SENT  ,tmplist); msgList.splice(msgList.end(),tmplist);
    // rsMail->getMessageSummaries(Rs::Msgs::BoxName::BOX_TRASH ,tmplist); msgList.splice(msgList.end(),tmplist);

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

        if (it->msgflags & RS_MSG_STAR) 
            ++starredCount;

        if (it->msgflags & RS_MSG_SYSTEM) 
            ++systemCount;

        if (it->msgflags & RS_MSG_SPAM) 
            ++spamCount;

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
        case RS_MSG_DRAFT:	// not RS_MSG_DRAFTBOX because drafts are not considered outgoing
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

std::cerr << "NewInboxCount = " << newInboxCount << " NewDraftCount = " << newDraftCount << std::endl;
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
        item->setIcon(FilesDefs::getIconFromQtResourcePath(":/images/folder-inbox-new.png"));
        item->setData(Qt::ForegroundRole, mTextColorInbox);
    }
    else
    {
        textItem = tr("Inbox");
        item->setText(textItem);
        QFont qf = item->font();
        qf.setBold(false);
        item->setFont(qf);
        item->setIcon(FilesDefs::getIconFromQtResourcePath(":/images/folder-inbox.png"));
        item->setData(Qt::ForegroundRole, QVariant());
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
        QListWidgetItem *qv_item = ui.quickViewWidget->item(row);
        switch (qv_item->data(ROLE_QUICKVIEW_TYPE).toInt()) {
        case QUICKVIEW_TYPE_TAG:
            {
                int count = tagCount[qv_item->data(ROLE_QUICKVIEW_ID).toInt()];

                QString text = qv_item->data(ROLE_QUICKVIEW_TEXT).toString();
                if (count) {
                    text += " (" + QString::number(count) + ")";
                }

                qv_item->setText(text);
            }
            break;
        case QUICKVIEW_TYPE_STATIC:
            {
                QString text = qv_item->data(ROLE_QUICKVIEW_TEXT).toString();
                switch (qv_item->data(ROLE_QUICKVIEW_ID).toInt()) {
                case QUICKVIEW_STATIC_ID_STARRED:
                    if(starredCount>0)
                        text += " (" + QString::number(starredCount) + ")";
                    break;
                case QUICKVIEW_STATIC_ID_SYSTEM:
                    if(systemCount > 0)
                        text += " (" + QString::number(systemCount) + ")";
                    break;
                case QUICKVIEW_STATIC_ID_SPAM:
                    if(spamCount > 0)
                        text += " (" + QString::number(spamCount) + ")";
                    break;
                case QUICKVIEW_STATIC_ID_ATTACHMENT:
                    if(attachmentCount > 0)
                        text += " (" + QString::number(attachmentCount) + ")";
                    break;
                }

                qv_item->setText(text);
            }
            break;
        }
    }
	updateInterface();
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
    rsMsgs->getMessageSummaries(Rs::Msgs::BoxName::BOX_TRASH,msgs);

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

	ui.actionPrint->disconnect();
	ui.actionPrintPreview->disconnect();
	ui.actionSaveAs->disconnect();

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
			//connect(ui.removemessageButton, SIGNAL(clicked()), this, SLOT(removemessage()));
		} else {
			//msg->connectAction(MessageWidget::ACTION_REMOVE, ui.removemessageButton);
		}
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

	ui.actionPrint->setEnabled(count == 1);
	ui.actionPrintPreview->setEnabled(count == 1);
	ui.actionSaveAs->setEnabled(count == 1);
	ui.tagButton->setEnabled(count >= 1);

	if (ui.listWidget->currentItem())
	{
		ui.tabWidget->setTabText(0, ui.listWidget->currentItem()->text());
		ui.tabWidget->setTabIcon(0, ui.listWidget->currentItem()->icon());
	}
	else if (ui.quickViewWidget->currentItem())
	{
		ui.tabWidget->setTabText(0, ui.quickViewWidget->currentItem()->text());
		ui.tabWidget->setTabIcon(0, ui.quickViewWidget->currentItem()->icon());
	}
	else
	{
		ui.tabWidget->setTabText(0, tr("No Box selected."));
        ui.tabWidget->setTabIcon(0, FilesDefs::getIconFromQtResourcePath(":/icons/warning_yellow_128.png"));
	}
}
