/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2008 Robert Fernie
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/

#include <QMessageBox>

#include "util/misc.h"
#include "GxsGroupDialog.h"
#include "gui/common/PeerDefs.h"
#include "gxs/rsgxsflags.h"

#include <algorithm>

#include <retroshare/rspeers.h>

#include <iostream>

// Control of Publish Signatures.
#define RSGXS_GROUP_SIGN_PUBLISH_MASK  		0x000000ff
#define RSGXS_GROUP_SIGN_PUBLISH_ENCRYPTED 	0x00000001
#define RSGXS_GROUP_SIGN_PUBLISH_ALLSIGNED 	0x00000002
#define RSGXS_GROUP_SIGN_PUBLISH_THREADHEAD	0x00000004
#define RSGXS_GROUP_SIGN_PUBLISH_NONEREQ	0x00000008

// Author Signature.
#define RSGXS_GROUP_SIGN_AUTHOR_MASK  		0x0000ff00
#define RSGXS_GROUP_SIGN_AUTHOR_GPG 		0x00000100
#define RSGXS_GROUP_SIGN_AUTHOR_REQUIRED 	0x00000200
#define RSGXS_GROUP_SIGN_AUTHOR_IFNOPUBSIGN	0x00000400
#define RSGXS_GROUP_SIGN_AUTHOR_NONE		0x00000800

#define GXSGROUP_NEWGROUPID		1
#define GXSGROUP_LOADGROUP		2

/** Constructor */
GxsGroupDialog::GxsGroupDialog(RsTokenService *service, QWidget *parent)
: QDialog(parent), mRsService(service)
{
	/* Invoke the Qt Designer generated object setup routine */
	ui.setupUi(this);

        mTokenQueue = new TokenQueue(service, this);

	// connect up the buttons.
	connect( ui.cancelButton, SIGNAL( clicked ( bool ) ), this, SLOT( cancelDialog( ) ) );
	connect( ui.createButton, SIGNAL( clicked ( bool ) ), this, SLOT( submitGroup( ) ) );
	connect( ui.pubKeyShare_cb, SIGNAL( clicked() ), this, SLOT( setShareList( ) ));

        connect( ui.groupLogo, SIGNAL(clicked() ), this , SLOT(addGroupLogo()));
        connect( ui.addLogoButton, SIGNAL(clicked() ), this , SLOT(addGroupLogo()));

	if (!ui.pubKeyShare_cb->isChecked()) {
		ui.contactsdockWidget->hide();
		this->resize(this->size().width() - ui.contactsdockWidget->size().width(), this->size().height());
	}

	/* initialize key share list */
	ui.keyShareList->setHeaderText(tr("Contacts:"));
	ui.keyShareList->setModus(FriendSelectionWidget::MODUS_CHECK);
	ui.keyShareList->start();


	/* Setup Reasonable Defaults */

	uint32_t enabledFlags = ( GXS_GROUP_FLAGS_ICON        | 
				GXS_GROUP_FLAGS_DESCRIPTION   | 
				GXS_GROUP_FLAGS_DISTRIBUTION  | 
				GXS_GROUP_FLAGS_PUBLISHSIGN   | 
				GXS_GROUP_FLAGS_SHAREKEYS     | 
				GXS_GROUP_FLAGS_PERSONALSIGN  | 
				GXS_GROUP_FLAGS_COMMENTS      );

	uint32_t readonlyFlags = 0;

	uint32_t defaultsFlags = ( GXS_GROUP_DEFAULTS_DISTRIB_PUBLIC    |
				//GXS_GROUP_DEFAULTS_DISTRIB_GROUP        |
				//GXS_GROUP_DEFAULTS_DISTRIB_LOCAL        |

				//GXS_GROUP_DEFAULTS_PUBLISH_OPEN         |
				GXS_GROUP_DEFAULTS_PUBLISH_THREADS      |
				//GXS_GROUP_DEFAULTS_PUBLISH_REQUIRED     |
				//GXS_GROUP_DEFAULTS_PUBLISH_ENCRYPTED    |

				//GXS_GROUP_DEFAULTS_PERSONAL_GPG         |
				//GXS_GROUP_DEFAULTS_PERSONAL_REQUIRED    |
				GXS_GROUP_DEFAULTS_PERSONAL_IFNOPUB     |

				GXS_GROUP_DEFAULTS_COMMENTS_YES         |
				//GXS_GROUP_DEFAULTS_COMMENTS_NO          |
				0);

	setFlags(enabledFlags, readonlyFlags, defaultsFlags);

}

void GxsGroupDialog::setFlags(uint32_t enabledFlags, uint32_t readonlyFlags, uint32_t defaultsFlags)
{
	mEnabledFlags = enabledFlags;
	mReadonlyFlags = readonlyFlags;
	mDefaultsFlags = defaultsFlags;
}

void GxsGroupDialog::setMode(uint32_t mode)
{
	mMode = mode;

	/* switch depending on mode */
	switch(mMode)
	{
		case GXS_GROUP_DIALOG_CREATE_MODE:
		{
			ui.createButton->setText(tr("Create Group"));
		}
		break;

		default:
		case GXS_GROUP_DIALOG_SHOW_MODE:
		{
			ui.cancelButton->setVisible(false);
			ui.createButton->setText(tr("Close"));
		}
		break;

		case GXS_GROUP_DIALOG_EDIT_MODE:
		{
			ui.createButton->setText(tr("Submit Changes"));
		}
		break;
	}
}


void GxsGroupDialog::clearForm()
{
	ui.groupName->clear();
	ui.groupDesc->clear();

	ui.groupName->setFocus();
}


void GxsGroupDialog::setupDefaults()
{
	/* Enable / Show Parts based on Flags */	

	if (mDefaultsFlags & GXS_GROUP_DEFAULTS_DISTRIB_MASK)
	{
		if (mDefaultsFlags & GXS_GROUP_DEFAULTS_DISTRIB_PUBLIC)
		{
			ui.typePublic->setChecked(true);
		}
		else if (mDefaultsFlags & GXS_GROUP_DEFAULTS_DISTRIB_GROUP)
		{
			ui.typeGroup->setChecked(true);
		}
		else if (mDefaultsFlags & GXS_GROUP_DEFAULTS_DISTRIB_LOCAL)
		{
			ui.typeLocal->setChecked(true);
		}
		else
		{
			// default
			ui.typePublic->setChecked(true);
		}
	}

	if (mDefaultsFlags & GXS_GROUP_DEFAULTS_PUBLISH_MASK)
	{
		if (mDefaultsFlags & GXS_GROUP_DEFAULTS_PUBLISH_ENCRYPTED)
		{
			ui.publish_encrypt->setChecked(true);
		}
		else if (mDefaultsFlags & GXS_GROUP_DEFAULTS_PUBLISH_REQUIRED)
		{
			ui.publish_required->setChecked(true);
		}
		else if (mDefaultsFlags & GXS_GROUP_DEFAULTS_PUBLISH_THREADS)
		{
			ui.publish_threads->setChecked(true);
		}
		else
		{
			// default
			ui.publish_open->setChecked(true);
		}
	}

	if (mDefaultsFlags & GXS_GROUP_DEFAULTS_PERSONAL_MASK)
	{
		if (mDefaultsFlags & GXS_GROUP_DEFAULTS_PERSONAL_PGP)
		{
			ui.personal_pgp->setChecked(true);
		}
		else if (mDefaultsFlags & GXS_GROUP_DEFAULTS_PERSONAL_REQUIRED)
		{
			ui.personal_required->setChecked(true);
		}
		else if (mDefaultsFlags & GXS_GROUP_DEFAULTS_PERSONAL_IFNOPUB)
		{
			ui.personal_ifnopub->setChecked(true);
		}
		else
		{
			// default
			ui.personal_ifnopub->setChecked(true);
		}
	}

	if (mDefaultsFlags & GXS_GROUP_DEFAULTS_COMMENTS_MASK)
	{
		if (mDefaultsFlags & GXS_GROUP_DEFAULTS_COMMENTS_YES)
		{
			ui.comments_allowed->setChecked(true);
		}
		else if (mDefaultsFlags & GXS_GROUP_DEFAULTS_COMMENTS_NO)
		{
			ui.comments_no->setChecked(true);
		}
		else
		{
			// default
			ui.comments_no->setChecked(true);
		}
	
	}
}

void GxsGroupDialog::setupVisibility()
{
	{
		ui.groupLogo->setVisible(mEnabledFlags & GXS_GROUP_FLAGS_ICON);
		ui.addLogoButton->setVisible(mEnabledFlags & GXS_GROUP_FLAGS_ICON);
	}

	{
		ui.groupDesc->setVisible(mEnabledFlags & GXS_GROUP_FLAGS_DESCRIPTION);
		ui.groupDescLabel->setVisible(mEnabledFlags & GXS_GROUP_FLAGS_DESCRIPTION);
	}

	{
		ui.distribGroupBox->setVisible(mEnabledFlags & GXS_GROUP_FLAGS_DISTRIBUTION);
	}

	{
		ui.publishGroupBox->setVisible(mEnabledFlags & GXS_GROUP_FLAGS_PUBLISHSIGN);
	}

	{
		ui.pubKeyShare_cb->setVisible(mEnabledFlags & GXS_GROUP_FLAGS_SHAREKEYS);
	}

	{
		ui.personalGroupBox->setVisible(mEnabledFlags & GXS_GROUP_FLAGS_PERSONALSIGN);
	}

	{
		ui.commentGroupBox->setVisible(mEnabledFlags & GXS_GROUP_FLAGS_COMMENTS);
	}

	{
		ui.extraFrame->setVisible(mEnabledFlags & GXS_GROUP_FLAGS_EXTRA);
	}
}


void GxsGroupDialog::newGroup()
{
	setupDefaults();
	setupVisibility();
	clearForm();

	setMode(GXS_GROUP_DIALOG_CREATE_MODE);

	service_NewGroup();
}


void GxsGroupDialog::existingGroup(std::string groupId, uint32_t mode)
{
	setupDefaults();
	setupVisibility();
	clearForm();

	setMode(mode);

	service_ExistingGroup();

	/* request data */
	{
                RsTokReqOptions opts;
		
		std::list<std::string> grpIds;
		grpIds.push_back(groupId);
		
		std::cerr << "GxsGroupDialog::existingGroup() Requesting Data.";
		std::cerr << std::endl;
		
		uint32_t token;
		mTokenQueue->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, grpIds, GXSGROUP_LOADGROUP);
	}
}

bool GxsGroupDialog::loadExistingGroupMetaData(const RsGroupMetaData &meta)
{
	/* should set stuff - according to parameters */
	setGroupSignFlags(meta.mSignFlags);
	ui.groupName->setText(QString::fromUtf8(meta.mGroupName.c_str()));

	return true;
}



bool GxsGroupDialog::service_NewGroup()
{
	/* setup any extra bits */
	return true;
}


bool GxsGroupDialog::service_ExistingGroup()
{
	/* setup any extra bits */
	return true;
}


void GxsGroupDialog::submitGroup()
{
	std::cerr << "GxsGroupDialog::submitGroup()";
	std::cerr << std::endl;

	/* switch depending on mode */
	switch(mMode)
	{
		case GXS_GROUP_DIALOG_CREATE_MODE:
		{
			/* just close if down */
			createGroup();
		}
		break;

		default:
		case GXS_GROUP_DIALOG_SHOW_MODE:
		{
			/* just close if down */
			cancelDialog();
		}
		break;

		case GXS_GROUP_DIALOG_EDIT_MODE:
		{
			/* TEMP: just close if down */
			cancelDialog();
		}
		break;
	}
}






void GxsGroupDialog::createGroup()
{
	QString name = misc::removeNewLine(ui.groupName->text());
	QString desc = ui.groupDesc->toPlainText(); //toHtml();
	uint32_t flags = 0;

	if(name.isEmpty()) 
	{
		/* error message */
		QMessageBox::warning(this, "RetroShare", tr("Please add a Name"), QMessageBox::Ok, QMessageBox::Ok);
		return; //Don't add  a empty name!!
	}

	if (mRsService) 
	{
		
		uint32_t token;
		RsGroupMetaData meta;

		// Fill in the MetaData as best we can.
		meta.mGroupName = std::string(name.toUtf8());
		//meta.mDesc = std::string(desc.toUtf8());

		meta.mGroupFlags = flags;
		meta.mSignFlags = getGroupSignFlags();

		// These ones shouldn't be needed here - but will be used, until we have fully functional backend.
                meta.mSubscribeFlags = GXS_SERV::GROUP_SUBSCRIBE_ADMIN;
		meta.mPublishTs = time(NULL);

		if (service_CreateGroup(token, meta))
		{
        		// get the Queue to handle response.
        		mTokenQueue->queueRequest(token, TOKENREQ_MSGINFO, RS_TOKREQ_ANSTYPE_SUMMARY, GXSGROUP_NEWGROUPID);
		}
	}
}
	
uint32_t GxsGroupDialog::getGroupSignFlags()
{
	/* grab from the ui options -> */
	uint32_t signFlags = 0;
	if (ui.publish_encrypt->isChecked()) {
		signFlags |= RSGXS_GROUP_SIGN_PUBLISH_ENCRYPTED;
	} else if (ui.publish_required->isChecked()) {
		signFlags |= RSGXS_GROUP_SIGN_PUBLISH_ALLSIGNED;
	} else if (ui.publish_threads->isChecked()) {
		signFlags |= RSGXS_GROUP_SIGN_PUBLISH_THREADHEAD;
	} else {  // publish_open (default).
		signFlags |= RSGXS_GROUP_SIGN_PUBLISH_NONEREQ;
	}

// Author Signature.
	if (ui.personal_pgp->isChecked()) {
		signFlags |= RSGXS_GROUP_SIGN_AUTHOR_GPG;
	} else if (ui.personal_required->isChecked()) {
		signFlags |= RSGXS_GROUP_SIGN_AUTHOR_REQUIRED;
	} else if (ui.personal_ifnopub->isChecked()) {
		signFlags |= RSGXS_GROUP_SIGN_AUTHOR_IFNOPUBSIGN;
	} else { // shouldn't allow this one.
		signFlags |= RSGXS_GROUP_SIGN_AUTHOR_NONE;
	}
	return signFlags;
}

void GxsGroupDialog::setGroupSignFlags(uint32_t signFlags)
{

	if (signFlags & RSGXS_GROUP_SIGN_PUBLISH_ENCRYPTED) {
		ui.publish_encrypt->setChecked(true);
	} else if (signFlags & RSGXS_GROUP_SIGN_PUBLISH_ALLSIGNED) {
		ui.publish_required->setChecked(true);
	} else if (signFlags & RSGXS_GROUP_SIGN_PUBLISH_THREADHEAD) {
		ui.publish_threads->setChecked(true);
	} else if (signFlags & RSGXS_GROUP_SIGN_PUBLISH_NONEREQ) {
		ui.publish_open->setChecked(true);
	}

	if (signFlags & RSGXS_GROUP_SIGN_AUTHOR_GPG) {
		ui.personal_pgp->setChecked(true);
	} else if (signFlags & RSGXS_GROUP_SIGN_AUTHOR_REQUIRED) {
		ui.personal_required->setChecked(true);
	} else if (signFlags & RSGXS_GROUP_SIGN_AUTHOR_IFNOPUBSIGN) {
		ui.personal_ifnopub->setChecked(true);
	} else if (signFlags & RSGXS_GROUP_SIGN_AUTHOR_NONE) {
		// Its the same... but not quite.
		//ui.personal_noifpub->setChecked();
	}

	/* guess at comments */
	if ((signFlags & RSGXS_GROUP_SIGN_PUBLISH_THREADHEAD) 
		&& (signFlags & RSGXS_GROUP_SIGN_AUTHOR_IFNOPUBSIGN))
	{ 
		ui.comments_allowed->setChecked(true);
	}
	else
	{ 
		ui.comments_no->setChecked(true);
	}


}



bool GxsGroupDialog::service_CreateGroup(uint32_t &token, const RsGroupMetaData &meta)
{
	token = 0;
	return false;
}

bool GxsGroupDialog::service_CompleteCreateGroup(const RsGroupMetaData &meta)
{
	/* dummy function - for overloading */
	return true;
}

void GxsGroupDialog::completeCreateGroup(const RsGroupMetaData &newMeta)
{
	std::cerr << "GxsGroupDialog::completeCreateGroup() Created Group with MetaData: ";
	std::cerr << std::endl;
	std::cerr << newMeta;
	std::cerr << std::endl;

	sendShareList(newMeta.mGroupId);

	service_CompleteCreateGroup(newMeta);

	std::cerr << "GxsGroupDialog::completeCreateGroup() Should Close!";
	std::cerr << std::endl;

	close();
}

void GxsGroupDialog::cancelDialog()
{
	std::cerr << "GxsGroupDialog::cancelDialog() Should Close!";
	std::cerr << std::endl;

	close();
}


void GxsGroupDialog::addGroupLogo() 
{
	QPixmap img = misc::getOpenThumbnailedPicture(this, tr("Load Group Logo"), 64, 64);
	
	if (img.isNull())
		return;
	
	picture = img;
	
	// to show the selected
	ui.groupLogo->setIcon(picture);
}



/***********************************************************************************
  Share Lists.
 ***********************************************************************************/

void GxsGroupDialog::sendShareList(std::string groupId)
{

	close();
}

void GxsGroupDialog::setShareList()
{
	if (ui.pubKeyShare_cb->isChecked()){
		this->resize(this->size().width() + ui.contactsdockWidget->size().width(), this->size().height());
		ui.contactsdockWidget->show();
	} else {  // hide share widget
		ui.contactsdockWidget->hide();
		this->resize(this->size().width() - ui.contactsdockWidget->size().width(), this->size().height());
	}
}


/***********************************************************************************
  Handle Callbacks for Load / Create.
 ***********************************************************************************/

void GxsGroupDialog::loadNewGroupId(const uint32_t &token)
{
	std::cerr << "GxsGroupDialog::loadNewGroupId()";
	std::cerr << std::endl;
	
	std::list<RsGroupMetaData> groupInfo;
	
	if (groupInfo.size() == 1)
	{
		RsGroupMetaData fi = groupInfo.front();
		completeCreateGroup(fi);
	}
	else
	{
		std::cerr << "GxsGroupDialog::loadNewGroupId() ERROR INVALID Number of Forums Created";
		std::cerr << std::endl;
	}
}


void GxsGroupDialog::service_loadExistingGroup(const uint32_t &token)
{
	std::cerr << "GxsGroupDialog::service_loadExistingGroup() ERROR Must be Overloaded";
	std::cerr << std::endl;

#if 0	
	std::list<RsGroupMetaData> groupInfo;
	mRsService->getGroupSummary(token, groupInfo);
	
	if (groupInfo.size() == 1)
	{
		RsGroupMetaData fi = groupInfo.front();
		completeCreateGroup(fi);
	}
	else
	{
		std::cerr << "GxsGroupDialog::loadNewGroupId() ERROR INVALID Number of Forums Created";
		std::cerr << std::endl;
	}
#endif

}


void GxsGroupDialog::loadRequest(const TokenQueue *queue, const TokenRequest &req)
{
	std::cerr << "GxsGroupDialog::loadRequest() UserType: " << req.mUserType;
	std::cerr << std::endl;
	
	if (queue != mTokenQueue)
	{
		std::cerr << "GxsGroupDialog::loadRequest() Queue ERROR";
		std::cerr << std::endl;
		return;
	}

	/* now switch on req */
	switch(req.mUserType)
	{
				
		case GXSGROUP_NEWGROUPID:
			loadNewGroupId(req.mToken);
			break;
		case GXSGROUP_LOADGROUP:
			service_loadExistingGroup(req.mToken);
			break;
		default:
			std::cerr << "GxsGroupDialog::loadRequest() UNKNOWN UserType ";
			std::cerr << std::endl;
				
	}
}

		
		
