/*
 * Retroshare Identity
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

#include "gui/Identity/IdEditDialog.h"
#include "util/TokenQueue.h"

#include <retroshare/rsidentity.h>
#include <retroshare/rspeers.h>

#include <iostream>

#define IDEDITDIALOG_LOADID	1

/** Constructor */
IdEditDialog::IdEditDialog(QWidget *parent)
: QWidget(parent)
{
	ui.setupUi(this);

	connect(ui.radioButton_GpgId, SIGNAL( toggled( bool ) ), this, SLOT( IdTypeToggled( bool ) ) );
	connect(ui.radioButton_Pseudo, SIGNAL( toggled( bool ) ), this, SLOT( IdTypeToggled( bool ) ) );
	connect(ui.pushButton_Update, SIGNAL( clicked( void ) ), this, SLOT( updateId( void ) ) );
	connect(ui.pushButton_Cancel, SIGNAL( clicked( void ) ), this, SLOT( cancelId( void ) ) );
#if 0
	mIdQueue = new TokenQueue(rsIdentity, this);
#endif
}

void IdEditDialog::setupNewId(bool pseudo)
{
	ui.checkBox_NewId->setChecked(true);
	ui.checkBox_NewId->setEnabled(false);
	ui.lineEdit_KeyId->setText("To Be Generated");
	ui.lineEdit_Nickname->setText("");
	ui.radioButton_GpgId->setEnabled(true);
	ui.radioButton_Pseudo->setEnabled(true);

	if (pseudo)
	{
		ui.radioButton_Pseudo->setChecked(true);
	}
	else
	{
		ui.radioButton_GpgId->setChecked(true);
	}

	// force - incase it wasn't triggered.
	IdTypeToggled(true);
	return;
}

void IdEditDialog::IdTypeToggled(bool checked)
{
	if (checked)
	{
		bool pseudo = ui.radioButton_Pseudo->isChecked();
		updateIdType(pseudo);
	}
}

void IdEditDialog::updateIdType(bool pseudo)
{
	if (pseudo)
	{
		ui.lineEdit_GpgHash->setText("N/A");
		ui.lineEdit_GpgId->setText("N/A");
		ui.lineEdit_GpgName->setText("N/A");
		ui.lineEdit_GpgEmail->setText("N/A");
	}
	else
	{
		/* get GPG Details from rsPeers */
		std::string gpgid  = rsPeers->getGPGOwnId();
		RsPeerDetails details;
		rsPeers->getPeerDetails(gpgid, details);

		ui.lineEdit_GpgId->setText(QString::fromStdString(gpgid));
		ui.lineEdit_GpgHash->setText("To Be Generated");
		ui.lineEdit_GpgName->setText(QString::fromStdString(details.name));
		ui.lineEdit_GpgEmail->setText(QString::fromStdString(details.email));
	}
	return;
}


void IdEditDialog::setupExistingId(std::string keyId)
{
        RsTokReqOptions opts;

        std::list<std::string> groupIds;
        groupIds.push_back(keyId);

        uint32_t token;
        mIdQueue->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, groupIds, IDEDITDIALOG_LOADID);
}


void IdEditDialog::loadExistingId(uint32_t token)
{
	ui.checkBox_NewId->setChecked(false);
	ui.checkBox_NewId->setEnabled(false);
	ui.radioButton_GpgId->setEnabled(false);
	ui.radioButton_Pseudo->setEnabled(false);

        /* get details from libretroshare */
        RsGxsIdGroup data;
#if 0
        if (!rsIdentity->getGroupData(token, data))
#endif
        {
		ui.lineEdit_KeyId->setText("ERROR KEYID INVALID");
		ui.lineEdit_Nickname->setText("");

		ui.lineEdit_GpgHash->setText("N/A");
		ui.lineEdit_GpgId->setText("N/A");
		ui.lineEdit_GpgName->setText("N/A");
		ui.lineEdit_GpgEmail->setText("N/A");
		return;
	}
	bool pseudo = (data.mIdType & RSID_TYPE_PSEUDONYM);

	if (pseudo)
	{
		ui.radioButton_Pseudo->setChecked(true);
	}
	else
	{
		ui.radioButton_GpgId->setChecked(true);
	}

	// DOES THIS TRIGGER ALREADY???
	// force - incase it wasn't triggered.
	IdTypeToggled(true);

	//ui.lineEdit_Nickname->setText(QString::fromStdString(data.mNickname));
        //ui.lineEdit_KeyId->setText(QString::fromStdString(data.mKeyId));
	ui.lineEdit_Nickname->setText(QString::fromStdString(data.mMeta.mGroupName));
        ui.lineEdit_KeyId->setText(QString::fromStdString(data.mMeta.mGroupId));

	if (pseudo)
	{
		ui.lineEdit_GpgHash->setText("N/A");
		ui.lineEdit_GpgId->setText("N/A");
		ui.lineEdit_GpgName->setText("N/A");
		ui.lineEdit_GpgEmail->setText("N/A");
	}
	else
	{
		ui.lineEdit_GpgHash->setText(QString::fromStdString(data.mGpgIdHash));

		if (data.mGpgIdKnown)
		{
			ui.lineEdit_GpgId->setText(QString::fromStdString(data.mGpgId));
			ui.lineEdit_GpgName->setText(QString::fromStdString(data.mGpgName));
			ui.lineEdit_GpgEmail->setText(QString::fromStdString(data.mGpgEmail));
		}
		else
		{
			ui.lineEdit_GpgId->setText("EXIST Unknown");
			ui.lineEdit_GpgName->setText("Unknown");
			ui.lineEdit_GpgEmail->setText("Unknown");
		}
	}

	return;
}

void IdEditDialog::updateId()
{
	RsGxsIdGroup rid;
	// Must set, Nickname, KeyId(if existing), mIdType, GpgId.

	rid.mMeta.mGroupName = ui.lineEdit_Nickname->text().toStdString();

	if (rid.mMeta.mGroupName.size() < 2)
	{
		std::cerr << "IdEditDialog::updateId() Nickname too short";
		std::cerr << std::endl;
		return;
	}

	rid.mIdType = RSID_RELATION_YOURSELF;
	if (ui.checkBox_NewId->isChecked())
	{
		rid.mMeta.mGroupId = "";
	}
	else
	{
		rid.mMeta.mGroupId = ui.lineEdit_KeyId->text().toStdString();
	}

	if (ui.radioButton_GpgId->isChecked())
	{
		rid.mIdType |= RSID_TYPE_REALID;

		rid.mGpgId = ui.lineEdit_GpgId->text().toStdString();
		rid.mGpgIdHash = ui.lineEdit_GpgHash->text().toStdString();
		rid.mGpgName = ui.lineEdit_GpgName->text().toStdString();
		rid.mGpgEmail = ui.lineEdit_GpgEmail->text().toStdString();
	}
	else
	{
		rid.mIdType |= RSID_TYPE_PSEUDONYM;

		rid.mGpgId = "";
		rid.mGpgIdHash = "";
		rid.mGpgName = "";
		rid.mGpgEmail = "";
	}

	// TODO.
	//rsIdentity->updateIdentity(rid);

	hide();
	return;
}

void IdEditDialog::cancelId()
{
	hide();
	return;
}

void IdEditDialog::clearDialog()
{
	return;
}


void IdEditDialog::loadRequest(const TokenQueue *queue, const TokenRequest &req)
{
	std::cerr << "IdDialog::loadRequest() UserType: " << req.mUserType;
	std::cerr << std::endl;
	
	// only one here!
	loadExistingId(req.mToken);
}

	
