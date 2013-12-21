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

#include "IdDialog.h"
#include "gui/gxs/GxsIdTreeWidgetItem.h"
#include "gui/common/UIStateHelper.h"

#include <retroshare/rspeers.h>
#include <retroshare/rsidentity.h>
#include "retroshare/rsgxsflags.h"

#include <iostream>

/******
 * #define ID_DEBUG 1
 *****/

// Data Requests.
#define IDDIALOG_IDLIST		1
#define IDDIALOG_IDDETAILS	2
#define IDDIALOG_REPLIST	3

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
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui.lineEdit_GpgHash);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui.lineEdit_GpgId);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui.lineEdit_GpgName);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui.pushButton_Reputation);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui.pushButton_Delete);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui.pushButton_EditId);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui.line_RatingOverall);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui.line_RatingImplicit);
	mStateHelper->addWidget(IDDIALOG_IDDETAILS, ui.line_RatingOwn);

	mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui.lineEdit_Nickname);
	mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui.lineEdit_GpgName);
	mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui.lineEdit_KeyId);
	mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui.lineEdit_GpgHash);
	mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui.lineEdit_GpgId);
	mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui.lineEdit_GpgName);
	mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui.line_RatingOverall);
	mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui.line_RatingImplicit);
	mStateHelper->addLoadPlaceholder(IDDIALOG_IDDETAILS, ui.line_RatingOwn);

	mStateHelper->addClear(IDDIALOG_IDDETAILS, ui.lineEdit_Nickname);
	mStateHelper->addClear(IDDIALOG_IDDETAILS, ui.lineEdit_GpgName);
	mStateHelper->addClear(IDDIALOG_IDDETAILS, ui.lineEdit_KeyId);
	mStateHelper->addClear(IDDIALOG_IDDETAILS, ui.lineEdit_GpgHash);
	mStateHelper->addClear(IDDIALOG_IDDETAILS, ui.lineEdit_GpgId);
	mStateHelper->addClear(IDDIALOG_IDDETAILS, ui.lineEdit_GpgName);
	mStateHelper->addClear(IDDIALOG_IDDETAILS, ui.line_RatingOverall);
	mStateHelper->addClear(IDDIALOG_IDDETAILS, ui.line_RatingImplicit);
	mStateHelper->addClear(IDDIALOG_IDDETAILS, ui.line_RatingOwn);

	mStateHelper->addWidget(IDDIALOG_REPLIST, ui.treeWidget_RepList);
	mStateHelper->addLoadPlaceholder(IDDIALOG_REPLIST, ui.treeWidget_RepList);
	mStateHelper->addClear(IDDIALOG_REPLIST, ui.treeWidget_RepList);

	/* Connect signals */
	connect(ui.pushButton_NewId, SIGNAL(clicked()), this, SLOT(addIdentity()));
	connect(ui.todoPushButton, SIGNAL(clicked()), this, SLOT(todo()));
	connect(ui.pushButton_EditId, SIGNAL(clicked()), this, SLOT(editIdentity()));
	connect( ui.treeWidget_IdList, SIGNAL(itemSelectionChanged()), this, SLOT(updateSelection()));

	connect(ui.filterComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(filterComboBoxChanged()));
	connect(ui.filterLineEdit, SIGNAL(textChanged(QString)), this, SLOT(filterChanged(QString)));

	/* Add filter types */
	ui.filterComboBox->addItem(tr("All"), RSID_FILTER_ALL);
	ui.filterComboBox->addItem(tr("Yourself"), RSID_FILTER_YOURSELF);
	ui.filterComboBox->addItem(tr("Friends / Friends of Friends"), RSID_FILTER_FRIENDS);
	ui.filterComboBox->addItem(tr("Others"), RSID_FILTER_OTHERS);
	ui.filterComboBox->addItem(tr("Pseudonyms"), RSID_FILTER_PSEUDONYMS);
	ui.filterComboBox->setCurrentIndex(0);

	/* Add filter actions */
	QTreeWidgetItem *headerItem = ui.treeWidget_IdList->headerItem();
	QString headerText = headerItem->text(RSID_COL_NICKNAME);
	ui.filterLineEdit->addFilter(QIcon(), headerText, RSID_COL_NICKNAME, QString("%1 %2").arg(tr("Search"), headerText));
	headerText = headerItem->text(RSID_COL_KEYID);
	ui.filterLineEdit->addFilter(QIcon(), headerItem->text(RSID_COL_KEYID), RSID_COL_KEYID, QString("%1 %2").arg(tr("Search"), headerText));

	/* Setup tree */
	ui.treeWidget_IdList->sortByColumn(RSID_COL_NICKNAME, Qt::AscendingOrder);

	mIdQueue = new TokenQueue(rsIdentity->getTokenService(), this);

	mStateHelper->setActive(IDDIALOG_IDDETAILS, false);
	mStateHelper->setActive(IDDIALOG_REPLIST, false);
}

void IdDialog::todo()
{
	QMessageBox::information(this, "Todo",
							 "<b>Open points:</b><ul>"
							 "<li>Delete ID"
							 "<li>Edit ID"
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
	std::string id;

	if (item)
	{
		id = item->text(RSID_COL_KEYID).toStdString();
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

bool IdDialog::fillIdListItem(const RsGxsIdGroup& data, QTreeWidgetItem *&item, const std::string &ownPgpId, int accept)
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
	item->setText(RSID_COL_KEYID, QString::fromStdString(data.mMeta.mGroupId));

	if (data.mMeta.mGroupFlags & RSGXSID_GROUPFLAG_REALID)
	{
		if (data.mPgpKnown)
		{
			RsPeerDetails details;
			rsPeers->getGPGDetails(data.mPgpId, details);
			item->setText(RSID_COL_IDTYPE, QString::fromUtf8(details.name.c_str()));
		}
	else
	{
			item->setText(RSID_COL_IDTYPE, tr("PGP Linked Id"));
	}
}
	else
	{
		item->setText(RSID_COL_IDTYPE, tr("Anon Id"));
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
		std::cerr << "IdDialog::insertIdList() Error getting GroupData";
		std::cerr << std::endl;

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

	std::string ownPgpId  = rsPeers->getGPGOwnId();

	/* Update existing and remove not existing items */
	QTreeWidgetItemIterator itemIterator(ui.treeWidget_IdList);
	QTreeWidgetItem *item = NULL;
	while ((item = *itemIterator) != NULL) {
		itemIterator++;

		for (vit = datavector.begin(); vit != datavector.end(); ++vit)
		{
			if (vit->mMeta.mGroupId == item->text(RSID_COL_KEYID).toStdString())
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

void IdDialog::requestIdDetails(std::string &id)
{
	mIdQueue->cancelActiveRequestTokens(IDDIALOG_IDDETAILS);

	if (id.empty())
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
	std::list<std::string> groupIds;
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
		std::cerr << "IdDialog::insertIdDetails() Invalid datavector size";

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
	std::string ownPgpId  = rsPeers->getGPGOwnId();

	ui.lineEdit_Nickname->setText(QString::fromUtf8(data.mMeta.mGroupName.c_str()));
	ui.lineEdit_KeyId->setText(QString::fromStdString(data.mMeta.mGroupId));
	ui.lineEdit_GpgHash->setText(QString::fromStdString(data.mPgpIdHash));
	ui.lineEdit_GpgId->setText(QString::fromStdString(data.mPgpId));

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

	bool isOwnId = (data.mPgpKnown && (data.mPgpId == ownPgpId)) || (data.mMeta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_ADMIN);

	if (isOwnId)
	{
		ui.radioButton_IdYourself->setChecked(true);
	}
	else if (data.mMeta.mGroupFlags & RSGXSID_GROUPFLAG_REALID)
	{
		if (data.mPgpKnown)
		{
			if (rsPeers->isGPGAccepted(data.mPgpId))
			{
				ui.radioButton_IdFriend->setChecked(true);
			}
			else
			{
				ui.radioButton_IdFOF->setChecked(true);
			}
		}
		else
		{
			ui.radioButton_IdOther->setChecked(true);
		}
	}
	else
	{
		ui.radioButton_IdPseudo->setChecked(true);
	}

	if (isOwnId)
	{
		mStateHelper->setWidgetEnabled(ui.pushButton_Reputation, false);
		// No Delete Ids yet!
		mStateHelper->setWidgetEnabled(ui.pushButton_Delete, /*true*/ false);
		mStateHelper->setWidgetEnabled(ui.pushButton_EditId, true);
	}
	else
	{
		// No Reputation yet!
		mStateHelper->setWidgetEnabled(ui.pushButton_Reputation, /*true*/ false);
		mStateHelper->setWidgetEnabled(ui.pushButton_Delete, false);
		mStateHelper->setWidgetEnabled(ui.pushButton_EditId, false);
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

	/* request network ratings */
	requestRepList(data.mMeta.mGroupId);
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

void IdDialog::editIdentity()
{
	QTreeWidgetItem *item = ui.treeWidget_IdList->currentItem();
	if (!item)
	{
		std::cerr << "IdDialog::editIdentity() Invalid item";
		std::cerr << std::endl;
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
	mStateHelper->setLoading(IDDIALOG_REPLIST, false);

	std::vector<RsGxsIdOpinion> opinions;
	std::vector<RsGxsIdOpinion>::iterator vit;
	if (!rsIdentity->getMsgData(token, opinions))
	{
		std::cerr << "IdDialog::insertRepList() Error getting Opinions";
		std::cerr << std::endl;

		mStateHelper->setActive(IDDIALOG_REPLIST, false);
		mStateHelper->clear(IDDIALOG_REPLIST);

		return;
	}

	mStateHelper->setActive(IDDIALOG_REPLIST, true);

	for(vit = opinions.begin(); vit != opinions.end(); vit++)
	{
		RsGxsIdOpinion &op = (*vit);
		GxsIdTreeWidgetItem *item = new GxsIdTreeWidgetItem();

		/* insert 4 columns */

		/* friend name */
		item->setId(op.mMeta.mGroupId, RSIDREP_COL_NAME);

		/* score */
		item->setText(RSIDREP_COL_OPINION, QString::number(op.getOpinion()));

		/* comment */
		item->setText(RSIDREP_COL_COMMENT, QString::fromUtf8(op.mComment.c_str()));

		/* local reputation */
		item->setText(RSIDREP_COL_REPUTATION, QString::number(op.getReputation()));

		ui.treeWidget_RepList->addTopLevelItem(item);
	}
}

void IdDialog::loadRequest(const TokenQueue */*queue*/, const TokenRequest &req)
{
	std::cerr << "IdDialog::loadRequest() UserType: " << req.mUserType;
	std::cerr << std::endl;

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

		default:
			std::cerr << "IdDialog::loadRequest() ERROR";
			std::cerr << std::endl;
			break;
	}
}
