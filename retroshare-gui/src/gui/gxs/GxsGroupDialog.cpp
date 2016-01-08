/*
 * Retroshare Gxs Support
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

#include "util/misc.h"
#include "util/DateTime.h"
#include "GxsGroupDialog.h"
#include "gui/common/PeerDefs.h"
#include "retroshare/rsgxsflags.h"

#include <algorithm>

#include <retroshare/rspeers.h>
#include <retroshare/rsgxscircles.h>

#include <iostream>

// Control of Publish Signatures.
// 
// These are now defined in rsgxsflags.h
// 
// #define FLAG_GROUP_SIGN_PUBLISH_MASK       0x000000ff
// #define FLAG_GROUP_SIGN_PUBLISH_ENCRYPTED  0x00000001
// #define FLAG_GROUP_SIGN_PUBLISH_ALLSIGNED  0x00000002
// #define FLAG_GROUP_SIGN_PUBLISH_THREADHEAD 0x00000004
// #define FLAG_GROUP_SIGN_PUBLISH_NONEREQ    0x00000008

// // Author Signature.
//
// These are now defined in rsgxsflags.h
//
// #define FLAG_AUTHOR_AUTHENTICATION_MASK        0x0000ff00
// #define FLAG_AUTHOR_AUTHENTICATION_NONE        0x00000000
// #define FLAG_AUTHOR_AUTHENTICATION_GPG         0x00000100
// #define FLAG_AUTHOR_AUTHENTICATION_REQUIRED    0x00000200
// #define FLAG_AUTHOR_AUTHENTICATION_IFNOPUBSIGN 0x00000400

#define GXSGROUP_NEWGROUPID         1
#define GXSGROUP_LOADGROUP          2
#define GXSGROUP_INTERNAL_LOADGROUP 3

/** Constructor */
GxsGroupDialog::GxsGroupDialog(TokenQueue *tokenExternalQueue, uint32_t enableFlags, uint32_t defaultFlags, QWidget *parent)
    : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint), mTokenService(NULL), mExternalTokenQueue(tokenExternalQueue), mInternalTokenQueue(NULL), mGrpMeta(), mMode(MODE_CREATE), mEnabledFlags(enableFlags), mReadonlyFlags(0), mDefaultsFlags(defaultFlags)
{
	/* Invoke the Qt Designer generated object setup routine */
	ui.setupUi(this);

	mInternalTokenQueue = NULL;

	init();
}

GxsGroupDialog::GxsGroupDialog(TokenQueue *tokenExternalQueue, RsTokenService *tokenService, Mode mode, RsGxsGroupId groupId, uint32_t enableFlags, uint32_t defaultFlags, QWidget *parent)
    : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint), mTokenService(NULL), mExternalTokenQueue(tokenExternalQueue), mInternalTokenQueue(NULL), mGrpMeta(), mMode(mode), mEnabledFlags(enableFlags), mReadonlyFlags(0), mDefaultsFlags(defaultFlags)
{
	/* Invoke the Qt Designer generated object setup routine */
	ui.setupUi(this);

	mTokenService = tokenService;
	mInternalTokenQueue = new TokenQueue(tokenService, this);
	mGrpMeta.mGroupId = groupId;

	init();
}

GxsGroupDialog::~GxsGroupDialog()
{
	if (mInternalTokenQueue) {
		delete(mInternalTokenQueue);
	}
}

void GxsGroupDialog::init()
{
	// connect up the buttons.
	connect(ui.buttonBox, SIGNAL(accepted()), this, SLOT(submitGroup()));
	connect(ui.buttonBox, SIGNAL(rejected()), this, SLOT(cancelDialog()));
	connect(ui.pubKeyShare_cb, SIGNAL(clicked()), this, SLOT(setShareList()));

	connect(ui.groupLogo, SIGNAL(clicked() ), this , SLOT(addGroupLogo()));
	connect(ui.addLogoButton, SIGNAL(clicked() ), this , SLOT(addGroupLogo()));

	ui.edit_typePublic->setChecked(true);
	ui.show_typePublic->setChecked(true);
	updateCircleOptions();

	connect(ui.edit_typePublic, SIGNAL(clicked()), this , SLOT(updateCircleOptions()));
	connect(ui.edit_typeGroup, SIGNAL(clicked()), this , SLOT(updateCircleOptions()));
	connect(ui.edit_typeLocal, SIGNAL(clicked()), this , SLOT(updateCircleOptions()));

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

	ui.idChooser->loadIds(0,RsGxsId());
	ui.circleComboBox->loadCircles(GXS_CIRCLE_CHOOSER_EXTERNAL, RsGxsCircleId());
	ui.localComboBox->loadCircles(GXS_CIRCLE_CHOOSER_PERSONAL, RsGxsCircleId());
	
	ui.edit_groupDesc->setPlaceholderText(tr("Set a descriptive description here"));

    	ui.personal_ifnopub->hide() ;
    	ui.personal_required->hide() ;
    	ui.personal_required->setChecked(true) ;	// this is always true

	initMode();
}

QIcon GxsGroupDialog::serviceWindowIcon()
{
	return qApp->windowIcon();
}

void GxsGroupDialog::showEvent(QShowEvent*)
{
	ui.headerFrame->setHeaderImage(serviceImage());
	setWindowIcon(serviceWindowIcon());

	initUi();
}

void GxsGroupDialog::setUiText(UiType uiType, const QString &text)
{
	switch (uiType)
	{
	case UITYPE_SERVICE_HEADER:
		setWindowTitle(text);
		ui.headerFrame->setHeaderText(text);
		break;
	case UITYPE_KEY_SHARE_CHECKBOX:
		ui.pubKeyShare_cb->setText(text);
		break;
	case UITYPE_CONTACTS_DOCK:
		ui.contactsdockWidget->setWindowTitle(text);
		break;
	case UITYPE_BUTTONBOX_OK:
		ui.buttonBox->button(QDialogButtonBox::Ok)->setText(text);
		break;
	}
}

void GxsGroupDialog::initMode()
{
	setAllReadonly();
	switch (mode())
	{
		case MODE_CREATE:
		{
            ui.stackedWidget->setCurrentIndex(0);
			ui.buttonBox->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
			newGroup();
		}
		break;

		case MODE_SHOW:
		{
			ui.stackedWidget->setCurrentIndex(1);
			mReadonlyFlags = 0xffffffff; // Force all to readonly.
			ui.buttonBox->setStandardButtons(QDialogButtonBox::Close);
			requestGroup(mGrpMeta.mGroupId);
		}
		break;

		case MODE_EDIT:
		{
            ui.stackedWidget->setCurrentIndex(0);
			ui.buttonBox->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
			ui.buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Submit Group Changes"));
			requestGroup(mGrpMeta.mGroupId);
		}
		break;
	}
}

void GxsGroupDialog::clearForm()
{
	ui.edit_groupName->clear();
	ui.edit_groupDesc->clear();
	ui.edit_groupName->setFocus();
}

void GxsGroupDialog::setupDefaults()
{
	/* Enable / Show Parts based on Flags */	

	if (mDefaultsFlags & GXS_GROUP_DEFAULTS_DISTRIB_MASK)
	{
		if (mDefaultsFlags & GXS_GROUP_DEFAULTS_DISTRIB_PUBLIC)
		{
			ui.edit_typePublic->setChecked(true);
			ui.show_typePublic->setChecked(true);
		}
		else if (mDefaultsFlags & GXS_GROUP_DEFAULTS_DISTRIB_GROUP)
		{
			ui.edit_typeGroup->setChecked(true);
			ui.show_typeGroup->setChecked(true);
		}
		else if (mDefaultsFlags & GXS_GROUP_DEFAULTS_DISTRIB_LOCAL)
		{
			ui.edit_typeLocal->setChecked(true);
			ui.show_typeLocal->setChecked(true);
		}
		else
		{
			// default
			ui.edit_typePublic->setChecked(true);
			ui.show_typePublic->setChecked(true);
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
			ui.edit_comments_allowed->setChecked(true);
			ui.show_comments_allowed->setChecked(true);
		}
		else if (mDefaultsFlags & GXS_GROUP_DEFAULTS_COMMENTS_NO)
		{
			ui.edit_comments_no->setChecked(true);
			ui.show_comments_no->setChecked(true);
		}
		else
		{
			// default
			ui.edit_comments_no->setChecked(true);
			ui.show_comments_no->setChecked(true);
		}
	}
	ui.edit_antiSpam_trackMessages->setChecked((bool)(mDefaultsFlags & GXS_GROUP_DEFAULTS_ANTISPAM_TRACK));
	ui.show_antiSpam_trackMessages->setChecked((bool)(mDefaultsFlags & GXS_GROUP_DEFAULTS_ANTISPAM_TRACK));
	ui.edit_antiSpam_signedIds->setChecked((bool)(mDefaultsFlags & GXS_GROUP_DEFAULTS_ANTISPAM_FAVOR_PGP));
	ui.show_antiSpam_signedIds->setChecked((bool)(mDefaultsFlags & GXS_GROUP_DEFAULTS_ANTISPAM_FAVOR_PGP));

#ifndef RS_USE_CIRCLES
	ui.edit_typeGroup->setEnabled(false);
	ui.show_typeGroup->setEnabled(false);
	ui.edit_typeLocal->setEnabled(false);
	ui.show_typeLocal->setEnabled(false);
#endif
}

void GxsGroupDialog::setupVisibility()
{
	ui.edit_groupName->setVisible(mEnabledFlags & GXS_GROUP_FLAGS_NAME);

	ui.groupLogo->setVisible(mEnabledFlags & GXS_GROUP_FLAGS_ICON);
	ui.addLogoButton->setVisible(mEnabledFlags & GXS_GROUP_FLAGS_ICON);

	ui.edit_groupDesc->setVisible(mEnabledFlags & GXS_GROUP_FLAGS_DESCRIPTION);

	ui.editDistribGBox->setVisible(mEnabledFlags & GXS_GROUP_FLAGS_DISTRIBUTION);
	ui.showDistribGBox->setVisible(mEnabledFlags & GXS_GROUP_FLAGS_DISTRIBUTION);
    
	ui.editSpamProtectionGBox->setVisible(mEnabledFlags & GXS_GROUP_FLAGS_ANTI_SPAM);
	ui.showSpamProtectionGBox->setVisible(mEnabledFlags & GXS_GROUP_FLAGS_ANTI_SPAM);

	ui.publishGBox->setVisible(mEnabledFlags & GXS_GROUP_FLAGS_PUBLISHSIGN);

	ui.pubKeyShare_cb->setVisible(mEnabledFlags & GXS_GROUP_FLAGS_SHAREKEYS);

	ui.personalGBox->setVisible(mEnabledFlags & GXS_GROUP_FLAGS_PERSONALSIGN);

	ui.editCommentGBox->setVisible(mEnabledFlags & GXS_GROUP_FLAGS_COMMENTS);
	ui.showCommentGBox->setVisible(mEnabledFlags & GXS_GROUP_FLAGS_COMMENTS);
	ui.commentslabel->setVisible(mEnabledFlags & GXS_GROUP_FLAGS_COMMENTS);

	ui.extraFrame->setVisible(mEnabledFlags & GXS_GROUP_FLAGS_EXTRA);
}

void GxsGroupDialog::setAllReadonly()
{
	uint32_t origReadonlyFlags = mReadonlyFlags;
	mReadonlyFlags = 0xffffffff;

	setupReadonly();

	mReadonlyFlags = origReadonlyFlags;
}

void GxsGroupDialog::setupReadonly()
{

	ui.addLogoButton->setEnabled(!(mReadonlyFlags & GXS_GROUP_FLAGS_ICON));

	ui.publishGBox->setEnabled(!(mReadonlyFlags & GXS_GROUP_FLAGS_PUBLISHSIGN));

	ui.pubKeyShare_cb->setEnabled(!(mReadonlyFlags & GXS_GROUP_FLAGS_SHAREKEYS));

	ui.personalGBox->setEnabled(!(mReadonlyFlags & GXS_GROUP_FLAGS_PERSONALSIGN));
	
	ui.idChooser->setEnabled(!(mReadonlyFlags & GXS_GROUP_FLAGS_PERSONALSIGN));

	ui.showDistribGBox->setEnabled(!(mReadonlyFlags & GXS_GROUP_FLAGS_DISTRIBUTION));
	ui.showCommentGBox->setEnabled(!(mReadonlyFlags & GXS_GROUP_FLAGS_COMMENTS));
	ui.editSpamProtectionGBox->setEnabled(!(mReadonlyFlags & GXS_GROUP_FLAGS_ANTI_SPAM));
	ui.showSpamProtectionGBox->setEnabled(!(mReadonlyFlags & GXS_GROUP_FLAGS_ANTI_SPAM));

	ui.extraFrame->setEnabled(!(mReadonlyFlags & GXS_GROUP_FLAGS_EXTRA));
#ifndef UNFINISHED
    ui.pubKeyShare_cb->setEnabled(false) ;
#endif
}

void GxsGroupDialog::newGroup()
{
	setupDefaults();
	setupVisibility();
	setupReadonly();
	clearForm();
}

void GxsGroupDialog::updateFromExistingMeta(const QString &description)
{
	std::cerr << "void GxsGroupDialog::updateFromExistingMeta()";
	std::cerr << std::endl;

	std::cerr << "void GxsGroupDialog::updateFromExistingMeta() mGrpMeta.mCircleType: ";
	std::cerr << mGrpMeta.mCircleType << " Internal: " << mGrpMeta.mInternalCircle;
	std::cerr << " External: " << mGrpMeta.mCircleId;
	std::cerr << std::endl;

	setupDefaults();
	setupVisibility();
	setupReadonly();
	clearForm();
    	setGroupSignFlags(mGrpMeta.mSignFlags) ;

	/* setup name */
	ui.edit_groupName->setText(QString::fromUtf8(mGrpMeta.mGroupName.c_str()));
	
	/* Show Mode */
	ui.nameLine->setText(QString::fromUtf8(mGrpMeta.mGroupName.c_str()));
	ui.popLine->setText(QString::number( mGrpMeta.mPop)) ;
	ui.postsLine->setText(QString::number(mGrpMeta.mVisibleMsgCount));
	ui.lastpostLine->setText(DateTime::formatLongDateTime(mGrpMeta.mLastPost));
	ui.authorLabel->setId(mGrpMeta.mAuthorId);
	ui.IDline->setText(QString::fromStdString(mGrpMeta.mGroupId.toStdString()));
	ui.descriptiontextEdit->setPlainText(description);
	
		switch (mode())
  {
		case MODE_CREATE:{
		}
		break;
		case MODE_SHOW:{
			ui.headerFrame->setHeaderText(QString::fromUtf8(mGrpMeta.mGroupName.c_str()));
			if (mPicture.isNull())
			return;
			ui.headerFrame->setHeaderImage(mPicture);
		}
		break;
		case MODE_EDIT:{
		}
		break;
  }
	/* set description */
	ui.edit_groupDesc->setPlainText(description);

	switch(mGrpMeta.mCircleType)
	{
		case GXS_CIRCLE_TYPE_YOUREYESONLY:
			ui.edit_typeLocal->setChecked(true);
			ui.show_typeLocal->setChecked(true);
			ui.localComboBox->loadCircles(GXS_CIRCLE_CHOOSER_PERSONAL, mGrpMeta.mInternalCircle);
			break;
		case GXS_CIRCLE_TYPE_PUBLIC:
			ui.edit_typePublic->setChecked(true);
			ui.show_typePublic->setChecked(true);
			break;
		case GXS_CIRCLE_TYPE_EXTERNAL:
			ui.edit_typeGroup->setChecked(true);
			ui.show_typeGroup->setChecked(true);
			ui.circleComboBox->loadCircles(GXS_CIRCLE_CHOOSER_EXTERNAL, mGrpMeta.mCircleId);
			break;
		default:
			std::cerr << "CreateCircleDialog::updateCircleGUI() INVALID mCircleType";
			std::cerr << std::endl;
			break;
	}

	ui.idChooser->loadIds(0, mGrpMeta.mAuthorId);

	updateCircleOptions();
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
			editGroup();
		}
		break;
	}
}

void GxsGroupDialog::editGroup()
{
	std::cerr << "GxsGroupDialog::editGroup()" << std::endl;

	RsGroupMetaData newMeta;
	newMeta.mGroupId = mGrpMeta.mGroupId;

	if(!prepareGroupMetaData(newMeta))
	{
		/* error message */
		QMessageBox::warning(this, "RetroShare", tr("Failed to Prepare Group MetaData - please Review"), QMessageBox::Ok, QMessageBox::Ok);
		return; //Don't add  a empty name!!
	}

	std::cerr << "GxsGroupDialog::editGroup() calling service_EditGroup";
	std::cerr << std::endl;

	uint32_t token;
	if (service_EditGroup(token, newMeta))
	{
		// get the Queue to handle response.
		if(mExternalTokenQueue != NULL)
			mExternalTokenQueue->queueRequest(token, TOKENREQ_GROUPINFO, RS_TOKREQ_ANSTYPE_ACK, GXSGROUP_NEWGROUPID);
	}
	else
	{
		std::cerr << "GxsGroupDialog::editGroup() ERROR";
		std::cerr << std::endl;
	}

	close();
}

bool GxsGroupDialog::prepareGroupMetaData(RsGroupMetaData &meta)
{
	std::cerr << "GxsGroupDialog::prepareGroupMetaData()";
	std::cerr << std::endl;

    // here would be the place to check for empty author id
    // but GXS_SERV::GRP_OPTION_AUTHEN_AUTHOR_SIGN is currently not used by any service
    ui.idChooser->getChosenId(meta.mAuthorId);

	QString name = getName();
	uint32_t flags = GXS_SERV::FLAG_PRIVACY_PUBLIC;

	if(name.isEmpty()) {
		std::cerr << "GxsGroupDialog::prepareGroupMetaData()";
		std::cerr << " Invalid GroupName";
		std::cerr << std::endl;
		return false;
	}//if(name.isEmpty())

	// Fill in the MetaData as best we can.
	meta.mGroupName = std::string(name.toUtf8());

	meta.mGroupFlags = flags;
	meta.mSignFlags = getGroupSignFlags();

	if (!setCircleParameters(meta)){
		std::cerr << "GxsGroupDialog::prepareGroupMetaData()";
		std::cerr << " Invalid Circles";
		std::cerr << std::endl;
		return false;
	}//if (!setCircleParameters(meta))

	std::cerr << "void GxsGroupDialog::prepareGroupMetaData() meta.mCircleType: ";
	std::cerr << meta.mCircleType << " Internal: " << meta.mInternalCircle;
	std::cerr << " External: " << meta.mCircleId;
	std::cerr << std::endl;

	return true;
}

void GxsGroupDialog::createGroup()
{
	std::cerr << "GxsGroupDialog::createGroup()";
	std::cerr << std::endl;

	/* Check name */
	QString name = getName();
	if(name.isEmpty())
	{
		/* error message */
		QMessageBox::warning(this, "RetroShare", tr("Please add a Name"), QMessageBox::Ok, QMessageBox::Ok);
		return; //Don't add  a empty name!!
	}

	uint32_t token;
	RsGroupMetaData meta;
	if (!prepareGroupMetaData(meta))
	{
		/* error message */
		QMessageBox::warning(this, "RetroShare", tr("Failed to Prepare Group MetaData - please Review"), QMessageBox::Ok, QMessageBox::Ok);
		return; //Don't add with invalid circle.
	}

	if (service_CreateGroup(token, meta))
	{
		// get the Queue to handle response.
		if(mExternalTokenQueue != NULL)
			mExternalTokenQueue->queueRequest(token, TOKENREQ_GROUPINFO, RS_TOKREQ_ANSTYPE_ACK, GXSGROUP_NEWGROUPID);
	}

	close();
}
	
uint32_t GxsGroupDialog::getGroupSignFlags()
{
	/* grab from the ui options -> */
	uint32_t signFlags = 0;
	if (ui.publish_encrypt->isChecked()) {
		signFlags |= GXS_SERV::FLAG_GROUP_SIGN_PUBLISH_ENCRYPTED;
	} else if (ui.publish_required->isChecked()) {
		signFlags |= GXS_SERV::FLAG_GROUP_SIGN_PUBLISH_ALLSIGNED;
	} else if (ui.publish_threads->isChecked()) {
		signFlags |= GXS_SERV::FLAG_GROUP_SIGN_PUBLISH_THREADHEAD;
	} else {  // publish_open (default).
		signFlags |= GXS_SERV::FLAG_GROUP_SIGN_PUBLISH_NONEREQ;
	}

	if (ui.personal_required->isChecked()) 
		signFlags |= GXS_SERV::FLAG_AUTHOR_AUTHENTICATION_REQUIRED;
    
	if (ui.personal_ifnopub->isChecked()) 
		signFlags |= GXS_SERV::FLAG_AUTHOR_AUTHENTICATION_IFNOPUBSIGN;
    
	// Author Signature.
	if (ui.edit_antiSpam_signedIds->isChecked())
		signFlags |= GXS_SERV::FLAG_AUTHOR_AUTHENTICATION_GPG;
    
	if (ui.edit_antiSpam_trackMessages->isChecked())
		signFlags |= GXS_SERV::FLAG_AUTHOR_AUTHENTICATION_TRACK_MESSAGES;
    
	return signFlags;
}

void GxsGroupDialog::setGroupSignFlags(uint32_t signFlags)
{
	if (signFlags & GXS_SERV::FLAG_GROUP_SIGN_PUBLISH_ENCRYPTED) {
		ui.publish_encrypt->setChecked(true);
	} else if (signFlags & GXS_SERV::FLAG_GROUP_SIGN_PUBLISH_ALLSIGNED) {
		ui.publish_required->setChecked(true);
	} else if (signFlags & GXS_SERV::FLAG_GROUP_SIGN_PUBLISH_THREADHEAD) {
		ui.publish_threads->setChecked(true);
	} else if (signFlags & GXS_SERV::FLAG_GROUP_SIGN_PUBLISH_NONEREQ) {
		ui.publish_open->setChecked(true);
	}

	if (signFlags & GXS_SERV::FLAG_AUTHOR_AUTHENTICATION_REQUIRED) 
		ui.personal_required->setChecked(true);
    
	if (signFlags & GXS_SERV::FLAG_AUTHOR_AUTHENTICATION_IFNOPUBSIGN) 
		ui.personal_ifnopub->setChecked(true);
    
		ui.edit_antiSpam_trackMessages  ->setChecked((bool)(signFlags & GXS_SERV::FLAG_AUTHOR_AUTHENTICATION_TRACK_MESSAGES) );
		ui.show_antiSpam_trackMessages  ->setChecked((bool)(signFlags & GXS_SERV::FLAG_AUTHOR_AUTHENTICATION_TRACK_MESSAGES) );
		ui.edit_antiSpam_signedIds      ->setChecked((bool)(signFlags & GXS_SERV::FLAG_AUTHOR_AUTHENTICATION_GPG) );
		ui.show_antiSpam_signedIds      ->setChecked((bool)(signFlags & GXS_SERV::FLAG_AUTHOR_AUTHENTICATION_GPG) );
		//ui.SignEdIds->setChecked((bool)(signFlags & GXS_SERV::FLAG_AUTHOR_AUTHENTICATION_GPG) );
		//ui.trackmessagesradioButton->setChecked((bool)(signFlags & GXS_SERV::FLAG_AUTHOR_AUTHENTICATION_TRACK_MESSAGES) );
    
	/* guess at comments */
	if ((signFlags & GXS_SERV::FLAG_GROUP_SIGN_PUBLISH_THREADHEAD) &&
	    (signFlags & GXS_SERV::FLAG_AUTHOR_AUTHENTICATION_IFNOPUBSIGN))
	{
		ui.edit_comments_allowed->setChecked(true);
		ui.show_comments_allowed->setChecked(true);
	}
	else
	{
		ui.edit_comments_no->setChecked(true);
		ui.show_comments_no->setChecked(true);
	}
}

/**** Above logic is flawed, and will be removed shortly
 *
 *
 ****/

void GxsGroupDialog::updateCircleOptions()
{
	if (ui.edit_typeGroup->isChecked())
	{
		ui.circleComboBox->setEnabled(!(mReadonlyFlags & GXS_GROUP_FLAGS_DISTRIBUTION));
		ui.circleComboBox->setVisible(true);
	}
	else 
	{
		ui.circleComboBox->setEnabled(false);
		ui.circleComboBox->setVisible(false);
	}

	if (ui.edit_typeLocal->isChecked())
	{
		ui.localComboBox->setEnabled(!(mReadonlyFlags & GXS_GROUP_FLAGS_DISTRIBUTION));
		ui.localComboBox->setVisible(true);
	}
	else 
	{
		ui.localComboBox->setEnabled(false);
		ui.localComboBox->setVisible(false);
	}
}

bool GxsGroupDialog::setCircleParameters(RsGroupMetaData &meta)
{
	meta.mCircleType = GXS_CIRCLE_TYPE_PUBLIC;
	meta.mCircleId.clear();
	meta.mOriginator.clear();
	meta.mInternalCircle.clear();

	if (ui.edit_typePublic->isChecked())
	{
		meta.mCircleType = GXS_CIRCLE_TYPE_PUBLIC;
		meta.mCircleId.clear();
	}
	else if (ui.edit_typeGroup->isChecked())
	{
		meta.mCircleType = GXS_CIRCLE_TYPE_EXTERNAL;
		if (!ui.circleComboBox->getChosenCircle(meta.mCircleId))
		{
			return false;
		}
	}
	else if (ui.edit_typeLocal->isChecked())
	{
		meta.mCircleType = GXS_CIRCLE_TYPE_YOUREYESONLY;
		meta.mCircleId.clear();
		meta.mOriginator.clear();
		meta.mInternalCircle.clear() ;
	
		if (!ui.localComboBox->getChosenCircle(meta.mInternalCircle))
		{
			return false;
		}
	}
	else
	{
		return false;
	}
	return true;
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

	setLogo(img);
}

QPixmap GxsGroupDialog::getLogo()
{
	return mPicture;
}

void GxsGroupDialog::setLogo(const QPixmap &pixmap)
{
	mPicture = pixmap;

	// to show the selected
	ui.groupLogo->setPixmap(mPicture);
}

QString GxsGroupDialog::getName()
{
	return misc::removeNewLine(ui.edit_groupName->text());
}

QString GxsGroupDialog::getDescription()
{
	return ui.edit_groupDesc->toPlainText();
}

/***********************************************************************************
  Share Lists.
 ***********************************************************************************/

void GxsGroupDialog::sendShareList(std::string /*groupId*/)
{
	close();
}

void GxsGroupDialog::setShareList()
{
	if (ui.pubKeyShare_cb->isChecked()) {
		QMessageBox::warning(this, "", "ToDo");
		ui.pubKeyShare_cb->setChecked(false);
	}
//	if (ui.pubKeyShare_cb->isChecked()){
//		this->resize(this->size().width() + ui.contactsdockWidget->size().width(), this->size().height());
//		ui.contactsdockWidget->show();
//	} else {  // hide share widget
//		ui.contactsdockWidget->hide();
//		this->resize(this->size().width() - ui.contactsdockWidget->size().width(), this->size().height());
//	}
}

/***********************************************************************************
  Loading Group.
 ***********************************************************************************/

void GxsGroupDialog::requestGroup(const RsGxsGroupId &groupId)
{
	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;

	std::list<RsGxsGroupId> groupIds;
	groupIds.push_back(groupId);

	std::cerr << "GxsGroupDialog::requestGroup() Requesting Group Summary(" << groupId << ")";
	std::cerr << std::endl;

	uint32_t token;
	mInternalTokenQueue->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, groupIds, GXSGROUP_INTERNAL_LOADGROUP);
}

void GxsGroupDialog::loadGroup(uint32_t token)
{
	std::cerr << "GxsGroupDialog::loadGroup(" << token << ")";
	std::cerr << std::endl;

	QString description;
	if (service_loadGroup(token, mMode, mGrpMeta, description))
	{
		updateFromExistingMeta(description);
	}
}

void GxsGroupDialog::loadRequest(const TokenQueue *queue, const TokenRequest &req)
{
	std::cerr << "GxsGroupDialog::loadRequest() UserType: " << req.mUserType;
	std::cerr << std::endl;

	if (queue == mInternalTokenQueue)
	{
		/* now switch on req */
		switch(req.mUserType)
		{
			case GXSGROUP_INTERNAL_LOADGROUP:
				loadGroup(req.mToken);
				break;
			default:
				std::cerr << "GxsGroupDialog::loadGroup() UNKNOWN UserType ";
				std::cerr << std::endl;
				break;
		}
	}
}
