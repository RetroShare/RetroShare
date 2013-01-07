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
#include <retroshare/rsgxscircles.h>

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
GxsGroupDialog::GxsGroupDialog(TokenQueue *tokenQueue, uint32_t enableFlags, uint16_t defaultFlags, QWidget *parent)
	: QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint), mTokenQueue(tokenQueue), mMode(MODE_CREATE), mEnabledFlags(enableFlags), mReadonlyFlags(0), mDefaultsFlags(defaultFlags)
{
	/* Invoke the Qt Designer generated object setup routine */
	ui.setupUi(this);

	init();
}

GxsGroupDialog::GxsGroupDialog(const RsGroupMetaData &grpMeta, Mode mode, QWidget *parent)
	: QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint), mTokenQueue(NULL), mGrpMeta(grpMeta), mMode(mode), mEnabledFlags(0), mReadonlyFlags(0), mDefaultsFlags(0)
{
	/* Invoke the Qt Designer generated object setup routine */
	ui.setupUi(this);

	init();
}

void GxsGroupDialog::init()
{
	// connect up the buttons.
	connect( ui.buttonBox, SIGNAL(accepted()), this, SLOT(submitGroup()));
	connect( ui.buttonBox, SIGNAL(rejected()), this, SLOT(cancelDialog()));
	connect( ui.pubKeyShare_cb, SIGNAL( clicked() ), this, SLOT( setShareList( ) ));

	connect( ui.groupLogo, SIGNAL(clicked() ), this , SLOT(addGroupLogo()));
	connect( ui.addLogoButton, SIGNAL(clicked() ), this , SLOT(addGroupLogo()));

	ui.typePublic->setChecked(true);
	updateCircleOptions();

	connect( ui.typePublic, SIGNAL(clicked()), this , SLOT(updateCircleOptions()));
	connect( ui.typeGroup, SIGNAL(clicked()), this , SLOT(updateCircleOptions()));
	connect( ui.typeLocal, SIGNAL(clicked()), this , SLOT(updateCircleOptions()));

	if (!ui.pubKeyShare_cb->isChecked())
	{
		ui.contactsdockWidget->hide();
		this->resize(this->size().width() - ui.contactsdockWidget->size().width(), this->size().height());
	}

	/* initialize key share list */
	ui.keyShareList->setHeaderText(tr("Contacts:"));
	ui.keyShareList->setModus(FriendSelectionWidget::MODUS_CHECK);
	ui.keyShareList->start();

	/* Setup Reasonable Defaults */

	ui.idChooser->loadIds(0,"");
	ui.circleComboBox->loadCircles(0);

	initMode();
}

void GxsGroupDialog::showEvent(QShowEvent*)
{
	QString header = uiText(UITYPE_SERVICE_HEADER);
	ui.headerFrame->setHeaderText(header);
	setWindowTitle(header);
	ui.headerFrame->setHeaderImage(serviceImage());

	QString text = uiText(UITYPE_KEY_SHARE_CHECKBOX);
	if (!text.isEmpty()) {
		ui.pubKeyShare_cb->setText(text);
	}

	text = uiText(UITYPE_CONTACTS_DOCK);
	if (!text.isEmpty()) {
		ui.contactsdockWidget->setWindowTitle(text);
	}
}

void GxsGroupDialog::initMode()
{
	switch (mode())
	{
		case MODE_CREATE:
		{
			ui.buttonBox->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
			ui.buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Create Group"));
			newGroup();
		}
		break;

		case MODE_SHOW:
		{
			ui.buttonBox->setStandardButtons(QDialogButtonBox::Close);
		}
		break;
//TODO
//		case MODE_EDIT:
//		{
//			ui.createButton->setText(tr("Submit Changes"));
//		}
//		break;
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

}

void GxsGroupDialog::submitGroup()
{
	std::cerr << "GxsGroupDialog::submitGroup()";
	std::cerr << std::endl;

	/* switch depending on mode */
	switch (mode())
	{
		case MODE_CREATE:
		{
			/* just close if down */
			createGroup();
		}
		break;

		case MODE_SHOW:
		{
			/* just close if down */
			cancelDialog();
		}
		break;

		case MODE_EDIT:
		{
			/* TEMP: just close if down */
			cancelDialog();
		}
		break;
	}
}

void GxsGroupDialog::createGroup()
{
	std::cerr << "GxsGroupDialog::createGroup()";
	std::cerr << std::endl;

	QString name = misc::removeNewLine(ui.groupName->text());
	uint32_t flags = 0;

	if(name.isEmpty())
	{
			/* error message */
			QMessageBox::warning(this, "RetroShare", tr("Please add a Name"), QMessageBox::Ok, QMessageBox::Ok);
			return; //Don't add  a empty name!!
	}

	uint32_t token;
	RsGroupMetaData meta;

	// Fill in the MetaData as best we can.
	meta.mGroupName = std::string(name.toUtf8());

	meta.mGroupFlags = flags;
	meta.mSignFlags = getGroupSignFlags();

	setCircleParameters(meta);

	if (service_CreateGroup(token, meta))
	{
		// get the Queue to handle response.
		if(mTokenQueue != NULL)
			mTokenQueue->queueRequest(token, TOKENREQ_GROUPINFO, RS_TOKREQ_ANSTYPE_ACK, GXSGROUP_NEWGROUPID);
	}
	close();
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



/**** Above logic is flawed, and will be removed shortly
 *
 *
 ****/

void GxsGroupDialog::updateCircleOptions()
{
	if (ui.typeGroup->isChecked())
	{
		ui.circleComboBox->setEnabled(true);
		ui.circleComboBox->setVisible(true);
	}
	else 
	{
		ui.circleComboBox->setEnabled(false);
		ui.circleComboBox->setVisible(false);
	}

	if (ui.typeLocal->isChecked())
	{
		ui.localComboBox->setEnabled(true);
		ui.localComboBox->setVisible(true);
	}
	else 
	{
		ui.localComboBox->setEnabled(false);
		ui.localComboBox->setVisible(false);
	}
}

void GxsGroupDialog::setCircleParameters(RsGroupMetaData &meta)
{
	bool problem = false;
	if (ui.typePublic->isChecked())
	{
		meta.mCircleType = GXS_CIRCLE_TYPE_PUBLIC;
		meta.mCircleId.clear();
	}
	else if (ui.typeGroup->isChecked())
	{
		meta.mCircleType = GXS_CIRCLE_TYPE_EXTERNAL;
		if (!ui.circleComboBox->getChosenCircle(meta.mCircleId))
		{
			problem = true;
		}
	}
	else if (ui.typeGroup->isChecked())
	{
		meta.mCircleType = GXS_CIRCLE_TYPE_YOUREYESONLY;
		meta.mCircleId.clear();
		meta.mOriginator.clear();
		meta.mInternalCircle = "Internal Circle Id";
	
		problem = true;
	}
	else
	{
		problem = true;
	}

	if (problem)
	{
		// error.
		meta.mCircleType = GXS_CIRCLE_TYPE_PUBLIC;
		meta.mCircleId.clear();
		meta.mOriginator.clear();
		meta.mInternalCircle.clear();
	}
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
	ui.groupLogo->setPixmap(picture);
}

QPixmap GxsGroupDialog::getLogo()
{
	return picture;
}

QString GxsGroupDialog::getDescription()
{
	return ui.groupDesc->document()->toPlainText();
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
