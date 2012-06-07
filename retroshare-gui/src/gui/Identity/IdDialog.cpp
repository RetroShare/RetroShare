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

#include <iostream>
#include <sstream>

#include <QTimer>

/******
 * #define ID_DEBUG 1
 *****/


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
	mWaitingForRequest = false;
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
	uint32_t token;

	std::list<std::string> ids;
	ids.push_back(id);
		
	rsIdentity->requestIdentities(token, ids);
	lockForRequest(token, RSID_REQ_IDDETAILS);

}


void IdDialog::insertIdDetails(uint32_t token)
{
	/* get details from libretroshare */
	RsIdData data;
	if (!rsIdentity->getIdentity(token, data))
	{
		ui.lineEdit_KeyId->setText("ERROR GETTING KEY!");
		return;
	}

	/* get GPG Details from rsPeers */
	std::string gpgid  = rsPeers->getGPGOwnId();
	RsPeerDetails details;
	rsPeers->getPeerDetails(gpgid, details);
	
	ui.lineEdit_Nickname->setText(QString::fromStdString(data.mNickname));
	ui.lineEdit_KeyId->setText(QString::fromStdString(data.mKeyId));
	ui.lineEdit_GpgHash->setText(QString::fromStdString(data.mGpgIdHash));
	ui.lineEdit_GpgId->setText(QString::fromStdString(data.mGpgId));
	ui.lineEdit_GpgName->setText(QString::fromStdString(data.mGpgName));
	ui.lineEdit_GpgEmail->setText(QString::fromStdString(data.mGpgEmail));
	
	if (data.mIdType & RSID_RELATION_YOURSELF)
	{
		ui.radioButton_IdYourself->setChecked(true);
	}
	else if (data.mIdType & RSID_TYPE_PSEUDONYM)
	{
		ui.radioButton_IdPseudo->setChecked(true);
	}
	else if (data.mIdType & RSID_RELATION_FRIEND)
	{
		ui.radioButton_IdFriend->setChecked(true);
	}
	else if (data.mIdType & RSID_RELATION_FOF)
	{
		ui.radioButton_IdFOF->setChecked(true);
	}
	else 
	{
		ui.radioButton_IdOther->setChecked(true);
	}
	
	ui.pushButton_NewId->setEnabled(true);
	if (data.mIdType & RSID_RELATION_YOURSELF)
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

	checkForRequest();

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
	requestIdEdit(keyId);
}


void IdDialog::requestIdEdit(std::string &id)
{
	uint32_t token;

	std::list<std::string> ids;
	ids.push_back(id);
		
	rsIdentity->requestIdentities(token, ids);
	lockForRequest(token, RSID_REQ_IDEDIT);

}


void IdDialog::showIdEdit(uint32_t token)
{
	if (mEditDialog)
	{
		mEditDialog->setupExistingId(token);

		mEditDialog->show();
	}
}



void IdDialog::requestIdList()
{
	uint32_t token;

	rsIdentity->requestIdentityList(token);
	
	lockForRequest(token, RSID_REQ_IDLIST);
}


void IdDialog::requestIdData(std::list<std::string> &ids)
{
	uint32_t token;

	rsIdentity->requestIdentities(token, ids);
	
	lockForRequest(token, RSID_REQ_IDLISTDATA);
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

	//rsIdentity->getIdentityList(ids);

	//for(it = ids.begin(); it != ids.end(); it++)
	//{
	RsIdData data;
	while(rsIdentity->getIdentity(token, data))
	{

		/* do filtering */
		bool ok = false;
		if (acceptAll)
		{
			ok = true;
		}
		else if (data.mIdType & RSID_TYPE_PSEUDONYM)
		{
 			if (acceptPseudo)
			{
				ok = true;
			}

			if ((data.mIdType & RSID_RELATION_YOURSELF) && (acceptYourself))
			{
				ok = true;
			}
		}
		else
		{
			if (data.mIdType & RSID_RELATION_YOURSELF)
			{
 				if (acceptYourself)
				{
					ok = true;
				}
			}
			else if (data.mIdType & (RSID_RELATION_FRIEND | RSID_RELATION_FOF)) 
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

		if (!ok)
		{
			continue;
		}


		QTreeWidgetItem *item = new QTreeWidgetItem(NULL);
		item->setText(RSID_COL_NICKNAME, QString::fromStdString(data.mNickname));
		item->setText(RSID_COL_KEYID, QString::fromStdString(data.mKeyId));
		item->setText(RSID_COL_IDTYPE, QString::fromStdString(rsIdTypeToString(data.mIdType)));

                tree->addTopLevelItem(item);
	}

	// fix up buttons.
	updateSelection();
}




void IdDialog::lockForRequest(uint32_t token, uint32_t reqtype)
{

	/* store token for later results retrival */
	if (mWaitingForRequest)
	{
		std::cerr << "IdDialog::lockForRequest() LOCKED ALREADY - BIG ERROR";
		std::cerr << std::endl;
	}
	
	mWaitingForRequest = true;
	mRequestToken = token;
	mRequestType = reqtype;

	std::cerr << "IdDialog::lockForRequest() Token: " << token << " ReqType: " << reqtype;
	std::cerr << std::endl;

}


void IdDialog::checkForRequest()
{

	if (mWaitingForRequest)
	{
		/* check token */
		if (4 == rsIdentity->requestStatus(mRequestToken))
			loadRequest();
	}
}


void IdDialog::loadRequest()
{
	if (!mWaitingForRequest)
	{
		std::cerr << "IdDialog::loadRequest() NOT LOCKED - BIG ERROR";
		std::cerr << std::endl;
	}

	/* unlock gui */
	mWaitingForRequest = false;


	switch (mRequestType)
	{
		case RSID_REQ_IDLIST:
		{
			std::cerr << "IdDialog::loadRequest() RSID_REQ_IDLIST";
			std::cerr << std::endl;

			std::list<std::string> ids;
			rsIdentity->getIdentityList(mRequestToken, ids);
	
			/* request data - straight away */	

			requestIdData(ids);
		}
		break;

		case RSID_REQ_IDLISTDATA:
		{
			std::cerr << "IdDialog::loadRequest() RSID_REQ_IDLISTDATA";
			std::cerr << std::endl;
			insertIdList(mRequestToken);
		}
		break;


		case RSID_REQ_IDDETAILS:
		{
			std::cerr << "IdDialog::loadRequest() RSID_REQ_IDDETAILS";
			std::cerr << std::endl;
			insertIdDetails(mRequestToken);
		}
		break;

		case RSID_REQ_IDEDIT:
		{
			std::cerr << "IdDialog::loadRequest() RSID_REQ_IDEDIT";
			std::cerr << std::endl;
			showIdEdit(mRequestToken);
		}
		break;

		default:
		break;
	}
}







