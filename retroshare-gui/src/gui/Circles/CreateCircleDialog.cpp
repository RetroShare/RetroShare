/*
 * Retroshare Circles.
 *
 * Copyright 2012-2013 by Robert Fernie.
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

#include "gui/Circles/CreateCircleDialog.h"

#include <retroshare/rspeers.h>
#include <retroshare/rsidentity.h>

#define CREATECIRCLEDIALOG_CIRCLEINFO 2
#define CREATECIRCLEDIALOG_IDINFO     3

#define RSCIRCLEID_COL_NICKNAME       0
#define RSCIRCLEID_COL_KEYID          1
#define RSCIRCLEID_COL_IDTYPE         2

/** Constructor */
CreateCircleDialog::CreateCircleDialog()
	: QDialog(NULL, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint)
{
	/* Invoke the Qt Designer generated object setup routine */
	ui.setupUi(this);

	/* Setup Queue */
	mCircleQueue = new TokenQueue(rsGxsCircles->getTokenService(), this);
	mIdQueue = new TokenQueue(rsIdentity->getTokenService(), this);

	//QString text = pId.empty() ? tr("Start New Thread") : tr("Post Forum Message");
	//setWindowTitle(text);
	//Settings->loadWidgetInformation(this);

	// connect up the buttons.
	connect(ui.addButton, SIGNAL(clicked()), this, SLOT(addMember()));
	connect(ui.removeButton, SIGNAL(clicked()), this, SLOT(removeMember()));

	connect(ui.buttonBox, SIGNAL(accepted()), this, SLOT(createCircle()));
	connect(ui.buttonBox, SIGNAL(rejected()), this, SLOT(close()));

	connect(ui.treeWidget_membership, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(selectedMember(QTreeWidgetItem*, QTreeWidgetItem*)));
	connect(ui.treeWidget_IdList, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(selectedId(QTreeWidgetItem*, QTreeWidgetItem*)));

	ui.removeButton->setEnabled(false);
	ui.addButton->setEnabled(false);
	ui.radioButton_ListKnownPGP->setChecked(true);

	mIsExistingCircle = false;
	mIsExternalCircle = true;

	ui.idChooser->loadIds(0,"");
	ui.circleComboBox->loadCircles(GXS_CIRCLE_CHOOSER_EXTERNAL);
}

CreateCircleDialog::~CreateCircleDialog()
{
	delete(mCircleQueue);
	delete(mIdQueue);
}

void CreateCircleDialog::editExistingId(std::string circleId)
{
	/* load this circle */
	mIsExistingCircle = true;

	/* setup personal or external circle */

	bool isExternal = false;
	if (isExternal)
	{
		setupForExternalCircle();
	}
	else
	{
		setupForPersonalCircle();
	}

	/* lock stuff they cannot edit */
}

void CreateCircleDialog::editNewId(bool isExternal)
{
	/* load this circle */
	mIsExistingCircle = false;

	/* setup personal or external circle */
	if (isExternal)
	{
		setupForExternalCircle();
	}
	else
	{
		setupForPersonalCircle();
	}

	/* enable stuff that might be locked */
}

void CreateCircleDialog::setupForPersonalCircle()
{
	mIsExternalCircle = false;

	/* hide distribution line */
	ui.groupBox_title->setTitle(tr("Personal Circle Details"));
	ui.frame_PgpTypes->hide();
	ui.frame_Distribution->hide();
	ui.idChooserLabel->hide();
	ui.idChooser->hide();

	getPgpIdentities();
}

void CreateCircleDialog::setupForExternalCircle()
{
	mIsExternalCircle = true;

	/* show distribution line */
	ui.groupBox_title->setTitle(tr("External Circle Details"));
	ui.frame_PgpTypes->show();
	ui.frame_Distribution->show();
	ui.idChooserLabel->show();
	ui.idChooser->show();

	requestGxsIdentities();
}

void CreateCircleDialog::selectedId(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
	Q_UNUSED(previous);
	ui.addButton->setEnabled(current != NULL);
}

void CreateCircleDialog::selectedMember(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
	Q_UNUSED(previous);
	ui.removeButton->setEnabled(current != NULL);
}

void CreateCircleDialog::addMember()
{
	QTreeWidgetItem *item = ui.treeWidget_IdList->currentItem();
	if (!item)
	{
		return;
	}

	/* check that its not there already */
	QString keyId = item->text(RSCIRCLEID_COL_KEYID);
	QTreeWidget *tree = ui.treeWidget_membership;

	int count = tree->topLevelItemCount();
	for(int i = 0; i < count; i++)
	{
		QTreeWidgetItem *item = tree->topLevelItem(i);
		if (keyId == item->text(RSCIRCLEID_COL_KEYID))
		{
			std::cerr << "CreateCircleDialog::addMember() Already is a Member: " << keyId.toStdString();
			std::cerr << std::endl;
			return;
		}
	}

	QTreeWidgetItem *member = new QTreeWidgetItem();
	member->setText(RSCIRCLEID_COL_NICKNAME, item->text(RSCIRCLEID_COL_NICKNAME));
	member->setText(RSCIRCLEID_COL_KEYID, item->text(RSCIRCLEID_COL_KEYID));
	member->setText(RSCIRCLEID_COL_IDTYPE, item->text(RSCIRCLEID_COL_IDTYPE));

	tree->addTopLevelItem(member);
}


void  CreateCircleDialog::removeMember()
{
	QTreeWidgetItem *item = ui.treeWidget_membership->currentItem();
	if (!item)
	{
		return;
	}

	// does this just work?
	delete(item);
}

void CreateCircleDialog::createCircle()
{
	std::cerr << "CreateCircleDialog::createCircle()";
	std::cerr << std::endl;

	QString name = ui.circleName->text();
	QString desc;

	if(name.isEmpty())
	{	/* error message */
		QMessageBox::warning(this, tr("RetroShare"),tr("Please set a name for your Circle"), QMessageBox::Ok, QMessageBox::Ok);

		return; //Don't add  a empty Subject!!
	}

	RsGxsCircleGroup circle;

	circle.mMeta.mGroupName = std::string(name.toUtf8());

	RsGxsId authorId;
	if (ui.idChooser->getChosenId(authorId))
	{
		circle.mMeta.mAuthorId = authorId;
		std::cerr << "CreateCircleDialog::createCircle() AuthorId: " << authorId;
		std::cerr << std::endl;
	}
	else
	{
		std::cerr << "CreateCircleDialog::createCircle() No AuthorId Chosen!";
		std::cerr << std::endl;
	}


	/* copy Ids from GUI */
	QTreeWidget *tree = ui.treeWidget_membership;
	int count = tree->topLevelItemCount();
	for(int i = 0; i < count; i++)
	{
		QTreeWidgetItem *item = tree->topLevelItem(i);
		QString keyId = item->text(RSCIRCLEID_COL_KEYID);

		/* insert into circle */
		if (mIsExternalCircle)
		{
			circle.mInvitedMembers.push_back(keyId.toStdString());	
			std::cerr << "CreateCircleDialog::createCircle() Inserting Member: " << keyId.toStdString();
			std::cerr << std::endl;
		}
		else
		{
			circle.mLocalFriends.push_back(keyId.toStdString());	
			std::cerr << "CreateCircleDialog::createCircle() Inserting Friend: " << keyId.toStdString();
			std::cerr << std::endl;
		}

	}

	if (mIsExistingCircle)
	{
		std::cerr << "CreateCircleDialog::createCircle() Existing Circle TODO";
		std::cerr << std::endl;

		// cannot edit these yet.
		QMessageBox::warning(this, tr("RetroShare"),tr("Cannot Edit Existing Circles Yet"), QMessageBox::Ok, QMessageBox::Ok);
		return; 
	}

	if (mIsExternalCircle)
	{
		std::cerr << "CreateCircleDialog::createCircle() External Circle";
		std::cerr << std::endl;

		// set distribution from GUI.
		circle.mMeta.mCircleId = "";
		if (ui.radioButton_Public->isChecked())
		{
			std::cerr << "CreateCircleDialog::createCircle() Public Circle";
			std::cerr << std::endl;

			circle.mMeta.mCircleType =  GXS_CIRCLE_TYPE_PUBLIC;
		}
		else if (ui.radioButton_Self->isChecked())
		{
			std::cerr << "CreateCircleDialog::createCircle() ExtSelfRef Circle";
			std::cerr << std::endl;

			circle.mMeta.mCircleType =  GXS_CIRCLE_TYPE_EXT_SELF;
		}
		else if (ui.radioButton_Restricted->isChecked())
		{
			std::cerr << "CreateCircleDialog::createCircle() External (Other) Circle";
			std::cerr << std::endl;

			circle.mMeta.mCircleType =  GXS_CIRCLE_TYPE_EXTERNAL;

			/* grab circle ID from chooser */
			RsGxsCircleId chosenId;
			if (ui.circleComboBox->getChosenCircle(chosenId))
			{
				std::cerr << "CreateCircleDialog::createCircle() ChosenId: " << chosenId;
				std::cerr << std::endl;

 				circle.mMeta.mCircleId = chosenId;
			}
			else
			{
				std::cerr << "CreateCircleDialog::createCircle() Error no Id Chosen";
				std::cerr << std::endl;

				QMessageBox::warning(this, tr("RetroShare"),tr("No Restriction Circle Selected"), QMessageBox::Ok, QMessageBox::Ok);
				return; 
			}
		}
		else
		{
			QMessageBox::warning(this, tr("RetroShare"),tr("No Circle Limitations Selected"), QMessageBox::Ok, QMessageBox::Ok);
			return; 
		}
	}
	else
	{
		std::cerr << "CreateCircleDialog::createCircle() Personal Circle";
		std::cerr << std::endl;

		// set personal distribution
		circle.mMeta.mCircleId = "";
		circle.mMeta.mCircleType = GXS_CIRCLE_TYPE_LOCAL;
	}

	std::cerr << "CreateCircleDialog::createCircle() Checks and Balances Okay - calling service proper..";
	std::cerr << std::endl;

	uint32_t token;
	rsGxsCircles->createGroup(token, circle);
	close();
}

void CreateCircleDialog::requestCircle(const RsGxsGroupId &groupId)
{
	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;

	std::list<RsGxsGroupId> groupIds;
	groupIds.push_back(groupId);

	std::cerr << "CreateCircleDialog::requestCircle() Requesting Group Summary(" << groupId << ")";
	std::cerr << std::endl;

	uint32_t token;
	mCircleQueue->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, groupIds, CREATECIRCLEDIALOG_CIRCLEINFO);
}

void CreateCircleDialog::loadCircle(uint32_t token)
{
	std::cerr << "CreateCircleDialog::loadCircle(" << token << ")";
	std::cerr << std::endl;

	QTreeWidget *tree = ui.treeWidget_membership;

	tree->clear();

	std::list<std::string> ids;
	std::list<std::string>::iterator it;

	std::vector<RsGxsCircleGroup> groups;
	if (!rsGxsCircles->getGroupData(token, groups))
	{
		std::cerr << "CreateCircleDialog::loadCircle() Error getting GroupData";
		std::cerr << std::endl;
		return;
	}

	if (groups.size() != 1)
	{
		std::cerr << "CreateCircleDialog::loadCircle() Error Group.size() != 1";
		std::cerr << std::endl;
		return;
	}
		
	std::cerr << "CreateCircleDialog::loadCircle() Unfinished Loading";
	std::cerr << std::endl;

	//mCircleGroup = groups[0];
}

void CreateCircleDialog::getPgpIdentities()
{
	std::cerr << "CreateCircleDialog::getPgpIdentities()";
	std::cerr << std::endl;

	QTreeWidget *tree = ui.treeWidget_IdList;

	tree->clear();

	std::list<std::string> ids;
	std::list<std::string>::iterator it;


	rsPeers->getGPGAcceptedList(ids);
	for(it = ids.begin(); it != ids.end(); it++)
	{
		QTreeWidgetItem *item = new QTreeWidgetItem();
		RsPeerDetails details;

		rsPeers->getGPGDetails(*it, details);

		item->setText(RSCIRCLEID_COL_IDTYPE, "PGP Identity");
		item->setText(RSCIRCLEID_COL_NICKNAME, QString::fromUtf8(details.name.c_str()));
		item->setText(RSCIRCLEID_COL_KEYID, QString::fromStdString(details.gpg_id));

		tree->addTopLevelItem(item);
	}
}


void CreateCircleDialog::requestGxsIdentities()
{
	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;

	std::cerr << "CreateCircleDialog::requestIdentities()";
	std::cerr << std::endl;

	uint32_t token;
	mIdQueue->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, CREATECIRCLEDIALOG_IDINFO);
}

void CreateCircleDialog::loadIdentities(uint32_t token)
{
	std::cerr << "CreateCircleDialog::loadIdentities(" << token << ")";
	std::cerr << std::endl;

	QTreeWidget *tree = ui.treeWidget_IdList;

	tree->clear();

	std::list<std::string> ids;
	std::list<std::string>::iterator it;

	bool acceptAllPGP = ui.radioButton_ListAllPGP->isChecked();
	bool acceptKnownPGP = ui.radioButton_ListKnownPGP->isChecked();

	RsGxsIdGroup data;
	std::vector<RsGxsIdGroup> datavector;
	std::vector<RsGxsIdGroup>::iterator vit;
	if (!rsIdentity->getGroupData(token, datavector))
	{
		std::cerr << "CreateCircleDialog::insertIdentities() Error getting GroupData";
		std::cerr << std::endl;
		return;
	}

	for(vit = datavector.begin(); vit != datavector.end(); vit++)
	{
		data = (*vit);

		/* do filtering */
		bool ok = false;
		if (data.mMeta.mGroupFlags & RSGXSID_GROUPFLAG_REALID)
		{
			if (acceptAllPGP)
			{
				ok = true;
			}
			else if (data.mPgpKnown)
			{
				ok = true;
			}
		}

		if (!ok)
		{
			std::cerr << "CreateCircleDialog::insertIdentities() Skipping ID: " << data.mMeta.mGroupId;
			std::cerr << std::endl;
			continue;
		}

		QTreeWidgetItem *item = new QTreeWidgetItem();
		item->setText(RSCIRCLEID_COL_NICKNAME, QString::fromUtf8(data.mMeta.mGroupName.c_str()));
		item->setText(RSCIRCLEID_COL_KEYID, QString::fromStdString(data.mMeta.mGroupId));
		if (data.mMeta.mGroupFlags & RSGXSID_GROUPFLAG_REALID)
		{
			if (data.mPgpKnown)
			{
				RsPeerDetails details;
				rsPeers->getGPGDetails(data.mPgpId, details);
				item->setText(RSCIRCLEID_COL_IDTYPE, QString::fromUtf8(details.name.c_str()));
			}
			else
			{
				item->setText(RSCIRCLEID_COL_IDTYPE, "PGP Linked Id");
			}
		}
		else
		{
			item->setText(RSCIRCLEID_COL_IDTYPE, "Anon Id");
		}
		tree->addTopLevelItem(item);
	}
}

void CreateCircleDialog::loadRequest(const TokenQueue *queue, const TokenRequest &req)
{
	std::cerr << "CreateCircleDialog::loadRequest() UserType: " << req.mUserType;
	std::cerr << std::endl;

	if (queue == mCircleQueue)
	{
		/* now switch on req */
		switch(req.mUserType)
		{
			case CREATECIRCLEDIALOG_CIRCLEINFO:
				loadCircle(req.mToken);
				break;
			default:
				std::cerr << "CreateCircleDialog::loadRequest() UNKNOWN UserType ";
				std::cerr << std::endl;
		}
	}

	if (queue == mIdQueue)
	{
		/* now switch on req */
		switch(req.mUserType)
		{
			case CREATECIRCLEDIALOG_IDINFO:
				loadIdentities(req.mToken);
				break;
			default:
				std::cerr << "CreateCircleDialog::loadRequest() UNKNOWN UserType ";
				std::cerr << std::endl;
		}
	}
}
