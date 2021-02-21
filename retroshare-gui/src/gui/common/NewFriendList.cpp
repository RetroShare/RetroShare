/*******************************************************************************
 * gui/common/NewFriendList.cpp                                                *
 *                                                                             *
 * Copyright (C) 2011, Retroshare Team <retroshare.project@gmail.com>          *
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

#include <algorithm>

#include <QShortcut>
#include <QWidgetAction>
#include <QDateTime>
#include <QPainter>
#include <QtXml>

#include "rsserver/rsaccounts.h"
#include "retroshare/rspeers.h"

#include "GroupDefs.h"
#include "gui/chat/ChatDialog.h"
#include "gui/common/AvatarDefs.h"
#include "gui/common/FilesDefs.h"
#include "gui/common/RSElidedItemDelegate.h"

#include "gui/connect/ConfCertDialog.h"
#include "gui/connect/PGPKeyDialog.h"
#include "gui/connect/ConnectFriendWizard.h"
#include "gui/groups/CreateGroup.h"
#include "gui/msgs/MessageComposer.h"
#include "gui/RetroShareLink.h"
#include "retroshare-gui/RsAutoUpdatePage.h"
#ifdef UNFINISHED_FD
#include "gui/unfinished/profile/ProfileView.h"
#endif
#include "RSTreeWidgetItem.h"
#include "StatusDefs.h"
#include "util/misc.h"
#include "util/qtthreadsutils.h"
#include "vmessagebox.h"
#include "util/QtVersion.h"
#include "gui/chat/ChatUserNotify.h"
#include "gui/connect/ConnectProgressDialog.h"
#include "gui/common/ElidedLabel.h"

#include "NewFriendList.h"
#include "ui_NewFriendList.h"

/* Images for context menu icons */
#define IMAGE_DENYFRIEND         ":/images/denied16.png"
#define IMAGE_REMOVEFRIEND       ":/images/remove_user24.png"
#define IMAGE_EXPORTFRIEND       ":/images/user/friend_suggestion16.png"
#define IMAGE_ADDFRIEND          ":/images/user/add_user16.png"
#define IMAGE_FRIENDINFO         ":/images/info16.png"
#define IMAGE_CHAT               ":/icons/png/chats.png"
#define IMAGE_MSG                ":/icons/mail/write-mail.png"
#define IMAGE_CONNECT            ":/images/connect_friend.png"
#define IMAGE_COPYLINK           ":/images/copyrslink.png"
#define IMAGE_GROUP16            ":/images/user/group16.png"
#define IMAGE_EDIT               ":/icons/png/pencil-edit-button.png"
#define IMAGE_REMOVE             ":/images/delete.png"
#define IMAGE_EXPAND             ":/images/edit_add24.png"
#define IMAGE_COLLAPSE           ":/images/edit_remove24.png"
/* Images for Status icons */
#define IMAGE_AVAILABLE          ":/images/user/identityavaiblecyan24.png"
#define IMAGE_PASTELINK          ":/images/pasterslink.png"
#define IMAGE_GROUP24            ":/images/user/group24.png"

#define COLUMN_DATA     0 // column for storing the userdata id

// #define ROLE_ID                  Qt::UserRole
// #define ROLE_STANDARD            Qt::UserRole + 1
// #define ROLE_SORT_GROUP          Qt::UserRole + 2
// #define ROLE_SORT_STANDARD_GROUP Qt::UserRole + 3
// #define ROLE_SORT_NAME           Qt::UserRole + 4
// #define ROLE_SORT_STATE          Qt::UserRole + 5
// #define ROLE_FILTER              Qt::UserRole + 6

// states for sorting (equal values are possible)
// used in BuildSortString - state + name
#define PEER_STATE_ONLINE       1
#define PEER_STATE_BUSY         2
#define PEER_STATE_AWAY         3
#define PEER_STATE_AVAILABLE    4
#define PEER_STATE_INACTIVE     5
#define PEER_STATE_OFFLINE      6

/******
 * #define FRIENDS_DEBUG 1
 *****/

Q_DECLARE_METATYPE(ElidedLabel*)

class FriendListSortFilterProxyModel: public QSortFilterProxyModel
{
public:
	explicit FriendListSortFilterProxyModel(const QHeaderView *header,QObject *parent = NULL)
	    : QSortFilterProxyModel(parent)
	    , m_header(header)
	    , m_sortingEnabled(false), m_sortByState(false)
        , m_showOfflineNodes(true)
    {
        setDynamicSortFilter(false);  // causes crashes when true.
    }

	bool lessThan(const QModelIndex& left, const QModelIndex& right) const override
	{
		bool is_group_1 = left.data(RsFriendListModel::TypeRole).toUInt() == (uint)RsFriendListModel::ENTRY_TYPE_GROUP;
		bool is_group_2 = right.data(RsFriendListModel::TypeRole).toUInt() == (uint)RsFriendListModel::ENTRY_TYPE_GROUP;

		if(is_group_1 ^ is_group_2)	// if the two are different, put the group first.
			return is_group_1 ;

		bool online1 = (left .data(RsFriendListModel::OnlineRole).toInt() != RS_STATUS_OFFLINE);
		bool online2 = (right.data(RsFriendListModel::OnlineRole).toInt() != RS_STATUS_OFFLINE);

		if(online1 != online2 && m_sortByState)
			return (m_header->sortIndicatorOrder()==Qt::AscendingOrder)?online1:online2 ;    // always put online nodes first

		return left.data(RsFriendListModel::SortRole) < right.data(RsFriendListModel::SortRole) ;
	}

	bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override
	{
		// do not show empty groups

		QModelIndex index = sourceModel()->index(source_row,0,source_parent);

		if(index.data(RsFriendListModel::TypeRole) == RsFriendListModel::ENTRY_TYPE_GROUP)  // always show groups, so we can delete them even when empty
			return true;

		// Filter offline friends

		if(!m_showOfflineNodes && (index.data(RsFriendListModel::OnlineRole).toInt() == RS_STATUS_OFFLINE))
			return false;

		return index.data(RsFriendListModel::FilterRole).toString() == RsFriendListModel::FilterString ;
	}

	void sort( int column, Qt::SortOrder order = Qt::AscendingOrder ) override
	{
        if(m_sortingEnabled)
            return QSortFilterProxyModel::sort(column,order) ;
	}

    void setSortingEnabled(bool b) { m_sortingEnabled = b ; }
    void setSortByState(bool b) { m_sortByState = b ; }
    void setShowOfflineNodes(bool b) { m_showOfflineNodes = b ; }

    bool showOfflineNodes() const { return m_showOfflineNodes ; }
    bool sortByState() const { return m_sortByState ; }

private:
    const QHeaderView *m_header ;
    bool m_sortingEnabled;
    bool m_sortByState;
    bool m_showOfflineNodes;
};

NewFriendList::NewFriendList(QWidget */*parent*/) : /* RsAutoUpdatePage(5000,parent),*/ ui(new Ui::NewFriendList())
{
	ui->setupUi(this);

    ui->filterLineEdit->setPlaceholderText(tr("Search")) ;
    ui->filterLineEdit->showFilterIcon();

    mEventHandlerId_peer=0; // forces initialization
    mEventHandlerId_gssp=0; // forces initialization

    rsEvents->registerEventsHandler( [this](std::shared_ptr<const RsEvent> e) { handleEvent(e); }, mEventHandlerId_peer, RsEventType::PEER_CONNECTION );
    rsEvents->registerEventsHandler( [this](std::shared_ptr<const RsEvent> e) { handleEvent(e); }, mEventHandlerId_gssp, RsEventType::GOSSIP_DISCOVERY );

    mModel = new RsFriendListModel();
	mProxyModel = new FriendListSortFilterProxyModel(ui->peerTreeWidget->header(),this);

    mProxyModel->setSourceModel(mModel);
    mProxyModel->setSortRole(RsFriendListModel::SortRole);
    mProxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
	mProxyModel->setFilterRole(RsFriendListModel::FilterRole);
	mProxyModel->setFilterRegExp(QRegExp(RsFriendListModel::FilterString));

	ui->peerTreeWidget->setModel(mProxyModel);
	ui->peerTreeWidget->setItemDelegate(new RSElidedItemDelegate());
	ui->peerTreeWidget->setWordWrap(false);

    /* Add filter actions */
    QString headerText = mModel->headerData(RsFriendListModel::COLUMN_THREAD_NAME,Qt::Horizontal,Qt::DisplayRole).toString();
    ui->filterLineEdit->addFilter(QIcon(), headerText, RsFriendListModel::COLUMN_THREAD_NAME, QString("%1 %2").arg(tr("Search"), headerText));
    ui->filterLineEdit->addFilter(QIcon(), tr("ID"), RsFriendListModel::COLUMN_THREAD_ID, tr("Search ID"));

    mActionSortByState = new QAction(tr("Online friends on top"), this);
    mActionSortByState->setCheckable(true);

    //setting default filter by column as subject
    ui->filterLineEdit->setCurrentFilter(RsFriendListModel::COLUMN_THREAD_NAME);
	ui->peerTreeWidget->setSortingEnabled(true);

    /* Set sort */
    sortColumn(RsFriendListModel::COLUMN_THREAD_NAME, Qt::AscendingOrder);
    toggleSortByState(false);
     // workaround for Qt bug, should be solved in next Qt release 4.7.0
    // http://bugreports.qt.nokia.com/browse/QTBUG-8270
    QShortcut *Shortcut = new QShortcut(QKeySequence(Qt::Key_Delete), ui->peerTreeWidget, 0, 0, Qt::WidgetShortcut);
	connect(Shortcut, SIGNAL(activated()), this, SLOT(removeItem()),Qt::QueuedConnection);

    QFontMetricsF fontMetrics(ui->peerTreeWidget->font());

    /* Set initial column width */
    int fontWidth = fontMetrics.width("W");
    ui->peerTreeWidget->setColumnWidth(RsFriendListModel::COLUMN_THREAD_NAME        , 22 * fontWidth);
    ui->peerTreeWidget->setColumnWidth(RsFriendListModel::COLUMN_THREAD_IP          , 15 * fontWidth);
    ui->peerTreeWidget->setColumnWidth(RsFriendListModel::COLUMN_THREAD_ID          , 32 * fontWidth);
    ui->peerTreeWidget->setColumnWidth(RsFriendListModel::COLUMN_THREAD_LAST_CONTACT, 12 * fontWidth);

    int avatarHeight = fontMetrics.height() * 3;
    ui->peerTreeWidget->setIconSize(QSize(avatarHeight, avatarHeight));

    mModel->checkInternalData(true);

	QHeaderView *h = ui->peerTreeWidget->header();
	h->setContextMenuPolicy(Qt::CustomContextMenu);

	connect(ui->peerTreeWidget->header(),SIGNAL(sortIndicatorChanged(int,Qt::SortOrder)), this, SLOT(sortColumn(int,Qt::SortOrder)));

	connect(ui->peerTreeWidget, SIGNAL(expanded(const QModelIndex&)), this, SLOT(itemExpanded(const QModelIndex&)));
	connect(ui->peerTreeWidget, SIGNAL(collapsed(const QModelIndex&)), this, SLOT(itemCollapsed(const QModelIndex&)));
    connect(mActionSortByState, SIGNAL(toggled(bool)), this, SLOT(toggleSortByState(bool)));
    connect(ui->peerTreeWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(peerTreeWidgetCustomPopupMenu()));

    connect(ui->actionShowOfflineFriends, SIGNAL(triggered(bool)), this, SLOT(setShowUnconnected(bool)));
    connect(ui->actionShowState,          SIGNAL(triggered(bool)), this, SLOT(setShowState(bool))      );
    connect(ui->actionShowGroups,         SIGNAL(triggered(bool)), this, SLOT(setShowGroups(bool))     );
    connect(ui->actionExportFriendlist,   SIGNAL(triggered())    , this, SLOT(exportFriendlistClicked()));
    connect(ui->actionImportFriendlist,   SIGNAL(triggered())    , this, SLOT(importFriendlistClicked()));

    connect(ui->filterLineEdit, SIGNAL(textChanged(QString)), this, SLOT(filterItems(QString)),Qt::QueuedConnection);
	connect(h, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(headerContextMenuRequested(QPoint)));

// #ifdef RS_DIRECT_CHAT
// 	connect(ui->peerTreeWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), this, SLOT(chatNode()));
// #endif

}

void NewFriendList::handleEvent(std::shared_ptr<const RsEvent> /*e*/)
{
	// /!\ The function we're in is called from a different thread. It's very important
	//     to use this trick in order to avoid data races.

	RsQThreadUtils::postToObject( [=]() { forceUpdateDisplay() ; }, this ) ;
}

NewFriendList::~NewFriendList()
{
    rsEvents->unregisterEventsHandler(mEventHandlerId_peer);
    rsEvents->unregisterEventsHandler(mEventHandlerId_gssp);

    delete mModel;
    delete mProxyModel;
    delete ui;
}

void NewFriendList::itemExpanded(const QModelIndex& index)
{
    mModel->expandItem(mProxyModel->mapToSource(index));
}
void NewFriendList::itemCollapsed(const QModelIndex& index)
{
    mModel->collapseItem(mProxyModel->mapToSource(index));
}

void NewFriendList::sortColumn(int col,Qt::SortOrder so)
{
    std::set<QString> expanded_indexes;
	std::set<QString> selected_indexes;

	saveExpandedPathsAndSelection(expanded_indexes, selected_indexes);
    mProxyModel->setSortingEnabled(true);
    mProxyModel->sort(col,so);
    mProxyModel->setSortingEnabled(false);
	restoreExpandedPathsAndSelection(expanded_indexes, selected_indexes);

    mLastSortColumn = col;
    mLastSortOrder = so;
}

void NewFriendList::headerContextMenuRequested(QPoint /*p*/)
{
	QMenu displayMenu(tr("Show Items"), this);

    QWidget *widget = new QWidget(&displayMenu);
    widget->setStyleSheet( ".QWidget{background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1,stop:0 #FEFEFE, stop:1 #E8E8E8); border: 1px solid #CCCCCC;}");

    // create menu header
    QHBoxLayout *hbox = new QHBoxLayout(widget);
    hbox->setMargin(0);
    hbox->setSpacing(6);

    QLabel *iconLabel = new QLabel(widget);
    QPixmap pix = FilesDefs::getPixmapFromQtResourcePath(":/images/user/friends24.png").scaledToHeight(QFontMetricsF(iconLabel->font()).height()*1.5);
    iconLabel->setPixmap(pix);
    iconLabel->setMaximumSize(iconLabel->frameSize().height() + pix.height(), pix.width());
    hbox->addWidget(iconLabel);

    QLabel *textLabel = new QLabel("<strong>Show/hide...</strong>", widget);
    hbox->addWidget(textLabel);

    QSpacerItem *spacerItem = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    hbox->addItem(spacerItem);

    widget->setLayout(hbox);

    QWidgetAction *widgetAction = new QWidgetAction(this);
    widgetAction->setDefaultWidget(widget);
    displayMenu.addAction(widgetAction);

    displayMenu.addAction(mActionSortByState);

    mActionSortByState->setChecked(mProxyModel->sortByState());

    displayMenu.addAction(ui->actionShowOfflineFriends);
    displayMenu.addAction(ui->actionShowState);
    displayMenu.addAction(ui->actionShowGroups);

    ui->actionShowOfflineFriends->setChecked(mProxyModel->showOfflineNodes());
    ui->actionShowState->setChecked(mModel->getDisplayStatusString());
    ui->actionShowGroups->setChecked(mModel->getDisplayGroups());

    QHeaderView *header = ui->peerTreeWidget->header();

    {
	QAction *action = displayMenu.addAction(QIcon(), tr("Last contact"), this, SLOT(toggleColumnVisible()));
	action->setCheckable(true);
	action->setData(RsFriendListModel::COLUMN_THREAD_LAST_CONTACT);
	action->setChecked(!header->isSectionHidden(RsFriendListModel::COLUMN_THREAD_LAST_CONTACT));
    }

    {
	QAction *action = displayMenu.addAction(QIcon(), tr("IP"), this, SLOT(toggleColumnVisible()));
	action->setCheckable(true);
	action->setData(RsFriendListModel::COLUMN_THREAD_IP);
	action->setChecked(!header->isSectionHidden(RsFriendListModel::COLUMN_THREAD_IP));
    }

    {
	QAction *action = displayMenu.addAction(QIcon(), tr("ID"), this, SLOT(toggleColumnVisible()));
	action->setCheckable(true);
	action->setData(RsFriendListModel::COLUMN_THREAD_ID);
	action->setChecked(!header->isSectionHidden(RsFriendListModel::COLUMN_THREAD_ID));
    }

    displayMenu.exec(QCursor::pos());
}

void NewFriendList::addToolButton(QToolButton *toolButton)
{
    if (!toolButton)
        return;

    /* Initialize button */
    toolButton->setAutoRaise(true);
    float S = QFontMetricsF(ui->filterLineEdit->font()).height() ;
    toolButton->setIconSize(QSize(S*1.5,S*1.5));
    toolButton->setFocusPolicy(Qt::NoFocus);

    ui->titleBarFrame->layout()->addWidget(toolButton);
}

void NewFriendList::saveExpandedPathsAndSelection(std::set<QString>& expanded_indexes, std::set<QString>& selected_indexes)
{
#ifdef DEBUG_NEW_FRIEND_LIST
    std::cerr << "Saving expended paths and selection..." << std::endl;
#endif

    for(int row = 0; row < mProxyModel->rowCount(); ++row)
        recursSaveExpandedItems(mProxyModel->index(row,0),QString(),expanded_indexes,selected_indexes);
}

void NewFriendList::restoreExpandedPathsAndSelection(const std::set<QString>& expanded_indexes, const std::set<QString>& selected_indexes)
{
#ifdef DEBUG_NEW_FRIEND_LIST
    std::cerr << "Restoring expended paths and selection..." << std::endl;
#endif

    ui->peerTreeWidget->blockSignals(true) ;

    for(int row = 0; row < mProxyModel->rowCount(); ++row)
        recursRestoreExpandedItems(mProxyModel->index(row,0),QString(),expanded_indexes,selected_indexes);

    ui->peerTreeWidget->blockSignals(false) ;
}

void NewFriendList::recursSaveExpandedItems(const QModelIndex& index,const QString& parent_path,std::set<QString>& exp, std::set<QString>& sel)
{
    QString local_path = parent_path + index.sibling(index.row(),RsFriendListModel::COLUMN_THREAD_ID).data(Qt::DisplayRole).toString() + " ";

    if(ui->peerTreeWidget->selectionModel()->selection().contains(index))
        sel.insert(local_path) ;

    if(ui->peerTreeWidget->isExpanded(index))
    {
#ifdef DEBUG_NEW_FRIEND_LIST
        std::cerr << "Index " << local_path.toStdString() << " is expanded." << std::endl;
#endif
        if(index.isValid())
            exp.insert(local_path) ;

        for(int row=0;row<mProxyModel->rowCount(index);++row)
            recursSaveExpandedItems(index.child(row,0),local_path,exp,sel) ;
    }
#ifdef DEBUG_NEW_FRIEND_LIST
    else
        std::cerr << "Index " << local_path.toStdString() << " is not expanded." << std::endl;
#endif
}

void NewFriendList::recursRestoreExpandedItems(const QModelIndex& index, const QString& parent_path, const std::set<QString>& exp, const std::set<QString> &sel)
{
    QString local_path = parent_path + index.sibling(index.row(),RsFriendListModel::COLUMN_THREAD_ID).data(Qt::DisplayRole).toString() + " ";
#ifdef DEBUG_NEW_FRIEND_LIST
    std::cerr << "at index " << index.row() << ". data[1]=" << local_path.toStdString() << std::endl;
#endif

    if(sel.find(local_path) != sel.end())
        ui->peerTreeWidget->selectionModel()->select(index, QItemSelectionModel::Select | QItemSelectionModel::Rows);

    if(exp.find(local_path) != exp.end())
    {
#ifdef DEBUG_NEW_FRIEND_LIST
        std::cerr << "re expanding index " << local_path.toStdString() << std::endl;
#endif

        ui->peerTreeWidget->setExpanded(index,true) ;

        for(int row=0;row<mProxyModel->rowCount(index);++row)
            recursRestoreExpandedItems(index.child(row,0),local_path,exp,sel) ;
    }
}


void NewFriendList::processSettings(bool load)
{
    // state of peer tree

    if (load) // load settings
    {
        std::cerr <<"Re-loading settings..." << std::endl;
        // states
        setShowUnconnected(!Settings->value("hideUnconnected", !mProxyModel->showOfflineNodes()).toBool());
        setShowState(Settings->value("showState", mModel->getDisplayStatusString()).toBool());
        setShowGroups(Settings->value("showGroups", mModel->getDisplayGroups()).toBool());

        setColumnVisible(RsFriendListModel::COLUMN_THREAD_IP,Settings->value("showIP", isColumnVisible(RsFriendListModel::COLUMN_THREAD_IP)).toBool());
        setColumnVisible(RsFriendListModel::COLUMN_THREAD_ID,Settings->value("showID", isColumnVisible(RsFriendListModel::COLUMN_THREAD_ID)).toBool());
        setColumnVisible(RsFriendListModel::COLUMN_THREAD_LAST_CONTACT,Settings->value("showLastContact", isColumnVisible(RsFriendListModel::COLUMN_THREAD_LAST_CONTACT)).toBool());
        ui->peerTreeWidget->header()->restoreState(Settings->value("headers").toByteArray());

        // sort
        toggleSortByState(Settings->value("sortByState", mProxyModel->sortByState()).toBool());

        // open groups
        int arrayIndex = Settings->beginReadArray("Groups");
        for (int index = 0; index < arrayIndex; ++index) {
            Settings->setArrayIndex(index);

            std::string gids = Settings->value("open").toString().toStdString();
        }
        Settings->endArray();
    }
    else
    {
        // save settings

        // states
        Settings->setValue("hideUnconnected", !mProxyModel->showOfflineNodes());
        Settings->setValue("showState", mModel->getDisplayStatusString());
        Settings->setValue("showGroups", mModel->getDisplayGroups());

        Settings->setValue("showIP",isColumnVisible(RsFriendListModel::COLUMN_THREAD_IP));
        Settings->setValue("showID",isColumnVisible(RsFriendListModel::COLUMN_THREAD_ID));
        Settings->setValue("showLastContact",isColumnVisible(RsFriendListModel::COLUMN_THREAD_LAST_CONTACT));
        Settings->setValue("headers",ui->peerTreeWidget->header()->saveState());

        // sort
        Settings->setValue("sortByState", mProxyModel->sortByState());

        Settings->endArray();
    }
}

void NewFriendList::toggleSortByState(bool sort)
{
    mProxyModel->setSortByState(sort);
	mProxyModel->setFilterRegExp(QRegExp(QString(RsFriendListModel::FilterString))) ;// triggers a re-display.
}

void NewFriendList::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::StyleChange:
        break;
    default:
        // remove compiler warnings
        break;
    }
}

/**
 * Creates the context popup menu and its submenus,
 * then shows it at the current cursor position.
 */
void NewFriendList::peerTreeWidgetCustomPopupMenu()
{
    QModelIndex index = getCurrentSourceIndex();
    RsFriendListModel::EntryType type = mModel->getType(index);

    QMenu contextMenu(this);

    QWidget *widget = new QWidget(&contextMenu);
    widget->setStyleSheet( ".QWidget{background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1,stop:0 #FEFEFE, stop:1 #E8E8E8); border: 1px solid #CCCCCC;}");

    // create menu header
    QHBoxLayout *hbox = new QHBoxLayout(widget);
    hbox->setMargin(0);
    hbox->setSpacing(6);

    QLabel *iconLabel = new QLabel(widget);
    QPixmap pix = FilesDefs::getPixmapFromQtResourcePath(":/images/user/friends24.png").scaledToHeight(QFontMetricsF(iconLabel->font()).height()*1.5);
    iconLabel->setPixmap(pix);
    iconLabel->setMaximumSize(iconLabel->frameSize().height() + pix.height(), pix.width());
    hbox->addWidget(iconLabel);

    QLabel *textLabel = new QLabel("<strong>Friend list</strong>", widget);
    hbox->addWidget(textLabel);

    QSpacerItem *spacerItem = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    hbox->addItem(spacerItem);

    widget->setLayout(hbox);

    QWidgetAction *widgetAction = new QWidgetAction(this);
    widgetAction->setDefaultWidget(widget);
    contextMenu.addAction(widgetAction);

    // create menu entries
    if(index.isValid())
	{
		// define header
		switch (type) {
        case RsFriendListModel::ENTRY_TYPE_GROUP:
			//this is a GPG key
			textLabel->setText("<strong>" + tr("Group") + "</strong>");
			break;
		case RsFriendListModel::ENTRY_TYPE_PROFILE:
			//this is a GPG key
			textLabel->setText("<strong>" + tr("Friend") + "</strong>");
			break;
		case RsFriendListModel::ENTRY_TYPE_NODE:
			//this is a SSL key
			textLabel->setText("<strong>" + tr("Node") + "</strong>");
			break;
		default:
			textLabel->setText("<strong>" + tr("UNKNOWN TYPE") + "</strong>");
		}

		switch (type)
		{
			case RsFriendListModel::ENTRY_TYPE_GROUP:
			{
				RsGroupInfo group_info ;
				mModel->getGroupData(index,group_info);

				bool standard = group_info.flag & RS_GROUP_FLAG_STANDARD;

				contextMenu.addAction(FilesDefs::getIconFromQtResourcePath(IMAGE_EDIT), tr("Edit Group"), this, SLOT(editGroup()));

				QAction *action = contextMenu.addAction(FilesDefs::getIconFromQtResourcePath(IMAGE_REMOVE), tr("Remove Group"), this, SLOT(removeGroup()));
				action->setDisabled(standard);
			}
			break;

			case RsFriendListModel::ENTRY_TYPE_PROFILE:
			{
				contextMenu.addAction(FilesDefs::getIconFromQtResourcePath(IMAGE_FRIENDINFO), tr("Profile details"), this, SLOT(configureProfile()));
				contextMenu.addAction(FilesDefs::getIconFromQtResourcePath(IMAGE_DENYFRIEND), tr("Deny connections"), this, SLOT(removeProfile()));

				RsFriendListModel::RsProfileDetails details;
				mModel->getProfileData(index,details);

				if(mModel->getDisplayGroups())
				{
					QMenu* addToGroupMenu = NULL;
					QMenu* moveToGroupMenu = NULL;

					std::list<RsGroupInfo> groupInfoList;
					rsPeers->getGroupInfoList(groupInfoList);

					GroupDefs::sortByName(groupInfoList);

					RsPgpId gpgId ( details.gpg_id );

					QModelIndex parent = mModel->parent(index);

					bool foundGroup = false;
					// add action for all groups, except the own group
					for (std::list<RsGroupInfo>::iterator groupIt = groupInfoList.begin(); groupIt != groupInfoList.end(); ++groupIt) {
						if (std::find(groupIt->peerIds.begin(), groupIt->peerIds.end(), gpgId) == groupIt->peerIds.end()) {
							if (parent.isValid())
							{
								if (addToGroupMenu == NULL)
									addToGroupMenu = new QMenu(tr("Add to group"), &contextMenu);

								QAction* addToGroupAction = new QAction(GroupDefs::name(*groupIt), addToGroupMenu);
								addToGroupAction->setData(QString::fromStdString(groupIt->id.toStdString()));
								connect(addToGroupAction, SIGNAL(triggered()), this, SLOT(addToGroup()));
								addToGroupMenu->addAction(addToGroupAction);
							}

							if (moveToGroupMenu == NULL) {
								moveToGroupMenu = new QMenu(tr("Move to group"), &contextMenu);
							}
							QAction* moveToGroupAction = new QAction(GroupDefs::name(*groupIt), moveToGroupMenu);
							moveToGroupAction->setData(QString::fromStdString(groupIt->id.toStdString()));
							connect(moveToGroupAction, SIGNAL(triggered()), this, SLOT(moveToGroup()));
							moveToGroupMenu->addAction(moveToGroupAction);
						} else {
							foundGroup = true;
						}
					}

					QMenu *groupsMenu = contextMenu.addMenu(FilesDefs::getIconFromQtResourcePath(IMAGE_GROUP16), tr("Groups"));
					groupsMenu->addAction(FilesDefs::getIconFromQtResourcePath(IMAGE_EXPAND), tr("Create new group"), this, SLOT(createNewGroup()));

					if (addToGroupMenu || moveToGroupMenu || foundGroup) {
						if (addToGroupMenu) {
							groupsMenu->addMenu(addToGroupMenu);
						}

						if (moveToGroupMenu) {
							groupsMenu->addMenu(moveToGroupMenu);
						}

						if (foundGroup)
						{
							// add remove from group
							if (parent.isValid() && mModel->getType(parent) == RsFriendListModel::ENTRY_TYPE_GROUP)
							{
								RsGroupInfo info ;
								mModel->getGroupData(parent,info);

								QAction *removeFromGroup = groupsMenu->addAction(tr("Remove from group ")+QString::fromUtf8(info.name.c_str()));
								removeFromGroup->setData(parent.sibling(parent.row(),RsFriendListModel::COLUMN_THREAD_ID).data(Qt::DisplayRole));
								connect(removeFromGroup, SIGNAL(triggered()), this, SLOT(removeFromGroup()));
							}

							QAction *removeFromAllGroups = groupsMenu->addAction(tr("Remove from all groups"));
							removeFromAllGroups->setData("");
							connect(removeFromAllGroups, SIGNAL(triggered()), this, SLOT(removeFromGroup()));
						}
					}
				}

			}
			break ;

			case RsFriendListModel::ENTRY_TYPE_NODE:
			{
#ifdef RS_DIRECT_CHAT
				contextMenu.addAction(FilesDefs::getIconFromQtResourcePath(IMAGE_CHAT), tr("Chat"), this, SLOT(chatNode()));
				contextMenu.addAction(FilesDefs::getIconFromQtResourcePath(IMAGE_MSG), tr("Send message to this node"), this, SLOT(msgNode()));
				contextMenu.addSeparator();
#endif // RS_DIRECT_CHAT

				contextMenu.addAction(FilesDefs::getIconFromQtResourcePath(IMAGE_FRIENDINFO), tr("Node details"), this, SLOT(configureNode()));

				if (type == RsFriendListModel::ENTRY_TYPE_PROFILE || type == RsFriendListModel::ENTRY_TYPE_NODE)
					contextMenu.addAction(FilesDefs::getIconFromQtResourcePath(IMAGE_EXPORTFRIEND), tr("Recommend this node to..."), this, SLOT(recommendNode()));

				RsFriendListModel::RsNodeDetails details;
				mModel->getNodeData(index,details);

				if(!rsPeers->isHiddenNode(rsPeers->getOwnId()) || rsPeers->isHiddenNode( details.id ))
					contextMenu.addAction(FilesDefs::getIconFromQtResourcePath(IMAGE_CONNECT), tr("Attempt to connect"), this, SLOT(connectNode()));

				contextMenu.addAction(FilesDefs::getIconFromQtResourcePath(IMAGE_COPYLINK), tr("Copy certificate link"), this, SLOT(copyFullCertificate()));

				//this is a SSL key
				contextMenu.addAction(FilesDefs::getIconFromQtResourcePath(IMAGE_REMOVEFRIEND), tr("Remove Friend Node"), this, SLOT(removeNode()));

			}
			break;

			default:
			{
				contextMenu.addSection("Report it to Devs!");
			}
		}

	}

    contextMenu.addSeparator();

    QAction *action = contextMenu.addAction(FilesDefs::getIconFromQtResourcePath(IMAGE_PASTELINK), tr("Paste certificate link"), this, SLOT(pastePerson()));
    if (RSLinkClipboard::empty(RetroShareLink::TYPE_CERTIFICATE))
        action->setDisabled(true);

    contextMenu.addAction(FilesDefs::getIconFromQtResourcePath(IMAGE_EXPAND), tr("Expand all"), ui->peerTreeWidget, SLOT(expandAll()));
    contextMenu.addAction(FilesDefs::getIconFromQtResourcePath(IMAGE_COLLAPSE), tr("Collapse all"), ui->peerTreeWidget, SLOT(collapseAll()));

    contextMenu.addSeparator();

    contextMenu.addAction(ui->actionExportFriendlist);
    contextMenu.addAction(ui->actionImportFriendlist);

    contextMenu.exec(QCursor::pos());
}

void NewFriendList::createNewGroup()
{
    CreateGroup createGrpDialog (RsNodeGroupId(), this);
    createGrpDialog.exec();
}

#ifdef NOT_USED
static QIcon createAvatar(const QPixmap &avatar, const QPixmap &overlay)
{
	int avatarWidth = avatar.width();
	int avatarHeight = avatar.height();

	QPixmap pixmap(avatar);

	int overlaySize = (avatarWidth > avatarHeight) ? (avatarWidth/2.5) :  (avatarHeight/2.5);
	int overlayX = avatarWidth - overlaySize;
	int overlayY = avatarHeight - overlaySize;

	QPainter painter(&pixmap);
	painter.drawPixmap(overlayX, overlayY, overlaySize, overlaySize, overlay);

	QIcon icon;
	icon.addPixmap(pixmap);
	return icon;
}
#endif

void NewFriendList::addFriend()
{
    std::string groupId = getSelectedGroupId();

    ConnectFriendWizard connwiz (this);

    if (groupId.empty() == false) {
        connwiz.setGroup(groupId);
    }

    connwiz.exec ();
}
void NewFriendList::msgProfile()
{
	RsFriendListModel::RsNodeDetails det;

	if(!getCurrentNode(det))
		return;

	MessageComposer::msgFriend(det.id);
}
void NewFriendList::msgGroup()
{
	RsFriendListModel::RsNodeDetails det;

	if(!getCurrentNode(det))
		return;

	MessageComposer::msgFriend(det.id);
}
void NewFriendList::msgNode()
{
	RsFriendListModel::RsNodeDetails det;

	if(!getCurrentNode(det))
		return;

	MessageComposer::msgFriend(det.id);
}
void NewFriendList::chatNode()
{
	RsFriendListModel::RsNodeDetails det;

	if(!getCurrentNode(det))
		return;

	ChatDialog::chatFriend(ChatId(det.id));
}

void NewFriendList::recommendNode()
{
	RsFriendListModel::RsNodeDetails det;

	if(!getCurrentNode(det))
		return;

	MessageComposer::recommendFriend(std::set<RsPeerId>({ det.id }));
}

void NewFriendList::pastePerson()
{
    //RSLinkClipboard::process(RetroShareLink::TYPE_PERSON);
    RSLinkClipboard::process(RetroShareLink::TYPE_CERTIFICATE);
}

void NewFriendList::copyFullCertificate()
{
	RsFriendListModel::RsNodeDetails det;

	if(!getCurrentNode(det))
		return;

	QList<RetroShareLink> urls;
	RetroShareLink link = RetroShareLink::createCertificate(det.id);
	urls.push_back(link);

	std::cerr << "link: " << std::endl;
	std::cerr<< link.toString().toStdString() << std::endl;

	RSLinkClipboard::copyLinks(urls);
}

/**
 * Find out which group is selected
 *
 * @return If a group item is selected, its groupId otherwise an empty string
 */
std::string NewFriendList::getSelectedGroupId() const
{
    RsGroupInfo ginfo;

    if(!getCurrentGroup(ginfo))
        return std::string();

    return ginfo.id.toStdString();
}

QModelIndex NewFriendList::getCurrentSourceIndex() const
{
	QModelIndexList selectedIndexes = ui->peerTreeWidget->selectionModel()->selectedIndexes();

    if(selectedIndexes.size() != RsFriendListModel::COLUMN_THREAD_NB_COLUMNS)	// check that a single row is selected
        return QModelIndex();

    return mProxyModel->mapToSource(*selectedIndexes.begin());
}
bool NewFriendList::getCurrentGroup(RsGroupInfo& info) const
{
    /* get the current, and extract the Id */

    QModelIndex index = getCurrentSourceIndex();

    if(!index.isValid())
        return false;

    return mModel->getGroupData(index,info);
}
bool NewFriendList::getCurrentNode(RsFriendListModel::RsNodeDetails& prof) const
{
    /* get the current, and extract the Id */

    QModelIndex index = getCurrentSourceIndex();

    if(!index.isValid())
        return false;

    return mModel->getNodeData(index,prof);
}
bool NewFriendList::getCurrentProfile(RsFriendListModel::RsProfileDetails& prof) const
{
    /* get the current, and extract the Id */

    QModelIndex index = getCurrentSourceIndex();

    if(!index.isValid())
        return false;

    return mModel->getProfileData(index,prof);
}

#ifdef UNFINISHED_FD
/* GUI stuff -> don't do anything directly with Control */
void FriendsDialog::viewprofile()
{
    /* display Dialog */

    QTreeWidgetItem *c = getCurrentPeer();


    //	static ProfileView *profileview = new ProfileView();


    if (!c)
        return;

    /* set the Id */
    std::string id = getRsId(c);

    profileview -> setPeerId(id);
    profileview -> show();
}
#endif

/* So from the Peers Dialog we can call the following control Functions:
 * (1) Remove Current.              FriendRemove(id)
 * (2) Allow/DisAllow.              FriendStatus(id, accept)
 * (2) Connect.                     FriendConnectAttempt(id, accept)
 * (3) Set Address.                 FriendSetAddress(id, str, port)
 * (4) Set Trust.                   FriendTrustSignature(id, bool)
 * (5) Configure (GUI Only) -> 3/4
 *
 * All of these rely on the finding of the current Id.
 */

void NewFriendList::removeItem()
{
	QModelIndex index = getCurrentSourceIndex();
	RsFriendListModel::EntryType type = mModel->getType(index);
	if(index.isValid())
	{
		switch (type) {
			case RsFriendListModel::ENTRY_TYPE_GROUP:   removeGroup();
			break;
			case RsFriendListModel::ENTRY_TYPE_PROFILE: removeProfile();
			break;
			case RsFriendListModel::ENTRY_TYPE_NODE:    removeNode();
			break;
			case RsFriendListModel::ENTRY_TYPE_UNKNOWN: RsErr()<<__PRETTY_FUNCTION__<<" Get Item of type unknow."<<std::endl;
		}
	}
}

void NewFriendList::removeNode()
{
	RsFriendListModel::RsNodeDetails det;
	if(!getCurrentNode(det) || !rsPeers)
		return;

	if ((QMessageBox::question(this, "RetroShare", tr("Do you want to remove this node?"), QMessageBox::Yes|QMessageBox::No, QMessageBox::Yes)) == QMessageBox::Yes)
		rsPeers->removeFriendLocation(det.id);

	checkInternalData(true);
}

void NewFriendList::removeProfile()
{
	RsFriendListModel::RsProfileDetails det;
	if(!getCurrentProfile(det) || !rsPeers)
		return;

	if ((QMessageBox::question(this, "RetroShare", tr("Do you want to remove this Friend?"), QMessageBox::Yes|QMessageBox::No, QMessageBox::Yes)) == QMessageBox::Yes)
		rsPeers->removeFriend(det.gpg_id);

	checkInternalData(true);
}

void NewFriendList::connectNode()
{
	RsFriendListModel::RsNodeDetails det;
	if(!getCurrentNode(det) || !rsPeers)
		return;

	rsPeers->connectAttempt(det.id);
	ConnectProgressDialog::showProgress(det.id);
}

/* GUI stuff -> don't do anything directly with Control */
void NewFriendList::configureNode()
{
	RsFriendListModel::RsNodeDetails det;

	if(!getCurrentNode(det))
		return;

	ConfCertDialog::showIt(det.id, ConfCertDialog::PageDetails);
}
void NewFriendList::configureProfile()
{
	RsFriendListModel::RsProfileDetails det;

	if(!getCurrentProfile(det))
		return;

	PGPKeyDialog::showIt(det.gpg_id, PGPKeyDialog::PageDetails);
}

void NewFriendList::addToGroup()
{
	RsFriendListModel::RsProfileDetails det;
    if(!getCurrentProfile(det) || !rsPeers)
        return;

    RsNodeGroupId groupId ( qobject_cast<QAction*>(sender())->data().toString().toStdString());
    RsPgpId gpgId (det.gpg_id);

    if (gpgId.isNull() || groupId.isNull())
        return;

    // automatically expand the group, the peer is added to
    expandGroup(groupId);

    // add to group
    rsPeers->assignPeerToGroup(groupId, gpgId, true);
    checkInternalData(true);
}
void NewFriendList::forceUpdateDisplay()
{
    checkInternalData(true);
}

void NewFriendList::moveToGroup()
{
    RsFriendListModel::RsProfileDetails pinfo;

    if(!getCurrentProfile(pinfo))
        return;

    RsNodeGroupId groupId ( qobject_cast<QAction*>(sender())->data().toString().toStdString());
    RsPgpId gpgId ( pinfo.gpg_id );

    if (gpgId.isNull() || groupId.isNull())
        return;

    // remove from all groups
    rsPeers->assignPeerToGroup(RsNodeGroupId(), gpgId, false);

    // automatically expand the group, the peer is added to
    expandGroup(groupId);

    // add to group
    rsPeers->assignPeerToGroup(groupId, gpgId, true);
    checkInternalData(true);
}

void NewFriendList::removeFromGroup()
{
	RsFriendListModel::RsProfileDetails pinfo;

    if(!getCurrentProfile(pinfo))
        return;

    RsNodeGroupId groupId ( qobject_cast<QAction*>(sender())->data().toString().toStdString());
    RsPgpId gpgId ( pinfo.gpg_id );

    if (gpgId.isNull())
        return;

    // remove from (all) group(s)
    rsPeers->assignPeerToGroup(groupId, gpgId, false);
    checkInternalData(true);
}

void NewFriendList::editGroup()
{
    RsGroupInfo pinfo;

    if(!getCurrentGroup(pinfo))
        return;

    RsNodeGroupId groupId ( pinfo.id );

    if (!groupId.isNull())
    {
        CreateGroup editGrpDialog(groupId, this);
        editGrpDialog.exec();
    }
    checkInternalData(true);
}

void NewFriendList::removeGroup()
{
	RsGroupInfo pinfo;

	if(!getCurrentGroup(pinfo))
		return;

	rsPeers->removeGroup(pinfo.id);
	checkInternalData(true);
}

void NewFriendList::applyWhileKeepingTree(std::function<void()> predicate)
{
    std::set<QString> expanded_indexes;
    std::set<QString> selected_indexes;

    saveExpandedPathsAndSelection(expanded_indexes, selected_indexes);

    // This is a hack to avoid crashes on windows while calling endInsertRows(). I'm not sure wether these crashes are
    // due to a Qt bug, or a misuse of the proxy model on my side. Anyway, this solves them for good.
    // As a side effect we need to save/restore hidden columns because setSourceModel() resets this setting.

    // save hidden columns and sizes
    std::vector<bool> col_visible(RsFriendListModel::COLUMN_THREAD_NB_COLUMNS);
    std::vector<int> col_sizes(RsFriendListModel::COLUMN_THREAD_NB_COLUMNS);

    for(int i=0;i<RsFriendListModel::COLUMN_THREAD_NB_COLUMNS;++i)
    {
        col_visible[i] = !ui->peerTreeWidget->isColumnHidden(i);
        col_sizes[i] = ui->peerTreeWidget->columnWidth(i);
    }


    mProxyModel->setSourceModel(nullptr);

    predicate();

    mProxyModel->setSourceModel(mModel);
    restoreExpandedPathsAndSelection(expanded_indexes, selected_indexes);

    // restore hidden columns
    for(uint32_t i=0;i<RsFriendListModel::COLUMN_THREAD_NB_COLUMNS;++i)
    {
        ui->peerTreeWidget->setColumnHidden(i,!col_visible[i]);
        ui->peerTreeWidget->setColumnWidth(i,col_sizes[i]);
    }

    // restore sorting
    sortColumn(mLastSortColumn,mLastSortOrder);
}

void NewFriendList::checkInternalData(bool force)
{
    applyWhileKeepingTree([force,this]() { mModel->checkInternalData(force) ; });
}

void NewFriendList::exportFriendlistClicked()
{
    QString fileName;
    if(!importExportFriendlistFileDialog(fileName, false))
        // error was already shown - just return
        return;

    if(!exportFriendlist(fileName))
        // error was already shown - just return
        return;

    QMessageBox mbox;
    mbox.setIcon(QMessageBox::Information);
    mbox.setText(tr("Done!"));
    mbox.setInformativeText(tr("Your friendlist is stored at:\n") + fileName +
                            tr("\n(keep in mind that the file is unencrypted!)"));
    mbox.setStandardButtons(QMessageBox::Ok);
    mbox.exec();
}

void NewFriendList::importFriendlistClicked()
{
    QString fileName;
    if(!importExportFriendlistFileDialog(fileName, true))
        // error was already shown - just return
        return;

    bool errorPeers, errorGroups;
    if(importFriendlist(fileName, errorPeers, errorGroups)) {
        QMessageBox mbox;
        mbox.setIcon(QMessageBox::Information);
        mbox.setText(tr("Done!"));
        mbox.setInformativeText(tr("Your friendlist was imported from:\n") + fileName);
        mbox.setStandardButtons(QMessageBox::Ok);
        mbox.exec();
    } else {
        QMessageBox mbox;
        mbox.setIcon(QMessageBox::Warning);
        mbox.setText(tr("Done - but errors happened!"));
        mbox.setInformativeText(tr("Your friendlist was imported from:\n") + fileName +
                                (errorPeers  ? tr("\nat least one peer was not added") : "") +
                                (errorGroups ? tr("\nat least one peer was not added to a group") : "")
                                );
        mbox.setStandardButtons(QMessageBox::Ok);
        mbox.exec();
    }
}

/**
 * @brief opens a file dialog to select a file containing a friendlist
 * @param fileName file containing a friendlist
 * @param import show dialog for importing (true) or exporting (false) friendlist
 * @return success or failure
 *
 * This function also shows an error message when no valid file was selected
 */
bool NewFriendList::importExportFriendlistFileDialog(QString &fileName, bool import)
{
	bool res = true;
	if (import) {
		res = misc::getOpenFileName(this, RshareSettings::LASTDIR_CERT
		                            , tr("Select file for importing your friendlist from")
		                            , tr("XML File (*.xml);;All Files (*)")
		                            , fileName
		                            , QFileDialog::DontConfirmOverwrite
		                            );
		} else {
		res = misc::getSaveFileName(this, RshareSettings::LASTDIR_CERT
		                            , tr("Select a file for exporting your friendlist to")
		                            , tr("XML File (*.xml);;All Files (*)")
		                            , fileName, NULL
		                            , QFileDialog::Options()
		                            );
	}
	if ( res && !fileName.endsWith(".xml",Qt::CaseInsensitive) )
		fileName = fileName.append(".xml");

	return res;
}

/**
 * @brief exports friendlist to a given file
 * @param fileName file for storing friendlist
 * @return success or failure
 *
 * This function also shows an error message when the selected file is invalid/not writable
 */
bool NewFriendList::exportFriendlist(QString &fileName)
{
    QDomDocument doc("FriendListWithGroups");
    QDomElement root = doc.createElement("root");
    doc.appendChild(root);

    QFile file(fileName);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        // show error to user
        QMessageBox mbox;
        mbox.setIcon(QMessageBox::Warning);
        mbox.setText(tr("Error"));
        mbox.setInformativeText(tr("File is not writeable!\n") + fileName);
        mbox.setStandardButtons(QMessageBox::Ok);
        mbox.exec();
        return false;
    }

    std::list<RsPgpId> gpg_ids;
    rsPeers->getGPGAcceptedList(gpg_ids);

    std::list<RsGroupInfo> group_info_list;
    rsPeers->getGroupInfoList(group_info_list);

    QDomElement pgpIDs = doc.createElement("pgpIDs");
    RsPeerDetails detailPGP;
    for(std::list<RsPgpId>::iterator list_iter = gpg_ids.begin(); list_iter !=  gpg_ids.end(); ++list_iter)	{
        rsPeers->getGPGDetails(*list_iter, detailPGP);
        QDomElement pgpID = doc.createElement("pgpID");
        // these values aren't used and just stored for better human readability
        pgpID.setAttribute("id", QString::fromStdString(detailPGP.gpg_id.toStdString()));
        pgpID.setAttribute("name", QString::fromUtf8(detailPGP.name.c_str()));

        std::list<RsPeerId> ssl_ids;
        rsPeers->getAssociatedSSLIds(*list_iter, ssl_ids);
        for(std::list<RsPeerId>::iterator list_iter2 = ssl_ids.begin(); list_iter2 !=  ssl_ids.end(); ++list_iter2) {
            RsPeerDetails detailSSL;
            if (!rsPeers->getPeerDetails(*list_iter2, detailSSL))
                continue;

            std::string certificate = rsPeers->GetRetroshareInvite(detailSSL.id, RetroshareInviteFlags::CURRENT_IP | RetroshareInviteFlags::DNS | RetroshareInviteFlags::RADIX_FORMAT);

            // remove \n from certificate
            certificate.erase(std::remove(certificate.begin(), certificate.end(), '\n'), certificate.end());

            QDomElement sslID = doc.createElement("sslID");
            // these values aren't used and just stored for better human readability
            sslID.setAttribute("sslID", QString::fromStdString(detailSSL.id.toStdString()));
            if(!detailSSL.location.empty())
                sslID.setAttribute("location", QString::fromUtf8(detailSSL.location.c_str()));

            // required values
            sslID.setAttribute("certificate", QString::fromStdString(certificate));
            sslID.setAttribute("service_perm_flags", detailSSL.service_perm_flags.toUInt32());

            pgpID.appendChild(sslID);
        }
        pgpIDs.appendChild(pgpID);
    }
    root.appendChild(pgpIDs);

    QDomElement groups = doc.createElement("groups");
    for(std::list<RsGroupInfo>::iterator list_iter = group_info_list.begin(); list_iter !=  group_info_list.end(); ++list_iter)	{
        RsGroupInfo group_info = *list_iter;

        //skip groups without peers
        if(group_info.peerIds.empty())
            continue;

        QDomElement group = doc.createElement("group");
        // id is not needed since it may differ between locatiosn / pgp ids (groups are identified by name)
        group.setAttribute("name", QString::fromUtf8(group_info.name.c_str()));
        group.setAttribute("flag", group_info.flag);

        for(std::set<RsPgpId>::iterator i = group_info.peerIds.begin(); i !=  group_info.peerIds.end(); ++i) {
            QDomElement pgpID = doc.createElement("pgpID");
            std::string pid = i->toStdString();
            pgpID.setAttribute("id", QString::fromStdString(pid));
            group.appendChild(pgpID);
        }
        groups.appendChild(group);
    }
    root.appendChild(groups);

    QTextStream ts(&file);
    ts.setCodec("UTF-8");
    ts << doc.toString();
    file.close();

    return true;
}

/**
 * @brief helper function to show a message box
 */
static void showXMLParsingError()
{
    // show error to user
    QMessageBox mbox;
    mbox.setIcon(QMessageBox::Warning);
    mbox.setText(QObject::tr("Error"));
    mbox.setInformativeText(QObject::tr("unable to parse XML file!"));
    mbox.setStandardButtons(QMessageBox::Ok);
    mbox.exec();
}

/**
 * @brief Imports friends from a given file
 * @param fileName file to load friends from
 * @param errorPeers an error occured while adding a peer
 * @param errorGroups an error occured while adding a peer to a group
 * @return success or failure (an error can also happen when adding a peer and/or adding a peer to a group fails at least once)
 */
bool NewFriendList::importFriendlist(QString &fileName, bool &errorPeers, bool &errorGroups)
{
	errorPeers = false;
	errorGroups = false;

    QDomDocument doc;
    // load from file
    {
        QFile file(fileName);

        if (!file.open(QIODevice::ReadOnly)) {
            // show error to user
            QMessageBox mbox;
            mbox.setIcon(QMessageBox::Warning);
            mbox.setText(tr("Error"));
            mbox.setInformativeText(tr("File is not readable!\n") + fileName);
            mbox.setStandardButtons(QMessageBox::Ok);
            mbox.exec();
            return false;
        }

        bool ok = doc.setContent(&file);
        file.close();

        if(!ok) {
            showXMLParsingError();
            return false;
        }
    }

    QDomElement root = doc.documentElement();
    if(root.tagName() != "root") {
        showXMLParsingError();
        return false;
    }

    RsPeerDetails rsPeerDetails;
    RsPeerId rsPeerID;
    RsPgpId rsPgpID;

    // lock all events for faster processing
    RsAutoUpdatePage::lockAllEvents();

    // pgp and ssl IDs
    QDomElement pgpIDs;
    {
        QDomNodeList nodes = root.elementsByTagName("pgpIDs");
        if(nodes.isEmpty() || nodes.size() != 1){
            showXMLParsingError();
            return false;
        }

        pgpIDs = nodes.item(0).toElement();
        if(pgpIDs.isNull()){
            showXMLParsingError();
            return false;
        }
    }
    QDomNode pgpIDElem = pgpIDs.firstChildElement("pgpID");
    while (!pgpIDElem.isNull()) {
        QDomElement sslIDElem = pgpIDElem.firstChildElement("sslID");
        while (!sslIDElem.isNull()) {
            rsPeerID.clear();
            rsPgpID.clear();

            // load everything needed from the pubkey string
            std::string pubkey = sslIDElem.attribute("certificate").toStdString();
			ServicePermissionFlags service_perm_flags(sslIDElem.attribute("service_perm_flags").toInt());
			if (!rsPeers->acceptInvite(pubkey, service_perm_flags)) {
                errorPeers = true;
				std::cerr << "FriendList::importFriendlist(): failed to get peer detaisl from public key (SSL id: " << sslIDElem.attribute("sslID", "invalid").toStdString() << ")" << std::endl;
            }
            sslIDElem = sslIDElem.nextSiblingElement("sslID");
        }
        pgpIDElem = pgpIDElem.nextSiblingElement("pgpID");
    }

    // groups
    QDomElement groups;
    {
        QDomNodeList nodes = root.elementsByTagName("groups");
        if(nodes.isEmpty() || nodes.size() != 1){
            showXMLParsingError();
            return false;
        }

        groups = nodes.item(0).toElement();
        if(groups.isNull()){
            showXMLParsingError();
            return false;
        }
    }
    QDomElement group = groups.firstChildElement("group");
    while (!group.isNull()) {
        // get name and flags and try to get the group ID
        std::string groupName = group.attribute("name").toStdString();
        uint32_t flag = group.attribute("flag").toInt();
        RsNodeGroupId groupId;
        if(getOrCreateGroup(groupName, flag, groupId)) {
            // group id found!
            QDomElement pgpID = group.firstChildElement("pgpID");
            while (!pgpID.isNull()) {
                // add pgp id to group
                RsPgpId rsPgpId(pgpID.attribute("id").toStdString());
                if(rsPgpID.isNull() || !rsPeers->assignPeerToGroup(groupId, rsPgpId, true)) {
                    errorGroups = true;
                    std::cerr << "FriendList::importFriendlist(): failed to add '" << rsPeers->getGPGName(rsPgpId) << "'' to group '" << groupName << "'" << std::endl;
                }

                pgpID = pgpID.nextSiblingElement("pgpID");
            }
            pgpID = pgpID.nextSiblingElement("pgpID");
        } else {
            errorGroups = true;
            std::cerr << "FriendList::importFriendlist(): failed to find/create group '" << groupName << "'" << std::endl;
        }
        group = group.nextSiblingElement("group");
    }

    // unlock events
    RsAutoUpdatePage::unlockAllEvents();

    return !(errorPeers || errorGroups);
}

/**
 * @brief Gets the groups ID for a given group name
 * @param name group name to search for
 * @param id groupd id for the given name
 * @return success or fail
 */
bool NewFriendList::getGroupIdByName(const std::string &name, RsNodeGroupId &id)
{
    std::list<RsGroupInfo> grpList;
    if(!rsPeers->getGroupInfoList(grpList))
        return false;

    foreach (const RsGroupInfo &grp, grpList) {
        if(grp.name == name) {
            id = grp.id;
            return true;
        }
    }

    return false;
}

/**
 * @brief Gets the groups ID for a given group name. If no groupd was it will create one
 * @param name group name to search for
 * @param flag flag to use when creating the group
 * @param id groupd id
 * @return success or failure
 */
bool NewFriendList::getOrCreateGroup(const std::string& name, uint flag, RsNodeGroupId &id)
{
    if(getGroupIdByName(name, id))
        return true;

    // -> create one
    RsGroupInfo grp;
    grp.id.clear(); // RS will generate an ID
    grp.name = name;
    grp.flag = flag;

    if(!rsPeers->addGroup(grp))
        return false;

    // try again
    return getGroupIdByName(name, id);
}


void NewFriendList::setShowUnconnected(bool show)
{
    mProxyModel->setShowOfflineNodes(show);
	mProxyModel->setFilterRegExp(QRegExp(QString(RsFriendListModel::FilterString))) ;// triggers a re-display.
}

bool NewFriendList::isColumnVisible(int col) const
{
    return !ui->peerTreeWidget->isColumnHidden(col);
}
void NewFriendList::setColumnVisible(int col,bool visible)
{
    std::cerr << "Setting column " << col << " to be visible: " << visible << std::endl;
    ui->peerTreeWidget->setColumnHidden(col, !visible);
}
void NewFriendList::toggleColumnVisible()
{
	QAction *action = dynamic_cast<QAction*>(sender());

	if (!action)
		return;

	int column = action->data().toInt();
	bool visible = action->isChecked();

    ui->peerTreeWidget->setColumnHidden(column, !visible);
}

void NewFriendList::setShowState(bool show)
{
    applyWhileKeepingTree([show,this]() { mModel->setDisplayStatusString(show) ; });
}

void NewFriendList::setShowGroups(bool show)
{
    applyWhileKeepingTree([show,this]() { mModel->setDisplayGroups(show) ; });
}

/**
 * Hides all items that don't contain text in the name column.
 */
void NewFriendList::filterItems(const QString &text)
{
    QStringList lst = text.split(' ',QString::SkipEmptyParts);
	int filterColumn = ui->filterLineEdit->currentFilter();

    if(filterColumn == 0)
		mModel->setFilter(RsFriendListModel::FILTER_TYPE_NAME,lst);
    else
		mModel->setFilter(RsFriendListModel::FILTER_TYPE_ID,lst);

    // We do this in order to trigger a new filtering action in the proxy model.
	mProxyModel->setFilterRegExp(QRegExp(QString(RsFriendListModel::FilterString))) ;

    if(!lst.empty())
		ui->peerTreeWidget->expandAll();
    else
		ui->peerTreeWidget->collapseAll();
}

void NewFriendList::expandGroup(const RsNodeGroupId& gid)
{
    QModelIndex index = mProxyModel->mapFromSource(mModel->getIndexOfGroup(gid));
	ui->peerTreeWidget->setExpanded(index,true) ;
}
