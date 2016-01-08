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
    
	allItem = new QTreeWidgetItem();
	allItem->setText(0, tr("All"));

	contactsItem = new QTreeWidgetItem();
	contactsItem->setText(0, tr("Contacts"));


	/* Setup UI helper */
	mStateHelper = new UIStateHelper(this);
//	mStateHelper->addWidget(IDDIALOG_IDLIST, ui->idTreeWidget);
	mStateHelper->addLoadPlaceholder(IDDIALOG_IDLIST, ui->idTreeWidget, false);
	mStateHelper->addClear(IDDIALOG_IDLIST, ui->idTreeWidget);

	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui->nickname_LEdit);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui->keyId_LEdit);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui->type_LEdit);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui->nodeId_LEdit);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui->nodeName_LEdit);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui->lastUsed_LEdit);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui->ownOpinion_CB);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui->overallOpinion_TF);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui->neighborNodesOpinion_TF);
	//	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui->lineEdit_GpgHash);

	mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui->nickname_LEdit);
	mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui->keyId_LEdit);
	mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui->type_LEdit);
	mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui->nodeId_LEdit);
	mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui->nodeName_LEdit);
	mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui->lastUsed_LEdit);
	//mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui->lineEdit_GpgHash);
	//mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui->line_RatingOverall);

	mStateHelper->addClear(IDDIALOG_IDDETAILS, ui->nickname_LEdit);
	mStateHelper->addClear(IDDIALOG_IDDETAILS, ui->keyId_LEdit);
	mStateHelper->addClear(IDDIALOG_IDDETAILS, ui->type_LEdit);
	mStateHelper->addClear(IDDIALOG_IDDETAILS, ui->nodeId_LEdit);
	mStateHelper->addClear(IDDIALOG_IDDETAILS, ui->nodeName_LEdit);
	mStateHelper->addClear(IDDIALOG_IDDETAILS, ui->lastUsed_LEdit);
//	mStateHelper->addClear(IDDIALOG_IDDETAILS, ui->lineEdit_GpgHash);

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
	connect(ui->ownOpinion_CB, SIGNAL(currentIndexChanged(int)), this, SLOT(modifyReputation()));
	
	connect(ui->inviteButton, SIGNAL(clicked()), this, SLOT(sendInvite()));

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
			<p>In this tab you can create/edit pseudo-anonymous identities. \
			</p>                                                   \
			<p>Identities are used to securely identify your data: sign forum and channel posts,\
				and receive feedback using Retroshare built-in email system, post comments \
				after channel posts, etc.</p> \
			<p>  \
			Identities can optionally be signed by your Retroshare node's certificate.   \
			Signed identities are easier to trust but are easily linked to your node's IP address.  \
			</p>  \
			<p>  \
			Anonymous identities allow you to anonymously interact with other users. They cannot be   \
			spoofed, but noone can prove who really owns a given identity.  \
			</p> \
			") ;

	registerHelpButton(ui->helpButton, hlp_str) ;

	// load settings
	processSettings(true);

}

IdDialog::~IdDialog()
{
	// save settings
	processSettings(false);

	delete(ui);
	delete(mIdQueue);
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
			if(item != allItem && item != contactsItem)
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

		ui->idTreeWidget->insertTopLevelItem(0, contactsItem );  
		ui->idTreeWidget->insertTopLevelItem(0, allItem);

		Settings->beginGroup("IdDialog");
		allItem->setExpanded(Settings->value("ExpandAll", QVariant(true)).toBool());
		contactsItem->setExpanded(Settings->value("ExpandContacts", QVariant(true)).toBool());
    	Settings->endGroup();
    	
		if (fillIdListItem(vit->second, item, ownPgpId, accept)) {
			if(vit->second.mIsAContact)
				contactsItem->addChild(item);
			else
				allItem->addChild(item);
		}
	}
	
	/* count items */
	int itemCount = contactsItem->childCount() + allItem->childCount();
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

		ui->keyId_LEdit->setText("ERROR GETTING KEY!");

		return;
	}

	if (datavector.size() != 1)
	{
#ifdef ID_DEBUG
		std::cerr << "IdDialog::insertIdDetails() Invalid datavector size";
#endif

		mStateHelper->setActive(IDDIALOG_IDDETAILS, false);
		mStateHelper->clear(IDDIALOG_IDDETAILS);

		ui->keyId_LEdit->setText("INVALID DV SIZE");

		return;
	}

	mStateHelper->setActive(IDDIALOG_IDDETAILS, true);

	data = datavector[0];

	/* get GPG Details from rsPeers */
	RsPgpId ownPgpId  = rsPeers->getGPGOwnId();

	ui->nickname_LEdit->setText(QString::fromUtf8(data.mMeta.mGroupName.c_str()).left(RSID_MAXIMUM_NICKNAME_SIZE));
	ui->keyId_LEdit->setText(QString::fromStdString(data.mMeta.mGroupId.toStdString()));
	//ui->lineEdit_GpgHash->setText(QString::fromStdString(data.mPgpIdHash.toStdString()));
    ui->nodeId_LEdit->setText(QString::fromStdString(data.mPgpId.toStdString()));

    time_t now = time(NULL) ;
    ui->lastUsed_LEdit->setText(getHumanReadableDuration(now - data.mLastUsageTS)) ;
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
		ui->nodeName_LEdit->setText(QString::fromUtf8(details.name.c_str()));
	}
	else
	{
		if (data.mMeta.mGroupFlags & RSGXSID_GROUPFLAG_REALID)
		{
			ui->nodeName_LEdit->setText(tr("Unknown real name"));
		}
		else
		{
			ui->nodeName_LEdit->setText(tr("Anonymous Id"));
		}
	}

	if(data.mPgpId.isNull())
	{
		ui->nodeId_LEdit->hide() ;
		ui->nodeName_LEdit->hide() ;
		ui->nodeId_Label->hide() ;
		ui->nodeName_Label->hide() ;
	}
	else
	{
		ui->nodeId_LEdit->show() ;
		ui->nodeName_LEdit->show() ;
		ui->nodeId_Label->show() ;
		ui->nodeName_Label->show() ;
	}

    bool isLinkedToOwnPgpId = (data.mPgpKnown && (data.mPgpId == ownPgpId)) ;
    bool isOwnId = (data.mMeta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_ADMIN);

    if(isOwnId)
        if (isLinkedToOwnPgpId)
            ui->type_LEdit->setText(tr("Identity owned by you, linked to your Retroshare node")) ;
        else
            ui->type_LEdit->setText(tr("Anonymous identity, owned by you")) ;
    else if (data.mMeta.mGroupFlags & RSGXSID_GROUPFLAG_REALID)
    {
        if (data.mPgpKnown)
            if (rsPeers->isGPGAccepted(data.mPgpId))
                ui->type_LEdit->setText(tr("Linked to a friend Retroshare node")) ;
            else
                ui->type_LEdit->setText(tr("Linked to a known Retroshare node")) ;
        else
            ui->type_LEdit->setText(tr("Linked to unknown Retroshare node")) ;
    }
    else
        ui->type_LEdit->setText(tr("Anonymous identity")) ;

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

	RsGxsId id(ui->keyId_LEdit->text().toStdString());
    
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
	std::cerr << "IdDialog::modifyReputation() ID: " << id << " Mod: " << mod;
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

#ifdef ID_DEBUG
	std::cerr << "IdDialog::modifyReputation() queuingRequest(), token: " << token;
	std::cerr << std::endl;
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

void IdDialog::loadRequest(const TokenQueue * /*queue*/, const TokenRequest &req)
{
#ifdef ID_DEBUG
	std::cerr << "IdDialog::loadRequest() UserType: " << req.mUserType;
	std::cerr << std::endl;
#endif

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
	    if(*it == allItem || *it == contactsItem)
	    {
		    root_node_present = true ;
		    continue ;
	    }

	    uint32_t item_flags = (*it)->data(RSID_COL_KEYID,Qt::UserRole).toUInt() ;

        if(item_flags & RSID_FILTER_OWNED_BY_YOU)
		one_item_owned_by_you = true ;
        
        	std::cerr << "  item flags = " << item_flags << std::endl;
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

