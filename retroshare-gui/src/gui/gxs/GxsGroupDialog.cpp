/*******************************************************************************
 * retroshare-gui/src/gui/gxs/GxsGroupDialog.cpp                               *
 *                                                                             *
 * Copyright 2012-2013  by Robert Fernie      <retroshare.project@gmail.com>   *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#include <QMessageBox>
#include <QPushButton>

#include "util/misc.h"
#include "util/DateTime.h"
#include "util/qtthreadsutils.h"
#include "GxsGroupDialog.h"
#include "gui/common/PeerDefs.h"
#include "gui/RetroShareLink.h"
#include "retroshare/rsgxsflags.h"

#include <algorithm>

#include <retroshare/rspeers.h>
#include <retroshare/rsgxscircles.h>

#include <gui/settings/rsharesettings.h>

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
GxsGroupDialog::GxsGroupDialog(uint32_t enableFlags, uint32_t defaultFlags, QWidget *parent)
    : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint), mGrpMeta(), mMode(MODE_CREATE), mEnabledFlags(enableFlags), mReadonlyFlags(0), mDefaultsFlags(defaultFlags)
{
	/* Invoke the Qt Designer generated object setup routine */
	ui.setupUi(this);
	
	init();
}

GxsGroupDialog::GxsGroupDialog(Mode mode, RsGxsGroupId groupId, uint32_t enableFlags, uint32_t defaultFlags, QWidget *parent)
    : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint), mGrpMeta(), mMode(mode), mEnabledFlags(enableFlags), mReadonlyFlags(0), mDefaultsFlags(defaultFlags)
{
	/* Invoke the Qt Designer generated object setup routine */
	ui.setupUi(this);

	mGrpMeta.mGroupId = groupId;

	init();
}

GxsGroupDialog::~GxsGroupDialog()
{
	Settings->saveWidgetInformation(this);
}

void GxsGroupDialog::init()
{
	// connect up the buttons.
	connect(ui.postButton, SIGNAL(clicked()), this, SLOT(submitGroup()));
	connect(ui.cancelButton, SIGNAL(clicked()), this, SLOT(cancelDialog()));
	connect(ui.pubKeyShare_cb, SIGNAL(clicked()), this, SLOT(setShareList()));
	connect(ui.addAdmins_cb, SIGNAL(clicked()), this, SLOT(setAdminsList()));
	connect(ui.filtercomboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(filterComboBoxChanged(int)));


	connect(ui.groupLogo, SIGNAL(clicked() ), this , SLOT(addGroupLogo()));

	ui.typePublic->setChecked(true);
	ui.distributionValueLabel->setText(tr("Public"));
	updateCircleOptions();

	connect(ui.typePublic, SIGNAL(clicked()), this , SLOT(updateCircleOptions()));
	connect(ui.typeGroup, SIGNAL(clicked()), this , SLOT(updateCircleOptions()));
	connect(ui.typeLocal, SIGNAL(clicked()), this , SLOT(updateCircleOptions()));

	if (!ui.pubKeyShare_cb->isChecked())
	{
		ui.shareKeyList->hide();
		//this->resize(this->size().width() - ui.contactsdockWidget->size().width(), this->size().height());
	}
	if (!ui.addAdmins_cb->isChecked())
	{
		ui.adminsList->hide();
		ui.filtercomboBox->hide();
		//this->resize(this->size().width() - ui.contactsdockWidget->size().width(), this->size().height());
	}
	
	/* Add filter types */
    ui.filtercomboBox->addItem(tr("All People"));
    ui.filtercomboBox->addItem(tr("My Contacts"));
	ui.filtercomboBox->setCurrentIndex(0);

	/* initialize key share list */
	ui.shareKeyList->setHeaderText(tr("Contacts:"));
	ui.shareKeyList->setModus(FriendSelectionWidget::MODUS_CHECK);
	ui.shareKeyList->start();

	/* initialize key share list */
	ui.adminsList->setHeaderText(tr("Moderators:"));
	ui.adminsList->setModus(FriendSelectionWidget::MODUS_CHECK);
	ui.adminsList->setShowType(FriendSelectionWidget::SHOW_GXS);
	ui.adminsList->start();
	

	/* Setup Reasonable Defaults */

	ui.idChooser->loadIds(0,RsGxsId());
    ui.circleComboBox->loadCircles(RsGxsCircleId());
    ui.localComboBox->loadGroups(0, RsNodeGroupId());
	
	ui.groupDesc->setPlaceholderText(tr("Set a descriptive description here"));

    	ui.personal_ifnopub->hide() ;
    	ui.personal_required->hide() ;
    	ui.personal_required->setChecked(true) ;	// this is always true

	initMode();
	Settings->loadWidgetInformation(this);
}

void GxsGroupDialog::injectExtraWidget(QWidget *widget)
{
    // add extra widget into layout.
    QVBoxLayout *vbox = new QVBoxLayout();
    vbox->addWidget(widget);
    ui.extraFrame->setLayout(vbox);
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

    if(!mGrpMeta.mGroupId.isNull() && mGrpMeta.mPublishTs == 0) // group not actually loaded yet
		loadGroup(mGrpMeta.mGroupId);
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
	case UITYPE_ADD_ADMINS_CHECKBOX:
		//ui.contactsdockWidget->setWindowTitle(text);
		break;
	case UITYPE_BUTTONBOX_OK:
		ui.postButton->setText(text);
		break;
	}
}

void GxsGroupDialog::setUiToolTip(UiType uiType, const QString &text)
{
	switch (uiType)
	{
	case UITYPE_KEY_SHARE_CHECKBOX:
		ui.pubKeyShare_cb->setToolTip(text);
		break;
	case UITYPE_ADD_ADMINS_CHECKBOX:
		ui.addAdmins_cb->setToolTip(text);
		break;
	case UITYPE_BUTTONBOX_OK:
		ui.postButton->setToolTip(text);
    default:
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
			newGroup();
		}
		break;

		case MODE_SHOW:
		{
			ui.stackedWidget->setCurrentIndex(1);
			mReadonlyFlags = 0xffffffff; // Force all to readonly.
			ui.postButton->hide();
		}
		break;

		case MODE_EDIT:
		{
            ui.stackedWidget->setCurrentIndex(0);
			ui.postButton->setText(tr("Submit Group Changes"));
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
			ui.distributionValueLabel->setText(tr("Public"));
            		ui.distributionCircleComboBox->setVisible(false) ;
		}
		else if (mDefaultsFlags & GXS_GROUP_DEFAULTS_DISTRIB_GROUP)
		{
			ui.typeGroup->setChecked(true);
			ui.distributionValueLabel->setText(tr("Restricted to circle:"));
            		ui.distributionCircleComboBox->setVisible(true) ;
		}
		else if (mDefaultsFlags & GXS_GROUP_DEFAULTS_DISTRIB_LOCAL)
		{
			ui.typeLocal->setChecked(true);
			ui.distributionValueLabel->setText(tr("Limited to your friends"));
            		ui.distributionCircleComboBox->setVisible(false) ;
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
			ui.commentsValueLabel->setText(tr("Allowed"));
		}
		else if (mDefaultsFlags & GXS_GROUP_DEFAULTS_COMMENTS_NO)
		{
			ui.comments_no->setChecked(true);
			ui.commentsValueLabel->setText(tr("Disallowed"));
		}
		else
		{
			// default
			ui.comments_no->setChecked(true);
			ui.commentsValueLabel->setText(tr("Allowed"));
		}
	}
        if( (mDefaultsFlags & GXS_GROUP_DEFAULTS_ANTISPAM_FAVOR_PGP) && (mDefaultsFlags & GXS_GROUP_DEFAULTS_ANTISPAM_FAVOR_PGP_KNOWN))
		ui.antiSpam_perms_CB->setCurrentIndex(2) ;
        else if(mDefaultsFlags & GXS_GROUP_DEFAULTS_ANTISPAM_FAVOR_PGP)
		ui.antiSpam_perms_CB->setCurrentIndex(1) ;
        else
		ui.antiSpam_perms_CB->setCurrentIndex(0) ;
        
        QString antispam_string ;
        if(mDefaultsFlags & GXS_GROUP_DEFAULTS_ANTISPAM_TRACK) antispam_string += tr("Message tracking") ;
	if(mDefaultsFlags & GXS_GROUP_DEFAULTS_ANTISPAM_FAVOR_PGP) antispam_string += (antispam_string.isNull()?"":" and ")+tr("PGP signature required") ;
    
    	ui.antiSpamValueLabel->setText(antispam_string) ;
        
#ifndef RS_USE_CIRCLES
    ui.typeGroup->setEnabled(false);
    ui.typeGroup_3->setEnabled(false);
    ui.typeLocal_3->setEnabled(false);
#endif
}

void GxsGroupDialog::setupVisibility()
{
	ui.groupName->setVisible(mEnabledFlags & GXS_GROUP_FLAGS_NAME);

	ui.groupLogo->setVisible(mEnabledFlags & GXS_GROUP_FLAGS_ICON);

	ui.groupDesc->setVisible(mEnabledFlags & GXS_GROUP_FLAGS_DESCRIPTION);

	ui.distribGroupBox->setVisible(mEnabledFlags & GXS_GROUP_FLAGS_DISTRIBUTION);
	ui.distributionLabel->setVisible(mEnabledFlags & GXS_GROUP_FLAGS_DISTRIBUTION);
	ui.distributionValueLabel->setVisible(mEnabledFlags & GXS_GROUP_FLAGS_DISTRIBUTION);
    
	ui.spamProtection_GB->setVisible(mEnabledFlags & GXS_GROUP_FLAGS_ANTI_SPAM);
	ui.antiSpamLabel->setVisible(mEnabledFlags & GXS_GROUP_FLAGS_ANTI_SPAM);
	ui.antiSpamValueLabel->setVisible(mEnabledFlags & GXS_GROUP_FLAGS_ANTI_SPAM);

	ui.publishGroupBox->setVisible(mEnabledFlags & GXS_GROUP_FLAGS_PUBLISHSIGN);

	ui.pubKeyShare_cb->setVisible(mEnabledFlags & GXS_GROUP_FLAGS_SHAREKEYS);
	ui.addAdmins_cb->setVisible(mEnabledFlags & GXS_GROUP_FLAGS_ADDADMINS);
	ui.moderatorsLabel->setVisible(mEnabledFlags & GXS_GROUP_FLAGS_ADDADMINS);
	ui.moderatorsValueLabel->setVisible(mEnabledFlags & GXS_GROUP_FLAGS_ADDADMINS);

	ui.personalGroupBox->setVisible(mEnabledFlags & GXS_GROUP_FLAGS_PERSONALSIGN);

	ui.commentGroupBox->setVisible(mEnabledFlags & GXS_GROUP_FLAGS_COMMENTS);
	ui.commentsLabel->setVisible(mEnabledFlags & GXS_GROUP_FLAGS_COMMENTS);
	ui.commentsValueLabel->setVisible(mEnabledFlags & GXS_GROUP_FLAGS_COMMENTS);

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

	ui.publishGroupBox->setEnabled(!(mReadonlyFlags & GXS_GROUP_FLAGS_PUBLISHSIGN));

	ui.pubKeyShare_cb->setEnabled(!(mReadonlyFlags & GXS_GROUP_FLAGS_SHAREKEYS));

	ui.personalGroupBox->setEnabled(!(mReadonlyFlags & GXS_GROUP_FLAGS_PERSONALSIGN));
	
	ui.idChooser->setEnabled(!(mReadonlyFlags & GXS_GROUP_FLAGS_PERSONALSIGN));

	//ui.distribGroupBox_2->setEnabled(!(mReadonlyFlags & GXS_GROUP_FLAGS_DISTRIBUTION));
	//ui.commentGroupBox_2->setEnabled(!(mReadonlyFlags & GXS_GROUP_FLAGS_COMMENTS));
	ui.spamProtection_GB->setEnabled(!(mReadonlyFlags & GXS_GROUP_FLAGS_ANTI_SPAM));
	//ui.spamProtection_GB_2->setEnabled(!(mReadonlyFlags & GXS_GROUP_FLAGS_ANTI_SPAM));

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
	
	RetroShareLink link;

    /* setup name */
    ui.groupName->setText(QString::fromUtf8(mGrpMeta.mGroupName.c_str()));

    /* Show Mode */
    ui.nameline->setText(QString::fromUtf8(mGrpMeta.mGroupName.c_str()));
    ui.popline->setText(QString::number( mGrpMeta.mPop)) ;
    ui.postsline->setText(QString::number(mGrpMeta.mVisibleMsgCount));
    if(mGrpMeta.mLastPost==0)
        ui.lastpostline->setText(tr("Never"));
    else
        ui.lastpostline->setText(DateTime::formatLongDateTime(mGrpMeta.mLastPost));
    ui.authorValueLabel->setId(mGrpMeta.mAuthorId);
	
    ui.createdline->setText(DateTime::formatLongDateTime(mGrpMeta.mPublishTs));

	link = RetroShareLink::createMessage(mGrpMeta.mAuthorId, "");
	ui.authorValueLabel->setText(link.toHtml());
	
    ui.IDline->setText(QString::fromStdString(mGrpMeta.mGroupId.toStdString()));
    ui.descriptiontextEdit->setPlainText(description);

    switch (mode())
    {
    case MODE_CREATE:{
    }
        break;
    case MODE_SHOW:{
        ui.headerFrame->setHeaderText(QString::fromUtf8(mGrpMeta.mGroupName.c_str()));
        if (!mPicture.isNull())
            ui.headerFrame->setHeaderImage(mPicture);
    }
        break;
    case MODE_EDIT:{
    }
        break;
    }
    /* set description */
    ui.groupDesc->setPlainText(description);
    QString distribution_string = "[Unknown]";

    switch(mGrpMeta.mCircleType)
    {
    case GXS_CIRCLE_TYPE_YOUR_FRIENDS_ONLY:
    {
        ui.typeLocal->setChecked(true);
        distribution_string = tr("Only friends nodes in group ") ;

        RsGroupInfo ginfo ;
        rsPeers->getGroupInfo(RsNodeGroupId(mGrpMeta.mInternalCircle),ginfo) ;

        QString desc;
        GroupChooser::makeNodeGroupDesc(ginfo, desc);
        distribution_string += desc ;

        ui.localComboBox->loadGroups(0, RsNodeGroupId(mGrpMeta.mInternalCircle));
        ui.distributionCircleComboBox->setVisible(false) ;
        ui.localComboBox->setVisible(true) ;
    }
        break;
    case GXS_CIRCLE_TYPE_PUBLIC:
        ui.typePublic->setChecked(true);
        distribution_string = tr("Public") ;
        ui.distributionCircleComboBox->setVisible(false) ;
        ui.localComboBox->setVisible(false) ;
        break;
    case GXS_CIRCLE_TYPE_EXTERNAL:
        ui.typeGroup->setChecked(true);
        distribution_string = tr("Restricted to circle:") ;
        ui.localComboBox->setVisible(false) ;
        ui.distributionCircleComboBox->setVisible(true) ;
        ui.distributionCircleComboBox->loadCircles(mGrpMeta.mCircleId);
        ui.circleComboBox->loadCircles(mGrpMeta.mCircleId);
        break;
    default:
        std::cerr << "CreateCircleDialog::updateCircleGUI() INVALID mCircleType";
        std::cerr << std::endl;
        break;
    }
    ui.distributionValueLabel->setText(distribution_string) ;

    ui.idChooser->loadIds(0, mGrpMeta.mAuthorId);

    if(!mGrpMeta.mAuthorId.isNull())
        ui.idChooser->setChosenId(mGrpMeta.mAuthorId) ;

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
	QString reason;
	if(!prepareGroupMetaData(newMeta, reason))
	{
		/* error message */
		QMessageBox::warning(this, "RetroShare", tr("Failed to Prepare Group MetaData: ") + reason, QMessageBox::Ok, QMessageBox::Ok);
		return; //Don't add  a empty name!!
	}

	std::cerr << "GxsGroupDialog::editGroup() calling service_EditGroup";
	std::cerr << std::endl;

	if (!service_updateGroup(newMeta))
	{
		std::cerr << "GxsGroupDialog::editGroup() ERROR";
		std::cerr << std::endl;
	}

	close();
}

bool GxsGroupDialog::prepareGroupMetaData(RsGroupMetaData &meta, QString &reason)
{
	std::cerr << "GxsGroupDialog::prepareGroupMetaData()";
	std::cerr << std::endl;

    // here would be the place to check for empty author id
    // but GXS_SERV::GRP_OPTION_AUTHEN_AUTHOR_SIGN is currently not used by any service

    ui.idChooser->getChosenId(meta.mAuthorId);
    if ((mDefaultsFlags & GXS_GROUP_DEFAULTS_PERSONAL_GROUP) && (meta.mAuthorId.isNull())) {
		std::cerr << "GxsGroupDialog::prepareGroupMetaData()";
		std::cerr << " Group needs a Personal Signature";
		std::cerr << std::endl;
		reason = "Missing AuthorId";
		return false;
	}

	QString name = getName();
	uint32_t flags = GXS_SERV::FLAG_PRIVACY_PUBLIC;

	if(name.isEmpty()) {
		std::cerr << "GxsGroupDialog::prepareGroupMetaData()";
		std::cerr << " Invalid GroupName";
		std::cerr << std::endl;
		reason = "Missing GroupName";
		return false;
	}

	// Fill in the MetaData as best we can.
	meta.mGroupName = std::string(name.toUtf8());

	meta.mGroupFlags = flags;
	meta.mSignFlags = getGroupSignFlags();

	if (!setCircleParameters(meta)){
		std::cerr << "GxsGroupDialog::prepareGroupMetaData()";
		std::cerr << " Invalid Circles";
		std::cerr << std::endl;
		reason = "Invalid Circle Parameters";
		return false;
	}

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

	RsGroupMetaData meta;
	QString reason;
	if (!prepareGroupMetaData(meta, reason))
	{
		/* error message */
		QMessageBox::warning(this, "RetroShare", tr("Failed to Prepare Group MetaData: ") + reason, QMessageBox::Ok, QMessageBox::Ok);
		return; //Don't add with invalid circle.
	}

	if (service_createGroup(meta))
	{
        // now update the UI
#warning Missing code here!
#ifdef TODO
        //
		// get the Queue to handle response. What is this for?
		if(mExternalTokenQueue != NULL)
			mExternalTokenQueue->queueRequest(token, TOKENREQ_GROUPINFO, RS_TOKREQ_ANSTYPE_ACK, GXSGROUP_NEWGROUPID);
#endif
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
    switch(ui.antiSpam_perms_CB->currentIndex()) 
    {
    case 0: break ;
    case 2: signFlags |= GXS_SERV::FLAG_AUTHOR_AUTHENTICATION_GPG_KNOWN;	// no break below, since we want *both* flags in this case.
        /* fallthrough */
    case 1:  signFlags |= GXS_SERV::FLAG_AUTHOR_AUTHENTICATION_GPG;
	    break ;
    }

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
    
        		if( (signFlags & GXS_SERV::FLAG_AUTHOR_AUTHENTICATION_GPG_KNOWN) && (signFlags & GXS_SERV::FLAG_AUTHOR_AUTHENTICATION_GPG))
                    ui.antiSpam_perms_CB->setCurrentIndex(2) ;
                else if(signFlags & GXS_SERV::FLAG_AUTHOR_AUTHENTICATION_GPG)
                    ui.antiSpam_perms_CB->setCurrentIndex(1) ;
		else
                    ui.antiSpam_perms_CB->setCurrentIndex(0) ;
                
        QString antispam_string ;
        if(signFlags & GXS_SERV::FLAG_AUTHOR_AUTHENTICATION_TRACK_MESSAGES) antispam_string += tr("Message tracking") ;
	if(signFlags & GXS_SERV::FLAG_AUTHOR_AUTHENTICATION_GPG_KNOWN)      antispam_string += (antispam_string.isNull()?"":" and ")+tr("PGP signature from known ID required") ;
    	else
	if(signFlags & GXS_SERV::FLAG_AUTHOR_AUTHENTICATION_GPG)            antispam_string += (antispam_string.isNull()?"":" and ")+tr("PGP signature required") ;
    
    	ui.antiSpamValueLabel->setText(antispam_string) ;
		//ui.antiSpam_trackMessages_2->setChecked((bool)(signFlags & GXS_SERV::FLAG_AUTHOR_AUTHENTICATION_TRACK_MESSAGES) );
		//ui.antiSpam_signedIds_2    ->setChecked((bool)(signFlags & GXS_SERV::FLAG_AUTHOR_AUTHENTICATION_GPG) );
		//ui.SignEdIds->setChecked((bool)(signFlags & GXS_SERV::FLAG_AUTHOR_AUTHENTICATION_GPG) );
		//ui.trackmessagesradioButton->setChecked((bool)(signFlags & GXS_SERV::FLAG_AUTHOR_AUTHENTICATION_TRACK_MESSAGES) );
    
	/* guess at comments */
	if ((signFlags & GXS_SERV::FLAG_GROUP_SIGN_PUBLISH_THREADHEAD) &&
	    (signFlags & GXS_SERV::FLAG_AUTHOR_AUTHENTICATION_IFNOPUBSIGN))
	{
        	// (cyril) very weird piece of code. Need to clear this up.
        
		ui.comments_allowed->setChecked(true);
		ui.commentsValueLabel->setText("Allowed") ;
	}
	else
	{
		ui.comments_no->setChecked(true);
		ui.commentsValueLabel->setText("Forbidden") ;
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
		ui.circleComboBox->setEnabled(!(mReadonlyFlags & GXS_GROUP_FLAGS_DISTRIBUTION));
		ui.circleComboBox->setVisible(true);
	}
	else 
	{
		ui.circleComboBox->setEnabled(false);
		ui.circleComboBox->setVisible(false);
	}

	if (ui.typeLocal->isChecked())
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
            return false;
        }
    }
    else if (ui.typeLocal->isChecked())
    {
        meta.mCircleType = GXS_CIRCLE_TYPE_YOUR_FRIENDS_ONLY;
        meta.mCircleId.clear();
        meta.mOriginator.clear();
        meta.mInternalCircle.clear() ;

        RsNodeGroupId ngi ;

        if (!ui.localComboBox->getChosenGroup(ngi))
            return false;

        meta.mInternalCircle = RsGxsCircleId(ngi) ;
    }
    else
        return false;

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
	return misc::removeNewLine(ui.groupName->text());
}

QString GxsGroupDialog::getDescription()
{
	return ui.groupDesc->toPlainText();
}

void GxsGroupDialog::getSelectedModerators(std::set<RsGxsId>& ids)
{
	ui.adminsList->selectedIds<RsGxsId,FriendSelectionWidget::IDTYPE_GXS>(ids, true);
}

void GxsGroupDialog::setSelectedModerators(const std::set<RsGxsId>& ids)
{
    ui.addAdmins_cb->setChecked(true);
	ui.adminsList->show();
	ui.filtercomboBox->show();

	ui.adminsList->setSelectedIds<RsGxsId,FriendSelectionWidget::IDTYPE_GXS>(ids, false);

	QString moderatorsListString ;
    RsIdentityDetails det;
	RetroShareLink link;

    for(auto it(ids.begin());it!=ids.end();++it)
    {
		rsIdentity->getIdDetails(*it,det);

        if(!moderatorsListString.isNull())
            moderatorsListString += ", " ;

        if(det.mNickname.empty())
			moderatorsListString += "[Unknown]";
		
		link = RetroShareLink::createMessage(det.mId, "");
		if (link.valid())
				moderatorsListString += link.toHtml() + "   ";

    }
	//ui.moderatorsValueLabel->setId(det.mId);
	ui.moderatorsValueLabel->setText(moderatorsListString);
}

/***********************************************************************************
  Share Lists.
 ***********************************************************************************/

void GxsGroupDialog::sendShareList(std::string /*groupId*/)
{
	close();
}

void GxsGroupDialog::setAdminsList()
{
	if (ui.addAdmins_cb->isChecked())
    {
		//this->resize(this->size().width() + ui.contactsdockWidget->size().width(), this->size().height());
		ui.adminsList->show();
		ui.filtercomboBox->show();
	}
    else
    {  // hide share widget
		ui.adminsList->hide();
		ui.filtercomboBox->hide();
		//this->resize(this->size().width() - ui.contactsdockWidget->size().width(), this->size().height());
	}
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

void GxsGroupDialog::filterComboBoxChanged(int i)
{
	switch(i)
	{
	default:
	case 0:
		ui.adminsList->setShowType(FriendSelectionWidget::SHOW_GXS);
		break;
	case 1:
		ui.adminsList->setShowType(FriendSelectionWidget::SHOW_CONTACTS);
		break;
	}
}


/***********************************************************************************
  Loading Group.
 ***********************************************************************************/

void GxsGroupDialog::loadGroup(const RsGxsGroupId& grpId)
{
	RsThread::async([this,grpId]()
	{
		RsGxsGenericGroupData *groupData;

		if(!service_getGroupData(grpId,groupData))
		{
			std::cerr << __PRETTY_FUNCTION__ << " failed to collect group info " << std::endl;
			return;
		}

		RsQThreadUtils::postToObject( [this,groupData]()
		{
			/* Here it goes any code you want to be executed on the Qt Gui
			 * thread, for example to update the data model with new information
			 * after a blocking call to RetroShare API complete, note that
			 * Qt::QueuedConnection is important!
			 */

            mGrpMeta = groupData->mMeta;

			QString description;

			if (service_loadGroup(groupData, mMode, description))
				updateFromExistingMeta(description);

            delete groupData;

		}, this );
	});
}

