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

#include <QMessageBox>
#include <QMenu>

#include "IdDialog.h"
#include "ui_IdDialog.h"
#include "IdEditDialog.h"
#include "gui/gxs/GxsIdDetails.h"
#include "gui/common/UIStateHelper.h"
#include "gui/chat/ChatDialog.h"
#include "gui/settings/rsharesettings.h"
#include "gui/msgs/MessageComposer.h"
#include "gui/Circles/CreateCircleDialog.h"

#include <retroshare/rspeers.h>
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

#define CIRCLEGROUP_CIRCLE_COL_GROUPNAME 0
#define CIRCLEGROUP_CIRCLE_COL_GROUPID   1

#define CIRCLEGROUP_FRIEND_COL_NAME 0
#define CIRCLEGROUP_FRIEND_COL_ID   1

#define CLEAR_BACKGROUND 0
#define GREEN_BACKGROUND 1
#define BLUE_BACKGROUND  2
#define RED_BACKGROUND   3
#define GRAY_BACKGROUND  4

#define CIRCLESDIALOG_GROUPMETA			1
/****************************************************************
 */

#define RSID_COL_NICKNAME   0
#define RSID_COL_KEYID      1
#define RSID_COL_LASTUSED   2
#define RSID_COL_IDTYPE     3

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

/** Constructor */
IdDialog::IdDialog(QWidget *parent) :
    RsGxsUpdateBroadcastPage(rsIdentity, parent),
    ui(new Ui::IdDialog)
{
	ui->setupUi(this);

	mIdQueue = NULL;

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
    mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui->toolButton_Reputation);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui->line_RatingOverall);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui->line_RatingImplicit);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui->line_RatingOwn);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui->line_RatingPeers);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui->repModButton);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui->repMod_Accept);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui->repMod_Ban);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui->repMod_Negative);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui->repMod_Positive);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui->repMod_Custom);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui->repMod_spinBox);

	mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui->lineEdit_Nickname);
	mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui->lineEdit_GpgName);
	mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui->lineEdit_KeyId);
//	mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui->lineEdit_GpgHash);
	mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui->lineEdit_GpgId);
	mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui->lineEdit_Type);
	mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui->lineEdit_GpgName);
    mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui->lineEdit_LastUsed);
    mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui->line_RatingOverall);
	mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui->line_RatingImplicit);
	mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui->line_RatingOwn);
	mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui->line_RatingPeers);

	mStateHelper->addClear(IDDIALOG_IDDETAILS, ui->lineEdit_Nickname);
	mStateHelper->addClear(IDDIALOG_IDDETAILS, ui->lineEdit_KeyId);
//	mStateHelper->addClear(IDDIALOG_IDDETAILS, ui->lineEdit_GpgHash);
	mStateHelper->addClear(IDDIALOG_IDDETAILS, ui->lineEdit_GpgId);
	mStateHelper->addClear(IDDIALOG_IDDETAILS, ui->lineEdit_Type);
    mStateHelper->addClear(IDDIALOG_IDDETAILS, ui->lineEdit_GpgName);
    mStateHelper->addClear(IDDIALOG_IDDETAILS, ui->lineEdit_LastUsed);
    mStateHelper->addClear(IDDIALOG_IDDETAILS, ui->line_RatingOverall);
	mStateHelper->addClear(IDDIALOG_IDDETAILS, ui->line_RatingImplicit);
	mStateHelper->addClear(IDDIALOG_IDDETAILS, ui->line_RatingOwn);
	mStateHelper->addClear(IDDIALOG_IDDETAILS, ui->line_RatingPeers);

	//mStateHelper->addWidget(IDDIALOG_REPLIST, ui->treeWidget_RepList);
	//mStateHelper->addLoadPlaceholder(IDDIALOG_REPLIST, ui->treeWidget_RepList);
	//mStateHelper->addClear(IDDIALOG_REPLIST, ui->treeWidget_RepList);

	/* Connect signals */
	connect(ui->toolButton_NewId, SIGNAL(clicked()), this, SLOT(addIdentity()));

	connect(ui->removeIdentity, SIGNAL(triggered()), this, SLOT(removeIdentity()));
	connect(ui->editIdentity, SIGNAL(triggered()), this, SLOT(editIdentity()));
	connect(ui->chatIdentity, SIGNAL(triggered()), this, SLOT(chatIdentity()));

	connect(ui->idTreeWidget, SIGNAL(itemSelectionChanged()), this, SLOT(updateSelection()));
	connect(ui->idTreeWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(IdListCustomPopupMenu(QPoint)));

	connect(ui->filterComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(filterComboBoxChanged()));
	connect(ui->filterLineEdit, SIGNAL(textChanged(QString)), this, SLOT(filterChanged(QString)));
	connect(ui->repModButton, SIGNAL(clicked()), this, SLOT(modifyReputation()));
	
	connect(ui->messageButton, SIGNAL(clicked()), this, SLOT(sendMsg()));


	ui->avlabel->setPixmap(QPixmap(":/images/user/friends64.png"));

	ui->headerTextLabel->setText(tr("People"));

	/* Initialize splitter */
	ui->splitter->setStretchFactor(0, 1);
	ui->splitter->setStretchFactor(1, 0);

	QList<int> sizes;
	sizes << width() << 500; // Qt calculates the right sizes
	ui->splitter->setSizes(sizes);

	/* Add filter types */
    ui->filterComboBox->addItem(tr("All"), RSID_FILTER_ALL);
    ui->filterComboBox->addItem(tr("Owned by you"), RSID_FILTER_OWNED_BY_YOU);
    ui->filterComboBox->addItem(tr("Linked to your node"), RSID_FILTER_YOURSELF);
    ui->filterComboBox->addItem(tr("Linked to neighbor nodes"), RSID_FILTER_FRIENDS);
    ui->filterComboBox->addItem(tr("Linked to distant nodes"), RSID_FILTER_OTHERS);
    ui->filterComboBox->addItem(tr("Anonymous"), RSID_FILTER_PSEUDONYMS);
	ui->filterComboBox->setCurrentIndex(0);

	/* Add filter actions */
	QTreeWidgetItem *headerItem = ui->idTreeWidget->headerItem();
	QString headerText = headerItem->text(RSID_COL_NICKNAME);
	ui->filterLineEdit->addFilter(QIcon(), headerText, RSID_COL_NICKNAME, QString("%1 %2").arg(tr("Search"), headerText));
	headerText = headerItem->text(RSID_COL_KEYID);
	ui->filterLineEdit->addFilter(QIcon(), headerItem->text(RSID_COL_KEYID), RSID_COL_KEYID, QString("%1 %2").arg(tr("Search"), headerText));
	
	/* Set initial section sizes */
  QHeaderView * circlesheader = ui->treeWidget_membership->header () ;
  circlesheader->resizeSection (CIRCLEGROUP_CIRCLE_COL_GROUPNAME, 280);

	/* Setup tree */
	ui->idTreeWidget->sortByColumn(RSID_COL_NICKNAME, Qt::AscendingOrder);

	ui->idTreeWidget->enableColumnCustomize(true);
	ui->idTreeWidget->setColumnCustomizable(RSID_COL_NICKNAME, false);
	
	ui->idTreeWidget->setColumnHidden(RSID_COL_IDTYPE, true);
	ui->idTreeWidget->setColumnHidden(RSID_COL_LASTUSED, true);
	

	/* Set initial column width */
	int fontWidth = QFontMetricsF(ui->idTreeWidget->font()).width("W");
	ui->idTreeWidget->setColumnWidth(RSID_COL_NICKNAME, 18 * fontWidth);
	ui->idTreeWidget->setColumnWidth(RSID_COL_KEYID, 25 * fontWidth);
	ui->idTreeWidget->setColumnWidth(RSID_COL_IDTYPE, 18 * fontWidth);

	mIdQueue = new TokenQueue(rsIdentity->getTokenService(), this);

	mStateHelper->setActive(IDDIALOG_IDDETAILS, false);
	mStateHelper->setActive(IDDIALOG_REPLIST, false);

	// Hiding RepList until that part is finished.
	//ui->treeWidget_RepList->setVisible(false);
	ui->toolButton_Reputation->setVisible(false);

	QString hlp_str = tr(
			" <h1><img width=\"32\" src=\":/icons/help_64.png\">&nbsp;&nbsp;Identities</h1>    \
			<p>In this tab you can create/edit pseudo-anonymous identities.</p>                \
			<p>Identities are used to securely identify your data: sign forum and channel posts,\
				and receive feedback using Retroshare built-in email system, post comments \
				after channel posts, etc.</p> \
			<p>Identities can optionally be signed by your Retroshare node's certificate.   \
			Signed identities are easier to trust but are easily linked to your node's IP address.</p>  \
			<p> Anonymous identities allow you to anonymously interact with other users. They cannot be   \
			spoofed, but noone can prove who really owns a given identity.</p>") ;

	registerHelpButton(ui->helpButton, hlp_str) ;

	// load settings
	processSettings(true);

    // hide reputation sice it's currently unused
    ui->reputationGroupBox->hide();
    ui->tweakGroupBox->hide();
    
    // circles stuff
    
    connect(ui->pushButton_extCircle, SIGNAL(clicked()), this, SLOT(createExternalCircle()));
    connect(ui->pushButton_editCircle, SIGNAL(clicked()), this, SLOT(editExistingCircle()));
    connect(ui->treeWidget_membership, SIGNAL(itemSelectionChanged()), this, SLOT(circle_selected()));
    
    /* Setup TokenQueue */
    mCircleQueue = new TokenQueue(rsGxsCircles->getTokenService(), this);
    
    requestCircleGroupMeta();
}	


/************************** Request / Response *************************/
/*** Loading Main Index ***/

void IdDialog::requestCircleGroupMeta()
{
	mStateHelper->setLoading(CIRCLESDIALOG_GROUPMETA, true);

	std::cerr << "CirclesDialog::requestGroupMeta()";
	std::cerr << std::endl;

	mCircleQueue->cancelActiveRequestTokens(CIRCLESDIALOG_GROUPMETA);

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_META;

	uint32_t token;
	mCircleQueue->requestGroupInfo(token,  RS_TOKREQ_ANSTYPE_SUMMARY, opts, CIRCLESDIALOG_GROUPMETA);
}

void IdDialog::loadCircleGroupMeta(const uint32_t &token)
{
	mStateHelper->setLoading(CIRCLESDIALOG_GROUPMETA, false);

	std::cerr << "CirclesDialog::loadCircleGroupMeta()";
	std::cerr << std::endl;

	ui->treeWidget_membership->clear();

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

	QTreeWidgetItem *externalAdminCirclesItem = new QTreeWidgetItem();
	externalAdminCirclesItem->setText(0, tr("Circles (Admin)"));
	ui->treeWidget_membership->addTopLevelItem(externalAdminCirclesItem);

	QTreeWidgetItem *externalSubCirclesItem = new QTreeWidgetItem();
	externalSubCirclesItem->setText(0, tr("Circles (Subscribed)"));
	ui->treeWidget_membership->addTopLevelItem(externalSubCirclesItem);

	QTreeWidgetItem *externalOtherCirclesItem = new QTreeWidgetItem();
	externalOtherCirclesItem->setText(0, tr("Circles (Other)"));
	ui->treeWidget_membership->addTopLevelItem(externalOtherCirclesItem);

	for(vit = groupInfo.begin(); vit != groupInfo.end(); ++vit)
	{
		/* Add Widget, and request Pages */
		std::cerr << "CirclesDialog::loadCircleGroupMeta() GroupId: " << vit->mGroupId;
		std::cerr << " Group: " << vit->mGroupName;
		std::cerr << std::endl;

		QTreeWidgetItem *groupItem = new QTreeWidgetItem();
		groupItem->setText(CIRCLEGROUP_CIRCLE_COL_GROUPNAME, QString::fromUtf8(vit->mGroupName.c_str()));
        groupItem->setText(CIRCLEGROUP_CIRCLE_COL_GROUPID, QString::fromStdString(vit->mGroupId.toStdString()));

		if (vit->mCircleType == GXS_CIRCLE_TYPE_LOCAL)
		{
			//personalCirclesItem->addChild(groupItem);
		}
		else
		{
			if (vit->mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_ADMIN)
			{
				externalAdminCirclesItem->addChild(groupItem);
			}
			else if (vit->mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_SUBSCRIBED)
			{
				externalSubCirclesItem->addChild(groupItem);
			}
			else
			{
				externalOtherCirclesItem->addChild(groupItem);
			}
		}
	}
}


void IdDialog::createExternalCircle()
{
	CreateCircleDialog dlg;
	dlg.editNewId(true);
	dlg.exec();
}
void IdDialog::editExistingCircle()
{
	QTreeWidgetItem *item = ui->treeWidget_membership->currentItem();
	if ((!item) || (!item->parent()))
	{
		return;
	}

	QString coltext = item->text(CIRCLEGROUP_CIRCLE_COL_GROUPID);
    RsGxsGroupId id ( coltext.toStdString());

	CreateCircleDialog dlg;
	dlg.editExistingId(id);
	dlg.exec();
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

	std::cerr << "CirclesDialog::circle_selected() valid circle chosen";
	std::cerr << std::endl;

	set_tree_background(ui->treeWidget_membership, CLEAR_BACKGROUND);
	//set_tree_background(ui->treeWidget_friends, CLEAR_BACKGROUND);
	//set_tree_background(ui->treeWidget_category, CLEAR_BACKGROUND);

	if ((!item) || (!item->parent()))
	{
		mStateHelper->setWidgetEnabled(ui->pushButton_editCircle, false);
		return;
	}

	set_item_background(item, BLUE_BACKGROUND);

	QString coltext = item->text(CIRCLEGROUP_CIRCLE_COL_GROUPID);
    RsGxsCircleId id ( coltext.toStdString()) ;

	/* update friend lists */
	RsGxsCircleDetails details;
	if (rsGxsCircles->getCircleDetails(id, details))
	{
		/* now mark all the members */
        std::set<RsPgpId> members;
		std::map<RsPgpId, std::list<RsGxsId> >::iterator it;
		for(it = details.mAllowedPeers.begin(); it != details.mAllowedPeers.end(); ++it)
		{
			members.insert(it->first);
			std::cerr << "Circle member: " << it->first;
			std::cerr << std::endl;
		}

//		mark_matching_tree(ui->treeWidget_friends, members, CIRCLEGROUP_FRIEND_COL_ID, GREEN_BACKGROUND);
	}
	else
	{
//		set_tree_background(ui->treeWidget_friends, GRAY_BACKGROUND);
	}
	mStateHelper->setWidgetEnabled(ui->pushButton_editCircle, true);
}


IdDialog::~IdDialog()
{
	// save settings
	processSettings(false);

	delete(ui);
	delete(mIdQueue);
}

void IdDialog::todo()
{
	QMessageBox::information(this, "Todo",
	                         "<b>Open points:</b><ul>"
	                         "<li>Reputation"
	                         "</ul>");
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
        item = new QTreeWidgetItem();

    item->setText(RSID_COL_NICKNAME, QString::fromUtf8(data.mMeta.mGroupName.c_str()).left(RSID_MAXIMUM_NICKNAME_SIZE));
    item->setText(RSID_COL_KEYID, QString::fromStdString(data.mMeta.mGroupId.toStdString()));
    
    time_t now = time(NULL) ;
    item->setText(RSID_COL_LASTUSED, getHumanReadableDuration(now - data.mLastUsageTS)) ;

    item->setData(RSID_COL_KEYID, Qt::UserRole,QVariant(item_flags)) ;
 
    item->setData(RSID_COL_LASTUSED, Qt::UserRole, QString::number(now - data.mLastUsageTS));

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

#ifdef ID_DEBUG
	std::cerr << "Setting item image : " << pixmap.width() << " x " << pixmap.height() << std::endl;
#endif
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
			item->setToolTip(RSID_COL_IDTYPE,QString::fromStdString(data.mPgpId.toStdString())) ;
			
			
			tooltip += tr("Node name:")+" " + QString::fromUtf8(details.name.c_str()) + "\n";
			tooltip += tr("Node Id  :")+" " + QString::fromStdString(data.mPgpId.toStdString()) ;
			item->setToolTip(RSID_COL_KEYID,tooltip) ;
		}
		else
		{
			item->setText(RSID_COL_IDTYPE, tr("Unknown PGP key"));
			item->setToolTip(RSID_COL_IDTYPE,tr("Unknown key ID")) ;
			item->setToolTip(RSID_COL_KEYID,tr("Unknown key ID")) ;

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

	mStateHelper->setActive(IDDIALOG_IDLIST, true);

	RsPgpId ownPgpId  = rsPeers->getGPGOwnId();

	/* Update existing and remove not existing items */
	QTreeWidgetItemIterator itemIterator(ui->idTreeWidget);
	QTreeWidgetItem *item = NULL;
	while ((item = *itemIterator) != NULL) {
		++itemIterator;

		for (vit = datavector.begin(); vit != datavector.end(); ++vit)
		{
			if (vit->mMeta.mGroupId == RsGxsGroupId(item->text(RSID_COL_KEYID).toStdString()))
			{
				break;
			}
		}
		if (vit == datavector.end())
		{
			delete(item);
		} else {
			if (!fillIdListItem(*vit, item, ownPgpId, accept))
			{
				delete(item);
			}
			datavector.erase(vit);
		}
	}

	/* Insert new items */
	for (vit = datavector.begin(); vit != datavector.end(); ++vit)
	{
		data = (*vit);

		item = NULL;
		if (fillIdListItem(*vit, item, ownPgpId, accept))
		{
			ui->idTreeWidget->addTopLevelItem(item);
		}
	}

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
    ui->lineEdit_GpgId->setText(QString::fromStdString(data.mPgpId.toStdString()));

    time_t now = time(NULL) ;
    ui->lineEdit_LastUsed->setText(getHumanReadableDuration(now - data.mLastUsageTS)) ;
    ui->headerTextLabel->setText(QString::fromUtf8(data.mMeta.mGroupName.c_str()).left(RSID_MAXIMUM_NICKNAME_SIZE));

    QPixmap pixmap ;

    if(data.mImage.mSize == 0 || !pixmap.loadFromData(data.mImage.mData, data.mImage.mSize, "PNG"))
        pixmap = QPixmap::fromImage(GxsIdDetails::makeDefaultIcon(RsGxsId(data.mMeta.mGroupId))) ;

#ifdef ID_DEBUG
	std::cerr << "Setting header frame image : " << pix.width() << " x " << pix.height() << std::endl;
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
		{
			ui->lineEdit_GpgName->setText(tr("Unknown real name"));
		}
		else
		{
			ui->lineEdit_GpgName->setText(tr("Anonymous Id"));
		}
	}

	if(data.mPgpId.isNull())
	{
		ui->lineEdit_GpgId->hide() ;
		ui->lineEdit_GpgName->hide() ;
		ui->PgpId_LB->hide() ;
		ui->PgpName_LB->hide() ;
	}
	else
	{
		ui->lineEdit_GpgId->show() ;
		ui->lineEdit_GpgName->show() ;
		ui->PgpId_LB->show() ;
		ui->PgpName_LB->show() ;
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

    //	if (isOwnId)
//	{
//		ui->radioButton_IdYourself->setChecked(true);
//	}
//	else if (data.mMeta.mGroupFlags & RSGXSID_GROUPFLAG_REALID)
//	{
//		if (data.mPgpKnown)
//		{
//			if (rsPeers->isGPGAccepted(data.mPgpId))
//			{
//				ui->radioButton_IdFriend->setChecked(true);
//			}
//			else
//			{
//				ui->radioButton_IdFOF->setChecked(true);
//			}
//		}
//		else
//		{
//			ui->radioButton_IdOther->setChecked(true);
//		}
//	}
//	else
//	{
//		ui->radioButton_IdPseudo->setChecked(true);
//	}

	if (isOwnId)
	{
		mStateHelper->setWidgetEnabled(ui->toolButton_Reputation, false);
		ui->editIdentity->setEnabled(true);
		ui->removeIdentity->setEnabled(true);
		ui->chatIdentity->setEnabled(false);
		ui->messageButton->setEnabled(false);
	}
	else
	{
		// No Reputation yet!
		mStateHelper->setWidgetEnabled(ui->toolButton_Reputation, /*true*/ false);
		ui->editIdentity->setEnabled(false);
		ui->removeIdentity->setEnabled(false);
		ui->chatIdentity->setEnabled(true);
    ui->messageButton->setEnabled(true);
	}

	/* now fill in the reputation information */
	ui->line_RatingOverall->setText("Overall Rating TODO");
	ui->line_RatingOwn->setText("Own Rating TODO");

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
		QString rating = QString::number(data.mReputation.mOverallScore);
		ui->line_RatingOverall->setText(rating);
	}

	{
		QString rating = QString::number(data.mReputation.mIdScore);
		ui->line_RatingImplicit->setText(rating);
	}

	{
		QString rating = QString::number(data.mReputation.mOwnOpinion);
		ui->line_RatingOwn->setText(rating);
	}

	{
		QString rating = QString::number(data.mReputation.mPeerOpinion);
		ui->line_RatingPeers->setText(rating);
	}
}

void IdDialog::modifyReputation()
{
#ifdef ID_DEBUG
	std::cerr << "IdDialog::modifyReputation()";
	std::cerr << std::endl;
#endif

	RsGxsId id(ui->lineEdit_KeyId->text().toStdString());

	int mod = 0;
	if (ui->repMod_Accept->isChecked())
	{
		mod += 100;
	}
	else if (ui->repMod_Positive->isChecked())
	{
		mod += 10;
	}
	else if (ui->repMod_Negative->isChecked())
	{
		mod += -10;
	}
	else if (ui->repMod_Ban->isChecked())
	{
		mod += -100;
	}
	else if (ui->repMod_Custom->isChecked())
	{
		mod += ui->repMod_spinBox->value();
	}
	else
	{
		// invalid
		return;
	}

#ifdef ID_DEBUG
	std::cerr << "IdDialog::modifyReputation() ID: " << id << " Mod: " << mod;
	std::cerr << std::endl;
#endif

	uint32_t token;
	if (!rsIdentity->submitOpinion(token, id, false, mod))
	{
#ifdef ID_DEBUG
		std::cerr << "IdDialog::modifyReputation() Error submitting Opinion";
		std::cerr << std::endl;
#endif
	}

#ifdef ID_DEBUG
	std::cerr << "IdDialog::modifyReputation() queuingRequest(), token: " << token;
	std::cerr << std::endl;
#endif

	// trigger refresh when finished.
	// basic / anstype are not needed.
	mIdQueue->queueRequest(token, 0, 0, IDDIALOG_REFRESH);

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
	    std::cerr << "CirclesDialog::loadRequest() UserType: " << req.mUserType;
	    std::cerr << std::endl;

	    /* now switch on req */
	    switch(req.mUserType)
	    {
	    case CIRCLESDIALOG_GROUPMETA:
		    loadCircleGroupMeta(req.mToken);
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

	QTreeWidgetItem *item = ui->idTreeWidget->currentItem();
	if (item) {
		uint32_t item_flags = item->data(RSID_COL_KEYID,Qt::UserRole).toUInt() ;

		if(!(item_flags & RSID_FILTER_OWNED_BY_YOU))
		{
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

			contextMnu.addAction(QIcon(":/images/mail_new.png"), tr("Send message to this person"), this, SLOT(sendMsg()));
		}
	}

	contextMnu.addSeparator();

	contextMnu.addAction(ui->editIdentity);
	contextMnu.addAction(ui->removeIdentity);
	
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

	if(!rsMsgs->initiateDistantChatConnexion(RsGxsId(keyId), from_gxs_id, error_code))
		QMessageBox::information(NULL, tr("Distant chat cannot work"), QString("%1 %2: %3").arg(tr("Distant chat refused with this person.")).arg(tr("Error code")).arg(error_code)) ;
}

void IdDialog::sendMsg()
{
	QTreeWidgetItem *item = ui->idTreeWidget->currentItem();
	if (!item)
	{
		return;
	}

	MessageComposer *nMsgDialog = MessageComposer::newMsg();
	if (nMsgDialog == NULL) {
		return;
	}

    std::string keyId = item->text(RSID_COL_KEYID).toStdString();

    nMsgDialog->addRecipient(MessageComposer::TO,  RsGxsId(keyId));
		nMsgDialog->show();
		nMsgDialog->activateWindow();

    /* window will destroy itself! */

}
