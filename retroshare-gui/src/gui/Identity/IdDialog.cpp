/*
 * Retroshare Identity.
 *
 * Copyright 2012-2012 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */

#include <unistd.h>

#include <QMessageBox>
#include <QMenu>

#include "IdDialog.h"
#include "ui_IdDialog.h"
#include "IdEditDialog.h"
#include "retroshare-gui/RsAutoUpdatePage.h"
#include "gui/gxs/GxsIdDetails.h"
#include "gui/gxs/RsGxsUpdateBroadcastBase.h"
#include "gui/common/UIStateHelper.h"
#include "gui/chat/ChatDialog.h"
#include "gui/settings/rsharesettings.h"
#include "gui/msgs/MessageComposer.h"
#include "gui/Circles/CreateCircleDialog.h"
#include "gui/RetroShareLink.h"
#include "util/QtVersion.h"

#include <retroshare/rspeers.h>
#include <retroshare/rsreputations.h>
#include "retroshare/rsgxsflags.h"
#include "retroshare/rsmsgs.h" 
#include <iostream>
#include <algorithm>

/******
 * #define ID_DEBUG 1
 *****/

// Data Requests.
#define IDDIALOG_IDLIST     1
#define IDDIALOG_IDDETAILS  2
#define IDDIALOG_REPLIST    3
#define IDDIALOG_REFRESH    4

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

#define CIRCLESDIALOG_GROUPMETA  1
#define CIRCLESDIALOG_GROUPDATA  2

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
#define RSID_FILTER_ALL          0xffff

#define IMAGE_EDIT                 ":/images/edit_16.png"
#define IMAGE_CREATE               ":/icons/circle_new_128.png"
#define IMAGE_INVITED              ":/icons/bullet_yellow_128.png"
#define IMAGE_MEMBER               ":/icons/bullet_green_128.png"
#define IMAGE_UNKNOWN              ":/icons/bullet_grey_128.png"

// comment this out in order to remove the sorting of circles into "belong to" and "other visible circles"
#define CIRCLE_MEMBERSHIP_CATEGORIES 1

// quick solution for RSID_COL_VOTES sorting
class TreeWidgetItem : public QTreeWidgetItem {
  public:
  TreeWidgetItem(int type=Type): QTreeWidgetItem(type) {}
  TreeWidgetItem(QTreeWidget *tree): QTreeWidgetItem(tree) {}
  TreeWidgetItem(const QStringList& strings): QTreeWidgetItem (strings) {}
  bool operator< (const QTreeWidgetItem& other ) const {
    int column = treeWidget()->sortColumn();
    const QVariant v1 = data(column, Qt::DisplayRole);
    const QVariant v2 = other.data(column, Qt::DisplayRole);
    double value1 = v1.toDouble();
    double value2 = v2.toDouble();
      if (value1 != value2) {
          return value1 < value2;
                }
      else {
           return (v1.toString().compare (v2.toString(), Qt::CaseInsensitive) < 0);
                }
  }
};
/** Constructor */
IdDialog::IdDialog(QWidget *parent) :
    RsGxsUpdateBroadcastPage(rsIdentity, parent),
    ui(new Ui::IdDialog)
{
	ui->setupUi(this);

	mIdQueue = NULL;
    
    	// This is used to grab the broadcast of changes from p3GxsCircles, which is discarded by the current dialog, since it expects data for p3Identity only.
	mCirclesBroadcastBase = new RsGxsUpdateBroadcastBase(rsGxsCircles, this);
	connect(mCirclesBroadcastBase, SIGNAL(fillDisplay(bool)), this, SLOT(updateCirclesDisplay(bool)));
    
	ownItem = new QTreeWidgetItem();
	ownItem->setText(0, tr("My own identities"));

	allItem = new QTreeWidgetItem();
	allItem->setText(0, tr("All"));

	contactsItem = new QTreeWidgetItem();
	contactsItem->setText(0, tr("My contacts"));

	ui->treeWidget_membership->clear();
    
    	mExternalOtherCircleItem = NULL ;
    	mExternalBelongingCircleItem = NULL ;

	/* Setup UI helper */
	mStateHelper = new UIStateHelper(this);
//	mStateHelper->addWidget(IDDIALOG_IDLIST, ui->idTreeWidget);
	mStateHelper->addLoadPlaceholder(IDDIALOG_IDLIST, ui->idTreeWidget, false);
	mStateHelper->addClear(IDDIALOG_IDLIST, ui->idTreeWidget);

	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui->lineEdit_Nickname);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui->lineEdit_KeyId);
//	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui->lineEdit_GpgHash);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui->lineEdit_GpgId);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui->lineEdit_GpgName);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui->lineEdit_Type);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui->lineEdit_LastUsed);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui->ownOpinion_CB);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui->overallOpinion_TF);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui->neighborNodesOpinion_TF);

	mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui->lineEdit_Nickname);
	mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui->lineEdit_GpgName);
	mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui->lineEdit_KeyId);
//	mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui->lineEdit_GpgHash);
	mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui->lineEdit_GpgId);
	mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui->lineEdit_Type);
	mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui->lineEdit_GpgName);
	mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui->lineEdit_LastUsed);
	//mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui->line_RatingOverall);

	mStateHelper->addClear(IDDIALOG_IDDETAILS, ui->lineEdit_Nickname);
	mStateHelper->addClear(IDDIALOG_IDDETAILS, ui->lineEdit_KeyId);
//	mStateHelper->addClear(IDDIALOG_IDDETAILS, ui->lineEdit_GpgHash);
	mStateHelper->addClear(IDDIALOG_IDDETAILS, ui->lineEdit_GpgId);
	mStateHelper->addClear(IDDIALOG_IDDETAILS, ui->lineEdit_Type);
	mStateHelper->addClear(IDDIALOG_IDDETAILS, ui->lineEdit_GpgName);
	mStateHelper->addClear(IDDIALOG_IDDETAILS, ui->lineEdit_LastUsed);

	//mStateHelper->addWidget(IDDIALOG_REPLIST, ui->treeWidget_RepList);
	//mStateHelper->addLoadPlaceholder(IDDIALOG_REPLIST, ui->treeWidget_RepList);
	//mStateHelper->addClear(IDDIALOG_REPLIST, ui->treeWidget_RepList);

	/* Connect signals */
	connect(ui->toolButton_NewId, SIGNAL(clicked()), this, SLOT(addIdentity()));
	connect(ui->toolButton_NewCircle, SIGNAL(clicked()), this, SLOT(createExternalCircle()));

	connect(ui->removeIdentity, SIGNAL(triggered()), this, SLOT(removeIdentity()));
	connect(ui->editIdentity, SIGNAL(triggered()), this, SLOT(editIdentity()));
	connect(ui->chatIdentity, SIGNAL(triggered()), this, SLOT(chatIdentity()));

	connect(ui->idTreeWidget, SIGNAL(itemSelectionChanged()), this, SLOT(updateSelection()));
	connect(ui->idTreeWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(IdListCustomPopupMenu(QPoint)));

	connect(ui->filterComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(filterComboBoxChanged()));
	connect(ui->filterLineEdit, SIGNAL(textChanged(QString)), this, SLOT(filterChanged(QString)));
	connect(ui->ownOpinion_CB, SIGNAL(currentIndexChanged(int)), this, SLOT(modifyReputation()));
	
	connect(ui->inviteButton, SIGNAL(clicked()), this, SLOT(sendInvite()));


	ui->avlabel->setPixmap(QPixmap(":/images/user/friends64.png"));
	ui->avlabel_Circles->setPixmap(QPixmap(":/icons/circles_128.png"));

	ui->headerTextLabel->setText(tr("People"));
	ui->headerTextLabel_Circles->setText(tr("Circles"));

	/* Initialize splitter */
	ui->splitter->setStretchFactor(0, 0);
	ui->splitter->setStretchFactor(1, 1);
	
  /*remove
	QList<int> sizes;
	sizes << width() << 500; // Qt calculates the right sizes
	ui->splitter->setSizes(sizes);*/

	/* Add filter types */
    ui->filterComboBox->addItem(tr("All"), RSID_FILTER_ALL);
    ui->filterComboBox->addItem(tr("Owned by myself"), RSID_FILTER_OWNED_BY_YOU);
    ui->filterComboBox->addItem(tr("Linked to my node"), RSID_FILTER_YOURSELF);
    ui->filterComboBox->addItem(tr("Linked to neighbor nodes"), RSID_FILTER_FRIENDS);
    ui->filterComboBox->addItem(tr("Linked to distant nodes"), RSID_FILTER_OTHERS);
    ui->filterComboBox->addItem(tr("Anonymous"), RSID_FILTER_PSEUDONYMS);
	ui->filterComboBox->setCurrentIndex(0);

	/* Add filter actions */
	QTreeWidgetItem *headerItem = ui->idTreeWidget->headerItem();
	QString headerText = headerItem->text(RSID_COL_NICKNAME);
	ui->filterLineEdit->addFilter(QIcon(), headerText, RSID_COL_NICKNAME, QString("%1 %2").arg(tr("Search"), headerText));

	/* Set initial section sizes */
  QHeaderView * circlesheader = ui->treeWidget_membership->header () ;
  circlesheader->resizeSection (CIRCLEGROUP_CIRCLE_COL_GROUPNAME, QFontMetricsF(ui->idTreeWidget->font()).width("Circle name")*1.5) ;

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
	ui->idTreeWidget->setColumnWidth(RSID_COL_VOTES, 7 * fontWidth);
	
	//QHeaderView_setSectionResizeMode(ui->idTreeWidget->header(), QHeaderView::ResizeToContents);

	mIdQueue = new TokenQueue(rsIdentity->getTokenService(), this);

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
                	<p><b>External circles</b> are groups of identities (anonymous or signed), that are shared at a distance over the network. They can be \
                		used to restrict the visibility to forums, channels, etc. </p> \
                	<p>An <b>external circle</b> can be restricted to another circle, thereby limiting its visibility to members of that circle \
                		or even self-restricted, meaning that it is only visible to its members.</p> \
                	<p>A <b>local circle</b> is a group of friend nodes (represented by their PGP Ids), and can also be used to restrict the \
                		visibility of forums and channels. They are not shared over the network, and their list of members is only visible to you.</p>") ;

	registerHelpButton(ui->helpButton, hlp_str) ;

	// load settings
	processSettings(true);

    // circles stuff
    
    //connect(ui->treeWidget_membership, SIGNAL(itemSelectionChanged()), this, SLOT(circle_selected()));
    connect(ui->treeWidget_membership, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(CircleListCustomPopupMenu(QPoint)));

    
    /* Setup TokenQueue */
    mCircleQueue = new TokenQueue(rsGxsCircles->getTokenService(), this);
    
    requestCircleGroupMeta();
    
    // This timer shouldn't be needed, but it is now, because the update of subscribe status and appartenance to the
    // circle doesn't trigger a proper GUI update.
    
    QTimer *tmer = new QTimer(this) ;
    connect(tmer,SIGNAL(timeout()),this,SLOT(updateCirclesDisplay())) ;
    
    tmer->start(10000) ;	// update every 10 secs. 
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
    requestCircleGroupMeta() ;
}

/************************** Request / Response *************************/
/*** Loading Main Index ***/

void IdDialog::requestCircleGroupMeta()
{
	mStateHelper->setLoading(CIRCLESDIALOG_GROUPMETA, true);

#ifdef ID_DEBUG
	std::cerr << "CirclesDialog::requestGroupMeta()";
	std::cerr << std::endl;
#endif

	mCircleQueue->cancelActiveRequestTokens(CIRCLESDIALOG_GROUPMETA);

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_META;

	uint32_t token;
	mCircleQueue->requestGroupInfo(token,  RS_TOKREQ_ANSTYPE_SUMMARY, opts, CIRCLESDIALOG_GROUPMETA);
}

void IdDialog::requestCircleGroupData(const RsGxsCircleId& circle_id)
{
	mStateHelper->setLoading(CIRCLESDIALOG_GROUPDATA, true);

#ifdef ID_DEBUG
	std::cerr << "CirclesDialog::requestGroupData()";
	std::cerr << std::endl;
#endif

	mCircleQueue->cancelActiveRequestTokens(CIRCLESDIALOG_GROUPDATA);

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;

	std::list<RsGxsGroupId> grps ;
	grps.push_back(RsGxsGroupId(circle_id));

	uint32_t token;
	mCircleQueue->requestGroupInfo(token,  RS_TOKREQ_ANSTYPE_DATA, opts, grps, CIRCLESDIALOG_GROUPDATA);
}

void IdDialog::loadCircleGroupMeta(const uint32_t &token)
{
	mStateHelper->setLoading(CIRCLESDIALOG_GROUPMETA, false);

#ifdef ID_DEBUG
	std::cerr << "CirclesDialog::loadCircleGroupMeta()";
	std::cerr << std::endl;
#endif

	std::list<RsGroupMetaData> groupInfo;
	std::list<RsGroupMetaData>::iterator vit;

	if (!rsGxsCircles->getGroupSummary(token,groupInfo))
	{
		std::cerr << "CirclesDialog::loadCircleGroupMeta() Error getting GroupMeta";
		std::cerr << std::endl;
		mStateHelper->setActive(CIRCLESDIALOG_GROUPMETA, false);
		return;
	}

	mStateHelper->setActive(CIRCLESDIALOG_GROUPMETA, true);

	/* add the top level item */
	//QTreeWidgetItem *personalCirclesItem = new QTreeWidgetItem();
	//personalCirclesItem->setText(0, tr("Personal Circles"));
	//ui->treeWidget_membership->addTopLevelItem(personalCirclesItem);

#ifdef CIRCLE_MEMBERSHIP_CATEGORIES
	if(!mExternalOtherCircleItem)
	{
		mExternalOtherCircleItem = new QTreeWidgetItem();
		mExternalOtherCircleItem->setText(0, tr("Other visible external circles"));

		ui->treeWidget_membership->addTopLevelItem(mExternalOtherCircleItem);
	}

	if(!mExternalBelongingCircleItem )
	{
		mExternalBelongingCircleItem = new QTreeWidgetItem();
		mExternalBelongingCircleItem->setText(0, tr("External circles my identities belong to"));
		ui->treeWidget_membership->addTopLevelItem(mExternalBelongingCircleItem);
	}
#endif

	std::list<RsGxsId> own_identities ;
	rsIdentity->getOwnIds(own_identities) ;

	for(vit = groupInfo.begin(); vit != groupInfo.end();++vit)
	{
#ifdef ID_DEBUG
		std::cerr << "CirclesDialog::loadCircleGroupMeta() GroupId: " << vit->mGroupId << " Group: " << vit->mGroupName << std::endl;
#endif
		RsGxsCircleDetails details;
		rsGxsCircles->getCircleDetails(RsGxsCircleId(vit->mGroupId), details) ;

		bool should_re_add = true ;
		bool am_I_in_circle = details.mAmIAllowed ;
		bool am_I_admin (vit->mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_ADMIN) ;
		bool am_I_subscribed (vit->mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED) ;
		QTreeWidgetItem *item = NULL ;

#ifdef ID_DEBUG
		std::cerr << "Loaded info for circle " << vit->mGroupId << ". am_I_in_circle=" << am_I_in_circle << std::endl;
#endif

		// find already existing items for this circle

		// implement the search manually, because there's no find based on user role.
		//QList<QTreeWidgetItem*> clist = ui->treeWidget_membership->findItems( QString::fromStdString(vit->mGroupId.toStdString()), Qt::MatchExactly|Qt::MatchRecursive, CIRCLEGROUP_CIRCLE_COL_GROUPID);
		QList<QTreeWidgetItem*> clist ;
		QString test_str = QString::fromStdString(vit->mGroupId.toStdString()) ;
		for(QTreeWidgetItemIterator itt(ui->treeWidget_membership);*itt;++itt)
			if( (*itt)->data(CIRCLEGROUP_CIRCLE_COL_GROUPID,Qt::UserRole).toString() == test_str)
				clist.push_back(*itt) ;

		if(!clist.empty())
		{
			// delete all duplicate items. This should not happen, but just in case it does.

			while(clist.size() > 1)	
			{
#ifdef ID_DEBUG
				std::cerr << "  more than 1 item correspond to this ID. Removing!" << std::endl;
#endif
				delete clist.front() ;
			}

			item = clist.front() ;

#ifdef CIRCLE_MEMBERSHIP_CATEGORIES
			if(am_I_in_circle && item->parent() != mExternalBelongingCircleItem)
			{
#ifdef ID_DEBUG
				std::cerr << "  Existing circle is not in subscribed items although it is subscribed. Removing." << std::endl;
#endif
				delete item ;
				item = NULL ;
			}
			else if(!am_I_in_circle && item->parent() != mExternalOtherCircleItem)
			{
#ifdef ID_DEBUG
				std::cerr << "  Existing circle is not in subscribed items although it is subscribed. Removing." << std::endl;
#endif
				delete item ;
				item = NULL ;
			}
			else
#endif
				should_re_add = false ;	// item already exists
		}

		/* Add Widget, and request Pages */

		if(should_re_add)
		{
			item = new QTreeWidgetItem();

			item->setText(CIRCLEGROUP_CIRCLE_COL_GROUPNAME, QString::fromUtf8(vit->mGroupName.c_str()));
			item->setData(CIRCLEGROUP_CIRCLE_COL_GROUPID,Qt::UserRole, QString::fromStdString(vit->mGroupId.toStdString()));
			item->setData(CIRCLEGROUP_CIRCLE_COL_GROUPFLAGS, Qt::UserRole, QVariant(vit->mSubscribeFlags));

#ifdef CIRCLE_MEMBERSHIP_CATEGORIES
			if(am_I_in_circle)
			{
#ifdef ID_DEBUG
				std::cerr << "  adding item for circle " << vit->mGroupId << " to own circles"<< std::endl;
#endif
				mExternalBelongingCircleItem->addChild(item);
			}
			else
			{
#ifdef ID_DEBUG
				std::cerr << "  adding item for circle " << vit->mGroupId << " to others"<< std::endl;
#endif
				mExternalOtherCircleItem->addChild(item);
			}
#else
			ui->treeWidget_membership->addTopLevelItem(item) ;
#endif
		}
		else  if(item->text(CIRCLEGROUP_CIRCLE_COL_GROUPNAME) != QString::fromUtf8(vit->mGroupName.c_str()))
		{
#ifdef ID_DEBUG
			std::cerr << "  Existing circle has a new name. Updating it in the tree." << std::endl;
#endif
			item->setText(CIRCLEGROUP_CIRCLE_COL_GROUPNAME, QString::fromUtf8(vit->mGroupName.c_str()));
		}
		// just in case.

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
		std::vector<QTreeWidgetItem*> to_delete ;

		for(uint32_t k=0;k<item->childCount();++k)
			if(details.mSubscriptionFlags.find(RsGxsId(item->child(k)->data(CIRCLEGROUP_CIRCLE_COL_GROUPID,Qt::UserRole).toString().toStdString())) == details.mSubscriptionFlags.end())
				to_delete.push_back(item->child(k));

		for(uint32_t k=0;k<to_delete.size();++k)
			delete to_delete[k] ;

		for(std::map<RsGxsId,uint32_t>::const_iterator it(details.mSubscriptionFlags.begin());it!=details.mSubscriptionFlags.end();++it)
		{
#ifdef ID_DEBUG
			std::cerr << "    ID " << *it << ": " ;
#endif
			bool is_own_id = rsIdentity->isOwnId(it->first) ;
			bool invited ( it->second & GXS_EXTERNAL_CIRCLE_FLAGS_IN_ADMIN_LIST );
			bool subscrb ( it->second & GXS_EXTERNAL_CIRCLE_FLAGS_SUBSCRIBED );

#ifdef ID_DEBUG
			std::cerr << "invited: " << invited << ", subscription: " << subscrb ;
#endif
			QTreeWidgetItem *subitem = NULL ;

			// see if the item already exists
			for(uint32_t k=0;k<item->childCount();++k)
				if(item->child(k)->data(CIRCLEGROUP_CIRCLE_COL_GROUPID,Qt::UserRole).toString().toStdString() == it->first.toStdString())
				{
					subitem = item->child(k);
#ifdef ID_DEBUG
					std::cerr << " found existing sub item." << std::endl;
#endif
					break ;
				}

			if(!(invited || subscrb))
			{
				if(subitem != NULL)
					delete subitem ;
#ifdef ID_DEBUG
				std::cerr << ". not relevant. Skipping." << std::endl;
#endif
				continue ;
			}
			// remove item if flags are not ok.

			if(subitem && subitem->data(CIRCLEGROUP_CIRCLE_COL_GROUPFLAGS, Qt::UserRole).toUInt() != it->second)
			{
				delete subitem ; 
				subitem = NULL ;
			}

			if(!subitem)
			{
#ifdef ID_DEBUG
				std::cerr << " no existing sub item. Creating new one." << std::endl;
#endif
				subitem = new QTreeWidgetItem(item);

				RsIdentityDetails idd ;
				bool has_id = rsIdentity->getIdDetails(it->first,idd) ;

				QPixmap pixmap ;

				if(idd.mAvatar.mSize == 0 || !pixmap.loadFromData(idd.mAvatar.mData, idd.mAvatar.mSize, "PNG"))
					pixmap = QPixmap::fromImage(GxsIdDetails::makeDefaultIcon(it->first)) ;

				if(has_id)
					subitem->setText(CIRCLEGROUP_CIRCLE_COL_GROUPNAME, QString::fromUtf8(idd.mNickname.c_str())) ;
				else
					subitem->setText(CIRCLEGROUP_CIRCLE_COL_GROUPNAME, tr("Unknown ID :")+QString::fromStdString(it->first.toStdString())) ;

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

				subitem->setIcon(RSID_COL_NICKNAME, QIcon(pixmap));

				item->addChild(subitem) ;
			}

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

		if(am_I_in_circle)
			item->setIcon(CIRCLEGROUP_CIRCLE_COL_GROUPNAME,QIcon(IMAGE_MEMBER)) ;
		else if(am_I_invited || am_I_pending)
			item->setIcon(CIRCLEGROUP_CIRCLE_COL_GROUPNAME,QIcon(IMAGE_INVITED)) ;
		else
			item->setIcon(CIRCLEGROUP_CIRCLE_COL_GROUPNAME,QIcon(IMAGE_UNKNOWN)) ;
	}
}

static void mark_matching_tree(QTreeWidget *w, const std::set<RsGxsId>& members, int col) 
{
    w->selectionModel()->clearSelection() ;
    
    for(std::set<RsGxsId>::const_iterator it(members.begin());it!=members.end();++it)
    {
	QList<QTreeWidgetItem*> clist = w->findItems( QString::fromStdString((*it).toStdString()), Qt::MatchExactly|Qt::MatchRecursive, col);
    
    	foreach(QTreeWidgetItem* item, clist)
		item->setSelected(true) ;
    }
}

void IdDialog::loadCircleGroupData(const uint32_t& token)
{
#ifdef ID_DEBUG
    std::cerr << "Loading circle info" << std::endl;
#endif
    
    std::vector<RsGxsCircleGroup> circle_grp_v ;
    rsGxsCircles->getGroupData(token, circle_grp_v);

    if (circle_grp_v.empty())
    {
        std::cerr << "(EE) unexpected empty result from getGroupData. Cannot process circle now!" << std::endl;
        return ;
    }
        
    if (circle_grp_v.size() != 1)
    {
        std::cerr << "(EE) very weird result from getGroupData. Should get exactly one circle" << std::endl;
        return ;
    }
    
    RsGxsCircleGroup cg = circle_grp_v.front();
    RsGxsCircleId requested_cid(cg.mMeta.mGroupId) ;

    QTreeWidgetItem *item = ui->treeWidget_membership->currentItem();

    RsGxsCircleId id ;
    if(!getItemCircleId(item,id))
        return ;
    
    if(requested_cid != id)
    {
        std::cerr << "(WW) not the same circle. Dropping request." << std::endl;
        return ;
    }
    
    /* now mark all the members */

    std::set<RsGxsId> members = cg.mInvitedMembers;

    mark_matching_tree(ui->idTreeWidget, members, RSID_COL_KEYID) ;
    
    mStateHelper->setLoading(CIRCLESDIALOG_GROUPDATA, false);
}

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
    
    requestCircleGroupMeta();	// update GUI
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
    
    requestCircleGroupMeta();	// update GUI
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
    
    if(item_id == RsGxsId(circle_id))	// is it a circle?
    {
	    uint32_t group_flags = item->data(CIRCLEGROUP_CIRCLE_COL_GROUPFLAGS, Qt::UserRole).toUInt();

#ifdef CIRCLE_MEMBERSHIP_CATEGORIES
	    if(item->parent() != NULL)
	    {
#endif
		    if(group_flags & GXS_SERV::GROUP_SUBSCRIBE_ADMIN)
			    contextMnu.addAction(QIcon(IMAGE_EDIT), tr("Edit Circle"), this, SLOT(showEditExistingCircle()));
		    else
			    contextMnu.addAction(QIcon(IMAGE_EDIT), tr("See details"), this, SLOT(showEditExistingCircle()));
#ifdef CIRCLE_MEMBERSHIP_CATEGORIES
	    }
#endif
        
	    std::cerr << "  Item is a circle item. Adding Edit/Details menu entry." << std::endl;
            is_circle = true ;

	    contextMnu.addSeparator() ;
    }
    else if(rsIdentity->isOwnId(item_id))	// is it one of our GXS ids?
    {
	    current_gxs_id = RsGxsId(item_id);
            is_circle =false ;

	    std::cerr << "  Item is a GxsId item. Requesting flags/group id from parent: " << circle_id << std::endl;
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
    const QString image_names[4] = { ":/images/edit_16.png",":/images/edit_16.png",":/images/edit_16.png",":/images/edit_16.png" } ;

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
			    id_name = tr("for identity ")+QString::fromUtf8(det.mNickname.c_str()) + "(ID=" + QString::fromStdString(ids[i][0].toStdString()) + ")" ;
		    else
			    id_name = tr("for identity ")+QString::fromStdString(ids[i][0].toStdString()) ;

		    QAction *action ;

		    if(is_circle)
			    action = new QAction(QIcon(image_names[i]), menu_titles[i] + " " + id_name,this) ;
		    else
			    action = new QAction(QIcon(image_names[i]), menu_titles[i],this) ;

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
				    id_name = tr("for identity ")+QString::fromUtf8(det.mNickname.c_str()) + "(ID=" + QString::fromStdString(ids[i][j].toStdString()) + ")" ;
			    else
				    id_name = tr("for identity ")+QString::fromStdString(ids[i][j].toStdString()) ;

			    QAction *action = new QAction(QIcon(image_names[i]), id_name,this) ;

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

    contextMnu.exec(QCursor::pos());
}

static void set_item_background(QTreeWidgetItem *item, uint32_t type)
{
	QBrush brush;
	switch(type)
	{
		default:
		case CLEAR_BACKGROUND:
			brush = QBrush(Qt::white);
			break;
		case GREEN_BACKGROUND:
			brush = QBrush(Qt::green);
			break;
		case BLUE_BACKGROUND:
			brush = QBrush(Qt::blue);
			break;
		case RED_BACKGROUND:
			brush = QBrush(Qt::red);
			break;
		case GRAY_BACKGROUND:
			brush = QBrush(Qt::gray);
			break;
	}
	item->setBackground (0, brush);
}

static void update_children_background(QTreeWidgetItem *item, uint32_t type)
{
	int count = item->childCount();
	for(int i = 0; i < count; ++i)
	{
		QTreeWidgetItem *child = item->child(i);

		if (child->childCount() > 0)
		{
			update_children_background(child, type);
		}
		set_item_background(child, type);
	}
}

#ifdef SUSPENDED
static void set_tree_background(QTreeWidget *tree, uint32_t type)
{
	std::cerr << "CirclesDialog set_tree_background()";
	std::cerr << std::endl;

	/* grab all toplevel */
	int count = tree->topLevelItemCount();
	for(int i = 0; i < count; ++i)
	{
		QTreeWidgetItem *item = tree->topLevelItem(i);
		/* resursively clear child backgrounds */
		update_children_background(item, type);
		set_item_background(item, type);
	}
}

static void check_mark_item(QTreeWidgetItem *item, const std::set<RsPgpId> &names, uint32_t col, uint32_t type)
{
	QString coltext = item->text(col);
    RsPgpId colstr ( coltext.toStdString());
	if (names.end() != names.find(colstr))
	{
		set_item_background(item, type);
		std::cerr << "CirclesDialog check_mark_item: found match: " << colstr;
		std::cerr << std::endl;
	}
}

void IdDialog::circle_selected()
{
	QTreeWidgetItem *item = ui->treeWidget_membership->currentItem();

#ifdef ID_DEBUG
	std::cerr << "CirclesDialog::circle_selected() valid circle chosen";
	std::cerr << std::endl;
#endif
	//set_item_background(item, BLUE_BACKGROUND);

	QString coltext = (item->parent()->parent())? (item->parent()->data(CIRCLEGROUP_CIRCLE_COL_GROUPID,Qt::UserRole).toString()) : (item->data(CIRCLEGROUP_CIRCLE_COL_GROUPID,Qt::UserRole).toString());
	RsGxsCircleId id ( coltext.toStdString()) ;

    	requestCircleGroupData(id) ;
}
#endif
    
IdDialog::~IdDialog()
{
	// save settings
	processSettings(false);

	delete(ui);
	delete(mIdQueue);
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
		ui->splitter->restoreState(Settings->value("splitter").toByteArray());
	} else {
		// save settings

		// filterColumn
		Settings->setValue("filterColumn", ui->filterLineEdit->currentFilter());

		// state of splitter
		Settings->setValue("splitter", ui->splitter->saveState());
		
		//save expanding
		Settings->setValue("ExpandAll", allItem->isExpanded());
		Settings->setValue("ExpandContacts", contactsItem->isExpanded());
		Settings->setValue("ExpandOwn", ownItem->isExpanded());
	}

	Settings->endGroup();
}

void IdDialog::filterComboBoxChanged()
{
	requestIdList();
}

void IdDialog::filterChanged(const QString& /*text*/)
{
	filterIds();
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
		requestIdDetails();
		requestRepList();
	}
}

void IdDialog::requestIdList()
{
	//Disable by default, will be enable by insertIdDetails()
	ui->removeIdentity->setEnabled(false);
	ui->editIdentity->setEnabled(false);

	if (!mIdQueue)
		return;

	mStateHelper->setLoading(IDDIALOG_IDLIST, true);

	mIdQueue->cancelActiveRequestTokens(IDDIALOG_IDLIST);

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;

	uint32_t token;

	mIdQueue->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, IDDIALOG_IDLIST);
}

bool IdDialog::fillIdListItem(const RsGxsIdGroup& data, QTreeWidgetItem *&item, const RsPgpId &ownPgpId, int accept)
{
    bool isLinkedToOwnNode = (data.mPgpKnown && (data.mPgpId == ownPgpId)) ;
    bool isOwnId = (data.mMeta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_ADMIN);
    uint32_t item_flags = 0 ;

	/* do filtering */
	bool ok = false;
	if (data.mMeta.mGroupFlags & RSGXSID_GROUPFLAG_REALID)
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

	if (!ok)
		return false;

	if (!item)
        item = new TreeWidgetItem();
        
        RsReputations::ReputationInfo info ;
        rsReputations->getReputationInfo(RsGxsId(data.mMeta.mGroupId),info) ;

    item->setText(RSID_COL_NICKNAME, QString::fromUtf8(data.mMeta.mGroupName.c_str()).left(RSID_MAXIMUM_NICKNAME_SIZE));
    item->setText(RSID_COL_KEYID, QString::fromStdString(data.mMeta.mGroupId.toStdString()));
    
    //time_t now = time(NULL) ;
    //item->setText(RSID_COL_LASTUSED, getHumanReadableDuration(now - data.mLastUsageTS)) ;

    item->setData(RSID_COL_KEYID, Qt::UserRole,QVariant(item_flags)) ;
 
    item->setTextAlignment(RSID_COL_VOTES, Qt::AlignRight);
    item->setData(RSID_COL_VOTES,Qt::DisplayRole, QString::number(info.mOverallReputationScore - 1.0f,'f',3));

	 if(isOwnId)
	 {
        QFont font = item->font(RSID_COL_NICKNAME) ;
		font.setBold(true) ;
		item->setFont(RSID_COL_NICKNAME,font) ;
		item->setFont(RSID_COL_IDTYPE,font) ;
		item->setFont(RSID_COL_KEYID,font) ;

		QString tooltip = tr("This identity is owned by you");
		item->setToolTip(RSID_COL_NICKNAME, tooltip) ;
		item->setToolTip(RSID_COL_KEYID, tooltip) ;
		item->setToolTip(RSID_COL_IDTYPE, tooltip) ;
	 }

    QPixmap pixmap ;

    if(data.mImage.mSize == 0 || !pixmap.loadFromData(data.mImage.mData, data.mImage.mSize, "PNG"))
        pixmap = QPixmap::fromImage(GxsIdDetails::makeDefaultIcon(RsGxsId(data.mMeta.mGroupId))) ;

    item->setIcon(RSID_COL_NICKNAME, QIcon(pixmap));

    QString tooltip;

	if (data.mMeta.mGroupFlags & RSGXSID_GROUPFLAG_REALID)
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

void IdDialog::insertIdList(uint32_t token)
{
	mStateHelper->setLoading(IDDIALOG_IDLIST, false);

	int accept = ui->filterComboBox->itemData(ui->filterComboBox->currentIndex()).toInt();
		
	RsGxsIdGroup data;
	std::vector<RsGxsIdGroup> datavector;
	std::vector<RsGxsIdGroup>::iterator vit;
    
	if (!rsIdentity->getGroupData(token, datavector))
	{
#ifdef ID_DEBUG
		std::cerr << "IdDialog::insertIdList() Error getting GroupData";
		std::cerr << std::endl;
#endif

		mStateHelper->setActive(IDDIALOG_IDLIST, false);
		mStateHelper->clear(IDDIALOG_IDLIST);

		return;
	}
    
    	// turn that vector into a std::set, to avoid a linear search
    
    	std::map<RsGxsGroupId,RsGxsIdGroup> ids_set ;
        
        for(uint32_t i=0;i<datavector.size();++i)
            ids_set[datavector[i].mMeta.mGroupId] = datavector[i] ;

	mStateHelper->setActive(IDDIALOG_IDLIST, true);

	RsPgpId ownPgpId  = rsPeers->getGPGOwnId();

	// Update existing and remove not existing items 
    	// Also remove items that do not have the correct parent
    	
	QTreeWidgetItemIterator itemIterator(ui->idTreeWidget);
	QTreeWidgetItem *item = NULL;
    
	while ((item = *itemIterator) != NULL) 
	{
		++itemIterator;
		std::map<RsGxsGroupId,RsGxsIdGroup>::iterator it = ids_set.find(RsGxsGroupId(item->text(RSID_COL_KEYID).toStdString())) ;

		if(it == ids_set.end())
		{
			if(item != allItem && item != contactsItem && item != ownItem)
				delete(item);
                        
                        continue ;
		} 
                
        	QTreeWidgetItem *parent_item = item->parent() ;
                    
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
	    data = vit->second ;

	    item = NULL;

	    ui->idTreeWidget->insertTopLevelItem(0, ownItem);
	    ui->idTreeWidget->insertTopLevelItem(0, allItem);
	    ui->idTreeWidget->insertTopLevelItem(0, contactsItem );  

	    Settings->beginGroup("IdDialog");
	    allItem->setExpanded(Settings->value("ExpandAll", QVariant(true)).toBool());
	    ownItem->setExpanded(Settings->value("ExpandOwn", QVariant(true)).toBool());
	    contactsItem->setExpanded(Settings->value("ExpandContacts", QVariant(true)).toBool());

	    Settings->endGroup();

                if (fillIdListItem(vit->second, item, ownPgpId, accept))
                {
		    if(vit->second.mMeta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_ADMIN)
			    ownItem->addChild(item);
		    else if(vit->second.mIsAContact)
			    contactsItem->addChild(item);
		    else
			    allItem->addChild(item);
                }
    }
	
	/* count items */
	int itemCount = contactsItem->childCount() + allItem->childCount() + ownItem->childCount();
	ui->label_count->setText( "(" + QString::number( itemCount ) + ")" );

	filterIds();
	updateSelection();
}

void IdDialog::requestIdDetails()
{
	mIdQueue->cancelActiveRequestTokens(IDDIALOG_IDDETAILS);

	if (mId.isNull())
	{
		mStateHelper->setActive(IDDIALOG_IDDETAILS, false);
		mStateHelper->setLoading(IDDIALOG_IDDETAILS, false);
		mStateHelper->clear(IDDIALOG_IDDETAILS);

		return;
	}

	mStateHelper->setLoading(IDDIALOG_IDDETAILS, true);

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;

	uint32_t token;
	std::list<RsGxsGroupId> groupIds;
	groupIds.push_back(mId);

	mIdQueue->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, groupIds, IDDIALOG_IDDETAILS);
}

void IdDialog::insertIdDetails(uint32_t token)
{
	mStateHelper->setLoading(IDDIALOG_IDDETAILS, false);

	/* get details from libretroshare */
	RsGxsIdGroup data;
	std::vector<RsGxsIdGroup> datavector;
	if (!rsIdentity->getGroupData(token, datavector))
	{
		mStateHelper->setActive(IDDIALOG_IDDETAILS, false);
		mStateHelper->clear(IDDIALOG_REPLIST);

		ui->lineEdit_KeyId->setText("ERROR GETTING KEY!");

		return;
	}

	if (datavector.size() != 1)
	{
#ifdef ID_DEBUG
		std::cerr << "IdDialog::insertIdDetails() Invalid datavector size";
#endif

		mStateHelper->setActive(IDDIALOG_IDDETAILS, false);
		mStateHelper->clear(IDDIALOG_IDDETAILS);

		ui->lineEdit_KeyId->setText("INVALID DV SIZE");

		return;
	}

	mStateHelper->setActive(IDDIALOG_IDDETAILS, true);

	data = datavector[0];

	/* get GPG Details from rsPeers */
	RsPgpId ownPgpId  = rsPeers->getGPGOwnId();

    ui->lineEdit_Nickname->setText(QString::fromUtf8(data.mMeta.mGroupName.c_str()).left(RSID_MAXIMUM_NICKNAME_SIZE));
	ui->lineEdit_KeyId->setText(QString::fromStdString(data.mMeta.mGroupId.toStdString()));
	//ui->lineEdit_GpgHash->setText(QString::fromStdString(data.mPgpIdHash.toStdString()));
    if(data.mPgpKnown)
	    ui->lineEdit_GpgId->setText(QString::fromStdString(data.mPgpId.toStdString()));
    else
	    ui->lineEdit_GpgId->setText(QString::fromStdString(data.mPgpId.toStdString()) + tr(" [unverified]"));

    time_t now = time(NULL) ;
    ui->lineEdit_LastUsed->setText(getHumanReadableDuration(now - data.mLastUsageTS)) ;
    ui->headerTextLabel->setText(QString::fromUtf8(data.mMeta.mGroupName.c_str()).left(RSID_MAXIMUM_NICKNAME_SIZE));

    QPixmap pixmap ;

    if(data.mImage.mSize == 0 || !pixmap.loadFromData(data.mImage.mData, data.mImage.mSize, "PNG"))
        pixmap = QPixmap::fromImage(GxsIdDetails::makeDefaultIcon(RsGxsId(data.mMeta.mGroupId))) ;

#ifdef ID_DEBUG
	std::cerr << "Setting header frame image : " << pixmap.width() << " x " << pixmap.height() << std::endl;
#endif

    ui->avlabel->setPixmap(pixmap);
    ui->avatarLabel->setPixmap(pixmap);

	if (data.mPgpKnown)
	{
		RsPeerDetails details;
		rsPeers->getGPGDetails(data.mPgpId, details);
		ui->lineEdit_GpgName->setText(QString::fromUtf8(details.name.c_str()));
	}
	else
	{
		if (data.mMeta.mGroupFlags & RSGXSID_GROUPFLAG_REALID)
			ui->lineEdit_GpgName->setText(tr("[Unknown node]"));
		else
			ui->lineEdit_GpgName->setText(tr("Anonymous Id"));
	}

	if(data.mPgpId.isNull())
	{
		ui->lineEdit_GpgId->hide() ;
		ui->PgpId_LB->hide() ;
	}
	else
	{
		ui->lineEdit_GpgId->show() ;
		ui->PgpId_LB->show() ;
	}
    
    if(data.mPgpKnown)
    {
		ui->lineEdit_GpgName->show() ;
		ui->PgpName_LB->show() ;
    }
    else
    {
		ui->lineEdit_GpgName->hide() ;
		ui->PgpName_LB->hide() ;
    }

    bool isLinkedToOwnPgpId = (data.mPgpKnown && (data.mPgpId == ownPgpId)) ;
    bool isOwnId = (data.mMeta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_ADMIN);

    if(isOwnId)
        if (isLinkedToOwnPgpId)
            ui->lineEdit_Type->setText(tr("Identity owned by you, linked to your Retroshare node")) ;
        else
            ui->lineEdit_Type->setText(tr("Anonymous identity, owned by you")) ;
    else if (data.mMeta.mGroupFlags & RSGXSID_GROUPFLAG_REALID)
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
        ui->lineEdit_Type->setText(tr("Anonymous identity")) ;

	if (isOwnId)
	{
		mStateHelper->setWidgetEnabled(ui->ownOpinion_CB, false);
		ui->editIdentity->setEnabled(true);
		ui->removeIdentity->setEnabled(true);
		ui->chatIdentity->setEnabled(false);
		ui->inviteButton->setEnabled(false);
	}
	else
	{
		// No Reputation yet!
		mStateHelper->setWidgetEnabled(ui->ownOpinion_CB, true);
		ui->editIdentity->setEnabled(false);
		ui->removeIdentity->setEnabled(false);
		ui->chatIdentity->setEnabled(true);
		ui->inviteButton->setEnabled(true);
	}

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

    RsReputations::ReputationInfo info ;
    rsReputations->getReputationInfo(RsGxsId(data.mMeta.mGroupId),info) ;
    
	ui->neighborNodesOpinion_TF->setText(QString::number(info.mFriendAverage - 1.0f));

	ui->overallOpinion_TF->setText(QString::number(info.mOverallReputationScore - 1.0f) +" ("+
	 ((info.mAssessment == RsReputations::ASSESSMENT_OK)? tr("OK") : tr("Banned")) +")" ) ;
    
    switch(info.mOwnOpinion)
	{
        case RsReputations::OPINION_NEGATIVE: ui->ownOpinion_CB->setCurrentIndex(0); break ;
        case RsReputations::OPINION_NEUTRAL : ui->ownOpinion_CB->setCurrentIndex(1); break ;
        case RsReputations::OPINION_POSITIVE: ui->ownOpinion_CB->setCurrentIndex(2); break ;
        default:
            std::cerr << "Unexpected value in own opinion: " << info.mOwnOpinion << std::endl;
	}
}

void IdDialog::modifyReputation()
{
#ifdef ID_DEBUG
	std::cerr << "IdDialog::modifyReputation()";
	std::cerr << std::endl;
#endif

	RsGxsId id(ui->lineEdit_KeyId->text().toStdString());
    
    	RsReputations::Opinion op ;

    	switch(ui->ownOpinion_CB->currentIndex())
        {
        	case 0: op = RsReputations::OPINION_NEGATIVE ; break ;
        	case 1: op = RsReputations::OPINION_NEUTRAL  ; break ;
        	case 2: op = RsReputations::OPINION_POSITIVE ; break ;
        default:
            std::cerr << "Wrong value from opinion combobox. Bug??" << std::endl;
            
        }
    	rsReputations->setOwnOpinion(id,op) ;

#ifdef ID_DEBUG
	std::cerr << "IdDialog::modifyReputation() ID: " << id << " Mod: " << op;
	std::cerr << std::endl;
#endif

#ifdef SUSPENDED
    	// Cyril: apparently the old reputation system was in used here. It's based on GXS data exchange, and probably not
    	// very efficient because of this.
    
	uint32_t token;
	if (!rsIdentity->submitOpinion(token, id, false, op))
	{
#ifdef ID_DEBUG
		std::cerr << "IdDialog::modifyReputation() Error submitting Opinion";
		std::cerr << std::endl;
#endif
	}
#endif

	// trigger refresh when finished.
	// basic / anstype are not needed.
    requestIdDetails();
    requestIdList();

	return;
}
	
void IdDialog::updateDisplay(bool complete)
{
	/* Update identity list */

	if (complete) {
		/* Fill complete */
		requestIdList();
		requestIdDetails();
		requestRepList();

		return;
	}
	requestCircleGroupMeta();

	std::list<RsGxsGroupId> grpIds;
	getAllGrpIds(grpIds);
	if (!getGrpIds().empty()) {
		requestIdList();

		if (!mId.isNull() && std::find(grpIds.begin(), grpIds.end(), mId) != grpIds.end()) {
			requestIdDetails();
			requestRepList();
		}
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

void IdDialog::requestRepList()
{
	// Removing this for the moment.
	return;

	mStateHelper->setLoading(IDDIALOG_REPLIST, true);

	mIdQueue->cancelActiveRequestTokens(IDDIALOG_REPLIST);

	std::list<RsGxsGroupId> groupIds;
	groupIds.push_back(mId);

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;

	uint32_t token;
	mIdQueue->requestMsgInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, groupIds, IDDIALOG_REPLIST);
}

void IdDialog::insertRepList(uint32_t token)
{
	Q_UNUSED(token)
	mStateHelper->setLoading(IDDIALOG_REPLIST, false);
	mStateHelper->setActive(IDDIALOG_REPLIST, true);
}

void IdDialog::loadRequest(const TokenQueue * queue, const TokenRequest &req)
{
#ifdef ID_DEBUG
	std::cerr << "IdDialog::loadRequest() UserType: " << req.mUserType;
	std::cerr << std::endl;
#endif

    if(queue == mIdQueue)
    {
	    switch(req.mUserType)
	    {
	    case IDDIALOG_IDLIST:
		    insertIdList(req.mToken);
		    break;

	    case IDDIALOG_IDDETAILS:
		    insertIdDetails(req.mToken);
		    break;

	    case IDDIALOG_REPLIST:
		    insertRepList(req.mToken);
		    break;

	    case IDDIALOG_REFRESH:
		    // replaced by RsGxsUpdateBroadcastPage
		    //			updateDisplay(true);
		    break;
	    default:
		    std::cerr << "IdDialog::loadRequest() ERROR";
		    std::cerr << std::endl;
		    break;
	    }
    }
    
    if(queue == mCircleQueue)
    {
#ifdef ID_DEBUG
	    std::cerr << "CirclesDialog::loadRequest() UserType: " << req.mUserType;
	    std::cerr << std::endl;
#endif

	    /* now switch on req */
	    switch(req.mUserType)
	    {
	    case CIRCLESDIALOG_GROUPMETA:
		    loadCircleGroupMeta(req.mToken);
		    break;

	    case CIRCLESDIALOG_GROUPDATA:
		    loadCircleGroupData(req.mToken);
		    break;

	    default:
		    std::cerr << "CirclesDialog::loadRequest() ERROR: INVALID TYPE";
		    std::cerr << std::endl;
		    break;
	    }
    }
}

void IdDialog::IdListCustomPopupMenu( QPoint )
{
    QMenu contextMnu( this );


    std::list<RsGxsId> own_identities ;
    rsIdentity->getOwnIds(own_identities) ;

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

	    RsReputations::ReputationInfo info ;
	    rsReputations->getReputationInfo(keyId,info) ;

        	switch(info.mOwnOpinion)
            {
            case RsReputations::OPINION_NEGATIVE:  ++n_negative_reputations ;
		    break ;
            
	    case RsReputations::OPINION_POSITIVE: ++n_positive_reputations ;
                break ;
                
	    case RsReputations::OPINION_NEUTRAL: ++n_neutral_reputations ;
                break ;
            }

	    ++n_selected_items ;

	    if(rsIdentity->isARegularContact(keyId))
		    ++n_is_a_contact ;
	    else
		    ++n_is_not_a_contact ;
    }

    if(root_node_present)	// don't show menu if some of the root nodes are present
	    return ;

    if(!one_item_owned_by_you)
    {
	    if(n_selected_items == 1)		// if only one item is selected, allow to chat with this item 
		    if(own_identities.size() <= 1)
		    {
			    QAction *action = contextMnu.addAction(QIcon(":/images/chat_24.png"), tr("Chat with this person"), this, SLOT(chatIdentity()));

			    if(own_identities.empty())
				    action->setEnabled(false) ;
			    else
				    action->setData(QString::fromStdString((own_identities.front()).toStdString())) ;
		    }
		    else
		    {
			    QMenu *mnu = contextMnu.addMenu(QIcon(":/images/chat_24.png"),tr("Chat with this person as...")) ;

			    for(std::list<RsGxsId>::const_iterator it=own_identities.begin();it!=own_identities.end();++it)
			    {
				    RsIdentityDetails idd ;
				    rsIdentity->getIdDetails(*it,idd) ;

				    QPixmap pixmap ;

				    if(idd.mAvatar.mSize == 0 || !pixmap.loadFromData(idd.mAvatar.mData, idd.mAvatar.mSize, "PNG"))
					    pixmap = QPixmap::fromImage(GxsIdDetails::makeDefaultIcon(*it)) ;

				    QAction *action = mnu->addAction(QIcon(pixmap), QString("%1 (%2)").arg(QString::fromUtf8(idd.mNickname.c_str()), QString::fromStdString((*it).toStdString())), this, SLOT(chatIdentity()));
				    action->setData(QString::fromStdString((*it).toStdString())) ;
			    }
		    }

	    // always allow to send messages
	    contextMnu.addAction(QIcon(":/images/mail_new.png"), tr("Send message"), this, SLOT(sendMsg()));

	    contextMnu.addSeparator();

	    if(n_is_a_contact == 0)
		    contextMnu.addAction(QIcon(), tr("Add to Contacts"), this, SLOT(addtoContacts()));
        
	    if(n_is_not_a_contact == 0)
		    contextMnu.addAction(QIcon(":/images/cancel.png"), tr("Remove from Contacts"), this, SLOT(removefromContacts()));

	    contextMnu.addSeparator();

	    if(n_positive_reputations == 0)	// only unban when all items are banned
		    contextMnu.addAction(QIcon(), tr("Set positive opinion"), this, SLOT(positivePerson()));

	    if(n_neutral_reputations == 0)	// only unban when all items are banned
		    contextMnu.addAction(QIcon(), tr("Set neutral opinion"), this, SLOT(neutralPerson()));
        
	    if(n_negative_reputations == 0)
		    contextMnu.addAction(QIcon(":/images/denied16.png"), tr("Set negative opinion"), this, SLOT(negativePerson()));
    }

    if(one_item_owned_by_you && n_selected_items==1)
    {
	    contextMnu.addSeparator();

	    contextMnu.addAction(ui->editIdentity);
	    contextMnu.addAction(ui->removeIdentity);
    }


    contextMnu.addSeparator();

    contextMnu.exec(QCursor::pos());
}

void IdDialog::chatIdentity()
{
	QTreeWidgetItem *item = ui->idTreeWidget->currentItem();
	if (!item)
	{
		std::cerr << "IdDialog::editIdentity() Invalid item";
		std::cerr << std::endl;
		return;
	}

	std::string keyId = item->text(RSID_COL_KEYID).toStdString();

	QAction *action = qobject_cast<QAction *>(QObject::sender());
	if (!action)
		return ;

	RsGxsId from_gxs_id(action->data().toString().toStdString());
	uint32_t error_code ;
    DistantChatPeerId did ;

	if(!rsMsgs->initiateDistantChatConnexion(RsGxsId(keyId), from_gxs_id, did, error_code))
		QMessageBox::information(NULL, tr("Distant chat cannot work"), QString("%1 %2: %3").arg(tr("Distant chat refused with this person.")).arg(tr("Error code")).arg(error_code)) ;
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
    /* create a message */
    MessageComposer *composer = MessageComposer::newMsg();

    composer->setTitleText(tr("You have a friend invite"));
    
    RsPeerId ownId = rsPeers->getOwnId();
    RetroShareLink link;
    link.createCertificate(ownId);
    
    std::string keyId = item->text(RSID_COL_KEYID).toStdString();
    
    QString sMsgText = inviteMessage();
    sMsgText += "<br><br>";
    sMsgText += tr("Respond now:") + "<br>";
    sMsgText += link.toHtml() + "<br>";
    sMsgText += "<br>";
    sMsgText += tr("Thanks, <br>") + QString::fromUtf8(rsPeers->getGPGName(rsPeers->getGPGOwnId()).c_str());
    composer->setMsgText(sMsgText);
    composer->addRecipient(MessageComposer::TO,  RsGxsId(keyId));

    composer->show();

}

void IdDialog::negativePerson()
{
    QList<QTreeWidgetItem *> selected_items = ui->idTreeWidget->selectedItems();
    for(QList<QTreeWidgetItem*>::const_iterator it(selected_items.begin());it!=selected_items.end();++it)
    {
        QTreeWidgetItem *item = *it ;
        
	std::string Id = item->text(RSID_COL_KEYID).toStdString();

	rsReputations->setOwnOpinion(RsGxsId(Id),RsReputations::OPINION_NEGATIVE) ;
    }

	requestIdDetails();
	requestIdList();
}

void IdDialog::neutralPerson()
{
    QList<QTreeWidgetItem *> selected_items = ui->idTreeWidget->selectedItems();
    for(QList<QTreeWidgetItem*>::const_iterator it(selected_items.begin());it!=selected_items.end();++it)
    {
        QTreeWidgetItem *item = *it ;

	std::string Id = item->text(RSID_COL_KEYID).toStdString();

	rsReputations->setOwnOpinion(RsGxsId(Id),RsReputations::OPINION_NEUTRAL) ;
    }

	requestIdDetails();
	requestIdList();
}
void IdDialog::positivePerson()
{
    QList<QTreeWidgetItem *> selected_items = ui->idTreeWidget->selectedItems();
    for(QList<QTreeWidgetItem*>::const_iterator it(selected_items.begin());it!=selected_items.end();++it)
    {
        QTreeWidgetItem *item = *it ;

	std::string Id = item->text(RSID_COL_KEYID).toStdString();

	rsReputations->setOwnOpinion(RsGxsId(Id),RsReputations::OPINION_POSITIVE) ;
    }

	requestIdDetails();
	requestIdList();
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

	requestIdList();
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

	requestIdList();
}

