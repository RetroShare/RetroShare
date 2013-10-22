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
	connect(ui.buttonBox, SIGNAL(accepted()), this, SLOT(submit()));
	connect(ui.buttonBox, SIGNAL(rejected()), this, SLOT(close()));

	connect(ui.plainTextEdit_Tag, SIGNAL(textChanged()), this, SLOT(checkNewTag()));
	connect(ui.pushButton_Tag, SIGNAL(clicked(bool)), this, SLOT(addRecognTag()));
	connect(ui.toolButton_Tag1, SIGNAL(clicked(bool)), this, SLOT(rmTag1()));
	connect(ui.toolButton_Tag2, SIGNAL(clicked(bool)), this, SLOT(rmTag2()));
	connect(ui.toolButton_Tag3, SIGNAL(clicked(bool)), this, SLOT(rmTag3()));
	connect(ui.toolButton_Tag4, SIGNAL(clicked(bool)), this, SLOT(rmTag4()));
	connect(ui.toolButton_Tag5, SIGNAL(clicked(bool)), this, SLOT(rmTag5()));

	mIdQueue = new TokenQueue(rsIdentity->getTokenService(), this);
	ui.pushButton_Tag->setEnabled(false);
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

	ui.frame_Tags->setHidden(true);
	ui.radioButton_GpgId->setEnabled(true);
	ui.radioButton_Pseudo->setEnabled(true);
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
		ui.lineEdit_GpgName->setText(QString::fromUtf8(details.name.c_str()));
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

	mEditGroup = datavector[0];

	bool realid = (mEditGroup.mMeta.mGroupFlags & RSGXSID_GROUPFLAG_REALID);

	if (realid)
	{
		ui.radioButton_GpgId->setChecked(true);
	}
	else
	{
		ui.radioButton_Pseudo->setChecked(true);
	}
	// these are not editable for existing Id.
	ui.radioButton_GpgId->setEnabled(false);
	ui.radioButton_Pseudo->setEnabled(false);

	// DOES THIS TRIGGER ALREADY???
	// force - incase it wasn't triggered.
	idTypeToggled(true);

	ui.lineEdit_Nickname->setText(QString::fromUtf8(mEditGroup.mMeta.mGroupName.c_str()));
	ui.lineEdit_KeyId->setText(QString::fromStdString(mEditGroup.mMeta.mGroupId));

	if (realid)
	{
		ui.lineEdit_GpgHash->setText(QString::fromStdString(mEditGroup.mPgpIdHash));

		if (mEditGroup.mPgpKnown)
		{
			RsPeerDetails details;
			rsPeers->getGPGDetails(mEditGroup.mPgpId, details);
			ui.lineEdit_GpgName->setText(QString::fromUtf8(details.name.c_str()));

			ui.lineEdit_GpgId->setText(QString::fromStdString(mEditGroup.mPgpId));
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

	// RecognTags.
	ui.frame_Tags->setHidden(false);

	loadRecognTags();
}

#define MAX_RECOGN_TAGS 	5

void IdEditDialog::checkNewTag()
{
	std::string tag = ui.plainTextEdit_Tag->toPlainText().toStdString();
	std::string id = ui.lineEdit_KeyId->text().toStdString();
	std::string name = ui.lineEdit_Nickname->text().toUtf8().data();

	QString desc;
	bool ok = tagDetails(id, name, tag, desc);
	ui.label_TagCheck->setText(desc);

	// hack to allow add invalid tags (for testing).
	if (!tag.empty())
	{
		ok = true;
	}


	if (mEditGroup.mRecognTags.size() >= MAX_RECOGN_TAGS)
	{
		ok = false;
	}

	ui.pushButton_Tag->setEnabled(ok);
}

void IdEditDialog::addRecognTag()
{
	std::string tag = ui.plainTextEdit_Tag->toPlainText().toStdString();
	if (mEditGroup.mRecognTags.size() >= MAX_RECOGN_TAGS)
	{
		std::cerr << "IdEditDialog::addRecognTag() Too many Tags, delete one first";
		std::cerr << std::endl;
	}

	mEditGroup.mRecognTags.push_back(tag);
	loadRecognTags();
}

void IdEditDialog::rmTag1()
{
	rmTag(0);
}

void IdEditDialog::rmTag2()
{
	rmTag(1);
}

void IdEditDialog::rmTag3()
{
	rmTag(2);
}

void IdEditDialog::rmTag4()
{
	rmTag(3);
}

void IdEditDialog::rmTag5()
{
	rmTag(4);
}
	
void IdEditDialog::rmTag(int idx)
{
	std::list<std::string>::iterator it;
	int i = 0;
	for(it = mEditGroup.mRecognTags.begin(); it != mEditGroup.mRecognTags.end() && (idx < i); it++, i++) ;

	if (it != mEditGroup.mRecognTags.end())
	{
		mEditGroup.mRecognTags.erase(it);
	}
	loadRecognTags();
}

bool IdEditDialog::tagDetails(const std::string &id, const std::string &name, const std::string &tag, QString &desc)
{
	if (tag.empty())
	{
		desc += "Empty Tag";
		return false;
	}
		
	/* extract details for each tag */
	RsRecognTagDetails tagDetails;

	bool ok = false;
	if (rsIdentity->parseRecognTag(id, name, tag, tagDetails))
	{
		desc += QString::number(tagDetails.tag_class);
		desc += ":";
		desc += QString::number(tagDetails.tag_type);

		if (tagDetails.is_valid)
		{
			ok = true;
			desc += " Valid";
		}
		else
		{
			desc += " Invalid";
		}

		if (tagDetails.is_pending)
		{
			ok = true;
			desc += " Pending";
		}
	}
	else
	{
		desc += "Unparseable";
	}
	return ok;
}


void IdEditDialog::loadRecognTags()
{
	std::cerr << "IdEditDialog::loadRecognTags()";
	std::cerr << std::endl;

	// delete existing items.
	ui.label_Tag1->setHidden(true);
	ui.label_Tag2->setHidden(true);
	ui.label_Tag3->setHidden(true);
	ui.label_Tag4->setHidden(true);
	ui.label_Tag5->setHidden(true);
	ui.toolButton_Tag1->setHidden(true);
	ui.toolButton_Tag2->setHidden(true);
	ui.toolButton_Tag3->setHidden(true);
	ui.toolButton_Tag4->setHidden(true);
	ui.toolButton_Tag5->setHidden(true);
	ui.plainTextEdit_Tag->setPlainText("");

	int i = 0;
	std::list<std::string>::const_iterator it;
	for(it = mEditGroup.mRecognTags.begin(); it != mEditGroup.mRecognTags.end(); it++, i++)
	{
		QString recognTag;
		tagDetails(mEditGroup.mMeta.mGroupId, mEditGroup.mMeta.mGroupName, *it, recognTag);

		switch(i)
		{
			default:
			case 0:
				ui.label_Tag1->setText(recognTag);
				ui.label_Tag1->setHidden(false);
				ui.toolButton_Tag1->setHidden(false);
				break;
			case 1:
				ui.label_Tag2->setText(recognTag);
				ui.label_Tag2->setHidden(false);
				ui.toolButton_Tag2->setHidden(false);
				break;
			case 2:
				ui.label_Tag3->setText(recognTag);
				ui.label_Tag3->setHidden(false);
				ui.toolButton_Tag3->setHidden(false);
				break;
			case 3:
				ui.label_Tag4->setText(recognTag);
				ui.label_Tag4->setHidden(false);
				ui.toolButton_Tag4->setHidden(false);
				break;
			case 4:
				ui.label_Tag5->setText(recognTag);
				ui.label_Tag5->setHidden(false);
				ui.toolButton_Tag5->setHidden(false);
				break;
		}
	}
}

void IdEditDialog::submit()
{
	if (mIsNew)
	{
		createId();
	}
	else
	{
		updateId();
	}
}


void IdEditDialog::createId()
{
	std::string groupname = ui.lineEdit_Nickname->text().toUtf8().constData();

	if (groupname.size() < 2)
	{
		std::cerr << "IdEditDialog::createId() Nickname too short";
		std::cerr << std::endl;
		return;
	}

	RsIdentityParameters params;
	params.nickname = groupname;
	params.isPgpLinked = (ui.radioButton_GpgId->isChecked());

	uint32_t dummyToken = 0;
	rsIdentity->createIdentity(dummyToken, params);

	close();
}


void IdEditDialog::updateId()
{
	/* submit updated details */
	std::string groupname = ui.lineEdit_Nickname->text().toUtf8().constData();

	if (groupname.size() < 2)
	{
		std::cerr << "IdEditDialog::updateId() Nickname too short";
		std::cerr << std::endl;
		return;
	}

	mEditGroup.mMeta.mGroupName = groupname;

	uint32_t dummyToken = 0;
	rsIdentity->updateIdentity(dummyToken, mEditGroup);

	close();
}




void IdEditDialog::loadRequest(const TokenQueue */*queue*/, const TokenRequest &req)
{
	std::cerr << "IdDialog::loadRequest() UserType: " << req.mUserType;
	std::cerr << std::endl;

	// only one here!
	loadExistingId(req.mToken);
}
