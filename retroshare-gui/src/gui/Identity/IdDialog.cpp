/*******************************************************************************
 * retroshare-gui/src/gui/Identity/IdDialog.cpp                                *
 *                                                                             *
 * Copyright (C) 2012 by Robert Fernie       <retroshare.project@gmail.com>    *
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

#include <unistd.h>

#include <QCheckBox>
#include <QMessageBox>
#include <QDateTime>
#include <QMenu>
#include <QWidgetAction>
#include <QStyledItemDelegate>
#include <QPainter>

#include "IdDialog.h"
#include "ui_IdDialog.h"
#include "IdEditDialog.h"
#include "gui/RetroShareLink.h"
#include "gui/chat/ChatDialog.h"
#include "gui/Circles/CreateCircleDialog.h"
#include "gui/common/FilesDefs.h"
#include "gui/common/UIStateHelper.h"
#include "gui/common/UserNotify.h"
#include "gui/gxs/GxsIdDetails.h"
#include "gui/gxs/GxsIdTreeWidgetItem.h"
//#include "gui/gxs/RsGxsUpdateBroadcastBase.h"
#include "gui/msgs/MessageComposer.h"
#include "gui/settings/rsharesettings.h"
#include "util/qtthreadsutils.h"
#include "retroshare-gui/RsAutoUpdatePage.h"
#include "util/misc.h"
#include "util/QtVersion.h"
#include "util/rstime.h"
#include "util/rsdebug.h"

#include "retroshare/rsgxsflags.h"
#include "retroshare/rsmsgs.h" 
#include "retroshare/rspeers.h"
#include "retroshare/rsservicecontrol.h"

#include <iostream>
#include <algorithm>
#include <memory>

/******
 * #define ID_DEBUG 1
 *****/

#define QT_BUG_CRASH_IN_TAKECHILD_WORKAROUND 1

// Data Requests.
#define IDDIALOG_IDLIST           1
#define IDDIALOG_IDDETAILS        2
#define IDDIALOG_REPLIST          3
#define IDDIALOG_REFRESH          4
#define IDDIALOG_SERIALIZED_GROUP 5

#define CIRCLEGROUP_CIRCLE_COL_GROUPNAME      0
#define CIRCLEGROUP_CIRCLE_COL_GROUPID        1
#define CIRCLEGROUP_CIRCLE_COL_GROUPFLAGS     2
#define CIRCLEGROUP_CIRCLE_COL_SUBSCRIBEFLAGS 3

#define CIRCLEGROUP_FRIEND_COL_NAME 0
#define CIRCLEGROUP_FRIEND_COL_ID   1

#define CLEAR_BACKGROUND 0
#define GREEN_BACKGROUND 1
#define BLUE_BACKGROUND  2
#define RED_BACKGROUND   3
#define GRAY_BACKGROUND  4

#define CIRCLESDIALOG_GROUPMETA    1
#define CIRCLESDIALOG_GROUPDATA    2
#define CIRCLESDIALOG_GROUPUPDATE  3

/****************************************************************
 */

#define RSID_COL_NICKNAME   0
#define RSID_COL_KEYID      1
#define RSID_COL_IDTYPE     2
#define RSID_COL_VOTES      3

#define RSIDREP_COL_NAME       0
#define RSIDREP_COL_OPINION    1
#define RSIDREP_COL_COMMENT    2
#define RSIDREP_COL_REPUTATION 3

#define RSID_FILTER_OWNED_BY_YOU 0x0001
#define RSID_FILTER_FRIENDS      0x0002
#define RSID_FILTER_OTHERS       0x0004
#define RSID_FILTER_PSEUDONYMS   0x0008
#define RSID_FILTER_YOURSELF     0x0010
#define RSID_FILTER_BANNED       0x0020
#define RSID_FILTER_ALL          0xffff

#define IMAGE_EDIT                 ":/images/edit_16.png"
#define IMAGE_CREATE               ":/icons/circle_new_128.png"
#define IMAGE_INVITED              ":/icons/bullet_yellow_128.png"
#define IMAGE_MEMBER               ":/icons/bullet_green_128.png"
#define IMAGE_UNKNOWN              ":/icons/bullet_grey_128.png"
#define IMAGE_ADMIN                ":/icons/bullet_blue_128.png"
#define IMAGE_INFO                 ":/images/info16.png"

// comment this out in order to remove the sorting of circles into "belong to" and "other visible circles"
#define CIRCLE_MEMBERSHIP_CATEGORIES 1

static const uint32_t SortRole = Qt::UserRole+1 ;

// quick solution for RSID_COL_VOTES sorting
class TreeWidgetItem : public QTreeWidgetItem
{
  public:
  TreeWidgetItem(int type=Type): QTreeWidgetItem(type) {}
  TreeWidgetItem(QTreeWidget *tree): QTreeWidgetItem(tree) {}
  TreeWidgetItem(const QStringList& strings): QTreeWidgetItem (strings) {}

  bool operator< (const QTreeWidgetItem& other ) const
  {
    int column = treeWidget()->sortColumn();

    if(column == RSID_COL_VOTES)
    {
    const unsigned int v1 = data(column, SortRole).toUInt();
    const unsigned int v2 = other.data(column, SortRole).toUInt();

	return v1 < v2;
    }
    else // case insensitive sorting
        return data(column,Qt::DisplayRole).toString().toUpper() < other.data(column,Qt::DisplayRole).toString().toUpper();
  }
};

/** Constructor */
IdDialog::IdDialog(QWidget *parent) : MainPage(parent), ui(new Ui::IdDialog)
{
	ui->setupUi(this);

	mEventHandlerId_identity = 0;
	rsEvents->registerEventsHandler(
	            [this](std::shared_ptr<const RsEvent> event)
	{ RsQThreadUtils::postToObject([=](){ handleEvent_main_thread(event); }, this); },
	            mEventHandlerId_identity, RsEventType::GXS_IDENTITY );

	mEventHandlerId_circles = 0;
	rsEvents->registerEventsHandler(
	            [this](std::shared_ptr<const RsEvent> event)
	{ RsQThreadUtils::postToObject([=](){ handleEvent_main_thread(event); }, this); },
	            mEventHandlerId_circles, RsEventType::GXS_CIRCLES );

	// This is used to grab the broadcast of changes from p3GxsCircles, which is discarded by the current dialog, since it expects data for p3Identity only.
	//mCirclesBroadcastBase = new RsGxsUpdateBroadcastBase(rsGxsCircles, this);
	//connect(mCirclesBroadcastBase, SIGNAL(fillDisplay(bool)), this, SLOT(updateCirclesDisplay(bool)));
    
	ownItem = new QTreeWidgetItem();
	ownItem->setText(0, tr("My own identities"));
	ownItem->setData(RSID_COL_VOTES, Qt::DecorationRole,0xff);	// this is in order to prevent displaying a reputaiton icon next to these items.

	allItem = new QTreeWidgetItem();
	allItem->setText(0, tr("All"));
	allItem->setData(RSID_COL_VOTES, Qt::DecorationRole,0xff);

	contactsItem = new QTreeWidgetItem();
	contactsItem->setText(0, tr("My contacts"));
	contactsItem->setData(RSID_COL_VOTES, Qt::DecorationRole,0xff);

	ui->treeWidget_membership->clear();
	ui->treeWidget_membership->setItemDelegateForColumn(CIRCLEGROUP_CIRCLE_COL_GROUPNAME,new GxsIdTreeItemDelegate());

	mExternalOtherCircleItem = NULL ;
	mExternalBelongingCircleItem = NULL ;
	mMyCircleItem = NULL ;

	/* Setup UI helper */
	mStateHelper = new UIStateHelper(this);
//	mStateHelper->addWidget(IDDIALOG_IDLIST, ui->idTreeWidget);
	mStateHelper->addLoadPlaceholder(IDDIALOG_IDLIST, ui->idTreeWidget, false);
	mStateHelper->addClear(IDDIALOG_IDLIST, ui->idTreeWidget);

	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui->lineEdit_Nickname);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui->lineEdit_PublishTS);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui->lineEdit_KeyId);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui->lineEdit_Type);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui->lineEdit_GpgId);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui->lineEdit_GpgName);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui->lineEdit_LastUsed);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui->ownOpinion_CB);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui->autoBanIdentities_CB);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui->neighborNodesOpinion_TF);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui->overallOpinion_TF);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui->usageStatistics_TB);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui->inviteButton);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui->label_positive);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui->label_negative);

	mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui->lineEdit_Nickname);
	mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui->lineEdit_PublishTS);
	mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui->lineEdit_KeyId);
	mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui->lineEdit_Type);
	mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui->lineEdit_GpgId);
	mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui->lineEdit_GpgName);
	mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui->lineEdit_LastUsed);
	mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui->neighborNodesOpinion_TF);
	mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui->overallOpinion_TF);
	mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui->usageStatistics_TB);

	mStateHelper->addClear(IDDIALOG_IDDETAILS, ui->lineEdit_Nickname);
	mStateHelper->addClear(IDDIALOG_IDDETAILS, ui->lineEdit_PublishTS);
	mStateHelper->addClear(IDDIALOG_IDDETAILS, ui->lineEdit_KeyId);
	mStateHelper->addClear(IDDIALOG_IDDETAILS, ui->lineEdit_Type);
	mStateHelper->addClear(IDDIALOG_IDDETAILS, ui->lineEdit_GpgId);
	mStateHelper->addClear(IDDIALOG_IDDETAILS, ui->lineEdit_GpgName);
	mStateHelper->addClear(IDDIALOG_IDDETAILS, ui->lineEdit_LastUsed);
	mStateHelper->addClear(IDDIALOG_IDDETAILS, ui->neighborNodesOpinion_TF);
	mStateHelper->addClear(IDDIALOG_IDDETAILS, ui->overallOpinion_TF);
	mStateHelper->addClear(IDDIALOG_IDDETAILS, ui->usageStatistics_TB);
	mStateHelper->addClear(IDDIALOG_IDDETAILS, ui->label_positive);
	mStateHelper->addClear(IDDIALOG_IDDETAILS, ui->label_negative);

	//mStateHelper->addWidget(IDDIALOG_REPLIST, ui->treeWidget_RepList);
	//mStateHelper->addLoadPlaceholder(IDDIALOG_REPLIST, ui->treeWidget_RepList);
	//mStateHelper->addClear(IDDIALOG_REPLIST, ui->treeWidget_RepList);

	/* Connect signals */

	connect(ui->removeIdentity, SIGNAL(triggered()), this, SLOT(removeIdentity()));
	connect(ui->editIdentity, SIGNAL(triggered()), this, SLOT(editIdentity()));
	connect(ui->chatIdentity, SIGNAL(triggered()), this, SLOT(chatIdentity()));

	connect(ui->idTreeWidget, SIGNAL(itemSelectionChanged()), this, SLOT(updateSelection()));
	connect(ui->idTreeWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(IdListCustomPopupMenu(QPoint)));

	connect(ui->filterLineEdit, SIGNAL(textChanged(QString)), this, SLOT(filterChanged(QString)));
	connect(ui->ownOpinion_CB, SIGNAL(currentIndexChanged(int)), this, SLOT(modifyReputation()));
	
	connect(ui->inviteButton, SIGNAL(clicked()), this, SLOT(sendInvite()));

	connect( ui->idTreeWidget, &RSTreeWidget::itemDoubleClicked,
	         this, &IdDialog::chatIdentityItem );


    ui->avlabel_Circles->setPixmap(FilesDefs::getPixmapFromQtResourcePath(":/icons/png/circles.png"));

	ui->headerTextLabel_Circles->setText(tr("Circles"));

	/* Initialize splitter */
	ui->mainSplitter->setStretchFactor(0, 0);
	ui->mainSplitter->setStretchFactor(1, 1);
	
	clearPerson();

	/* Add filter types */
	QMenu *idTWHMenu = new QMenu(tr("Show Items"), this);
	ui->idTreeWidget->addContextMenuMenu(idTWHMenu);

	QActionGroup *idTWHActionGroup = new QActionGroup(this);
	QAction *idTWHAction = new QAction(QIcon(),tr("All"), this);
	idTWHAction->setActionGroup(idTWHActionGroup);
	idTWHAction->setCheckable(true);
	idTWHAction->setChecked(true);
	filter = RSID_FILTER_ALL;
	idTWHAction->setData(RSID_FILTER_ALL);
	connect(idTWHAction, SIGNAL(toggled(bool)), this, SLOT(filterToggled(bool)));
	idTWHMenu->addAction(idTWHAction);

	idTWHAction = new QAction(QIcon(),tr("Owned by myself"), this);
	idTWHAction->setActionGroup(idTWHActionGroup);
	idTWHAction->setCheckable(true);
	idTWHAction->setData(RSID_FILTER_OWNED_BY_YOU);
	connect(idTWHAction, SIGNAL(toggled(bool)), this, SLOT(filterToggled(bool)));
	idTWHMenu->addAction(idTWHAction);

	idTWHAction = new QAction(QIcon(),tr("Linked to my node"), this);
	idTWHAction->setActionGroup(idTWHActionGroup);
	idTWHAction->setCheckable(true);
	idTWHAction->setData(RSID_FILTER_YOURSELF);
	connect(idTWHAction, SIGNAL(toggled(bool)), this, SLOT(filterToggled(bool)));
	idTWHMenu->addAction(idTWHAction);

	idTWHAction = new QAction(QIcon(),tr("Linked to neighbor nodes"), this);
	idTWHAction->setActionGroup(idTWHActionGroup);
	idTWHAction->setCheckable(true);
	idTWHAction->setData(RSID_FILTER_FRIENDS);
	connect(idTWHAction, SIGNAL(toggled(bool)), this, SLOT(filterToggled(bool)));
	idTWHMenu->addAction(idTWHAction);

	idTWHAction = new QAction(QIcon(),tr("Linked to distant nodes"), this);
	idTWHAction->setActionGroup(idTWHActionGroup);
	idTWHAction->setCheckable(true);
	idTWHAction->setData(RSID_FILTER_OTHERS);
	connect(idTWHAction, SIGNAL(toggled(bool)), this, SLOT(filterToggled(bool)));
	idTWHMenu->addAction(idTWHAction);

	idTWHAction = new QAction(QIcon(),tr("Anonymous"), this);
	idTWHAction->setActionGroup(idTWHActionGroup);
	idTWHAction->setCheckable(true);
	idTWHAction->setData(RSID_FILTER_PSEUDONYMS);
	connect(idTWHAction, SIGNAL(toggled(bool)), this, SLOT(filterToggled(bool)));
	idTWHMenu->addAction(idTWHAction);

	idTWHAction = new QAction(QIcon(),tr("Banned"), this);
	idTWHAction->setActionGroup(idTWHActionGroup);
	idTWHAction->setCheckable(true);
	idTWHAction->setData(RSID_FILTER_BANNED);
	connect(idTWHAction, SIGNAL(toggled(bool)), this, SLOT(filterToggled(bool)));
	idTWHMenu->addAction(idTWHAction);
	
    QAction *CreateIDAction = new QAction(FilesDefs::getIconFromQtResourcePath(":/icons/png/person.png"),tr("Create new Identity"), this);
	connect(CreateIDAction, SIGNAL(triggered()), this, SLOT(addIdentity()));
	
    QAction *CreateCircleAction = new QAction(FilesDefs::getIconFromQtResourcePath(":/icons/png/circles.png"),tr("Create new circle"), this);
	connect(CreateCircleAction, SIGNAL(triggered()), this, SLOT(createExternalCircle()));
	
	QMenu *menu = new QMenu();
	menu->addAction(CreateIDAction);
	menu->addAction(CreateCircleAction);
	ui->toolButton_New->setMenu(menu);
	
	/* Add filter actions */
	QTreeWidgetItem *headerItem = ui->idTreeWidget->headerItem();
	QString headerText = headerItem->text(RSID_COL_NICKNAME);
	ui->filterLineEdit->addFilter(QIcon(), headerText, RSID_COL_NICKNAME, QString("%1 %2").arg(tr("Search"), headerText));

    headerItem->setData(RSID_COL_VOTES,Qt::UserRole,tr("Reputation"));

	/* Set initial section sizes */
  QHeaderView * circlesheader = ui->treeWidget_membership->header () ;
  circlesheader->resizeSection (CIRCLEGROUP_CIRCLE_COL_GROUPNAME, QFontMetricsF(ui->idTreeWidget->font()).width("Circle name")*1.5) ;
  ui->treeWidget_membership->setColumnWidth(CIRCLEGROUP_CIRCLE_COL_GROUPNAME, 270);

	ui->filterLineEdit->addFilter(QIcon(), tr("ID"), RSID_COL_KEYID, tr("Search ID"));

	/* Setup tree */
	ui->idTreeWidget->sortByColumn(RSID_COL_NICKNAME, Qt::AscendingOrder);

	ui->idTreeWidget->enableColumnCustomize(true);
	ui->idTreeWidget->setColumnCustomizable(RSID_COL_NICKNAME, false);

	ui->idTreeWidget->setColumnHidden(RSID_COL_IDTYPE, true);
	ui->idTreeWidget->setColumnHidden(RSID_COL_KEYID, true);
	
	/* Set initial column width */
	int fontWidth = QFontMetricsF(ui->idTreeWidget->font()).width("W");
	ui->idTreeWidget->setColumnWidth(RSID_COL_NICKNAME, 14 * fontWidth);
	ui->idTreeWidget->setColumnWidth(RSID_COL_KEYID, 20 * fontWidth);
	ui->idTreeWidget->setColumnWidth(RSID_COL_IDTYPE, 18 * fontWidth);
	ui->idTreeWidget->setColumnWidth(RSID_COL_VOTES, 2 * fontWidth);
	
	ui->idTreeWidget->setItemDelegateForColumn(
	            RSID_COL_NICKNAME,
	            new GxsIdTreeItemDelegate());
	ui->idTreeWidget->setItemDelegateForColumn(
	            RSID_COL_VOTES,
	            new ReputationItemDelegate(RsReputationLevel(0xff)));

	/* Set header resize modes and initial section sizes */
	QHeaderView * idheader = ui->idTreeWidget->header();
	QHeaderView_setSectionResizeModeColumn(idheader, RSID_COL_VOTES, QHeaderView::ResizeToContents);

	mStateHelper->setActive(IDDIALOG_IDDETAILS, false);
	mStateHelper->setActive(IDDIALOG_REPLIST, false);

	QString hlp_str = tr(
			" <h1><img width=\"32\" src=\":/icons/help_64.png\">&nbsp;&nbsp;Identities</h1>    \
			<p>In this tab you can create/edit <b>pseudo-anonymous identities</b>, and <b>circles</b>.</p>                \
			<p><b>Identities</b> are used to securely identify your data: sign messages in chat lobbies, forum and channel posts,\
				receive feedback using the Retroshare built-in email system, post comments \
				after channel posts, chat using secured tunnels, etc.</p> \
			<p>Identities can optionally be <b>signed</b> by your Retroshare node's certificate.   \
			Signed identities are easier to trust but are easily linked to your node's IP address.</p>  \
			<p><b>Anonymous identities</b> allow you to anonymously interact with other users. They cannot be   \
			spoofed, but noone can prove who really owns a given identity.</p> \
                    <p><b>Circles</b> are groups of identities (anonymous or signed), that are shared at a distance over the network. They can be \
                		used to restrict the visibility to forums, channels, etc. </p> \
                    <p>An <b>circle</b> can be restricted to another circle, thereby limiting its visibility to members of that circle \
                        or even self-restricted, meaning that it is only visible to invited members.</p>") ;

	registerHelpButton(ui->helpButton, hlp_str,"PeopleDialog") ;

	// load settings
	processSettings(true);

    // circles stuff
    
    //connect(ui->treeWidget_membership, SIGNAL(itemSelectionChanged()), this, SLOT(circle_selected()));
    connect(ui->treeWidget_membership, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(CircleListCustomPopupMenu(QPoint)));
    connect(ui->autoBanIdentities_CB, SIGNAL(toggled(bool)), this, SLOT(toggleAutoBanIdentities(bool)));

    updateCircles();
    updateIdList();
}

void IdDialog::handleEvent_main_thread(std::shared_ptr<const RsEvent> event)
{
	if(event->mType == RsEventType::GXS_IDENTITY)
	{
		const RsGxsIdentityEvent *e = dynamic_cast<const RsGxsIdentityEvent*>(event.get());

		if(!e)
			return;

		switch(e->mIdentityEventCode)
		{
		case RsGxsIdentityEventCode::DELETED_IDENTITY:
		case RsGxsIdentityEventCode::NEW_IDENTITY:
		case RsGxsIdentityEventCode::UPDATED_IDENTITY:

			updateIdList();

    		if(!mId.isNull() && mId == e->mIdentityId)
				updateIdentity();

			break;
		default:
			break;
		}
	}
    else if(event->mType == RsEventType::GXS_CIRCLES)
    {
		const RsGxsCircleEvent *e = dynamic_cast<const RsGxsCircleEvent*>(event.get());

		if(!e)
			return;

		switch(e->mCircleEventType)
		{
		case RsGxsCircleEventCode::CIRCLE_MEMBERSHIP_REQUEST:
		case RsGxsCircleEventCode::CIRCLE_MEMBERSHIP_ID_ADDED_TO_INVITEE_LIST:
		case RsGxsCircleEventCode::CIRCLE_MEMBERSHIP_LEAVE:
		case RsGxsCircleEventCode::CIRCLE_MEMBERSHIP_ID_REMOVED_FROM_INVITEE_LIST:
		case RsGxsCircleEventCode::NEW_CIRCLE:
		case RsGxsCircleEventCode::CACHE_DATA_UPDATED:

			updateCircles();
		default:
			break;
		}

    }
}

void IdDialog::clearPerson()
{
	QFontMetricsF f(ui->avLabel_Person->font()) ;

    ui->avLabel_Person->setPixmap(FilesDefs::getPixmapFromQtResourcePath(":/icons/png/people.png").scaled(f.height()*4,f.height()*4,Qt::IgnoreAspectRatio,Qt::SmoothTransformation));
	ui->headerTextLabel_Person->setText(tr("People"));

	ui->inviteFrame->hide();
	ui->avatarLabel->clear();

	whileBlocking(ui->ownOpinion_CB)->setCurrentIndex(1);
	whileBlocking(ui->autoBanIdentities_CB)->setChecked(false);

}

void IdDialog::toggleAutoBanIdentities(bool b)
{
    RsPgpId id(ui->lineEdit_GpgId->text().left(16).toStdString());

    if(!id.isNull())
    {
        rsReputations->banNode(id,b) ;
        updateIdList();
    }
}

void IdDialog::updateCirclesDisplay()
{
    if(RsAutoUpdatePage::eventsLocked())
        return ;
    
    if(!isVisible())
        return ;
    
#ifdef ID_DEBUG
    std::cerr << "!!Updating circles display!" << std::endl;
#endif
    updateCircles() ;
}

/************************** Request / Response *************************/
/*** Loading Main Index ***/

void IdDialog::updateCircles()
{
	RsThread::async([this]()
	{
        // 1 - get message data from p3GxsForums

#ifdef DEBUG_FORUMS
        std::cerr << "Retrieving post data for post " << mThreadId << std::endl;
#endif

		/* This can be big so use a smart pointer to just copy the pointer
		 * instead of copying the whole list accross the lambdas */
		auto circle_metas = std::make_unique<std::list<RsGroupMetaData>>();

		if(!rsGxsCircles->getCirclesSummaries(*circle_metas))
		{
			RS_ERR("failed to retrieve circles group info list");
			return;
		}

		RsQThreadUtils::postToObject(
		            [circle_metas = std::move(circle_metas), this]()
		{
			/* Here it goes any code you want to be executed on the Qt Gui
			 * thread, for example to update the data model with new information
			 * after a blocking call to RetroShare API complete */

			loadCircles(*circle_metas);

		}, this );

    });
}

static QTreeWidgetItem *setChildItem(QTreeWidgetItem *item, const RsGroupMetaData& circle_group)
{
    QString test_str = QString::fromStdString(circle_group.mGroupId.toStdString());

    // 1 - check if the item already exists and remove possible duplicates

    std::vector<uint32_t> found_indices;

	for(uint32_t k=0; k < (uint32_t)item->childCount(); ++k)
		if( item->child(k)->data(CIRCLEGROUP_CIRCLE_COL_GROUPID,Qt::UserRole).toString() == test_str)
            found_indices.push_back(k);

    while(found_indices.size() > 1)							// delete duplicates, starting from the end in order that deletion preserves indices
    {
        delete item->takeChild(found_indices.back());
        found_indices.pop_back();
    }

    if(!found_indices.empty())
    {
		QTreeWidgetItem *subitem = item->child(found_indices[0]);

        if(subitem->text(CIRCLEGROUP_CIRCLE_COL_GROUPNAME) != QString::fromUtf8(circle_group.mGroupName.c_str()))
		{
#ifdef ID_DEBUG
			std::cerr << "  Existing circle has a new name. Updating it in the tree." << std::endl;
#endif
			subitem->setText(CIRCLEGROUP_CIRCLE_COL_GROUPNAME, QString::fromUtf8(circle_group.mGroupName.c_str()));
		}

        return subitem;
    }

    // 2 - if not, create

	QTreeWidgetItem *subitem = new QTreeWidgetItem();

	subitem->setText(CIRCLEGROUP_CIRCLE_COL_GROUPNAME, QString::fromUtf8(circle_group.mGroupName.c_str()));
	subitem->setData(CIRCLEGROUP_CIRCLE_COL_GROUPID,Qt::UserRole, QString::fromStdString(circle_group.mGroupId.toStdString()));
	subitem->setData(CIRCLEGROUP_CIRCLE_COL_GROUPFLAGS, Qt::UserRole, QVariant(circle_group.mSubscribeFlags));

	item->addChild(subitem);

    return subitem;
}

static void removeChildItem(QTreeWidgetItem *item, const RsGroupMetaData& circle_group)
{
    QString test_str = QString::fromStdString(circle_group.mGroupId.toStdString());

    // 1 - check if the item already exists and remove possible duplicates

    std::list<uint32_t> found_indices;

	for(uint32_t k=0; k < (uint32_t)item->childCount(); ++k)
		if( item->child(k)->data(CIRCLEGROUP_CIRCLE_COL_GROUPID,Qt::UserRole).toString() == test_str)
            found_indices.push_front(k);

    for(auto k:found_indices)
        delete item->takeChild(k);	// delete items in the reverse order (because of the push_front()), so that indices are preserved
}

void IdDialog::loadCircles(const std::list<RsGroupMetaData>& groupInfo)
{
#ifdef ID_DEBUG
	std::cerr << "CirclesDialog::loadCircleGroupMeta()";
	std::cerr << std::endl;
#endif

	mStateHelper->setActive(CIRCLESDIALOG_GROUPMETA, true);

    std::vector<bool> expanded_top_level_items;
    std::set<RsGxsCircleId> expanded_circle_items;
    saveExpandedCircleItems(expanded_top_level_items,expanded_circle_items);

#ifdef QT_BUG_CRASH_IN_TAKECHILD_WORKAROUND
    // These 3 lines are normally not needed. But apparently a bug (in Qt ??) causes Qt to crash when takeChild() is called. If we remove everything from the
    // tree widget before updating it, takeChild() is never called, but the all tree is filled again from scratch. This is less efficient obviously, and
    // also collapses the tree. Because it is a *temporary* fix, I dont take the effort to save open/collapsed items yet. If we cannot find a proper way to fix
    // this, then we'll need to implement the two missing functions to save open/collapsed items.

	ui->treeWidget_membership->clear();
	mExternalOtherCircleItem = NULL ;
	mExternalBelongingCircleItem = NULL ;
#endif

	/* add the top level item */
	//QTreeWidgetItem *personalCirclesItem = new QTreeWidgetItem();
	//personalCirclesItem->setText(0, tr("Personal Circles"));
	//ui->treeWidget_membership->addTopLevelItem(personalCirclesItem);

#ifdef CIRCLE_MEMBERSHIP_CATEGORIES
	if(!mExternalOtherCircleItem)
	{
		mExternalOtherCircleItem = new QTreeWidgetItem();
		mExternalOtherCircleItem->setText(0, tr("Other circles"));

		ui->treeWidget_membership->addTopLevelItem(mExternalOtherCircleItem);
	}

	if(!mExternalBelongingCircleItem )
	{
		mExternalBelongingCircleItem = new QTreeWidgetItem();
		mExternalBelongingCircleItem->setText(0, tr("Circles I belong to"));
		ui->treeWidget_membership->addTopLevelItem(mExternalBelongingCircleItem);
	}
#endif

	std::list<RsGxsId> own_identities ;
	rsIdentity->getOwnIds(own_identities) ;

	for(auto vit = groupInfo.begin(); vit != groupInfo.end();++vit)
	{
#ifdef ID_DEBUG
		std::cerr << "CirclesDialog::loadCircleGroupMeta() GroupId: " << vit->mGroupId << " Group: " << vit->mGroupName << std::endl;
#endif
		RsGxsCircleDetails details;
		rsGxsCircles->getCircleDetails(RsGxsCircleId(vit->mGroupId), details) ;

		bool am_I_in_circle = details.mAmIAllowed ;
		bool am_I_admin (vit->mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_ADMIN) ;
		bool am_I_subscribed (vit->mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED) ;
#ifdef ID_DEBUG
		std::cerr << "Loaded info for circle " << vit->mGroupId << ". am_I_in_circle=" << am_I_in_circle << std::endl;
#endif

		// Find already existing items for this circle, or create one.
		QTreeWidgetItem *item = NULL ;

        if(am_I_in_circle)
        {
            item = setChildItem(mExternalBelongingCircleItem,*vit);
            removeChildItem(mExternalOtherCircleItem,*vit);
        }
        else
        {
            item = setChildItem(mExternalOtherCircleItem,*vit);
            removeChildItem(mExternalBelongingCircleItem,*vit);
        }
		item->setData(CIRCLEGROUP_CIRCLE_COL_GROUPFLAGS, Qt::UserRole, QVariant(vit->mSubscribeFlags));

		QString tooltip ;
		tooltip += tr("Circle ID: ")+QString::fromStdString(vit->mGroupId.toStdString()) ;
		tooltip += "\n"+tr("Visibility: ");

		if(details.mRestrictedCircleId == details.mCircleId)
			tooltip += tr("Private (only visible to invited members)") ;
		else if(!details.mRestrictedCircleId.isNull())
			tooltip += tr("Only visible to full members of circle ")+QString::fromStdString(details.mRestrictedCircleId.toStdString()) ;
		else
			tooltip += tr("Public") ;

		tooltip += "\n"+tr("Your role: ");

		if(am_I_admin)
			tooltip += tr("Administrator (Can edit invite list, and request membership).") ;
		else
			tooltip += tr("User (Can only request membership).") ;

		tooltip += "\n"+tr("Distribution: ");
		if(am_I_subscribed)
			tooltip += tr("subscribed (Receive/forward membership requests from others and invite list).") ;
		else
			tooltip += tr("unsubscribed (Only receive invite list).") ;

		tooltip += "\n"+tr("Your status: ") ;

		if(am_I_in_circle)
			tooltip += tr("Full member (you have access to data limited to this circle)") ;
		else
			tooltip += tr("Not a member (do not have access to data limited to this circle)") ;

		item->setToolTip(CIRCLEGROUP_CIRCLE_COL_GROUPNAME,tooltip);

		if (am_I_admin)
		{
			QFont font = item->font(CIRCLEGROUP_CIRCLE_COL_GROUPNAME) ;
			font.setBold(true) ;
			item->setFont(CIRCLEGROUP_CIRCLE_COL_GROUPNAME,font) ;
			item->setFont(CIRCLEGROUP_CIRCLE_COL_GROUPID,font) ;
			item->setFont(CIRCLEGROUP_CIRCLE_COL_GROUPFLAGS,font) ;
		}

		// now determine for this circle wether we have pending invites
		// we add a sub-item to the circle (to show the invite system info) in the following two cases:
		//	- own GXS id is in admin list but not subscribed
		//	- own GXS id is is admin and subscribed
		//	- own GXS id is is subscribed

		bool am_I_invited = false ;
		bool am_I_pending = false ;
#ifdef ID_DEBUG
		std::cerr << "  updating status of all identities for this circle:" << std::endl;
#endif
		// remove any identity that has an item, but no subscription flag entry
		std::list<int> to_delete ;

		for(uint32_t k=0; k < (uint32_t)item->childCount(); ++k)
			if(details.mSubscriptionFlags.find(RsGxsId(item->child(k)->data(CIRCLEGROUP_CIRCLE_COL_GROUPID,Qt::UserRole).toString().toStdString())) == details.mSubscriptionFlags.end())
				to_delete.push_front(k);	// front, so that we delete starting from the last one

		for(auto index:to_delete)
			delete item->takeChild(index);	// delete items starting from the largest index, because otherwise the count changes while deleting...

		for(std::map<RsGxsId,uint32_t>::const_iterator it(details.mSubscriptionFlags.begin());it!=details.mSubscriptionFlags.end();++it)
		{
#ifdef ID_DEBUG
			std::cerr << "    ID " << it->first << ": " ;
#endif
			bool is_own_id = rsIdentity->isOwnId(it->first) ;
			bool invited ( it->second & GXS_EXTERNAL_CIRCLE_FLAGS_IN_ADMIN_LIST );
			bool subscrb ( it->second & GXS_EXTERNAL_CIRCLE_FLAGS_SUBSCRIBED );

#ifdef ID_DEBUG
			std::cerr << "invited: " << invited << ", subscription: " << subscrb ;
#endif
            int subitem_index = -1;

			// see if the item already exists
			for(uint32_t k=0; k < (uint32_t)item->childCount(); ++k)
				if(item->child(k)->data(CIRCLEGROUP_CIRCLE_COL_GROUPID,Qt::UserRole).toString().toStdString() == it->first.toStdString())
				{
                    subitem_index = k;
#ifdef ID_DEBUG
					std::cerr << " found existing sub item." << std::endl;
#endif
					break ;
				}

			if(!(invited || subscrb))
			{
				if(subitem_index >= 0)
					delete item->takeChild(subitem_index) ;
#ifdef ID_DEBUG
				std::cerr << ". not relevant. Skipping." << std::endl;
#endif
				continue ;
			}
			// remove item if flags are not ok.

			if(subitem_index >= 0 && item->child(subitem_index)->data(CIRCLEGROUP_CIRCLE_COL_GROUPFLAGS, Qt::UserRole).toUInt() != it->second)
			{
				delete item->takeChild(subitem_index) ;
				subitem_index = -1 ;
			}

			QTreeWidgetItem *subitem(NULL);

			if(subitem_index == -1)
			{
#ifdef ID_DEBUG
				std::cerr << " no existing sub item. Creating new one." << std::endl;
#endif
				subitem = new RSTreeWidgetItem(NULL);
				subitem->setData(CIRCLEGROUP_CIRCLE_COL_GROUPNAME,Qt::UserRole,QString::fromStdString(it->first.toStdString()));
				//Icon PlaceHolder
				subitem->setIcon(CIRCLEGROUP_CIRCLE_COL_GROUPNAME,FilesDefs::getIconFromQtResourcePath(":/icons/png/anonymous.png"));

				RsIdentityDetails idd ;
				//bool has_id =
				rsIdentity->getIdDetails(it->first,idd) ;

				// QPixmap pixmap ;

				// if(idd.mAvatar.mSize == 0 || !GxsIdDetails::loadPixmapFromData(idd.mAvatar.mData, idd.mAvatar.mSize, pixmap,GxsIdDetails::SMALL))
				// 	pixmap = GxsIdDetails::makeDefaultIcon(it->first,GxsIdDetails::SMALL) ;

				// if(has_id)
				// 	subitem->setText(CIRCLEGROUP_CIRCLE_COL_GROUPNAME, QString::fromUtf8(idd.mNickname.c_str())) ;
				// else
				// 	subitem->setText(CIRCLEGROUP_CIRCLE_COL_GROUPNAME, tr("Unknown ID:")+QString::fromStdString(it->first.toStdString())) ;

				QString tooltip ;
				tooltip += tr("Identity ID: ")+QString::fromStdString(it->first.toStdString()) ;
				tooltip += "\n"+tr("Status: ") ;
				if(invited)
					if(subscrb)
						tooltip += tr("Full member") ;
					else
						tooltip += tr("Invited by admin") ;
				else
					if(subscrb)
						tooltip += tr("Subscription request pending") ;
					else
						tooltip += tr("unknown") ;

				subitem->setToolTip(CIRCLEGROUP_CIRCLE_COL_GROUPNAME, tooltip) ;

				subitem->setData(CIRCLEGROUP_CIRCLE_COL_GROUPFLAGS, Qt::UserRole, QVariant(it->second)) ;
				subitem->setData(CIRCLEGROUP_CIRCLE_COL_GROUPID, Qt::UserRole, QString::fromStdString(it->first.toStdString())) ;

				//subitem->setIcon(RSID_COL_NICKNAME, QIcon(pixmap));

				item->addChild(subitem) ;
			}
            else
                subitem = item->child(subitem_index);

			if(invited && !subscrb)
			{
				subitem->setText(CIRCLEGROUP_CIRCLE_COL_GROUPID, tr("Invited")) ;

				if(is_own_id)
					am_I_invited = true ;
			}
			if(!invited && subscrb)
			{
				subitem->setText(CIRCLEGROUP_CIRCLE_COL_GROUPID, tr("Subscription pending")) ;

				if(is_own_id)
					am_I_pending = true ;
			}
			if(invited && subscrb)
				subitem->setText(CIRCLEGROUP_CIRCLE_COL_GROUPID, tr("Member")) ;

			if (is_own_id)
			{
				QFont font = subitem->font(CIRCLEGROUP_CIRCLE_COL_GROUPNAME) ;
				font.setBold(true) ;
				subitem->setFont(CIRCLEGROUP_CIRCLE_COL_GROUPNAME,font) ;
				subitem->setFont(CIRCLEGROUP_CIRCLE_COL_GROUPID,font) ;
				subitem->setFont(CIRCLEGROUP_CIRCLE_COL_GROUPFLAGS,font) ;
			}
		}    

		// The bullet colors below are for the *Membership*. This is independent from admin rights, which cannot be shown as a color.
		// Admin/non admin is shows using Bold font.

		if(am_I_in_circle)
			item->setIcon(CIRCLEGROUP_CIRCLE_COL_GROUPNAME,FilesDefs::getIconFromQtResourcePath(IMAGE_MEMBER)) ;
		else if(am_I_invited || am_I_pending)
			item->setIcon(CIRCLEGROUP_CIRCLE_COL_GROUPNAME,FilesDefs::getIconFromQtResourcePath(IMAGE_INVITED)) ;
		else
			item->setIcon(CIRCLEGROUP_CIRCLE_COL_GROUPNAME,FilesDefs::getIconFromQtResourcePath(IMAGE_UNKNOWN)) ;
	}
    restoreExpandedCircleItems(expanded_top_level_items,expanded_circle_items);
}

//static void mark_matching_tree(QTreeWidget *w, const std::set<RsGxsId>& members, int col)
//{
//    w->selectionModel()->clearSelection() ;
//
//    for(std::set<RsGxsId>::const_iterator it(members.begin());it!=members.end();++it)
//    {
//	QList<QTreeWidgetItem*> clist = w->findItems( QString::fromStdString((*it).toStdString()), Qt::MatchExactly|Qt::MatchRecursive, col);
//
//    	foreach(QTreeWidgetItem* item, clist)
//		item->setSelected(true) ;
//    }
//}

bool IdDialog::getItemCircleId(QTreeWidgetItem *item,RsGxsCircleId& id)
{
#ifdef CIRCLE_MEMBERSHIP_CATEGORIES    
    if ((!item) || (!item->parent()))
	    return false;
    
	QString coltext = (item->parent()->parent())? (item->parent()->data(CIRCLEGROUP_CIRCLE_COL_GROUPID,Qt::UserRole).toString()) : (item->data(CIRCLEGROUP_CIRCLE_COL_GROUPID,Qt::UserRole).toString());
    id = RsGxsCircleId( coltext.toStdString()) ;
#else
    if(!item)
	    return false;
    
    QString coltext = (item->parent())? (item->parent()->data(CIRCLEGROUP_CIRCLE_COL_GROUPID,Qt::UserRole).toString()) : (item->data(CIRCLEGROUP_CIRCLE_COL_GROUPID,Qt::UserRole).toString());
    id = RsGxsCircleId( coltext.toStdString()) ;
#endif
    return true ;
}

void IdDialog::createExternalCircle()
{
	CreateCircleDialog dlg;
	dlg.editNewId(true);
	dlg.exec();
}
void IdDialog::showEditExistingCircle()
{
    RsGxsCircleId id ;
    
    if(!getItemCircleId(ui->treeWidget_membership->currentItem(),id))
       return ;
    
    uint32_t subscribe_flags = ui->treeWidget_membership->currentItem()->data(CIRCLEGROUP_CIRCLE_COL_GROUPFLAGS, Qt::UserRole).toUInt();
	
    CreateCircleDialog dlg;
    
    dlg.editExistingId(RsGxsGroupId(id),true,!(subscribe_flags & GXS_SERV::GROUP_SUBSCRIBE_ADMIN)) ;
    dlg.exec();
}

void IdDialog::grantCircleMembership() 
{
	RsGxsCircleId circle_id ;

    if(!getItemCircleId(ui->treeWidget_membership->currentItem(),circle_id))
	    return;

    RsGxsId gxs_id_to_grant(qobject_cast<QAction*>(sender())->data().toString().toStdString());

	RsThread::async([circle_id,gxs_id_to_grant]()
	{
        // 1 - set message data in p3GxsCircles

        rsGxsCircles->inviteIdsToCircle(std::set<RsGxsId>( { gxs_id_to_grant } ),circle_id);
    });
}

void IdDialog::revokeCircleMembership() 
{
	RsGxsCircleId circle_id ;

    if(!getItemCircleId(ui->treeWidget_membership->currentItem(),circle_id))
	    return;

    if(circle_id.isNull())
    {
		RsErr() << __PRETTY_FUNCTION__ << " : got a null circle ID. Cannot revoke an identity from that circle!" << std::endl;
        return ;
    }

    RsGxsId gxs_id_to_revoke(qobject_cast<QAction*>(sender())->data().toString().toStdString());

    if(gxs_id_to_revoke.isNull())
		RsErr() << __PRETTY_FUNCTION__ << " : got a null ID. Cannot revoke it from circle " << circle_id << "!" << std::endl;
	else
		RsThread::async([circle_id,gxs_id_to_revoke]()
		{
			// 1 - get message data from p3GxsForums

            std::set<RsGxsId> ids;
            ids.insert(gxs_id_to_revoke);

			rsGxsCircles->revokeIdsFromCircle(ids,circle_id);
		});
}

void IdDialog::acceptCircleSubscription() 
{
    RsGxsCircleId circle_id ;
    
    if(!getItemCircleId(ui->treeWidget_membership->currentItem(),circle_id))
        return;

    RsGxsId own_id(qobject_cast<QAction*>(sender())->data().toString().toStdString());
    
    rsGxsCircles->requestCircleMembership(own_id,circle_id) ;
}

void IdDialog::cancelCircleSubscription() 
{	
    RsGxsCircleId circle_id ;
    
    if(!getItemCircleId(ui->treeWidget_membership->currentItem(),circle_id))
        return;

    RsGxsId own_id(qobject_cast<QAction*>(sender())->data().toString().toStdString());

    rsGxsCircles->cancelCircleMembership(own_id,circle_id) ;
}
    
void IdDialog::CircleListCustomPopupMenu( QPoint )
{
    QMenu contextMnu( this );

    RsGxsCircleId circle_id ;
    QTreeWidgetItem *item = ui->treeWidget_membership->currentItem();
            
    if(!getItemCircleId(item,circle_id))
        return ;

    RsGxsId current_gxs_id ;
    RsGxsId item_id(item->data(CIRCLEGROUP_CIRCLE_COL_GROUPID,Qt::UserRole).toString().toStdString());
    bool is_circle ;
    bool am_I_circle_admin = false ;
    
    if(item_id == RsGxsId(circle_id))	// is it a circle?
    {
	    uint32_t group_flags = item->data(CIRCLEGROUP_CIRCLE_COL_GROUPFLAGS, Qt::UserRole).toUInt();

#ifdef CIRCLE_MEMBERSHIP_CATEGORIES
	    if(item->parent() != NULL)
	    {
#endif
		    if(group_flags & GXS_SERV::GROUP_SUBSCRIBE_ADMIN)
		    {
                contextMnu.addAction(FilesDefs::getIconFromQtResourcePath(IMAGE_EDIT), tr("Edit Circle"), this, SLOT(showEditExistingCircle()));
			    am_I_circle_admin = true ;
		    }
		    else
                contextMnu.addAction(FilesDefs::getIconFromQtResourcePath(IMAGE_INFO), tr("See details"), this, SLOT(showEditExistingCircle()));
#ifdef CIRCLE_MEMBERSHIP_CATEGORIES
	}
#endif
        
#ifdef ID_DEBUG
	    std::cerr << "  Item is a circle item. Adding Edit/Details menu entry." << std::endl;
#endif
            is_circle = true ;

	    contextMnu.addSeparator() ;
    }
    else 
    {
	    current_gxs_id = RsGxsId(item_id);
	    is_circle =false ;

	    if(item->parent() != NULL)
	    {
		    uint32_t group_flags = item->parent()->data(CIRCLEGROUP_CIRCLE_COL_GROUPFLAGS, Qt::UserRole).toUInt();
		    am_I_circle_admin = bool(group_flags & GXS_SERV::GROUP_SUBSCRIBE_ADMIN) ;
	    }

#ifdef ID_DEBUG
	    std::cerr << "  Item is a GxsId item. Requesting flags/group id from parent: " << circle_id << std::endl;
#endif
    }
 
    RsGxsCircleDetails details ;
    
    if(!rsGxsCircles->getCircleDetails(circle_id,details))// grab real circle ID from parent. Make sure circle id is used correctly afterwards!
    {
        std::cerr << "  (EE) cannot get circle info for ID " << circle_id << ". Not in cache?" << std::endl;
        return ;
    }

    static const int REQUES = 0 ; // Admin list: no          Subscription request:  no
    static const int ACCEPT = 1 ; // Admin list: yes         Subscription request:  no
    static const int REMOVE = 2 ; // Admin list: yes         Subscription request:  yes
    static const int CANCEL = 3 ; // Admin list: no          Subscription request:  yes

    const QString menu_titles[4] = { tr("Request subscription"), tr("Accept circle invitation"), tr("Quit this circle"),tr("Cancel subscribe request")} ;
    const QString image_names[4] = { ":/images/edit_add24.png",":/images/accepted16.png",":/icons/png/enter.png",":/images/cancel.png" } ;

    std::vector< std::vector<RsGxsId> > ids(4) ;

    std::list<RsGxsId> own_identities ;
    rsIdentity->getOwnIds(own_identities) ;

    // policy is:
    //	- if on a circle item
    //		=> add possible subscription requests for all ids
    //	- if on a Id item
    //		=> only add subscription requests for that ID

    for(std::list<RsGxsId>::const_iterator it(own_identities.begin());it!=own_identities.end();++it)
	    if(is_circle || current_gxs_id == *it)
	    {
		    std::map<RsGxsId,uint32_t>::const_iterator vit = details.mSubscriptionFlags.find(*it) ;
		    uint32_t subscribe_flags = (vit == details.mSubscriptionFlags.end())?0:(vit->second) ;

		    if(subscribe_flags & GXS_EXTERNAL_CIRCLE_FLAGS_SUBSCRIBED)
			    if(subscribe_flags & GXS_EXTERNAL_CIRCLE_FLAGS_IN_ADMIN_LIST)
				    ids[REMOVE].push_back(*it) ;
			    else
				    ids[CANCEL].push_back(*it) ;
		    else 
			    if(subscribe_flags & GXS_EXTERNAL_CIRCLE_FLAGS_IN_ADMIN_LIST)
				    ids[ACCEPT].push_back(*it) ;
			    else
				    ids[REQUES].push_back(*it) ;
	    }
    contextMnu.addSeparator() ;

    for(int i=0;i<4;++i)
    {
	    if(ids[i].size() == 1)
	    {
		    RsIdentityDetails det ;
		    QString id_name ;
		    if(rsIdentity->getIdDetails(ids[i][0],det))
			    id_name = tr("for identity ")+QString::fromUtf8(det.mNickname.c_str()) + " (ID=" + QString::fromStdString(ids[i][0].toStdString()) + ")" ;
		    else
			    id_name = tr("for identity ")+QString::fromStdString(ids[i][0].toStdString()) ;

		    QAction *action ;

		    if(is_circle)
                action = new QAction(FilesDefs::getIconFromQtResourcePath(image_names[i]), menu_titles[i] + " " + id_name,this) ;
		    else
                action = new QAction(FilesDefs::getIconFromQtResourcePath(image_names[i]), menu_titles[i],this) ;

		    if(i <2)
			    QObject::connect(action,SIGNAL(triggered()), this, SLOT(acceptCircleSubscription()));
		    else
			    QObject::connect(action,SIGNAL(triggered()), this, SLOT(cancelCircleSubscription()));

		    action->setData(QString::fromStdString(ids[i][0].toStdString()));
		    contextMnu.addAction(action) ;
	    }
	    else if(ids[i].size() > 1)
	    {
		    QMenu *menu = new QMenu(menu_titles[i],this) ;

		    for(uint32_t j=0;j<ids[i].size();++j)
		    {
			    RsIdentityDetails det ;
			    QString id_name ;
			    if(rsIdentity->getIdDetails(ids[i][j],det))
				    id_name = tr("for identity ")+QString::fromUtf8(det.mNickname.c_str()) + " (ID=" + QString::fromStdString(ids[i][j].toStdString()) + ")" ;
			    else
				    id_name = tr("for identity ")+QString::fromStdString(ids[i][j].toStdString()) ;

                QAction *action = new QAction(FilesDefs::getIconFromQtResourcePath(image_names[i]), id_name,this) ;

			    if(i <2)
				    QObject::connect(action,SIGNAL(triggered()), this, SLOT(acceptCircleSubscription()));
			    else
				    QObject::connect(action,SIGNAL(triggered()), this, SLOT(cancelCircleSubscription()));

			    action->setData(QString::fromStdString(ids[i][j].toStdString()));
			    menu->addAction(action) ;
		    }

		    contextMnu.addMenu(menu) ;
	    }
    }
    
    if(!is_circle && am_I_circle_admin)	// I am circle admin. I can therefore revoke/accept membership
    {
	std::map<RsGxsId,uint32_t>::const_iterator it = details.mSubscriptionFlags.find(current_gxs_id) ;
    
        if(!current_gxs_id.isNull() && it != details.mSubscriptionFlags.end())
        {
	    contextMnu.addSeparator() ;

	    if(it->second & GXS_EXTERNAL_CIRCLE_FLAGS_IN_ADMIN_LIST)
	    { 
		    QAction *action = new QAction(tr("Revoke this member"),this) ;
		    action->setData(QString::fromStdString(current_gxs_id.toStdString()));
		    QObject::connect(action,SIGNAL(triggered()), this, SLOT(revokeCircleMembership()));
		    contextMnu.addAction(action) ;
	    }
	    else
	    {
		    QAction *action = new QAction(tr("Grant membership"),this) ;
		    action->setData(QString::fromStdString(current_gxs_id.toStdString()));
		    QObject::connect(action,SIGNAL(triggered()), this, SLOT(grantCircleMembership()));
		    contextMnu.addAction(action) ;
	    }

        }
    }

    contextMnu.exec(QCursor::pos());
}

IdDialog::~IdDialog()
{
	rsEvents->unregisterEventsHandler(mEventHandlerId_identity);
	rsEvents->unregisterEventsHandler(mEventHandlerId_circles);

	// save settings
	processSettings(false);

	delete(ui);
}

static QString getHumanReadableDuration(uint32_t seconds)
{
    if(seconds < 60)
        return QString(QObject::tr("%1 seconds ago")).arg(seconds) ;
    else if(seconds < 120)
        return QString(QObject::tr("%1 minute ago")).arg(seconds/60) ;
    else if(seconds < 3600)
        return QString(QObject::tr("%1 minutes ago")).arg(seconds/60) ;
    else if(seconds < 7200)
        return QString(QObject::tr("%1 hour ago")).arg(seconds/3600) ;
    else if(seconds < 24*3600)
        return QString(QObject::tr("%1 hours ago")).arg(seconds/3600) ;
    else if(seconds < 2*24*3600)
        return QString(QObject::tr("%1 day ago")).arg(seconds/86400) ;
    else 
        return QString(QObject::tr("%1 days ago")).arg(seconds/86400) ;
}

void IdDialog::processSettings(bool load)
{
	Settings->beginGroup("IdDialog");

	// state of peer tree
	ui->idTreeWidget->processSettings(load);

	if (load) {
		// load settings

		// filterColumn
		ui->filterLineEdit->setCurrentFilter(Settings->value("filterColumn", RSID_COL_NICKNAME).toInt());

		// state of splitter
		ui->mainSplitter->restoreState(Settings->value("splitter").toByteArray());
	} else {
		// save settings

		// filterColumn
		Settings->setValue("filterColumn", ui->filterLineEdit->currentFilter());

		// state of splitter
		Settings->setValue("splitter", ui->mainSplitter->saveState());
		
		//save expanding
		Settings->setValue("ExpandAll", allItem->isExpanded());
		Settings->setValue("ExpandContacts", contactsItem->isExpanded());
		Settings->setValue("ExpandOwn", ownItem->isExpanded());
	}

	Settings->endGroup();
}

void IdDialog::filterChanged(const QString& /*text*/)
{
	filterIds();
}

void IdDialog::filterToggled(const bool &value)
{
	if (value) {
		QAction *source = qobject_cast<QAction *>(QObject::sender());
		if (source) {
			filter = source->data().toInt();
			updateIdList();
		}
	}
}

void IdDialog::updateSelection()
{
	QTreeWidgetItem *item = ui->idTreeWidget->currentItem();
	RsGxsGroupId id;

	if (item) {
		id = RsGxsGroupId(item->text(RSID_COL_KEYID).toStdString());
	}

	if (id != mId) {
		mId = id;
		updateIdentity();
		//updateRepList();
	}
}



void IdDialog::updateIdList()
{
	//Disable by default, will be enable by insertIdDetails()
	ui->removeIdentity->setEnabled(false);
	ui->editIdentity->setEnabled(false);

	//int accept = filter;

 	RsThread::async([this]()
	{
        // 1 - get message data from p3GxsForums

#ifdef DEBUG_FORUMS
        std::cerr << "Retrieving post data for post " << mThreadId << std::endl;
#endif

        std::list<RsGroupMetaData> identity_metas ;

		if (!rsIdentity->getIdentitiesSummaries(identity_metas))
		{
			std::cerr << "IdDialog::insertIdList() Error getting GroupData" << std::endl;
			return;
		}

        std::set<RsGxsId> ids;
        for(auto it(identity_metas.begin());it!=identity_metas.end();++it)
            ids.insert(RsGxsId((*it).mGroupId));

        std::vector<RsGxsIdGroup> groups;

        if(!rsIdentity->getIdentitiesInfo(ids,groups))
		{
			std::cerr << "IdDialog::insertIdList() Error getting identities info" << std::endl;
			return;
		}

		auto ids_set = std::make_unique<std::map<RsGxsGroupId,RsGxsIdGroup>>();
		for(auto it(groups.begin()); it!=groups.end(); ++it)
			(*ids_set)[(*it).mMeta.mGroupId] = *it;

		RsQThreadUtils::postToObject(
		            [ids_set = std::move(ids_set), this] ()
		{
			/* Here it goes any code you want to be executed on the Qt Gui
			 * thread, for example to update the data model with new information
			 * after a blocking call to RetroShare API complete */
			loadIdentities(*ids_set);
		}, this );

    });
}

bool IdDialog::fillIdListItem(const RsGxsIdGroup& data, QTreeWidgetItem *&item, const RsPgpId &ownPgpId, int accept)
{
	bool isLinkedToOwnNode = (data.mPgpKnown && (data.mPgpId == ownPgpId)) ;
	bool isOwnId = (data.mMeta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_ADMIN);
	RsIdentityDetails idd ;
	rsIdentity->getIdDetails(RsGxsId(data.mMeta.mGroupId),idd) ;

	bool isBanned = idd.mReputation.mOverallReputationLevel == RsReputationLevel::LOCALLY_NEGATIVE;
	uint32_t item_flags = 0;

	/* do filtering */
	bool ok = false;
	if (data.mMeta.mGroupFlags & RSGXSID_GROUPFLAG_REALID_kept_for_compatibility)
    {
        if (isLinkedToOwnNode && (accept & RSID_FILTER_YOURSELF))
        {
            ok = true;
            item_flags |= RSID_FILTER_YOURSELF ;
        }

        if (data.mPgpKnown && (accept & RSID_FILTER_FRIENDS))
        {
            ok = true;
            item_flags |= RSID_FILTER_FRIENDS ;
        }

        if (accept & RSID_FILTER_OTHERS)
        {
            ok = true;
            item_flags |= RSID_FILTER_OTHERS ;
        }
    }
    else if (accept & RSID_FILTER_PSEUDONYMS)
    {
            ok = true;
            item_flags |= RSID_FILTER_PSEUDONYMS ;
    }

    if (isOwnId && (accept & RSID_FILTER_OWNED_BY_YOU))
    {
        ok = true;
            item_flags |= RSID_FILTER_OWNED_BY_YOU ;
    }

	if (isBanned && (accept & RSID_FILTER_BANNED))
	{
		ok = true;
		item_flags |= RSID_FILTER_BANNED ;
	}

	if (!ok)
		return false;

	if (!item)
    {
        item = new TreeWidgetItem();
    }
        

	item->setText(RSID_COL_NICKNAME, QString::fromUtf8(data.mMeta.mGroupName.c_str()).left(RSID_MAXIMUM_NICKNAME_SIZE));
	item->setData(RSID_COL_NICKNAME, Qt::UserRole, QString::fromStdString(data.mMeta.mGroupId.toStdString()));
	item->setText(RSID_COL_KEYID, QString::fromStdString(data.mMeta.mGroupId.toStdString()));

	if(isBanned)
	{
		//TODO (Phenom): Add qproperty for these text colors in stylesheets
		item->setData(RSID_COL_NICKNAME, Qt::ForegroundRole, QColor(Qt::red));
		item->setData(RSID_COL_KEYID   , Qt::ForegroundRole, QColor(Qt::red));
		item->setData(RSID_COL_IDTYPE  , Qt::ForegroundRole, QColor(Qt::red));
		item->setData(RSID_COL_VOTES   , Qt::ForegroundRole, QColor(Qt::red));
	}
	else
	{
		item->setData(RSID_COL_NICKNAME, Qt::ForegroundRole, QVariant());
		item->setData(RSID_COL_KEYID   , Qt::ForegroundRole, QVariant());
		item->setData(RSID_COL_IDTYPE  , Qt::ForegroundRole, QVariant());
		item->setData(RSID_COL_VOTES   , Qt::ForegroundRole, QVariant());
	}

	item->setData(RSID_COL_KEYID, Qt::UserRole,QVariant(item_flags)) ;
	item->setTextAlignment(RSID_COL_VOTES, Qt::AlignRight | Qt::AlignVCenter);
	item->setData(
	            RSID_COL_VOTES,Qt::DecorationRole,
	            static_cast<uint32_t>(idd.mReputation.mOverallReputationLevel));
	item->setData(
	            RSID_COL_VOTES,SortRole,
	            static_cast<uint32_t>(idd.mReputation.mOverallReputationLevel));

    if(isOwnId)
    {
	    QFont font = item->font(RSID_COL_NICKNAME) ;

	    font.setBold(true) ;
	    item->setFont(RSID_COL_NICKNAME,font) ;
	    item->setFont(RSID_COL_IDTYPE,font) ;
	    item->setFont(RSID_COL_KEYID,font) ;

	    QString tooltip = tr("This identity is owned by you");

		if(idd.mFlags & RS_IDENTITY_FLAGS_IS_DEPRECATED)
		{
			//TODO (Phenom): Add qproperty for these text colors in stylesheets
			item->setData(RSID_COL_NICKNAME, Qt::ForegroundRole, QColor(Qt::red));
			item->setData(RSID_COL_KEYID   , Qt::ForegroundRole, QColor(Qt::red));
			item->setData(RSID_COL_IDTYPE  , Qt::ForegroundRole, QColor(Qt::red));

			tooltip += tr("\nThis identity has a unsecure fingerprint (It's probably quite old).\nYou should get rid of it now and use a new one.\nThese identities will soon be not supported anymore.") ;
		}

	    item->setToolTip(RSID_COL_NICKNAME, tooltip) ;
	    item->setToolTip(RSID_COL_KEYID, tooltip) ;
	    item->setToolTip(RSID_COL_IDTYPE, tooltip) ;
    }

	//QPixmap pixmap ;
	//
	//if(data.mImage.mSize == 0 || !GxsIdDetails::loadPixmapFromData(data.mImage.mData, data.mImage.mSize, pixmap,GxsIdDetails::SMALL))
	//    pixmap = GxsIdDetails::makeDefaultIcon(RsGxsId(data.mMeta.mGroupId),GxsIdDetails::SMALL) ;
	//
	//item->setIcon(RSID_COL_NICKNAME, QIcon(pixmap));
	// Icon Place Holder
	item->setIcon(RSID_COL_NICKNAME,FilesDefs::getIconFromQtResourcePath(":/icons/png/anonymous.png"));

	QString tooltip;

	if (data.mMeta.mGroupFlags & RSGXSID_GROUPFLAG_REALID_kept_for_compatibility)
	{
		if (data.mPgpKnown)
		{
			RsPeerDetails details;
			rsPeers->getGPGDetails(data.mPgpId, details);
			item->setText(RSID_COL_IDTYPE, QString::fromUtf8(details.name.c_str()));
			item->setToolTip(RSID_COL_IDTYPE,"Verified signature from node "+QString::fromStdString(data.mPgpId.toStdString())) ;
			
			
			tooltip += tr("Node name:")+" " + QString::fromUtf8(details.name.c_str()) + "\n";
			tooltip += tr("Node Id  :")+" " + QString::fromStdString(data.mPgpId.toStdString()) ;
			item->setToolTip(RSID_COL_KEYID,tooltip) ;
		}
		else 
		{
            		QString txt =  tr("[Unknown node]");
			item->setText(RSID_COL_IDTYPE, txt);
            
            		if(!data.mPgpId.isNull())
                    {
			item->setToolTip(RSID_COL_IDTYPE,tr("Unverified signature from node ")+QString::fromStdString(data.mPgpId.toStdString())) ;
			item->setToolTip(RSID_COL_KEYID,tr("Unverified signature from node ")+QString::fromStdString(data.mPgpId.toStdString())) ;
                    }
                    else
                    {
			item->setToolTip(RSID_COL_IDTYPE,tr("Unchecked signature")) ;
			item->setToolTip(RSID_COL_KEYID,tr("Unchecked signature")) ;
                    }
		}
	}
	else
	{
		item->setText(RSID_COL_IDTYPE, QString()) ;
		item->setToolTip(RSID_COL_IDTYPE,QString()) ;
	}

	return true;
}

void IdDialog::loadIdentities(const std::map<RsGxsGroupId,RsGxsIdGroup>& ids_set_const)
{
    auto ids_set(ids_set_const);

	//First: Get current item to restore after
	RsGxsGroupId oldCurrentId = mIdToNavigate;
	{
		QTreeWidgetItem *oldCurrent = ui->idTreeWidget->currentItem();
		if (oldCurrent) {
			oldCurrentId = RsGxsGroupId(oldCurrent->text(RSID_COL_KEYID).toStdString());
		}
	}
    int accept = filter;

	mStateHelper->setActive(IDDIALOG_IDLIST, true);

	RsPgpId ownPgpId  = rsPeers->getGPGOwnId();

	// Update existing and remove not existing items 
	// Also remove items that do not have the correct parent
    	
	QTreeWidgetItemIterator itemIterator(ui->idTreeWidget);
	QTreeWidgetItem *item = NULL;
    
	while ((item = *itemIterator) != NULL) 
	{
		++itemIterator;
		auto it = ids_set.find(RsGxsGroupId(item->text(RSID_COL_KEYID).toStdString())) ;

		if(it == ids_set.end())
		{
			if(item != allItem && item != contactsItem && item != ownItem)
				delete(item);

			continue ;
		}

		QTreeWidgetItem *parent_item = item->parent() ;

//        if(it->second.mMeta.mPublishTs > time(NULL) - 20 || it->second.mMeta.mGroupId == RsGxsGroupId("3de2172503675206b3a23c997e5ee688"))
//            std::cerr << "Captured ID " <<it->second.mMeta.mGroupId << std::endl;

		if(    (parent_item == allItem && it->second.mIsAContact) || (parent_item == contactsItem && !it->second.mIsAContact))
		{
			delete item ;	// do not remove from the list, so that it is added again in the correct place.
			continue ;
		}

		if (!fillIdListItem(it->second, item, ownPgpId, accept))
			delete(item);

		ids_set.erase(it);	// erase, so it is not considered to be a new item
	}

	/* Insert new items */
	for (std::map<RsGxsGroupId,RsGxsIdGroup>::const_iterator vit = ids_set.begin(); vit != ids_set.end(); ++vit)
	{
		RsGxsIdGroup data = vit->second ;

	    item = NULL;

	    ui->idTreeWidget->insertTopLevelItem(0, ownItem);
	    ui->idTreeWidget->insertTopLevelItem(0, allItem);
	    ui->idTreeWidget->insertTopLevelItem(0, contactsItem );  

	    Settings->beginGroup("IdDialog");
	    allItem->setExpanded(Settings->value("ExpandAll", QVariant(true)).toBool());
	    ownItem->setExpanded(Settings->value("ExpandOwn", QVariant(true)).toBool());
	    contactsItem->setExpanded(Settings->value("ExpandContacts", QVariant(true)).toBool());

	    Settings->endGroup();

		if (fillIdListItem(data, item, ownPgpId, accept))
		{
			if(data.mMeta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_ADMIN)
				ownItem->addChild(item);
			else if(data.mIsAContact)
				contactsItem->addChild(item);
			else
				allItem->addChild(item);

		}
	}
	
	/* count items */
	int itemCount = contactsItem->childCount() + allItem->childCount() + ownItem->childCount();
	ui->label_count->setText( "(" + QString::number( itemCount ) + ")" );
	
	int contactsCount = contactsItem->childCount() ;
	int allCount = allItem->childCount() ;
	int ownCount = ownItem->childCount();

	contactsItem->setText(0, tr("My contacts") + " (" + QString::number( contactsCount ) + ")" );
	allItem->setText(0, tr("All") + " (" + QString::number( allCount ) + ")" );
	ownItem->setText(0, tr("My own identities") + " (" +  QString::number( ownCount ) + ")" );

	navigate(RsGxsId(oldCurrentId));
	filterIds();
	updateSelection();
}

void IdDialog::updateIdentity()
{
	if (mId.isNull())
	{
		mStateHelper->setActive(IDDIALOG_IDDETAILS, false);
		mStateHelper->setLoading(IDDIALOG_IDDETAILS, false);
		mStateHelper->clear(IDDIALOG_IDDETAILS);
		clearPerson();

		return;
	}

	mStateHelper->setLoading(IDDIALOG_IDDETAILS, true);

	RsThread::async([this]()
	{
#ifdef ID_DEBUG
        std::cerr << "Retrieving post data for identity " << mThreadId << std::endl;
#endif

        std::set<RsGxsId> ids( { RsGxsId(mId) } ) ;
        std::vector<RsGxsIdGroup> ids_data;

		if(!rsIdentity->getIdentitiesInfo(ids,ids_data))
		{
			std::cerr << __PRETTY_FUNCTION__ << " failed to retrieve identities group info for id " << mId << std::endl;
			return;
        }

        if(ids_data.size() != 1)
		{
			std::cerr << __PRETTY_FUNCTION__ << " failed to retrieve exactly one group info for id " << mId << std::endl;
			return;
        }
        RsGxsIdGroup group(ids_data[0]);

        RsQThreadUtils::postToObject( [group,this]()
		{
			/* Here it goes any code you want to be executed on the Qt Gui
			 * thread, for example to update the data model with new information
			 * after a blocking call to RetroShare API complete */

            loadIdentity(group);

		}, this );
	});
}

void IdDialog::loadIdentity(RsGxsIdGroup data)
{
	mStateHelper->setLoading(IDDIALOG_IDDETAILS, false);

	/* get details from libretroshare */

	mStateHelper->setActive(IDDIALOG_IDDETAILS, true);

	/* get GPG Details from rsPeers */
	RsPgpId ownPgpId  = rsPeers->getGPGOwnId();

    ui->lineEdit_PublishTS->setText(QDateTime::fromMSecsSinceEpoch(qint64(1000)*data.mMeta.mPublishTs).toString(Qt::SystemLocaleShortDate));
    ui->lineEdit_Nickname->setText(QString::fromUtf8(data.mMeta.mGroupName.c_str()).left(RSID_MAXIMUM_NICKNAME_SIZE));
	ui->lineEdit_KeyId->setText(QString::fromStdString(data.mMeta.mGroupId.toStdString()));
	//ui->lineEdit_GpgHash->setText(QString::fromStdString(data.mPgpIdHash.toStdString()));
    if(data.mPgpKnown)
	    ui->lineEdit_GpgId->setText(QString::fromStdString(data.mPgpId.toStdString()));
    else
	    ui->lineEdit_GpgId->setText(QString::fromStdString(data.mPgpId.toStdString()) + tr(" [unverified]"));

    ui->autoBanIdentities_CB->setVisible(!data.mPgpId.isNull()) ;
    ui->banoption_label->setVisible(!data.mPgpId.isNull()) ;

    time_t now = time(NULL) ;
    ui->lineEdit_LastUsed->setText(getHumanReadableDuration(now - data.mLastUsageTS)) ;
    ui->headerTextLabel_Person->setText(QString::fromUtf8(data.mMeta.mGroupName.c_str()).left(RSID_MAXIMUM_NICKNAME_SIZE));

    QPixmap pixmap ;

    if(data.mImage.mSize == 0 || !GxsIdDetails::loadPixmapFromData(data.mImage.mData, data.mImage.mSize, pixmap,GxsIdDetails::LARGE))
        pixmap = GxsIdDetails::makeDefaultIcon(RsGxsId(data.mMeta.mGroupId),GxsIdDetails::LARGE) ;

#ifdef ID_DEBUG
	std::cerr << "Setting header frame image : " << pixmap.width() << " x " << pixmap.height() << std::endl;
#endif

    //ui->avLabel_Person->setPixmap(pixmap);
    //ui->avatarLabel->setPixmap(pixmap);
	QFontMetricsF f(ui->avLabel_Person->font()) ;
    ui->avLabel_Person->setPixmap(pixmap.scaled(f.height()*4,f.height()*4,Qt::KeepAspectRatio,Qt::SmoothTransformation));

	QFontMetricsF g(ui->inviteButton->font()) ;
    ui->avatarLabel->setPixmap(pixmap.scaled(ui->inviteButton->width(),ui->inviteButton->width(),Qt::IgnoreAspectRatio,Qt::SmoothTransformation));
	ui->avatarLabel->setScaledContents(true);

	if (data.mPgpKnown)
	{
		RsPeerDetails details;
		rsPeers->getGPGDetails(data.mPgpId, details);
		ui->lineEdit_GpgName->setText(QString::fromUtf8(details.name.c_str()));
	}
	else
	{
		if (data.mMeta.mGroupFlags & RSGXSID_GROUPFLAG_REALID_kept_for_compatibility)
			ui->lineEdit_GpgName->setText(tr("[Unknown node]"));
		else
			ui->lineEdit_GpgName->setText(tr("Anonymous Id"));
	}

	if(data.mPgpId.isNull())
	{
		ui->lineEdit_GpgId->hide() ;
		ui->label_GpgId->hide() ;
	}
	else
	{
		ui->lineEdit_GpgId->show() ;
		ui->label_GpgId->show() ;
	}
    
    if(data.mPgpKnown)
    {
		ui->lineEdit_GpgName->show() ;
		ui->label_GpgName->show() ;
    }
    else
    {
		ui->lineEdit_GpgName->hide() ;
		ui->label_GpgName->hide() ;
    }

    bool isLinkedToOwnPgpId = (data.mPgpKnown && (data.mPgpId == ownPgpId)) ;
    bool isOwnId = (data.mMeta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_ADMIN);

    if(isOwnId)
        if (isLinkedToOwnPgpId)
            ui->lineEdit_Type->setText(tr("Identity owned by you, linked to your Retroshare node")) ;
        else
            if (data.mMeta.mGroupFlags & (GXS_SERV::FLAG_PRIVACY_PRIVATE | RSGXSID_GROUPFLAG_REALID))
                ui->lineEdit_Type->setText(tr("Identity owned by you, linked to your Retroshare node but not yet validated")) ;
            else
                ui->lineEdit_Type->setText(tr("Anonymous identity, owned by you")) ;
    else if (data.mMeta.mGroupFlags & RSGXSID_GROUPFLAG_REALID_kept_for_compatibility)
    {
        if (data.mPgpKnown)
            if (rsPeers->isGPGAccepted(data.mPgpId))
                ui->lineEdit_Type->setText(tr("Linked to a friend Retroshare node")) ;
            else
                ui->lineEdit_Type->setText(tr("Linked to a known Retroshare node")) ;
        else
            ui->lineEdit_Type->setText(tr("Linked to unknown Retroshare node")) ;
    }
    else
    {
        ui->lineEdit_Type->setText(tr("Anonymous identity")) ;
    }

	if (isOwnId)
	{
		mStateHelper->setWidgetEnabled(ui->ownOpinion_CB, false);
		mStateHelper->setWidgetEnabled(ui->autoBanIdentities_CB, false);
		ui->editIdentity->setEnabled(true);
		ui->removeIdentity->setEnabled(true);
		ui->chatIdentity->setEnabled(false);
		ui->inviteButton->setEnabled(false);
	}
	else
	{
		// No Reputation yet!
		mStateHelper->setWidgetEnabled(ui->ownOpinion_CB, true);
		mStateHelper->setWidgetEnabled(ui->autoBanIdentities_CB, true);
		ui->editIdentity->setEnabled(false);
		ui->removeIdentity->setEnabled(false);
		ui->chatIdentity->setEnabled(true);
		ui->inviteButton->setEnabled(true);
	}

    ui->autoBanIdentities_CB->setChecked(rsReputations->isNodeBanned(data.mPgpId));

	/* now fill in the reputation information */

#ifdef SUSPENDED
	if (data.mPgpKnown)
	{
		ui->line_RatingImplicit->setText(tr("+50 Known PGP"));
	}
	else if (data.mMeta.mGroupFlags & RSGXSID_GROUPFLAG_REALID)
	{
		ui->line_RatingImplicit->setText(tr("+10 UnKnown PGP"));
	}
	else
	{
		ui->line_RatingImplicit->setText(tr("+5 Anon Id"));
	}
	{
		QString rating = QString::number(data.mReputation.mIdScore);
		ui->line_RatingImplicit->setText(rating);
	}

#endif

	RsReputationInfo info;
    rsReputations->getReputationInfo(RsGxsId(data.mMeta.mGroupId),data.mPgpId,info) ;

    QString frep_string ;
    if(info.mFriendsPositiveVotes > 0) frep_string += QString::number(info.mFriendsPositiveVotes) + tr(" positive ") ;
    if(info.mFriendsNegativeVotes > 0) frep_string += QString::number(info.mFriendsNegativeVotes) + tr(" negative ") ;

    if(info.mFriendsPositiveVotes==0 && info.mFriendsNegativeVotes==0)
        frep_string = tr("No votes from friends") ;

    ui->neighborNodesOpinion_TF->setText(frep_string) ;
    
    ui->label_positive->setText(QString::number(info.mFriendsPositiveVotes));
    ui->label_negative->setText(QString::number(info.mFriendsNegativeVotes));

	switch(info.mOverallReputationLevel)
	{
	case RsReputationLevel::LOCALLY_POSITIVE:
		ui->overallOpinion_TF->setText(tr("Positive")); break;
	case RsReputationLevel::LOCALLY_NEGATIVE:
		ui->overallOpinion_TF->setText(tr("Negative (Banned by you)")); break;
	case RsReputationLevel::REMOTELY_POSITIVE:
		ui->overallOpinion_TF->setText(tr("Positive (according to your friends)"));
		break;
	case RsReputationLevel::REMOTELY_NEGATIVE:
		ui->overallOpinion_TF->setText(tr("Negative (according to your friends)"));
		break;
	case RsReputationLevel::NEUTRAL: // fallthrough
	default:
		ui->overallOpinion_TF->setText(tr("Neutral")) ; break ;
	}

	switch(info.mOwnOpinion)
	{
	case RsOpinion::NEGATIVE: ui->ownOpinion_CB->setCurrentIndex(0); break;
	case RsOpinion::NEUTRAL : ui->ownOpinion_CB->setCurrentIndex(1); break;
	case RsOpinion::POSITIVE: ui->ownOpinion_CB->setCurrentIndex(2); break;
	default:
		std::cerr << "Unexpected value in own opinion: "
		          << static_cast<uint32_t>(info.mOwnOpinion) << std::endl;
		break;
	}

    // now fill in usage cases

	RsIdentityDetails det ;
	rsIdentity->getIdDetails(RsGxsId(data.mMeta.mGroupId),det) ;

    QString usage_txt ;
	std::map<rstime_t,RsIdentityUsage> rmap;
	for(auto it(det.mUseCases.begin()); it!=det.mUseCases.end(); ++it)
		rmap.insert(std::make_pair(it->second,it->first));

	for(auto it(rmap.begin()); it!=rmap.end(); ++it)
        usage_txt += QString("<b>")+ getHumanReadableDuration(now - data.mLastUsageTS) + "</b> \t: " + createUsageString(it->second) + "<br/>" ;

    if(usage_txt.isNull())
        usage_txt = tr("<b>[No record in current session]</b>") ;

    ui->usageStatistics_TB->setText(usage_txt) ;
}

QString IdDialog::createUsageString(const RsIdentityUsage& u) const
{
    QString service_name;
    RetroShareLink::enumType service_type = RetroShareLink::TYPE_UNKNOWN;

    switch(u.mServiceId)
	{
	case RsServiceType::CHANNELS:  service_name = tr("Channels") ;service_type = RetroShareLink::TYPE_CHANNEL   ; break ;
	case RsServiceType::FORUMS:    service_name = tr("Forums") ;  service_type = RetroShareLink::TYPE_FORUM     ; break ;
	case RsServiceType::POSTED:    service_name = tr("Boards") ;  service_type = RetroShareLink::TYPE_POSTED    ; break ;
	case RsServiceType::CHAT:      service_name = tr("Chat")   ;  service_type = RetroShareLink::TYPE_CHAT_ROOM ; break ;

	case RsServiceType::GXS_TRANS: return tr("GxsMail author ");
#ifdef TODO
    // We need a RS link for circles if we want to do that.
    //
	case RsServiceType::GXSCIRCLE: service_name = tr("GxsCircles");  service_type = RetroShareLink::TYPE_CIRCLES; break ;
#endif
    default:
        service_name = tr("Unknown (service=")+QString::number((int)u.mServiceId,16)+")"; service_type = RetroShareLink::TYPE_UNKNOWN ;
    }

    switch(u.mUsageCode)
    {
	case RsIdentityUsage::UNKNOWN_USAGE:
        	return tr("[Unknown]") ;
	case RsIdentityUsage::GROUP_ADMIN_SIGNATURE_CREATION:       // These 2 are normally not normal GXS identities, but nothing prevents it to happen either.
        	return tr("Admin signature in service %1").arg(service_name);
    case RsIdentityUsage::GROUP_ADMIN_SIGNATURE_VALIDATION:
        	return tr("Admin signature verification in service %1").arg(service_name);
    case RsIdentityUsage::GROUP_AUTHOR_SIGNATURE_CREATION:      // not typically used, since most services do not require group author signatures
        	return tr("Creation of author signature in service %1").arg(service_name);
    case RsIdentityUsage::MESSAGE_AUTHOR_SIGNATURE_CREATION:    // most common use case. Messages are signed by authors in e.g. forums.
        	return tr("Message signature creation in group %1 of service %2").arg(QString::fromStdString(u.mGrpId.toStdString())).arg(service_name);
    case RsIdentityUsage::GROUP_AUTHOR_KEEP_ALIVE:               // Identities are stamped regularly by crawlign the set of messages for all groups. That helps keepign the useful identities in hand.
    case RsIdentityUsage::GROUP_AUTHOR_SIGNATURE_VALIDATION:
        	return tr("Group author for group %1 in service %2").arg(QString::fromStdString(u.mGrpId.toStdString())).arg(service_name);
        break ;
    case RsIdentityUsage::MESSAGE_AUTHOR_SIGNATURE_VALIDATION:
    case RsIdentityUsage::MESSAGE_AUTHOR_KEEP_ALIVE:             // Identities are stamped regularly by crawling the set of messages for all groups. That helps keepign the useful identities in hand.
	{
        RetroShareLink l;

        std::cerr << "Signature validation/keep alive signature:" << std::endl;
        std::cerr << "   service ID  = " << std::hex << (uint16_t)u.mServiceId << std::dec << std::endl;
        std::cerr << "   u.mGrpId    = " << u.mGrpId << std::endl;
        std::cerr << "   u.mMsgId    = " << u.mMsgId << std::endl;
        std::cerr << "   u.mParentId = " << u.mParentId << std::endl;
        std::cerr << "   u.mThreadId = " << u.mThreadId << std::endl;

		if(service_type == RetroShareLink::TYPE_CHANNEL && !u.mThreadId.isNull())
			l = RetroShareLink::createGxsMessageLink(service_type,u.mGrpId,u.mThreadId,tr("Vote/comment"));
		else if(service_type == RetroShareLink::TYPE_POSTED && !u.mThreadId.isNull())
			l = RetroShareLink::createGxsMessageLink(service_type,u.mGrpId,u.mThreadId,tr("Vote"));
		else
			l = RetroShareLink::createGxsMessageLink(service_type,u.mGrpId,u.mMsgId,tr("Message"));

		return tr("%1 in %2 service").arg(l.toHtml()).arg(service_name) ;
	}
    case RsIdentityUsage::CHAT_LOBBY_MSG_VALIDATION:             // Chat lobby msgs are signed, so each time one comes, or a chat lobby event comes, a signature verificaiton happens.
    {
		ChatId id = ChatId(ChatLobbyId(u.mAdditionalId));
		ChatLobbyInfo linfo ;
		rsMsgs->getChatLobbyInfo(ChatLobbyId(u.mAdditionalId),linfo);
		RetroShareLink l = RetroShareLink::createChatRoom(id, QString::fromUtf8(linfo.lobby_name.c_str()));
		return tr("Message in chat room %1").arg(l.toHtml()) ;
    }
    case RsIdentityUsage::GLOBAL_ROUTER_SIGNATURE_CHECK:         // Global router message validation
    {
		return tr("Distant message signature validation.");
    }
    case RsIdentityUsage::GLOBAL_ROUTER_SIGNATURE_CREATION:      // Global router message signature
    {
		return tr("Distant message signature creation.");
    }
    case RsIdentityUsage::GXS_TUNNEL_DH_SIGNATURE_CHECK:         //
    {
		return tr("Signature validation in distant tunnel system.");
    }
    case RsIdentityUsage::GXS_TUNNEL_DH_SIGNATURE_CREATION:      //
    {
		return tr("Signature in distant tunnel system.");
    }
    case RsIdentityUsage::IDENTITY_NEW_FROM_GXS_SYNC:            // Group update on that identity data. Can be avatar, name, etc.
    {
		return tr("Received from GXS sync.");
    }
    case RsIdentityUsage::IDENTITY_NEW_FROM_DISCOVERY:           // Own friend sended his own ids
    {
		return tr("Friend node identity received through discovery.");
    }
    case RsIdentityUsage::IDENTITY_GENERIC_SIGNATURE_CHECK:      // Any signature verified for that identity
    {
		return tr("Generic signature validation.");
    }
    case RsIdentityUsage::IDENTITY_GENERIC_SIGNATURE_CREATION:   // Any signature made by that identity
    {
		return tr("Generic signature creation (e.g. chat room message, global router,...).");
    }
	case RsIdentityUsage::IDENTITY_GENERIC_ENCRYPTION: return tr("Generic encryption.");
	case RsIdentityUsage::IDENTITY_GENERIC_DECRYPTION: return tr("Generic decryption.");
	case RsIdentityUsage::CIRCLE_MEMBERSHIP_CHECK:
    {
        RsGxsCircleDetails det;
        if(rsGxsCircles->getCircleDetails(RsGxsCircleId(u.mGrpId),det))
			return tr("Membership verification in circle \"%1\" (%2).").arg(QString::fromUtf8(det.mCircleName.c_str())).arg(QString::fromStdString(u.mGrpId.toStdString()));
        else
			return tr("Membership verification in circle (ID=%1).").arg(QString::fromStdString(u.mGrpId.toStdString()));
    }

#warning TODO! csoler 2017-01-03: Add the different strings and translations here.
	default:
		return QString("Undone yet");
    }
    return QString("Unknown");
}

void IdDialog::modifyReputation()
{
#ifdef ID_DEBUG
	std::cerr << "IdDialog::modifyReputation()";
	std::cerr << std::endl;
#endif

	RsGxsId id(ui->lineEdit_KeyId->text().toStdString());

	RsOpinion op;

	switch(ui->ownOpinion_CB->currentIndex())
	{
	case 0: op = RsOpinion::NEGATIVE; break;
	case 1: op = RsOpinion::NEUTRAL ; break;
	case 2: op = RsOpinion::POSITIVE; break;
	default:
		std::cerr << "Wrong value from opinion combobox. Bug??" << std::endl;
		return;
	}
	rsReputations->setOwnOpinion(id,op);

#ifdef ID_DEBUG
	std::cerr << "IdDialog::modifyReputation() ID: " << id << " Mod: " << static_cast<int>(op);
	std::cerr << std::endl;
#endif

	// trigger refresh when finished.
	// basic / anstype are not needed.
    updateIdentity();
    updateIdList();

	return;
}
	
void IdDialog::navigate(const RsGxsId& gxs_id)
{
#ifdef ID_DEBUG
	std::cerr << "IdDialog::navigate to " << gxs_id.toStdString() << std::endl;
#endif

	// in order to do this, we just select the correct ID in the ID list
	if (!gxs_id.isNull())
	{
		QList<QTreeWidgetItem*> select = ui->idTreeWidget->findItems(QString::fromStdString(gxs_id.toStdString()),Qt::MatchExactly | Qt::MatchRecursive | Qt::MatchWrap,RSID_COL_KEYID) ;

		if(select.empty())
		{
			mIdToNavigate = RsGxsGroupId(gxs_id);
			std::cerr << "Cannot find item with ID " << gxs_id << " in ID list." << std::endl;
			return;
		}
		ui->idTreeWidget->setCurrentItem(*select.begin(),true);
	}

	mIdToNavigate = RsGxsGroupId();
}

void IdDialog::updateDisplay(bool complete)
{
	/* Update identity list */

	if (complete) {
		/* Fill complete */
		updateIdList();
		//requestIdDetails();
		//requestRepList();

		updateCircles();
		return;
	}
}

void IdDialog::addIdentity()
{
	IdEditDialog dlg(this);
	dlg.setupNewId(false);
	dlg.exec();
}

void IdDialog::removeIdentity()
{
	QTreeWidgetItem *item = ui->idTreeWidget->currentItem();
	if (!item)
	{
#ifdef ID_DEBUG
		std::cerr << "IdDialog::editIdentity() Invalid item";
		std::cerr << std::endl;
#endif
		return;
	}

	if ((QMessageBox::question(this, tr("Really delete?"), tr("Do you really want to delete this identity?"), QMessageBox::Yes|QMessageBox::No, QMessageBox::No))== QMessageBox::Yes)
	{
		std::string keyId = item->text(RSID_COL_KEYID).toStdString();

		uint32_t dummyToken = 0;
		RsGxsIdGroup group;
		group.mMeta.mGroupId=RsGxsGroupId(keyId);
		rsIdentity->deleteIdentity(dummyToken, group);
	}
}

void IdDialog::editIdentity()
{
	QTreeWidgetItem *item = ui->idTreeWidget->currentItem();
	if (!item)
	{
#ifdef ID_DEBUG
		std::cerr << "IdDialog::editIdentity() Invalid item";
		std::cerr << std::endl;
#endif
		return;
	}

	RsGxsGroupId keyId = RsGxsGroupId(item->text(RSID_COL_KEYID).toStdString());
	if (keyId.isNull()) {
		return;
	}

	IdEditDialog dlg(this);
	dlg.setupExistingId(keyId);
	dlg.exec();
}

void IdDialog::filterIds()
{
	int filterColumn = ui->filterLineEdit->currentFilter();
	QString text = ui->filterLineEdit->text();

	ui->idTreeWidget->filterItems(filterColumn, text);
}

void IdDialog::IdListCustomPopupMenu( QPoint )
{
	QMenu *contextMenu = new QMenu(this);


	std::list<RsGxsId> own_identities;
	rsIdentity->getOwnIds(own_identities);

	// make some stats about what's selected. If the same value is used for all selected items, it can be switched.

	QList<QTreeWidgetItem *> selected_items = ui->idTreeWidget->selectedItems();

	bool root_node_present = false ;
	bool one_item_owned_by_you = false ;
	uint32_t n_positive_reputations = 0 ;
	uint32_t n_negative_reputations = 0 ;
	uint32_t n_neutral_reputations = 0 ;
	uint32_t n_is_a_contact = 0 ;
	uint32_t n_is_not_a_contact = 0 ;
	uint32_t n_selected_items =0 ;

	for(QList<QTreeWidgetItem*>::const_iterator it(selected_items.begin());it!=selected_items.end();++it)
	{
		if(*it == allItem || *it == contactsItem || *it == ownItem)
		{
			root_node_present = true ;
			continue ;
		}

		uint32_t item_flags = (*it)->data(RSID_COL_KEYID,Qt::UserRole).toUInt() ;

		if(item_flags & RSID_FILTER_OWNED_BY_YOU)
			one_item_owned_by_you = true ;

#ifdef ID_DEBUG
		std::cerr << "  item flags = " << item_flags << std::endl;
#endif
		RsGxsId keyId((*it)->text(RSID_COL_KEYID).toStdString());

		RsIdentityDetails det ;
		rsIdentity->getIdDetails(keyId,det) ;

		switch(det.mReputation.mOwnOpinion)
		{
		case RsOpinion::NEGATIVE: ++n_negative_reputations; break;
		case RsOpinion::POSITIVE: ++n_positive_reputations; break;
		case RsOpinion::NEUTRAL:  ++n_neutral_reputations;  break;
		}

		++n_selected_items;

		if(rsIdentity->isARegularContact(keyId))
			++n_is_a_contact ;
		else
			++n_is_not_a_contact ;
	}

	if(!root_node_present)	// don't show menu if some of the root nodes are present
	{

		if(!one_item_owned_by_you)
		{
			QWidget *widget = new QWidget(contextMenu);
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

			QLabel *textLabel = new QLabel("<strong>" + ui->titleBarLabel->text() + "</strong>", widget);
			hbox->addWidget(textLabel);

			QSpacerItem *spacerItem = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
			hbox->addItem(spacerItem);

			widget->setLayout(hbox);

			QWidgetAction *widgetAction = new QWidgetAction(this);
			widgetAction->setDefaultWidget(widget);
			contextMenu->addAction(widgetAction);

			if(n_selected_items == 1)		// if only one item is selected, allow to chat with this item
			{
				if(own_identities.size() <= 1)
				{
                    QAction *action = contextMenu->addAction(FilesDefs::getIconFromQtResourcePath(":/icons/png/chats.png"), tr("Chat with this person"), this, SLOT(chatIdentity()));

					if(own_identities.empty())
						action->setEnabled(false) ;
					else
						action->setData(QString::fromStdString((own_identities.front()).toStdString())) ;
				}
				else
				{
                    QMenu *mnu = contextMenu->addMenu(FilesDefs::getIconFromQtResourcePath(":/icons/png/chats.png"),tr("Chat with this person as...")) ;

					for(std::list<RsGxsId>::const_iterator it=own_identities.begin();it!=own_identities.end();++it)
					{
						RsIdentityDetails idd ;
						rsIdentity->getIdDetails(*it,idd) ;

						QPixmap pixmap ;

						if(idd.mAvatar.mSize == 0 || !GxsIdDetails::loadPixmapFromData(idd.mAvatar.mData, idd.mAvatar.mSize, pixmap,GxsIdDetails::SMALL))
							pixmap = GxsIdDetails::makeDefaultIcon(*it,GxsIdDetails::SMALL) ;

						QAction *action = mnu->addAction(QIcon(pixmap), QString("%1 (%2)").arg(QString::fromUtf8(idd.mNickname.c_str()), QString::fromStdString((*it).toStdString())), this, SLOT(chatIdentity()));
						action->setData(QString::fromStdString((*it).toStdString())) ;
					}
				}
			}
			// always allow to send messages
            contextMenu->addAction(FilesDefs::getIconFromQtResourcePath(":/icons/mail/write-mail.png"), tr("Send message"), this, SLOT(sendMsg()));

			contextMenu->addSeparator();

			if(n_is_a_contact == 0)
				contextMenu->addAction(QIcon(), tr("Add to Contacts"), this, SLOT(addtoContacts()));

			if (n_selected_items==1)
				contextMenu->addAction(QIcon(""),tr("Copy identity to clipboard"),this,SLOT(copyRetroshareLink())) ;

			if(n_is_not_a_contact == 0)
                contextMenu->addAction(FilesDefs::getIconFromQtResourcePath(":/images/cancel.png"), tr("Remove from Contacts"), this, SLOT(removefromContacts()));

			contextMenu->addSeparator();

			if(n_positive_reputations == 0)	// only unban when all items are banned
                contextMenu->addAction(FilesDefs::getIconFromQtResourcePath(":/icons/png/thumbs-up.png"), tr("Set positive opinion"), this, SLOT(positivePerson()));

			if(n_neutral_reputations == 0)	// only unban when all items are banned
                contextMenu->addAction(FilesDefs::getIconFromQtResourcePath(":/icons/png/thumbs-neutral.png"), tr("Set neutral opinion"), this, SLOT(neutralPerson()));

			if(n_negative_reputations == 0)
                contextMenu->addAction(FilesDefs::getIconFromQtResourcePath(":/icons/png/thumbs-down.png"), tr("Set negative opinion"), this, SLOT(negativePerson()));
		}

		if(one_item_owned_by_you && n_selected_items==1)
		{
			contextMenu->addSeparator();

			contextMenu->addAction(QIcon(""),tr("Copy identity to clipboard"),this,SLOT(copyRetroshareLink())) ;
			contextMenu->addAction(ui->editIdentity);
			contextMenu->addAction(ui->removeIdentity);
		}

	}

	contextMenu = ui->idTreeWidget->createStandardContextMenu(contextMenu);

	contextMenu->exec(QCursor::pos());
	delete contextMenu;
}

void IdDialog::copyRetroshareLink()
{
	QTreeWidgetItem *item = ui->idTreeWidget->currentItem();

	if (!item)
	{
		std::cerr << "IdDialog::editIdentity() Invalid item";
		std::cerr << std::endl;
		return;
	}

	RsGxsId gxs_id(item->text(RSID_COL_KEYID).toStdString());

    if(gxs_id.isNull())
    {
        std::cerr << "Null GXS id. Something went wrong." << std::endl;
        return ;
    }

    RsIdentityDetails details ;

	if(! rsIdentity->getIdDetails(gxs_id,details))
		return ;

	RsThread::async([gxs_id,details,this]()
	{
#ifdef ID_DEBUG
        std::cerr << "Retrieving post data for identity " << mThreadId << std::endl;
#endif
        std::string radix,errMsg;

		if(!rsIdentity->exportIdentityLink( radix, gxs_id, true, std::string(), errMsg))
		{
			std::cerr << "Cannot retrieve identity data " << mId << " to create a link. Error:" << errMsg << std::endl;
            return ;
		}

        RsQThreadUtils::postToObject( [radix,details,this]()
		{
			/* Here it goes any code you want to be executed on the Qt Gui
			 * thread, for example to update the data model with new information
			 * after a blocking call to RetroShare API complete */

			QList<RetroShareLink> urls ;

			RetroShareLink link = RetroShareLink::createIdentity(details.mId,QString::fromUtf8(details.mNickname.c_str()),QString::fromStdString(radix)) ;
			urls.push_back(link);

			RSLinkClipboard::copyLinks(urls) ;

			QMessageBox::information(NULL,tr("information"),tr("This identity link was copied to your clipboard. Paste it in a mail, or a message to transmit the identity to someone.")) ;

		}, this );
	});
}

void IdDialog::chatIdentity()
{
	QTreeWidgetItem* item = ui->idTreeWidget->currentItem();
	if (!item)
	{
		std::cerr << __PRETTY_FUNCTION__ << " Error. Invalid item!" << std::endl;
		return;
	}

	chatIdentityItem(item);
}

void IdDialog::chatIdentityItem(QTreeWidgetItem* item)
{
	if(!item)
	{
		std::cerr << __PRETTY_FUNCTION__ << " Error. Invalid item." << std::endl;
		return;
	}

	std::string&& toIdString(item->text(RSID_COL_KEYID).toStdString());
	RsGxsId toGxsId(toIdString);
	if(toGxsId.isNull())
	{
		std::cerr << __PRETTY_FUNCTION__ << " Error. Invalid destination id: "
		          << toIdString << std::endl;
		return;
	}

	RsGxsId fromGxsId;
	QAction* action = qobject_cast<QAction*>(QObject::sender());
	if(!action)
	{
		std::list<RsGxsId> ownIdentities;
		rsIdentity->getOwnIds(ownIdentities);
		if(ownIdentities.empty())
		{
			std::cerr << __PRETTY_FUNCTION__ << " Error. Own identities list "
			          << " is empty!" << std::endl;
			return;
		}
		else fromGxsId = ownIdentities.front();
	}
	else fromGxsId = RsGxsId(action->data().toString().toStdString());

	if(fromGxsId.isNull())
	{
		std::cerr << __PRETTY_FUNCTION__ << " Error. Could not determine sender"
		          << " identity to open chat toward: " << toIdString << std::endl;
		return;
	}

	uint32_t error_code;
	DistantChatPeerId did;

	if(!rsMsgs->initiateDistantChatConnexion(toGxsId, fromGxsId, did, error_code))
		QMessageBox::information(
		            nullptr, tr("Distant chat cannot work"),
		            QString("%1 %2: %3")
		                .arg(tr("Distant chat refused with this person."))
		                .arg(tr("Error code")).arg(error_code) ) ;
}

void IdDialog::sendMsg()
{
    QList<QTreeWidgetItem *> selected_items = ui->idTreeWidget->selectedItems();

    if(selected_items.empty())
	    return ;

    MessageComposer *nMsgDialog = MessageComposer::newMsg();
    if (nMsgDialog == NULL)
	    return;

    for(QList<QTreeWidgetItem*>::const_iterator it(selected_items.begin());it!=selected_items.end();++it)
    {
	    QTreeWidgetItem *item = *it ;

	    std::string keyId = item->text(RSID_COL_KEYID).toStdString();

	    nMsgDialog->addRecipient(MessageComposer::TO,  RsGxsId(keyId));
    }
    nMsgDialog->show();
    nMsgDialog->activateWindow();

    /* window will destroy itself! */
}

QString IdDialog::inviteMessage()
{
    return tr("Hi,<br>I want to be friends with you on RetroShare.<br>");
}

void IdDialog::sendInvite()
{
	QTreeWidgetItem *item = ui->idTreeWidget->currentItem();
	if (!item)
	{
		return;
	}

    RsGxsId id(ui->lineEdit_KeyId->text().toStdString());
    
    //if ((QMessageBox::question(this, tr("Send invite?"),tr("Do you really want send a invite with your Certificate?"),QMessageBox::Yes|QMessageBox::No, QMessageBox::Yes))== QMessageBox::Yes)
	{
        MessageComposer::sendInvite(id,false);

        ui->inviteFrame->show();
        ui->inviteButton->setEnabled(false);
	}
    

}

void IdDialog::negativePerson()
{
    QList<QTreeWidgetItem *> selected_items = ui->idTreeWidget->selectedItems();
    for(QList<QTreeWidgetItem*>::const_iterator it(selected_items.begin());it!=selected_items.end();++it)
    {
        QTreeWidgetItem *item = *it ;
        
	std::string Id = item->text(RSID_COL_KEYID).toStdString();

	    rsReputations->setOwnOpinion(RsGxsId(Id), RsOpinion::NEGATIVE);
    }

	updateIdentity();
	updateIdList();
}

void IdDialog::neutralPerson()
{
    QList<QTreeWidgetItem *> selected_items = ui->idTreeWidget->selectedItems();
    for(QList<QTreeWidgetItem*>::const_iterator it(selected_items.begin());it!=selected_items.end();++it)
    {
        QTreeWidgetItem *item = *it ;

	std::string Id = item->text(RSID_COL_KEYID).toStdString();

	    rsReputations->setOwnOpinion(RsGxsId(Id), RsOpinion::NEUTRAL);
    }

	updateIdentity();
	updateIdList();
}
void IdDialog::positivePerson()
{
    QList<QTreeWidgetItem *> selected_items = ui->idTreeWidget->selectedItems();
    for(QList<QTreeWidgetItem*>::const_iterator it(selected_items.begin());it!=selected_items.end();++it)
    {
        QTreeWidgetItem *item = *it ;

		std::string Id = item->text(RSID_COL_KEYID).toStdString();

		rsReputations->setOwnOpinion(RsGxsId(Id), RsOpinion::POSITIVE);
    }

	updateIdentity();
	updateIdList();
}

void IdDialog::addtoContacts()
{
    QList<QTreeWidgetItem *> selected_items = ui->idTreeWidget->selectedItems();
    for(QList<QTreeWidgetItem*>::const_iterator it(selected_items.begin());it!=selected_items.end();++it)
    {
        QTreeWidgetItem *item = *it ;
	std::string Id = item->text(RSID_COL_KEYID).toStdString();

	rsIdentity->setAsRegularContact(RsGxsId(Id),true);
    }

	updateIdList();
}

void IdDialog::removefromContacts()
{
QList<QTreeWidgetItem *> selected_items = ui->idTreeWidget->selectedItems();
    for(QList<QTreeWidgetItem*>::const_iterator it(selected_items.begin());it!=selected_items.end();++it)
    {
        QTreeWidgetItem *item = *it ;
	std::string Id = item->text(RSID_COL_KEYID).toStdString();

	rsIdentity->setAsRegularContact(RsGxsId(Id),false);
    }

	updateIdList();
}

void IdDialog::on_closeInfoFrameButton_clicked()
{
	ui->inviteFrame->setVisible(false);
}

// We need to use indexes here because saving items is not possible since they can be re-created.

void IdDialog::saveExpandedCircleItems(std::vector<bool>& expanded_root_items, std::set<RsGxsCircleId>& expanded_circle_items) const
{
    expanded_root_items.clear();
    expanded_root_items.resize(3,false);
    expanded_circle_items.clear();

    auto saveTopLevel = [&](const QTreeWidgetItem* top_level_item,uint32_t index){
        if(!top_level_item)
            return;

        if(top_level_item->isExpanded())
        {
            expanded_root_items[index] = true;

            for(int row=0;row<top_level_item->childCount();++row)
                if(top_level_item->child(row)->isExpanded())
                    expanded_circle_items.insert(RsGxsCircleId(top_level_item->child(row)->data(CIRCLEGROUP_CIRCLE_COL_GROUPID,Qt::UserRole).toString().toStdString()));
		}
    };

    saveTopLevel(mExternalBelongingCircleItem,0);
    saveTopLevel(mExternalOtherCircleItem,1);
    saveTopLevel(mMyCircleItem,2);
}

void IdDialog::restoreExpandedCircleItems(const std::vector<bool>& expanded_root_items,const std::set<RsGxsCircleId>& expanded_circle_items)
{
	auto restoreTopLevel = [=](QTreeWidgetItem* top_level_item,uint32_t index){
		if(!top_level_item)
			return;

		top_level_item->setExpanded(expanded_root_items[index]);

		for(int row=0;row<top_level_item->childCount();++row)
        {
            RsGxsCircleId circle_id(RsGxsCircleId(top_level_item->child(row)->data(CIRCLEGROUP_CIRCLE_COL_GROUPID,Qt::UserRole).toString().toStdString()));
            bool expanded = expanded_circle_items.find(circle_id) != expanded_circle_items.end();

			top_level_item->child(row)->setExpanded(expanded_circle_items.find(circle_id) != expanded_circle_items.end());
		}
	};

    restoreTopLevel(mExternalBelongingCircleItem,0);
    restoreTopLevel(mExternalOtherCircleItem,1);
    restoreTopLevel(mMyCircleItem,2);
}

