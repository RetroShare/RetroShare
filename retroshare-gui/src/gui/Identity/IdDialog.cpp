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

#include "IdDialog.h"

#include <retroshare/rspeers.h>
#include <retroshare/rsidentity.h>
#include "gxs/rsgxsflags.h"

#include <iostream>
#include <sstream>

#include <QTimer>

/******
 * #define ID_DEBUG 1
 *****/

// Data Requests.
#define IDDIALOG_IDLIST		1
#define IDDIALOG_IDDETAILS	2

/****************************************************************
 */


#define RSID_COL_NICKNAME	0
#define RSID_COL_KEYID		1
#define RSID_COL_IDTYPE		2



#define RSID_REQ_IDLIST		1
#define RSID_REQ_IDDETAILS	2
#define RSID_REQ_IDLISTDATA	3
#define RSID_REQ_IDEDIT		4

/** Constructor */
IdDialog::IdDialog(QWidget *parent)
: MainPage(parent)
{
	ui.setupUi(this);

	mEditDialog = NULL;
	//mPulseSelected = NULL;

	ui.radioButton_ListAll->setChecked(true);
	connect( ui.pushButton_NewId, SIGNAL(clicked()), this, SLOT(OpenOrShowAddDialog()));
	connect( ui.pushButton_EditId, SIGNAL(clicked()), this, SLOT(OpenOrShowEditDialog()));
	connect( ui.treeWidget_IdList, SIGNAL(itemSelectionChanged()), this, SLOT(updateSelection()));

	connect( ui.radioButton_ListYourself, SIGNAL(toggled( bool ) ), this, SLOT(ListTypeToggled( bool ) ) );
	connect( ui.radioButton_ListFriends, SIGNAL(toggled( bool ) ), this, SLOT(ListTypeToggled( bool ) ) );
	connect( ui.radioButton_ListOthers, SIGNAL(toggled( bool ) ), this, SLOT(ListTypeToggled( bool ) ) );
	connect( ui.radioButton_ListPseudo, SIGNAL(toggled( bool ) ), this, SLOT(ListTypeToggled( bool ) ) );
	connect( ui.radioButton_ListAll, SIGNAL(toggled( bool ) ), this, SLOT(ListTypeToggled( bool ) ) );

	QTimer *timer = new QTimer(this);
	timer->connect(timer, SIGNAL(timeout()), this, SLOT(checkUpdate()));
	timer->start(1000);

	rsIdentity->generateDummyData();
	mIdQueue = new TokenQueue(rsIdentity->getTokenService(), this);

}

void IdDialog::ListTypeToggled(bool checked)
{
        if (checked)
        {
                requestIdList();
        }
}



void IdDialog::updateSelection()
{
	/* */
	QTreeWidgetItem *item = ui.treeWidget_IdList->currentItem();

	if (!item)
	{
		blankSelection();
	}
	else
	{
		std::string id = item->text(RSID_COL_KEYID).toStdString();
		requestIdDetails(id);
	}
}


void IdDialog::blankSelection()
{
	/* blank it all - and fix buttons */
	ui.lineEdit_Nickname->setText("");
	ui.lineEdit_KeyId->setText("");
	ui.lineEdit_GpgHash->setText("");
	ui.lineEdit_GpgId->setText("");
	ui.lineEdit_GpgName->setText("");
	ui.lineEdit_GpgEmail->setText("");

	ui.pushButton_Reputation->setEnabled(false);
	ui.pushButton_Delete->setEnabled(false);
	ui.pushButton_EditId->setEnabled(false);
	ui.pushButton_NewId->setEnabled(true);
}




void IdDialog::requestIdDetails(std::string &id)
{
	RsTokReqOptions opts;
        opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;

	uint32_t token;
	std::list<std::string> groupIds;
	groupIds.push_back(id);

        mIdQueue->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, groupIds, IDDIALOG_IDDETAILS);
}


void IdDialog::insertIdDetails(uint32_t token)
{
	/* get details from libretroshare */
	RsGxsIdGroup data;
	std::vector<RsGxsIdGroup> datavector;
	if (!rsIdentity->getGroupData(token, datavector))
	{
		ui.lineEdit_KeyId->setText("ERROR GETTING KEY!");
		return;
	}

	if (datavector.size() != 1)
	{
		std::cerr << "IdDialog::insertIdDetails() Invalid datavector size";
		ui.lineEdit_KeyId->setText("INVALID DV SIZE");
		return;
	}

	data = datavector[0];

	/* get GPG Details from rsPeers */
	std::string ownPgpId  = rsPeers->getGPGOwnId();
	
	ui.lineEdit_Nickname->setText(QString::fromStdString(data.mMeta.mGroupName));
	ui.lineEdit_KeyId->setText(QString::fromStdString(data.mMeta.mGroupId));
	ui.lineEdit_GpgHash->setText(QString::fromStdString(data.mPgpIdHash));
	ui.lineEdit_GpgId->setText(QString::fromStdString(data.mPgpId));

	if (data.mPgpKnown)
	{
		RsPeerDetails details;
		rsPeers->getGPGDetails(data.mPgpId, details);
		ui.lineEdit_GpgName->setText(QString::fromStdString(details.name));
		ui.lineEdit_GpgEmail->setText(QString::fromStdString(details.email));
	}
	else
	{
		if (data.mMeta.mGroupFlags & RSGXSID_GROUPFLAG_REALID)
		{
			ui.lineEdit_GpgName->setText("Unknown Real Name");
			ui.lineEdit_GpgEmail->setText("Unknown Email");
		}
		else
		{
			ui.lineEdit_GpgName->setText("Anonymous Id");
			ui.lineEdit_GpgEmail->setText("-- N/A --");
		}
	}

	bool isOwnId  = (data.mPgpKnown && (data.mPgpId == ownPgpId)) ||
		(data.mMeta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_ADMIN);

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

	ui.pushButton_NewId->setEnabled(true);

	if (isOwnId)
	{
		ui.pushButton_Reputation->setEnabled(false);
		ui.pushButton_Delete->setEnabled(true);
		ui.pushButton_EditId->setEnabled(true);
	}
	else
	{
		ui.pushButton_Reputation->setEnabled(true);
		ui.pushButton_Delete->setEnabled(false);
		ui.pushButton_EditId->setEnabled(false);
	}
}

void IdDialog::checkUpdate()
{
	/* update */
	if (!rsIdentity)
		return;

	if (rsIdentity->updated())
	{
		requestIdList();
	}
	return;
}


void IdDialog::OpenOrShowAddDialog()
{
	if (!mEditDialog)
	{
		mEditDialog = new IdEditDialog(NULL);
	}
	bool pseudo = false;
	mEditDialog->setupNewId(pseudo);

	mEditDialog->show();

}


void IdDialog::OpenOrShowEditDialog()
{
	if (!mEditDialog)
	{
		mEditDialog = new IdEditDialog(NULL);
	}


	/* */
	QTreeWidgetItem *item = ui.treeWidget_IdList->currentItem();

	if (!item)
	{
		std::cerr << "IdDialog::OpenOrShowEditDialog() Invalid item";
		std::cerr << std::endl;
		return;
	}

	std::string keyId = item->text(RSID_COL_KEYID).toStdString();

	if (mEditDialog)
	{
		mEditDialog->setupExistingId(keyId);
		mEditDialog->show();
	}
}

void IdDialog::requestIdList()
{
	RsTokReqOptions opts;
        opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;

	uint32_t token;
	std::list<std::string> groupIds;

        //mIdQueue->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, groupIds, IDDIALOG_IDLIST);
        mIdQueue->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, IDDIALOG_IDLIST);
}


void IdDialog::insertIdList(uint32_t token)
{
	QTreeWidget *tree = ui.treeWidget_IdList;

	tree->clear();

	std::list<std::string> ids;
	std::list<std::string>::iterator it;

	bool acceptAll = ui.radioButton_ListAll->isChecked();
	bool acceptPseudo = ui.radioButton_ListPseudo->isChecked();
	bool acceptYourself = ui.radioButton_ListYourself->isChecked();
	bool acceptFriends = ui.radioButton_ListFriends->isChecked();
	bool acceptOthers = ui.radioButton_ListOthers->isChecked();

	RsGxsIdGroup data;
	std::vector<RsGxsIdGroup> datavector;
	std::vector<RsGxsIdGroup>::iterator vit;
	if (!rsIdentity->getGroupData(token, datavector))
	{
		std::cerr << "IdDialog::insertIdList() Error getting GroupData";
		std::cerr << std::endl;
		return;
	}

	std::string ownPgpId  = rsPeers->getGPGOwnId();

	for(vit = datavector.begin(); vit != datavector.end(); vit++)
	{
		data = (*vit);

		bool isOwnId  = (data.mPgpKnown && (data.mPgpId == ownPgpId)) ||
			(data.mMeta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_ADMIN);

		/* do filtering */
		bool ok = false;
		if (acceptAll)
		{
			ok = true;
		}
		else if (data.mMeta.mGroupFlags & RSGXSID_GROUPFLAG_REALID)
		{
			if (isOwnId && acceptYourself)
			{
				ok = true;
			}
			else
			{
				if (data.mPgpKnown)
				{
					if (acceptFriends)
					{
						ok = true;
					}
				}
				else 
				{
					if (acceptOthers)
					{
						ok = true;
					}
				}
			}
		}
		else
		{
 			if (acceptPseudo)
			{
				ok = true;
			}

			if (isOwnId && acceptYourself)
			{
				ok = true;
			}
		}

		if (!ok)
		{
			continue;
		}

		QTreeWidgetItem *item = new QTreeWidgetItem();
		item->setText(RSID_COL_NICKNAME, QString::fromStdString(data.mMeta.mGroupName));
		item->setText(RSID_COL_KEYID, QString::fromStdString(data.mMeta.mGroupId));
		if (data.mMeta.mGroupFlags & RSGXSID_GROUPFLAG_REALID)
		{
			item->setText(RSID_COL_IDTYPE, "PGP Linked Id");
		}
		else
		{
			item->setText(RSID_COL_IDTYPE, "Anon Id");
		}

                tree->addTopLevelItem(item);
	}

	// fix up buttons.
	updateSelection();
}

void IdDialog::loadRequest(const TokenQueue *queue, const TokenRequest &req)
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
		
		default:
        		std::cerr << "IdDialog::loadRequest() ERROR";
        		std::cerr << std::endl;
			break;

	}
}

		


