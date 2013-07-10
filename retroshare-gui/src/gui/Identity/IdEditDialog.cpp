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
#include "gui/common/UIStateHelper.h"
#include "util/TokenQueue.h"

#include <retroshare/rsidentity.h>
#include <retroshare/rspeers.h>

#include <iostream>

#define IDEDITDIALOG_LOADID	1

/** Constructor */
IdEditDialog::IdEditDialog(QWidget *parent)
: QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint)
{
	mIsNew = true;

	ui.setupUi(this);

	/* Setup UI helper */
	mStateHelper = new UIStateHelper(this);

	mStateHelper->addWidget(IDEDITDIALOG_LOADID, ui.lineEdit_Nickname);
	mStateHelper->addWidget(IDEDITDIALOG_LOADID, ui.lineEdit_KeyId);
	mStateHelper->addWidget(IDEDITDIALOG_LOADID, ui.lineEdit_GpgHash);
	mStateHelper->addWidget(IDEDITDIALOG_LOADID, ui.lineEdit_GpgId);
	mStateHelper->addWidget(IDEDITDIALOG_LOADID, ui.lineEdit_GpgName);
	mStateHelper->addWidget(IDEDITDIALOG_LOADID, ui.radioButton_GpgId);
	mStateHelper->addWidget(IDEDITDIALOG_LOADID, ui.radioButton_Pseudo);

	mStateHelper->addLoadPlaceholder(IDEDITDIALOG_LOADID, ui.lineEdit_Nickname);
	mStateHelper->addLoadPlaceholder(IDEDITDIALOG_LOADID, ui.lineEdit_GpgName);
	mStateHelper->addLoadPlaceholder(IDEDITDIALOG_LOADID, ui.lineEdit_KeyId);
	mStateHelper->addLoadPlaceholder(IDEDITDIALOG_LOADID, ui.lineEdit_GpgHash);
	mStateHelper->addLoadPlaceholder(IDEDITDIALOG_LOADID, ui.lineEdit_GpgId);
	mStateHelper->addLoadPlaceholder(IDEDITDIALOG_LOADID, ui.lineEdit_GpgName);

	/* Connect signals */
	connect(ui.radioButton_GpgId, SIGNAL(toggled(bool)), this, SLOT(idTypeToggled(bool)));
	connect(ui.radioButton_Pseudo, SIGNAL(toggled(bool)), this, SLOT(idTypeToggled(bool)));
	connect(ui.buttonBox, SIGNAL(accepted()), this, SLOT(updateId()));
	connect(ui.buttonBox, SIGNAL(rejected()), this, SLOT(close()));

	mIdQueue = new TokenQueue(rsIdentity->getTokenService(), this);
}

void IdEditDialog::setupNewId(bool pseudo)
{
	setWindowTitle(tr("New identity"));

	mIsNew = true;

	ui.lineEdit_KeyId->setText(tr("To be generated"));
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
	idTypeToggled(true);
}

void IdEditDialog::idTypeToggled(bool checked)
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
		ui.lineEdit_GpgHash->setText(tr("N/A"));
		ui.lineEdit_GpgId->setText(tr("N/A"));
		ui.lineEdit_GpgName->setText(tr("N/A"));
	}
	else
	{
		/* get GPG Details from rsPeers */
		std::string gpgid  = rsPeers->getGPGOwnId();
		RsPeerDetails details;
		rsPeers->getPeerDetails(gpgid, details);

		ui.lineEdit_GpgId->setText(QString::fromStdString(gpgid));
		ui.lineEdit_GpgHash->setText(tr("To be generated"));
		ui.lineEdit_GpgName->setText(QString::fromStdString(details.name));
	}
}

void IdEditDialog::setupExistingId(std::string keyId)
{
	setWindowTitle(tr("Edit identity"));

	mIsNew = false;

	mStateHelper->setLoading(IDEDITDIALOG_LOADID, true);

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;

	std::list<std::string> groupIds;
	groupIds.push_back(keyId);

	uint32_t token;
	mIdQueue->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, groupIds, IDEDITDIALOG_LOADID);
}

void IdEditDialog::loadExistingId(uint32_t token)
{
	mStateHelper->setLoading(IDEDITDIALOG_LOADID, false);

	/* get details from libretroshare */
	RsGxsIdGroup data;
	std::vector<RsGxsIdGroup> datavector;
	if (!rsIdentity->getGroupData(token, datavector))
	{
		ui.lineEdit_KeyId->setText(tr("Error getting key!"));
		return;
	}
	
	if (datavector.size() != 1)
	{
		std::cerr << "IdDialog::insertIdDetails() Invalid datavector size";
		std::cerr << std::endl;

		ui.lineEdit_KeyId->setText(tr("Error KeyID invalid"));
		ui.lineEdit_Nickname->setText("");

		ui.lineEdit_GpgHash->setText(tr("N/A"));
		ui.lineEdit_GpgId->setText(tr("N/A"));
		ui.lineEdit_GpgName->setText(tr("N/A"));
		return;
	}

	data = datavector[0];

	bool realid = (data.mMeta.mGroupFlags & RSGXSID_GROUPFLAG_REALID);

	if (realid)
	{
		ui.radioButton_GpgId->setChecked(true);
	}
	else
	{
		ui.radioButton_Pseudo->setChecked(true);
	}

	// DOES THIS TRIGGER ALREADY???
	// force - incase it wasn't triggered.
	idTypeToggled(true);

	ui.lineEdit_Nickname->setText(QString::fromStdString(data.mMeta.mGroupName));
	ui.lineEdit_KeyId->setText(QString::fromStdString(data.mMeta.mGroupId));

	if (realid)
	{
		ui.lineEdit_GpgHash->setText(QString::fromStdString(data.mPgpIdHash));

		if (data.mPgpKnown)
		{
			RsPeerDetails details;
			rsPeers->getGPGDetails(data.mPgpId, details);
			ui.lineEdit_GpgName->setText(QString::fromStdString(details.name));

			ui.lineEdit_GpgId->setText(QString::fromStdString(data.mPgpId));
		}
		else
		{
			ui.lineEdit_GpgId->setText(tr("Unknown GpgId"));
			ui.lineEdit_GpgName->setText(tr("Unknown real name"));
		}
	}
	else
	{
		ui.lineEdit_GpgHash->setText(tr("N/A"));
		ui.lineEdit_GpgId->setText(tr("N/A"));
		ui.lineEdit_GpgName->setText(tr("N/A"));
	}
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

	//rid.mIdType = RSID_RELATION_YOURSELF;
	if (mIsNew)
	{
		rid.mMeta.mGroupId = "";
	}
	else
	{
		rid.mMeta.mGroupId = ui.lineEdit_KeyId->text().toStdString();
	}

	if (ui.radioButton_GpgId->isChecked())
	{
		//rid.mIdType |= RSID_TYPE_REALID;

		//rid.mGpgId = ui.lineEdit_GpgId->text().toStdString();
		rid.mPgpIdHash = ui.lineEdit_GpgHash->text().toStdString();
		//rid.mGpgName = ui.lineEdit_GpgName->text().toStdString();
	}
	else
	{
		//rid.mIdType |= RSID_TYPE_PSEUDONYM;

		//rid.mGpgId = "";
		rid.mPgpIdHash = "";
		//rid.mGpgName = "";
		//rid.mGpgEmail = "";
	}
	
	// Can only create Identities for the moment!
	RsIdentityParameters params;
	params.nickname = rid.mMeta.mGroupName;
	params.isPgpLinked = (ui.radioButton_GpgId->isChecked());

	uint32_t dummyToken = 0;
	rsIdentity->createIdentity(dummyToken, params);

	close();
}

void IdEditDialog::loadRequest(const TokenQueue */*queue*/, const TokenRequest &req)
{
	std::cerr << "IdDialog::loadRequest() UserType: " << req.mUserType;
	std::cerr << std::endl;
	
	// only one here!
	loadExistingId(req.mToken);
}
