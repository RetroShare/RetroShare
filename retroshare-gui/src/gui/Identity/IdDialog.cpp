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
#include "gui/gxs/GxsIdDetails.h"
#include "gui/gxs/GxsIdTreeWidgetItem.h"
#include "gui/common/UIStateHelper.h"
#include "gui/chat/ChatDialog.h"

#include <retroshare/rspeers.h>
//#include <retroshare/rsidentity.h> //On header
#include "retroshare/rsgxsflags.h"
#include "retroshare/rsmsgs.h"

#include <iostream>

/******
 * #define ID_DEBUG 1
 *****/

// Data Requests.
#define IDDIALOG_IDLIST		1
#define IDDIALOG_IDDETAILS	2
#define IDDIALOG_REPLIST	3
#define IDDIALOG_REFRESH	4

/****************************************************************
 */

#define RSID_COL_NICKNAME   0
#define RSID_COL_KEYID      1
#define RSID_COL_IDTYPE     2

#define RSIDREP_COL_NAME       0
#define RSIDREP_COL_OPINION    1
#define RSIDREP_COL_COMMENT    2
#define RSIDREP_COL_REPUTATION 3

#define RSID_FILTER_YOURSELF     0x0001
#define RSID_FILTER_FRIENDS      0x0002
#define RSID_FILTER_OTHERS       0x0004
#define RSID_FILTER_PSEUDONYMS   0x0008
#define RSID_FILTER_ALL          0xffff

#define IMAGE_EDIT                 ":/images/edit_16.png"

/** Constructor */
IdDialog::IdDialog(QWidget *parent)
: RsGxsUpdateBroadcastPage(rsIdentity, parent)
{
	ui.setupUi(this);

	mIdQueue = NULL;

	/* Setup UI helper */
	mStateHelper = new UIStateHelper(this);
	mStateHelper->addWidget(IDDIALOG_IDLIST, ui.treeWidget_IdList);
	mStateHelper->addLoadPlaceholder(IDDIALOG_IDLIST, ui.treeWidget_IdList, false);
	mStateHelper->addClear(IDDIALOG_IDLIST, ui.treeWidget_IdList);

	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui.lineEdit_Nickname);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui.lineEdit_KeyId);
//	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui.lineEdit_GpgHash);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui.lineEdit_GpgId);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui.lineEdit_GpgName);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui.lineEdit_Type);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui.toolButton_Reputation);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui.line_RatingOverall);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui.line_RatingImplicit);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui.line_RatingOwn);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui.line_RatingPeers);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui.repModButton);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui.repMod_Accept);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui.repMod_Ban);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui.repMod_Negative);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui.repMod_Positive);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui.repMod_Custom);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui.repMod_spinBox);

	mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui.lineEdit_Nickname);
	mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui.lineEdit_GpgName);
	mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui.lineEdit_KeyId);
//	mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui.lineEdit_GpgHash);
	mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui.lineEdit_GpgId);
	mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui.lineEdit_Type);
	mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui.lineEdit_GpgName);
	mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui.line_RatingOverall);
	mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui.line_RatingImplicit);
	mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui.line_RatingOwn);
	mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui.line_RatingPeers);

	mStateHelper->addClear(IDDIALOG_IDDETAILS, ui.lineEdit_Nickname);
	mStateHelper->addClear(IDDIALOG_IDDETAILS, ui.lineEdit_KeyId);
//	mStateHelper->addClear(IDDIALOG_IDDETAILS, ui.lineEdit_GpgHash);
	mStateHelper->addClear(IDDIALOG_IDDETAILS, ui.lineEdit_GpgId);
	mStateHelper->addClear(IDDIALOG_IDDETAILS, ui.lineEdit_Type);
	mStateHelper->addClear(IDDIALOG_IDDETAILS, ui.lineEdit_GpgName);
	mStateHelper->addClear(IDDIALOG_IDDETAILS, ui.line_RatingOverall);
	mStateHelper->addClear(IDDIALOG_IDDETAILS, ui.line_RatingImplicit);
	mStateHelper->addClear(IDDIALOG_IDDETAILS, ui.line_RatingOwn);
	mStateHelper->addClear(IDDIALOG_IDDETAILS, ui.line_RatingPeers);

	//mStateHelper->addWidget(IDDIALOG_REPLIST, ui.treeWidget_RepList);
	//mStateHelper->addLoadPlaceholder(IDDIALOG_REPLIST, ui.treeWidget_RepList);
	//mStateHelper->addClear(IDDIALOG_REPLIST, ui.treeWidget_RepList);

	/* Connect signals */
	connect(ui.toolButton_NewId, SIGNAL(clicked()), this, SLOT(addIdentity()));
	connect(ui.todoPushButton, SIGNAL(clicked()), this, SLOT(todo()));
	//connect(ui.toolButton_Delete, SIGNAL(clicked()), this, SLOT(removeIdentity()));
	//connect(ui.toolButton_EditId, SIGNAL(clicked()), this, SLOT(editIdentity()));
	connect(ui.removeIdentity, SIGNAL(triggered()), this, SLOT(removeIdentity()));
	connect(ui.editIdentity, SIGNAL(triggered()), this, SLOT(editIdentity()));
	connect(ui.chatIdentity, SIGNAL(triggered()), this, SLOT(chatIdentity()));

	connect(ui.treeWidget_IdList, SIGNAL(itemSelectionChanged()), this, SLOT(updateSelection()));
	connect(ui.treeWidget_IdList, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(IdListCustomPopupMenu(QPoint)));

	connect(ui.filterComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(filterComboBoxChanged()));
	connect(ui.filterLineEdit, SIGNAL(textChanged(QString)), this, SLOT(filterChanged(QString)));
	connect(ui.repModButton, SIGNAL(clicked()), this, SLOT(modifyReputation()));

	ui.headerFrame->setHeaderImage(QPixmap(":/images/identity/identity_64.png"));
	ui.headerFrame->setHeaderText(tr("Identities"));

	/* Add filter types */
	ui.filterComboBox->addItem(tr("All"), RSID_FILTER_ALL);
	ui.filterComboBox->addItem(tr("Owned by you"), RSID_FILTER_YOURSELF);
	ui.filterComboBox->addItem(tr("Owned by neighbor nodes"), RSID_FILTER_FRIENDS);
	ui.filterComboBox->addItem(tr("Owned by distant nodes"), RSID_FILTER_OTHERS);
	ui.filterComboBox->addItem(tr("Anonymous"), RSID_FILTER_PSEUDONYMS);
	ui.filterComboBox->setCurrentIndex(0);

	/* Add filter actions */
	QTreeWidgetItem *headerItem = ui.treeWidget_IdList->headerItem();
	QString headerText = headerItem->text(RSID_COL_NICKNAME);
	ui.filterLineEdit->addFilter(QIcon(), headerText, RSID_COL_NICKNAME, QString("%1 %2").arg(tr("Search"), headerText));
	headerText = headerItem->text(RSID_COL_KEYID);
	ui.filterLineEdit->addFilter(QIcon(), headerItem->text(RSID_COL_KEYID), RSID_COL_KEYID, QString("%1 %2").arg(tr("Search"), headerText));
	
	    
   /* Set header resize modes and initial section sizes ID TreeView*/
   QHeaderView * _idheader = ui.treeWidget_IdList->header () ;
   _idheader->resizeSection ( RSID_COL_NICKNAME, 170 );

	/* Setup tree */
	ui.treeWidget_IdList->sortByColumn(RSID_COL_NICKNAME, Qt::AscendingOrder);
	ui.treeWidget_IdList->setContextMenuPolicy(Qt::CustomContextMenu) ;


	mIdQueue = new TokenQueue(rsIdentity->getTokenService(), this);

	mStateHelper->setActive(IDDIALOG_IDDETAILS, false);
	mStateHelper->setActive(IDDIALOG_REPLIST, false);

	// Hiding RepList until that part is finished.
	//ui.treeWidget_RepList->setVisible(false);
	ui.toolButton_Reputation->setVisible(false);
#ifndef UNFINISHED
	ui.todoPushButton->hide() ;
#endif

	QString hlp_str = tr(
			" <h1><img width=\"32\" src=\":/images/64px_help.png\">&nbsp;&nbsp;Identities</h1>    \
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

	registerHelpButton(ui.helpButton, hlp_str) ;
}

IdDialog::~IdDialog()
{
	delete(mIdQueue);
}

void IdDialog::todo()
{
	QMessageBox::information(this, "Todo",
							 "<b>Open points:</b><ul>"
//							 "<li>Delete ID"
							 "<li>Reputation"
							 "<li>Load/save settings"
							 "</ul>");
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
	QTreeWidgetItem *item = ui.treeWidget_IdList->currentItem();
    RsGxsGroupId id;

	if (item)
	{
        id = RsGxsGroupId(item->text(RSID_COL_KEYID).toStdString());
	}

	requestIdDetails(id);
}

void IdDialog::requestIdList()
{
	if (!mIdQueue)
		return;

	mStateHelper->setLoading(IDDIALOG_IDLIST, true);
	mStateHelper->setLoading(IDDIALOG_IDDETAILS, true);
	mStateHelper->setLoading(IDDIALOG_REPLIST, true);

	mIdQueue->cancelActiveRequestTokens(IDDIALOG_IDLIST);

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;

	uint32_t token;

	mIdQueue->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, IDDIALOG_IDLIST);
}

bool IdDialog::fillIdListItem(const RsGxsIdGroup& data, QTreeWidgetItem *&item, const RsPgpId &ownPgpId, int accept)
{
	bool isOwnId = (data.mPgpKnown && (data.mPgpId == ownPgpId)) || (data.mMeta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_ADMIN);

	/* do filtering */
	bool ok = false;
	if (data.mMeta.mGroupFlags & RSGXSID_GROUPFLAG_REALID)
	{
		if (isOwnId && (accept & RSID_FILTER_YOURSELF))
		{
			ok = true;
		}
		else
		{
			if (data.mPgpKnown)
			{
				if (accept & RSID_FILTER_FRIENDS)
				{
					ok = true;
				}
			}
			else
			{
				if (accept & RSID_FILTER_OTHERS)
				{
					ok = true;
				}
			}
		}
	}
	else
	{
		if (accept & RSID_FILTER_PSEUDONYMS)
		{
			ok = true;
		}

		if (isOwnId && (accept & RSID_FILTER_YOURSELF))
		{
			ok = true;
		}
	}

	if (!ok)
	{
		return false;
	}

	if (!item)
	{
		item = new QTreeWidgetItem();
	}
	item->setText(RSID_COL_NICKNAME, QString::fromUtf8(data.mMeta.mGroupName.c_str()));
    item->setText(RSID_COL_KEYID, QString::fromStdString(data.mMeta.mGroupId.toStdString()));

		 QFont font("Courier New",10,50,false);
		 font.setStyleHint(QFont::TypeWriter,QFont::PreferMatch);
		 item->setFont(RSID_COL_KEYID,font) ;

	 if(isOwnId)
	 {
		 QFont font = item->font(RSID_COL_NICKNAME) ;
		 font.setWeight(QFont::Bold) ;
		 item->setFont(RSID_COL_NICKNAME,font) ;
		 item->setFont(RSID_COL_IDTYPE,font) ;

		 font = item->font(RSID_COL_KEYID) ;
		 font.setWeight(QFont::Bold) ;
		 item->setFont(RSID_COL_KEYID,font) ;

		 item->setToolTip(RSID_COL_NICKNAME,tr("This identity is owned by you")) ;
		 item->setToolTip(RSID_COL_KEYID   ,tr("This identity is owned by you")) ;
		 item->setToolTip(RSID_COL_IDTYPE  ,tr("This identity is owned by you")) ;

	 }

	QPixmap pixmap = QPixmap::fromImage(GxsIdDetails::makeDefaultIcon(RsGxsId(data.mMeta.mGroupId))) ;
#ifdef ID_DEBUG
	std::cerr << "Setting item image : " << pixmap.width() << " x " << pixmap.height() << std::endl;
#endif
	item->setIcon(RSID_COL_NICKNAME, QIcon(pixmap));

	if (data.mMeta.mGroupFlags & RSGXSID_GROUPFLAG_REALID)
	{
		if (data.mPgpKnown)
		{
			RsPeerDetails details;
			rsPeers->getGPGDetails(data.mPgpId, details);
			item->setText(RSID_COL_IDTYPE, QString::fromUtf8(details.name.c_str()));
			item->setToolTip(RSID_COL_IDTYPE,QString::fromStdString(data.mPgpId.toStdString())) ;
		}
		else
		{
			item->setText(RSID_COL_IDTYPE, tr("Unknown PGP key"));
			item->setToolTip(RSID_COL_IDTYPE,tr("Unknown key ID")) ;
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

	int accept = ui.filterComboBox->itemData(ui.filterComboBox->currentIndex()).toInt();

	RsGxsIdGroup data;
	std::vector<RsGxsIdGroup> datavector;
	std::vector<RsGxsIdGroup>::iterator vit;
	if (!rsIdentity->getGroupData(token, datavector))
	{
#ifdef ID_DEBUG
		std::cerr << "IdDialog::insertIdList() Error getting GroupData";
		std::cerr << std::endl;
#endif

		mStateHelper->setLoading(IDDIALOG_IDDETAILS, false);
		mStateHelper->setLoading(IDDIALOG_REPLIST, false);
		mStateHelper->setActive(IDDIALOG_IDLIST, false);
		mStateHelper->setActive(IDDIALOG_IDDETAILS, false);
		mStateHelper->setActive(IDDIALOG_REPLIST, false);
		mStateHelper->clear(IDDIALOG_IDLIST);
		mStateHelper->clear(IDDIALOG_IDDETAILS);
		mStateHelper->clear(IDDIALOG_REPLIST);

		return;
	}

	mStateHelper->setActive(IDDIALOG_IDLIST, true);

	RsPgpId ownPgpId  = rsPeers->getGPGOwnId();

	/* Update existing and remove not existing items */
	QTreeWidgetItemIterator itemIterator(ui.treeWidget_IdList);
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
			ui.treeWidget_IdList->addTopLevelItem(item);
		}
	}

	filterIds();

	// fix up buttons.
	updateSelection();
}

void IdDialog::requestIdDetails(RsGxsGroupId &id)
{
	mIdQueue->cancelActiveRequestTokens(IDDIALOG_IDDETAILS);

    if (id.isNull())
	{
		mStateHelper->setActive(IDDIALOG_IDDETAILS, false);
		mStateHelper->setActive(IDDIALOG_REPLIST, false);
		mStateHelper->setLoading(IDDIALOG_IDDETAILS, false);
		mStateHelper->setLoading(IDDIALOG_REPLIST, false);
		mStateHelper->clear(IDDIALOG_IDDETAILS);
		mStateHelper->clear(IDDIALOG_REPLIST);

		return;
	}

	mStateHelper->setLoading(IDDIALOG_IDDETAILS, true);
	mStateHelper->setLoading(IDDIALOG_REPLIST, true);

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;

	uint32_t token;
    std::list<RsGxsGroupId> groupIds;
	groupIds.push_back(id);

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
		mStateHelper->setActive(IDDIALOG_REPLIST, false);
		mStateHelper->setLoading(IDDIALOG_REPLIST, false);
		mStateHelper->clear(IDDIALOG_IDDETAILS);
		mStateHelper->clear(IDDIALOG_REPLIST);

		ui.lineEdit_KeyId->setText("ERROR GETTING KEY!");

		return;
	}

	if (datavector.size() != 1)
	{
#ifdef ID_DEBUG
		std::cerr << "IdDialog::insertIdDetails() Invalid datavector size";
#endif

		mStateHelper->setActive(IDDIALOG_IDDETAILS, false);
		mStateHelper->setActive(IDDIALOG_REPLIST, false);
		mStateHelper->setLoading(IDDIALOG_REPLIST, false);
		mStateHelper->clear(IDDIALOG_IDDETAILS);
		mStateHelper->clear(IDDIALOG_REPLIST);

		ui.lineEdit_KeyId->setText("INVALID DV SIZE");

		return;
	}

	mStateHelper->setActive(IDDIALOG_IDDETAILS, true);

	data = datavector[0];

	/* get GPG Details from rsPeers */
	RsPgpId ownPgpId  = rsPeers->getGPGOwnId();

	ui.lineEdit_Nickname->setText(QString::fromUtf8(data.mMeta.mGroupName.c_str()));
  ui.lineEdit_KeyId->setText(QString::fromStdString(data.mMeta.mGroupId.toStdString()));
	//ui.lineEdit_GpgHash->setText(QString::fromStdString(data.mPgpIdHash.toStdString()));
	ui.lineEdit_GpgId->setText(QString::fromStdString(data.mPgpId.toStdString()));
	
	ui.headerFrame->setHeaderText(QString::fromUtf8(data.mMeta.mGroupName.c_str()));

	QPixmap pix = QPixmap::fromImage(GxsIdDetails::makeDefaultIcon(RsGxsId(data.mMeta.mGroupId))) ;
#ifdef ID_DEBUG
    std::cerr << "Setting header frame image : " << pix.width() << " x " << pix.height() << std::endl;
#endif
	ui.headerFrame->setHeaderImage(pix);

	if (data.mPgpKnown)
	{
		RsPeerDetails details;
		rsPeers->getGPGDetails(data.mPgpId, details);
		ui.lineEdit_GpgName->setText(QString::fromUtf8(details.name.c_str()));
	}
	else
	{
		if (data.mMeta.mGroupFlags & RSGXSID_GROUPFLAG_REALID)
		{
			ui.lineEdit_GpgName->setText(tr("Unknown real name"));
		}
		else
		{
			ui.lineEdit_GpgName->setText(tr("Anonymous Id"));
		}
	}

	if(data.mPgpId.isNull())
	{
		ui.lineEdit_GpgId->hide() ;
		ui.lineEdit_GpgName->hide() ;
		ui.PgpId_LB->hide() ;
		ui.PgpName_LB->hide() ;
	}
	else
	{
		ui.lineEdit_GpgId->show() ;
		ui.lineEdit_GpgName->show() ;
		ui.PgpId_LB->show() ;
		ui.PgpName_LB->show() ;
	}

	bool isOwnId = (data.mPgpKnown && (data.mPgpId == ownPgpId)) || (data.mMeta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_ADMIN);

	if(isOwnId)
		if (data.mPgpKnown)
			ui.lineEdit_Type->setText(tr("Identity owned by you, linked to your Retroshare node")) ;
		else
			ui.lineEdit_Type->setText(tr("Anonymous identity, owned by you")) ;
	else if (data.mMeta.mGroupFlags & RSGXSID_GROUPFLAG_REALID)
	{
		if (data.mPgpKnown)
			if (rsPeers->isGPGAccepted(data.mPgpId))
				ui.lineEdit_Type->setText(tr("Owned by a friend Retroshare node")) ;
			else
				ui.lineEdit_Type->setText(tr("Owned by 2-hops Retroshare node")) ;
		else
			ui.lineEdit_Type->setText(tr("Owned by unknown Retroshare node")) ;
	}
	else
		ui.lineEdit_Type->setText(tr("Anonymous identity")) ;

//	if (isOwnId)
//	{
//		ui.radioButton_IdYourself->setChecked(true);
//	}
//	else if (data.mMeta.mGroupFlags & RSGXSID_GROUPFLAG_REALID)
//	{
//		if (data.mPgpKnown)
//		{
//			if (rsPeers->isGPGAccepted(data.mPgpId))
//			{
//				ui.radioButton_IdFriend->setChecked(true);
//			}
//			else
//			{
//				ui.radioButton_IdFOF->setChecked(true);
//			}
//		}
//		else
//		{
//			ui.radioButton_IdOther->setChecked(true);
//		}
//	}
//	else
//	{
//		ui.radioButton_IdPseudo->setChecked(true);
//	}

	if (isOwnId)
	{
		mStateHelper->setWidgetEnabled(ui.toolButton_Reputation, false);
		ui.editIdentity->setEnabled(true);
		ui.removeIdentity->setEnabled(true);
		ui.chatIdentity->setEnabled(false);
	}
	else
	{
		// No Reputation yet!
		mStateHelper->setWidgetEnabled(ui.toolButton_Reputation, /*true*/ false);
		ui.editIdentity->setEnabled(false);
		ui.removeIdentity->setEnabled(false);
		ui.chatIdentity->setEnabled(true);
	}

	/* now fill in the reputation information */
	ui.line_RatingOverall->setText("Overall Rating TODO");
	ui.line_RatingOwn->setText("Own Rating TODO");

	if (data.mPgpKnown)
	{
		ui.line_RatingImplicit->setText("+50 Known PGP");
	}
	else if (data.mMeta.mGroupFlags & RSGXSID_GROUPFLAG_REALID)
	{
		ui.line_RatingImplicit->setText("+10 UnKnown PGP");
	}
	else
	{
		ui.line_RatingImplicit->setText("+5 Anon Id");
	}

	{
		QString rating = QString::number(data.mReputation.mOverallScore);
		ui.line_RatingOverall->setText(rating);
	}

	{
		QString rating = QString::number(data.mReputation.mIdScore);
		ui.line_RatingImplicit->setText(rating);
	}

	{
		QString rating = QString::number(data.mReputation.mOwnOpinion);
		ui.line_RatingOwn->setText(rating);
	}

	{
		QString rating = QString::number(data.mReputation.mPeerOpinion);
		ui.line_RatingPeers->setText(rating);
	}

	/* request network ratings */
	// Removing this for the moment.
	// requestRepList(data.mMeta.mGroupId);
}

void IdDialog::modifyReputation()
{
#ifdef ID_DEBUG
	std::cerr << "IdDialog::modifyReputation()";
	std::cerr << std::endl;
#endif

	RsGxsId id(ui.lineEdit_KeyId->text().toStdString());

	int mod = 0;
	if (ui.repMod_Accept->isChecked())
	{
		mod += 100;
	}
	else if (ui.repMod_Positive->isChecked())
	{
		mod += 10;
	}
	else if (ui.repMod_Negative->isChecked())
	{
		mod += -10;
	}
	else if (ui.repMod_Ban->isChecked())
	{
		mod += -100;
	}
	else if (ui.repMod_Custom->isChecked())
	{
		mod += ui.repMod_spinBox->value();
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
	
void IdDialog::updateDisplay(bool /*complete*/)
{
	/* Update identity list */
	requestIdList();
}

void IdDialog::addIdentity()
{
	IdEditDialog dlg(this);
	dlg.setupNewId(false);
	dlg.exec();
}

void IdDialog::removeIdentity()
{
	QTreeWidgetItem *item = ui.treeWidget_IdList->currentItem();
	if (!item)
	{
#ifdef ID_DEBUG
		std::cerr << "IdDialog::editIdentity() Invalid item";
		std::cerr << std::endl;
#endif
		return;
	}
	
		QString queryWrn;
	  queryWrn.clear();
	  queryWrn.append(tr("Do you really want to delete this Identity ?"));
	
	if ((QMessageBox::question(this, tr("Really delete? "),queryWrn,QMessageBox::Yes|QMessageBox::No, QMessageBox::Yes))== QMessageBox::Yes)
		{
      std::string keyId = item->text(RSID_COL_KEYID).toStdString();

      uint32_t dummyToken = 0;
      RsGxsIdGroup group;
      group.mMeta.mGroupId=RsGxsGroupId(keyId);
      rsIdentity->deleteIdentity(dummyToken, group);
		}
		else
			return;



}

void IdDialog::editIdentity()
{
	QTreeWidgetItem *item = ui.treeWidget_IdList->currentItem();
	if (!item)
	{
#ifdef ID_DEBUG
		std::cerr << "IdDialog::editIdentity() Invalid item";
		std::cerr << std::endl;
#endif
		return;
	}

	std::string keyId = item->text(RSID_COL_KEYID).toStdString();

	IdEditDialog dlg(this);
	dlg.setupExistingId(keyId);
	dlg.exec();
}

void IdDialog::filterIds()
{
	int filterColumn = ui.filterLineEdit->currentFilter();
	QString text = ui.filterLineEdit->text();

	ui.treeWidget_IdList->filterItems(filterColumn, text);
}

void IdDialog::requestRepList(const RsGxsGroupId &aboutId)
{
	mStateHelper->setLoading(IDDIALOG_REPLIST, true);

	mIdQueue->cancelActiveRequestTokens(IDDIALOG_REPLIST);

	std::list<RsGxsGroupId> groupIds;
	groupIds.push_back(aboutId);

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;

	uint32_t token;
	mIdQueue->requestMsgInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, groupIds, IDDIALOG_REPLIST);
}

void IdDialog::insertRepList(uint32_t token)
{
	Q_UNUSED(token)
	mStateHelper->setLoading(IDDIALOG_REPLIST, false);
#if 0

	std::vector<RsGxsIdOpinion> opinions;
	std::vector<RsGxsIdOpinion>::iterator vit;
	if (!rsIdentity->getMsgData(token, opinions))
	{
#ifdef ID_DEBUG
		std::cerr << "IdDialog::insertRepList() Error getting Opinions";
		std::cerr << std::endl;
#endif

		mStateHelper->setActive(IDDIALOG_REPLIST, false);
		mStateHelper->clear(IDDIALOG_REPLIST);

		return;
	}

	for(vit = opinions.begin(); vit != opinions.end(); ++vit)
	{
		RsGxsIdOpinion &op = (*vit);
		GxsIdTreeWidgetItem *item = new GxsIdTreeWidgetItem();

		/* insert 4 columns */

		/* friend name */
        item->setId(op.mMeta.mAuthorId, RSIDREP_COL_NAME);

		/* score */
		item->setText(RSIDREP_COL_OPINION, QString::number(op.getOpinion()));

		/* comment */
		item->setText(RSIDREP_COL_COMMENT, QString::fromUtf8(op.mComment.c_str()));

		/* local reputation */
		item->setText(RSIDREP_COL_REPUTATION, QString::number(op.getReputation()));

		ui.treeWidget_RepList->addTopLevelItem(item);
	}
#endif
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
			updateDisplay(true);
			break;
		default:
			std::cerr << "IdDialog::loadRequest() ERROR";
			std::cerr << std::endl;
			break;
	}
}

void IdDialog::IdListCustomPopupMenu( QPoint )
{
    //Disable by default, will be enable by insertIdDetails()
    ui.removeIdentity->setEnabled(false);
    ui.editIdentity->setEnabled(false);

    QMenu contextMnu( this );

    contextMnu.addAction(ui.editIdentity);
	 contextMnu.addAction(ui.removeIdentity);
	 contextMnu.addAction(ui.chatIdentity);

    contextMnu.exec(QCursor::pos());
}

void IdDialog::chatIdentity()
{
	QTreeWidgetItem *item = ui.treeWidget_IdList->currentItem();
	if (!item)
	{
		std::cerr << "IdDialog::editIdentity() Invalid item";
		std::cerr << std::endl;
		return;
	}

	std::string keyId = item->text(RSID_COL_KEYID).toStdString();

	uint32_t error_code ;

	if(!rsMsgs->initiateDistantChatConnexion(RsGxsId(keyId), error_code))
		QMessageBox::information(NULL,"Distant cannot work","Distant chat refused with this peer. Reason: "+QString::number(error_code)) ;
}


