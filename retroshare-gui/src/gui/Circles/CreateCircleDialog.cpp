/*******************************************************************************
 * gui/Circles/CreateCirclesDialog.cpp                                         *
 *                                                                             *
 * LibResAPI: API for local socket server                                      *
 *                                                                             *
 * Copyright (C) 2012, Robert Fernie <retroshare.project@gmail.com>            *
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
#include <QMenu>

#include <algorithm>

#include <retroshare/rspeers.h>
#include <retroshare/rsidentity.h>

#include "gui/common/AvatarDefs.h"
#include "gui/Circles/CreateCircleDialog.h"
#include "gui/gxs/GxsIdDetails.h"
#include "gui/Identity/IdDialog.h"
#include "gui/Identity/IdEditDialog.h"

//#define DEBUG_CREATE_CIRCLE_DIALOG 1

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
			
	ui.headerFrame->setHeaderImage(QPixmap(":/icons/png/circles.png"));

	// connect up the buttons.
	connect(ui.addButton, SIGNAL(clicked()), this, SLOT(addMember()));
	connect(ui.removeButton, SIGNAL(clicked()), this, SLOT(removeMember()));

	connect(ui.buttonBox, SIGNAL(accepted()), this, SLOT(createCircle()));
	connect(ui.buttonBox, SIGNAL(rejected()), this, SLOT(close()));

	connect(ui.treeWidget_membership, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(selectedMember(QTreeWidgetItem*, QTreeWidgetItem*)));
	connect(ui.treeWidget_IdList, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(selectedId(QTreeWidgetItem*, QTreeWidgetItem*)));
	
	connect(ui.treeWidget_IdList, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(IdListCustomPopupMenu(QPoint)));
	connect(ui.treeWidget_membership, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(MembershipListCustomPopupMenu(QPoint)));
	
	connect(ui.IdFilter, SIGNAL(textChanged(QString)), this, SLOT(filterChanged(QString)));

	//connect(ui.toolButton_NewId, SIGNAL(clicked()), this, SLOT(createNewGxsId()));

	/* Add filter actions */
	QTreeWidgetItem *headerItem = ui.treeWidget_IdList->headerItem();
	QString headerText = headerItem->text(RSCIRCLEID_COL_NICKNAME);
	ui.IdFilter->addFilter(QIcon(), headerText, RSCIRCLEID_COL_NICKNAME, QString("%1 %2").arg(tr("Search"), headerText));
	headerText = headerItem->text(RSCIRCLEID_COL_KEYID);
	ui.IdFilter->addFilter(QIcon(), headerText, RSCIRCLEID_COL_KEYID, QString("%1 %2").arg(tr("Search"), headerText));
	
	ui.removeButton->setEnabled(false);
	ui.addButton->setEnabled(false);
	ui.radioButton_ListAll->setChecked(true);

	QObject::connect(ui.radioButton_ListAll, SIGNAL(toggled(bool)), this, SLOT(idTypeChanged())) ;
	QObject::connect(ui.radioButton_ListAllPGP, SIGNAL(toggled(bool)), this, SLOT(idTypeChanged())) ;
	QObject::connect(ui.radioButton_ListKnownPGP, SIGNAL(toggled(bool)), this, SLOT(idTypeChanged())) ;

	QObject::connect(ui.radioButton_Public, SIGNAL(toggled(bool)), this, SLOT(updateCircleType(bool))) ;
	QObject::connect(ui.radioButton_Self, SIGNAL(toggled(bool)), this, SLOT(updateCircleType(bool))) ;
	QObject::connect(ui.radioButton_Restricted, SIGNAL(toggled(bool)), this, SLOT(updateCircleType(bool))) ;
    
	ui.radioButton_Public->setChecked(true) ;
    
	mIsExistingCircle = false;
	mIsExternalCircle = true;
	mClearList = true;
#if QT_VERSION >= 0x040700
        ui.circleName->setPlaceholderText(QApplication::translate("CreateCircleDialog", "Circle name", 0));
#endif
        
    //ui.idChooser->loadIds(0,RsGxsId());
    ui.circleComboBox->loadCircles(RsGxsCircleId());
}

CreateCircleDialog::~CreateCircleDialog()
{
	delete(mCircleQueue);
	delete(mIdQueue);
}

void CreateCircleDialog::editExistingId(const RsGxsGroupId &circleId, const bool &clearList /*= true*/,bool readonly)
{
	/* load this circle */
	mIsExistingCircle = true;
    	mReadOnly=readonly;

	mClearList = clearList;
	
    if(readonly)
	ui.headerFrame->setHeaderText(tr("Circle Details"));
        else
	ui.headerFrame->setHeaderText(tr("Edit Circle"));

    ui.radioButton_Public->setEnabled(!readonly) ;
    ui.radioButton_Self->setEnabled(!readonly) ;
    ui.radioButton_Restricted->setEnabled(!readonly) ;
    ui.circleName->setReadOnly(readonly) ;
    
    if(readonly)
    {
	    ui.circleAdminLabel->setVisible(true) ;
	    ui.idChooser->setVisible(false) ;
    }
    else
    {
	    ui.circleAdminLabel->setVisible(false) ;
	    ui.circleAdminLabel->hide();
	    ui.idChooser->setVisible(true) ;
    }
    
    ui.buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Update"));
    
	ui.addButton->setEnabled(!readonly) ;
	ui.removeButton->setEnabled(!readonly) ;
    
    if(readonly)
    {
	ui.buttonBox->setStandardButtons(QDialogButtonBox::Cancel);
	ui.buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("Close"));
    	ui.peersSelection_GB->hide() ;
	ui.addButton->hide() ;
	ui.removeButton->hide() ;
    }
	requestCircle(circleId);
}


void CreateCircleDialog::editNewId(bool isExternal)
{
	/* load this circle */
	mIsExistingCircle = false;
    	mReadOnly = false ;

	/* setup personal or external circle */
	if (isExternal)
	{
		setupForExternalCircle();
		ui.headerFrame->setHeaderText(tr("Create New Circle"));	
		ui.buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Create"));
	}
	else
	{
		setupForPersonalCircle();
		ui.headerFrame->setHeaderText(tr("Create New Circle"));	
	}

	/* enable stuff that might be locked */
}

void CreateCircleDialog::updateCircleType(bool b)
{
    if(!b)
	    return ;	// no need to change when b<-false

    //if(ui.radioButton_Self->isChecked())
	   // setupForPersonalCircle() ;
    //else
    setupForExternalCircle() ;

    if(ui.radioButton_Restricted->isChecked())
    {
	    ui.circleComboBox->setEnabled(true) ;
		ui.circleComboBox->show() ;
    }
    else
    {
	    ui.circleComboBox->setEnabled(false) ;
		ui.circleComboBox->hide() ;
    }
}

void CreateCircleDialog::setupForPersonalCircle()
{
	mIsExternalCircle = false;

	/* hide distribution line */

	ui.groupBox_title->setTitle(tr("Circle Details"));
	ui.frame_PgpTypes->hide();
	//ui.frame_Distribution->hide();
	ui.idChooserLabel->hide();
	ui.circleAdminLabel->hide();
	ui.idChooser->hide();
	//ui.toolButton_NewId->hide();

	//getPgpIdentities();
}

void CreateCircleDialog::setupForExternalCircle()
{
	mIsExternalCircle = true;

	/* show distribution line */
	ui.groupBox_title->setTitle(tr("Circle Details"));
        
	ui.frame_PgpTypes->show();
	ui.frame_Distribution->show();
	ui.idChooserLabel->show();
	ui.circleAdminLabel->show();
	ui.idChooser->show();
	//ui.toolButton_NewId->show();
	
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
	if (!item) return;

	/* check that its not there already */
	QString keyId    = item->text(RSCIRCLEID_COL_KEYID);
	QString idtype   = item->text(RSCIRCLEID_COL_IDTYPE);
	QString nickname = item->text(RSCIRCLEID_COL_NICKNAME);
	QIcon   icon     = item->icon(RSCIRCLEID_COL_NICKNAME);

	addMember(keyId, idtype, nickname, icon);
}

void CreateCircleDialog::addMember(const RsGxsIdGroup &idGroup)
{
	QString  keyId = QString::fromStdString(idGroup.mMeta.mGroupId.toStdString());
	QString  nickname = QString::fromUtf8(idGroup.mMeta.mGroupName.c_str());
	QString  idtype = tr("Anon Id");

	QPixmap pixmap ;

	if(idGroup.mImage.mSize == 0 || !pixmap.loadFromData(idGroup.mImage.mData, idGroup.mImage.mSize, "PNG"))
		pixmap = QPixmap::fromImage(GxsIdDetails::makeDefaultIcon(RsGxsId(idGroup.mMeta.mGroupId)));

	if (idGroup.mPgpKnown){
		RsPeerDetails details;
		rsPeers->getGPGDetails(idGroup.mPgpId, details);
		idtype = QString::fromUtf8(details.name.c_str());
	}//if (idGroup.mPgpKnown)
	addMember(keyId, idtype, nickname, QIcon(pixmap));
}

void CreateCircleDialog::addMember(const QString& keyId, const QString& idtype, const QString& nickname )
{
	QIcon icon;
	addMember(keyId, idtype, nickname, icon);
}

void CreateCircleDialog::addMember(const QString& keyId, const QString& idtype, const QString& nickname, const QIcon& icon)
{
	QTreeWidget *tree = ui.treeWidget_membership;

	int count = tree->topLevelItemCount();
	for(int i = 0; i < count; ++i){
		QTreeWidgetItem *item = tree->topLevelItem(i);
		if (keyId == item->text(RSCIRCLEID_COL_KEYID)) {
#ifdef DEBUG_CREATE_CIRCLE_DIALOG 
			std::cerr << "CreateCircleDialog::addMember() Already is a Member: " << keyId.toStdString();
			std::cerr << std::endl;
#endif
			return;
		}//if (keyId == item->text(RSCIRCLEID_COL_KEYID))
	}//for(int i = 0; i < count; ++i)

	QTreeWidgetItem *member = new QTreeWidgetItem();
	member->setText(RSCIRCLEID_COL_NICKNAME, nickname);
	member->setIcon(RSCIRCLEID_COL_NICKNAME, icon);
	member->setText(RSCIRCLEID_COL_KEYID, keyId);
	member->setText(RSCIRCLEID_COL_IDTYPE, idtype);

	tree->addTopLevelItem(member);
	
	ui.members_groupBox->setTitle( tr("Invited Members") + " (" + QString::number(ui.treeWidget_membership->topLevelItemCount()) + ")" );
}

/** Maybe we can use RsGxsCircleGroup instead of RsGxsCircleDetails ??? (TODO)**/
void CreateCircleDialog::addCircle(const RsGxsCircleDetails &cirDetails)
{
	typedef std::set<RsGxsId>::iterator itUnknownPeers;
	for (itUnknownPeers it = cirDetails.mAllowedGxsIds.begin()
	     ; it != cirDetails.mAllowedGxsIds.end()
	     ; ++it) {
		RsGxsId gxs_id = *it;
		RsIdentityDetails gxs_details ;
		if(!gxs_id.isNull() && rsIdentity->getIdDetails(gxs_id,gxs_details)) {

			QString  keyId = QString::fromStdString(gxs_id.toStdString());
			QString  nickname = QString::fromUtf8(gxs_details.mNickname.c_str());
			QString  idtype = tr("Anon Id");

			QPixmap pixmap ;

			if(gxs_details.mAvatar.mSize == 0 || !pixmap.loadFromData(gxs_details.mAvatar.mData, gxs_details.mAvatar.mSize, "PNG"))
				pixmap = QPixmap::fromImage(GxsIdDetails::makeDefaultIcon(gxs_details.mId));

			addMember(keyId, idtype, nickname, QIcon(pixmap));

		}//if(!gxs_id.isNull() && rsIdentity->getIdDetails(gxs_id,gxs_details))
	}//for (itUnknownPeers it = cirDetails.mUnknownPeers.begin()

	typedef std::set<RsPgpId>::const_iterator itAllowedPeers;
	for (itAllowedPeers it = cirDetails.mAllowedNodes.begin() ; it != cirDetails.mAllowedNodes.end() ; ++it ) 
	{
		RsPgpId gpg_id = *it;
		RsPeerDetails details ;
		if(!gpg_id.isNull() && rsPeers->getGPGDetails(gpg_id,details)) {

			QString  keyId = QString::fromStdString(details.gpg_id.toStdString());
			QString  nickname = QString::fromUtf8(details.name.c_str());
			QString  idtype = tr("PGP Identity");

			QPixmap avatar;
			AvatarDefs::getAvatarFromGpgId(gpg_id, avatar);

			addMember(keyId, idtype, nickname, QIcon(avatar));

		}//if(!gpg_id.isNull() && rsPeers->getGPGDetails(gpg_id,details))
	}//for (itAllowedPeers it = cirDetails.mAllowedPeers.begin()
}

void  CreateCircleDialog::removeMember()
{
	QTreeWidgetItem *item = ui.treeWidget_membership->currentItem();
	if (!item) return;

	// does this just work? (TODO)
	delete(item);
}

void CreateCircleDialog::createCircle()
{
    if(mReadOnly)
    {
        close() ;
        return ;
    }
    
#ifdef DEBUG_CREATE_CIRCLE_DIALOG 
    std::cerr << "CreateCircleDialog::createCircle()";
    std::cerr << std::endl;
#endif

    QString name = ui.circleName->text();

    if(name.isEmpty()) {
	    /* error message */
	    QMessageBox::warning(this, tr("RetroShare"),tr("Please set a name for your Circle"), QMessageBox::Ok, QMessageBox::Ok);

	    return; //Don't add  a empty Subject!!
    }//if(name.isEmpty())

    RsGxsCircleGroup circle = mCircleGroup;	// init with loaded group

    circle.mMeta.mGroupName = std::string(name.toUtf8());
    circle.mInvitedMembers.clear() ;
    circle.mLocalFriends.clear() ;

    RsGxsId authorId;
    switch (ui.idChooser->getChosenId(authorId))
    {
    case GxsIdChooser::KnowId:
    case GxsIdChooser::UnKnowId:
	    circle.mMeta.mAuthorId = authorId;
#ifdef DEBUG_CREATE_CIRCLE_DIALOG 
	    std::cerr << "CreateCircleDialog::createCircle() AuthorId: " << authorId;
	    std::cerr << std::endl;
#endif

	    break;
    case GxsIdChooser::NoId:
    case GxsIdChooser::None:
    default: ;
#ifdef DEBUG_CREATE_CIRCLE_DIALOG 
	    std::cerr << "CreateCircleDialog::createCircle() No AuthorId Chosen!";
	    std::cerr << std::endl;
#endif
    }


    /* copy Ids from GUI */
    QTreeWidget *tree = ui.treeWidget_membership;
    int count = tree->topLevelItemCount();
    for(int i = 0; i < count; ++i)  
    {
	    QTreeWidgetItem *item = tree->topLevelItem(i);
	    QString keyId = item->text(RSCIRCLEID_COL_KEYID);

	    /* insert into circle */
	    if (mIsExternalCircle) 
	    {
		    RsGxsId key_id_gxs(keyId.toStdString()) ;

		    if(key_id_gxs.isNull())
		    {
			    std::cerr << "Error: Not a proper keyID: " << keyId.toStdString() << std::endl;
			    continue ;
		    } 

		    circle.mInvitedMembers.insert(key_id_gxs) ;
#ifdef DEBUG_CREATE_CIRCLE_DIALOG 
		    std::cerr << "CreateCircleDialog::createCircle() Inserting Member: " << keyId.toStdString();
		    std::cerr << std::endl;
#endif
	    } 
	    else 
	    {
		    RsPgpId key_id_pgp(keyId.toStdString()) ;

		    if(key_id_pgp.isNull())
		    {
			    std::cerr << "Error: Not a proper PGP keyID: " << keyId.toStdString() << std::endl;
			    continue ;
		    } 

		    circle.mLocalFriends.insert(key_id_pgp) ;
#ifdef DEBUG_CREATE_CIRCLE_DIALOG 
		    std::cerr << "CreateCircleDialog::createCircle() Inserting Friend: " << keyId.toStdString();
		    std::cerr << std::endl;
#endif
	    }

    }

    //	if (mIsExistingCircle) 
    //    {
    //		std::cerr << "CreateCircleDialog::createCircle() Existing Circle TODO";
    //		std::cerr << std::endl;
    //
    //		// cannot edit these yet.
    //		QMessageBox::warning(this, tr("RetroShare"),tr("Cannot Edit Existing Circles Yet"), QMessageBox::Ok, QMessageBox::Ok);
    //		return; 
    //	}

    if (mIsExternalCircle) 
    {
#ifdef DEBUG_CREATE_CIRCLE_DIALOG 
	    std::cerr << "CreateCircleDialog::createCircle() External Circle";
	    std::cerr << std::endl;
#endif

	    // set distribution from GUI.
	    circle.mMeta.mCircleId.clear() ;
        circle.mMeta.mGroupFlags = GXS_SERV::FLAG_PRIVACY_PUBLIC;

	    if (ui.radioButton_Public->isChecked()) {
#ifdef DEBUG_CREATE_CIRCLE_DIALOG 
		    std::cerr << "CreateCircleDialog::createCircle() Public Circle";
		    std::cerr << std::endl;
#endif

		    circle.mMeta.mCircleType =  GXS_CIRCLE_TYPE_PUBLIC;

	    } else if (ui.radioButton_Self->isChecked()) {
#ifdef DEBUG_CREATE_CIRCLE_DIALOG 
		    std::cerr << "CreateCircleDialog::createCircle() ExtSelfRef Circle";
		    std::cerr << std::endl;
#endif

		    circle.mMeta.mCircleType =  GXS_CIRCLE_TYPE_EXT_SELF;
	    } else if (ui.radioButton_Restricted->isChecked()) {
#ifdef DEBUG_CREATE_CIRCLE_DIALOG 
		    std::cerr << "CreateCircleDialog::createCircle() External (Other) Circle";
		    std::cerr << std::endl;
#endif

		    circle.mMeta.mCircleType =  GXS_CIRCLE_TYPE_EXTERNAL;

		    /* grab circle ID from chooser */
		    RsGxsCircleId chosenId;
		    if (ui.circleComboBox->getChosenCircle(chosenId)) {
#ifdef DEBUG_CREATE_CIRCLE_DIALOG 
			    std::cerr << "CreateCircleDialog::createCircle() ChosenId: " << chosenId;
			    std::cerr << std::endl;
#endif

			    circle.mMeta.mCircleId = chosenId;
		    } else {//if (ui.circleComboBox->getChosenCircle(chosenId))
#ifdef DEBUG_CREATE_CIRCLE_DIALOG 
			    std::cerr << "CreateCircleDialog::createCircle() Error no Id Chosen";
			    std::cerr << std::endl;
#endif

			    QMessageBox::warning(this, tr("RetroShare"),tr("No Restriction Circle Selected"), QMessageBox::Ok, QMessageBox::Ok);
			    return; 
		    }//else (ui.circleComboBox->getChosenCircle(chosenId))
	    } 
	    else 
	    {
		    QMessageBox::warning(this, tr("RetroShare"),tr("No Circle Limitations Selected"), QMessageBox::Ok, QMessageBox::Ok);
		    return; 
	    }
    } 
    else 
    {
#ifdef DEBUG_CREATE_CIRCLE_DIALOG 
	    std::cerr << "CreateCircleDialog::createCircle() Personal Circle";
	    std::cerr << std::endl;
#endif

	    // set personal distribution
	    circle.mMeta.mCircleId.clear() ;
	    circle.mMeta.mCircleType = GXS_CIRCLE_TYPE_LOCAL;
    }

    uint32_t token;
    
    if(mIsExistingCircle) 
    {
#ifdef DEBUG_CREATE_CIRCLE_DIALOG 
	    std::cerr << "CreateCircleDialog::updateCircle() : mCircleType: " << circle.mMeta.mCircleType << std::endl;
	    std::cerr << "CreateCircleDialog::updateCircle() : mCircleId: " << circle.mMeta.mCircleId << std::endl;
	    std::cerr << "CreateCircleDialog::updateCircle() : mGroupId: " << circle.mMeta.mGroupId << std::endl;

	    std::cerr << "CreateCircleDialog::updateCircle() Checks and Balances Okay - calling service proper.."<< std::endl;
#endif

	    rsGxsCircles->updateGroup(token, circle);
    }
    else
    {    
#ifdef DEBUG_CREATE_CIRCLE_DIALOG 
	    std::cerr << "CreateCircleDialog::createCircle() : mCircleType: " << circle.mMeta.mCircleType << std::endl;
	    std::cerr << "CreateCircleDialog::createCircle() : mCircleId: " << circle.mMeta.mCircleId << std::endl;

	    std::cerr << "CreateCircleDialog::createCircle() Checks and Balances Okay - calling service proper.."<< std::endl;
#endif

	    rsGxsCircles->createGroup(token, circle);
    }

    close();
}

void CreateCircleDialog::updateCircleGUI()
{
#ifdef DEBUG_CREATE_CIRCLE_DIALOG 
	std::cerr << "CreateCircleDialog::updateCircleGUI()";
	std::cerr << std::endl;
#endif

	ui.circleName->setText(QString::fromUtf8(mCircleGroup.mMeta.mGroupName.c_str()));

	bool isExternal = true;
#ifdef DEBUG_CREATE_CIRCLE_DIALOG 
	std::cerr << "CreateCircleDialog::updateCircleGUI() : CIRCLETYPE: " << mCircleGroup.mMeta.mCircleType;
	std::cerr << std::endl;
#endif

    ui.radioButton_Public->setChecked(false);
				ui.radioButton_Self->setChecked(false);
				ui.radioButton_Restricted->setChecked(false);
            
	switch(mCircleGroup.mMeta.mCircleType) 
    	{
		case GXS_CIRCLE_TYPE_LOCAL:
#ifdef DEBUG_CREATE_CIRCLE_DIALOG 
			std::cerr << "CreateCircleDialog::updateCircleGUI() : LOCAL CIRCLETYPE";
			std::cerr << std::endl;
#endif

			isExternal = false;
			break;

		case GXS_CIRCLE_TYPE_PUBLIC:
#ifdef DEBUG_CREATE_CIRCLE_DIALOG 
			std::cerr << "CreateCircleDialog::updateCircleGUI() : PUBLIC CIRCLETYPE";
			std::cerr << std::endl;
#endif

			ui.radioButton_Public->setChecked(true);
			break;

		case GXS_CIRCLE_TYPE_EXT_SELF:
#ifdef DEBUG_CREATE_CIRCLE_DIALOG 
			std::cerr << "CreateCircleDialog::updateCircleGUI() : EXT_SELF CIRCLE (fallthrough)"<< std::endl;
#endif
		case GXS_CIRCLE_TYPE_EXTERNAL:
        
#ifdef DEBUG_CREATE_CIRCLE_DIALOG 
			std::cerr << "CreateCircleDialog::updateCircleGUI() : EXTERNAL CIRCLETYPE"<< std::endl;
#endif

			if (RsGxsGroupId(mCircleGroup.mMeta.mCircleId) == mCircleGroup.mMeta.mGroupId) 
				ui.radioButton_Self->setChecked(true);
            else
				ui.radioButton_Restricted->setChecked(true);

            ui.circleComboBox->loadCircles(mCircleGroup.mMeta.mCircleId);
			
			break;

		default:
			std::cerr << "CreateCircleDialog::updateCircleGUI() INVALID mCircleType";
			std::cerr << std::endl;
	}

	/* setup personal or external circle */
	if (isExternal) 
		setupForExternalCircle();
	else 
		setupForPersonalCircle();
    
	// set preferredId.
    std::cerr << "LoadCircle: setting author id to be " << mCircleGroup.mMeta.mAuthorId << std::endl;
	//ui.idChooser->loadIds(0,mCircleGroup.mMeta.mAuthorId);
    
    if(mReadOnly)
    {
	    ui.circleAdminLabel->setId(mCircleGroup.mMeta.mAuthorId) ;
        	ui.idChooser->setVisible(false) ;
    }
    else
    {
	    //std::set<RsGxsId> ids ;
	    //ids.insert(mCircleGroup.mMeta.mAuthorId) ;
	    ui.idChooser->setDefaultId(mCircleGroup.mMeta.mAuthorId) ;
	    ui.idChooser->setChosenId(mCircleGroup.mMeta.mAuthorId) ;
	    //ui.idChooser->setIdConstraintSet(ids) ;
	    ui.idChooser->setFlags(IDCHOOSER_NO_CREATE) ;
	    ui.circleAdminLabel->setVisible(false) ;
	    ui.circleAdminLabel->hide();
    }
}

void CreateCircleDialog::requestCircle(const RsGxsGroupId &groupId)
{
	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;

	std::list<RsGxsGroupId> groupIds;
	groupIds.push_back(groupId);

#ifdef DEBUG_CREATE_CIRCLE_DIALOG 
	std::cerr << "CreateCircleDialog::requestCircle() Requesting Group Summary(" << groupId << ")";
	std::cerr << std::endl;
#endif

	uint32_t token;
	mCircleQueue->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, groupIds, CREATECIRCLEDIALOG_CIRCLEINFO);
}

void CreateCircleDialog::loadCircle(uint32_t token)
{
#ifdef DEBUG_CREATE_CIRCLE_DIALOG 
	std::cerr << "CreateCircleDialog::loadCircle(" << token << ")";
	std::cerr << std::endl;
#endif

	QTreeWidget *tree = ui.treeWidget_membership;

	if (mClearList) tree->clear();

	std::vector<RsGxsCircleGroup> groups;
	if (!rsGxsCircles->getGroupData(token, groups)) {
		std::cerr << "CreateCircleDialog::loadCircle() Error getting GroupData";
		std::cerr << std::endl;
		return;
	}

	if (groups.size() != 1) {
		std::cerr << "CreateCircleDialog::loadCircle() Error Group.size() != 1";
		std::cerr << std::endl;
		return;
	}
		
	mCircleGroup = groups[0];
    
#ifdef DEBUG_CREATE_CIRCLE_DIALOG 
	std::cerr << "CreateCircleDialog::loadCircle() LoadedGroup.meta: " << mCircleGroup.mMeta << std::endl;
#endif
	updateCircleGUI();
}

/*void CreateCircleDialog::getPgpIdentities()
{
	std::cerr << "CreateCircleDialog::getPgpIdentities()";
	std::cerr << std::endl;

	QTreeWidget *tree = ui.treeWidget_IdList;

	tree->clear();
	std::list<RsPgpId> ids;
	std::list<RsPgpId>::iterator it;

	rsPeers->getGPGAcceptedList(ids);
	for(it = ids.begin(); it != ids.end(); ++it) {
		RsPeerDetails details;

		rsPeers->getGPGDetails(*it, details);

		QString  keyId = QString::fromStdString(details.gpg_id.toStdString());
		QString  nickname = QString::fromUtf8(details.name.c_str());
		QString  idtype = tr("PGP Identity");

		QTreeWidgetItem *item = new QTreeWidgetItem();
		item->setText(RSCIRCLEID_COL_NICKNAME, nickname);
		item->setText(RSCIRCLEID_COL_KEYID, keyId);
		item->setText(RSCIRCLEID_COL_IDTYPE, idtype);
		tree->addTopLevelItem(item);

		// Local Circle.
		if (mIsExistingCircle) 
			if ( mCircleGroup.mLocalFriends.find(details.gpg_id) != mCircleGroup.mLocalFriends.end())  // check if its in the circle.
				addMember(keyId, idtype, nickname);
	}
	
	filterIds();
}*/


void CreateCircleDialog::requestGxsIdentities()
{
	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;

#ifdef DEBUG_CREATE_CIRCLE_DIALOG 
	std::cerr << "CreateCircleDialog::requestIdentities()";
	std::cerr << std::endl;
#endif

	uint32_t token;
	mIdQueue->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, CREATECIRCLEDIALOG_IDINFO);
}

void CreateCircleDialog::loadIdentities(uint32_t token)
{
#ifdef DEBUG_CREATE_CIRCLE_DIALOG 
	std::cerr << "CreateCircleDialog::loadIdentities(" << token << ")";
	std::cerr << std::endl;
#endif

	QTreeWidget *tree = ui.treeWidget_IdList;

	tree->clear();

	bool acceptAnonymous = ui.radioButton_ListAll->isChecked();
	bool acceptAllPGP = ui.radioButton_ListAllPGP->isChecked();
	//bool acceptKnownPGP = ui.radioButton_ListKnownPGP->isChecked();

	RsGxsIdGroup idGroup;
	std::vector<RsGxsIdGroup> datavector;
	std::vector<RsGxsIdGroup>::iterator vit;
	if (!rsIdentity->getGroupData(token, datavector)) {
		std::cerr << "CreateCircleDialog::insertIdentities() Error getting GroupData";
		std::cerr << std::endl;
		return;
	}

	for(vit = datavector.begin(); vit != datavector.end(); ++vit)
	{
		idGroup = (*vit);

		/* do filtering */
		bool ok = false;
		if (acceptAnonymous)
			ok = true;
		else if (acceptAllPGP)
			ok = idGroup.mMeta.mGroupFlags & RSGXSID_GROUPFLAG_REALID_kept_for_compatibility ;
		else if (idGroup.mPgpKnown)
			ok = idGroup.mMeta.mGroupFlags & RSGXSID_GROUPFLAG_REALID_kept_for_compatibility ;

		if (!ok) {
#ifdef DEBUG_CREATE_CIRCLE_DIALOG 
			std::cerr << "CreateCircleDialog::insertIdentities() Skipping ID: " << data.mMeta.mGroupId;
			std::cerr << std::endl;
#endif
			continue;
		}

		QString  keyId = QString::fromStdString(idGroup.mMeta.mGroupId.toStdString());
		QString  nickname = QString::fromUtf8(idGroup.mMeta.mGroupName.c_str());
		QString  idtype = tr("Anon Id");

		QPixmap pixmap ;

		if(idGroup.mImage.mSize == 0 || !pixmap.loadFromData(idGroup.mImage.mData, idGroup.mImage.mSize, "PNG"))
			pixmap = QPixmap::fromImage(GxsIdDetails::makeDefaultIcon(RsGxsId(idGroup.mMeta.mGroupId))) ;

		if (idGroup.mMeta.mGroupFlags & RSGXSID_GROUPFLAG_REALID_kept_for_compatibility)
		{
			if (idGroup.mPgpKnown) {
				RsPeerDetails details;
				rsPeers->getGPGDetails(idGroup.mPgpId, details);
				idtype = QString::fromUtf8(details.name.c_str());
			}
			else
				idtype = tr("PGP Linked Id");

		}

		QTreeWidgetItem *item = new QTreeWidgetItem();
		item->setText(RSCIRCLEID_COL_NICKNAME, nickname);
		item->setIcon(RSCIRCLEID_COL_NICKNAME, QIcon(pixmap));
		item->setText(RSCIRCLEID_COL_KEYID, keyId);
		item->setText(RSCIRCLEID_COL_IDTYPE, idtype);
		tree->addTopLevelItem(item);

		// External Circle.
		if (mIsExistingCircle)
		{
			// check if its in the circle.

			// We use an explicit cast
			//

			if ( mCircleGroup.mInvitedMembers.find(RsGxsId(idGroup.mMeta.mGroupId)) != mCircleGroup.mInvitedMembers.end())  /* found it */
				addMember(keyId, idtype, nickname, QIcon(pixmap));

		}
	}
}

void CreateCircleDialog::loadRequest(const TokenQueue *queue, const TokenRequest &req)
{
#ifdef DEBUG_CREATE_CIRCLE_DIALOG 
	std::cerr << "CreateCircleDialog::loadRequest() UserType: " << req.mUserType;
	std::cerr << std::endl;
#endif

	if (queue == mCircleQueue) {
		/* now switch on req */
		switch(req.mUserType) {
			case CREATECIRCLEDIALOG_CIRCLEINFO:
				loadCircle(req.mToken);
				break;

			default:
				std::cerr << "CreateCircleDialog::loadRequest() UNKNOWN UserType ";
				std::cerr << std::endl;
		}
	}

	if (queue == mIdQueue) {
		/* now switch on req */
		switch(req.mUserType) {
			case CREATECIRCLEDIALOG_IDINFO:
				loadIdentities(req.mToken);
				break;

			default:
				std::cerr << "CreateCircleDialog::loadRequest() UNKNOWN UserType ";
				std::cerr << std::endl;
		}
	}
}

void CreateCircleDialog::idTypeChanged()
{
	requestGxsIdentities();
}
void CreateCircleDialog::filterChanged(const QString &text)
{
	Q_UNUSED(text);
	filterIds();
}

void CreateCircleDialog::filterIds()
{
	int filterColumn = ui.IdFilter->currentFilter();
	QString text = ui.IdFilter->text();

	ui.treeWidget_IdList->filterItems(filterColumn, text);
}

void CreateCircleDialog::createNewGxsId()
{
	IdEditDialog dlg(this);
	dlg.setupNewId(false);
	dlg.exec();
	//ui.idChooser->setDefaultId(dlg.getLastIdName());
}

void CreateCircleDialog::IdListCustomPopupMenu( QPoint )
{
	QMenu contextMnu( this );

	QTreeWidgetItem *item = ui.treeWidget_IdList->currentItem();
	if (item) {

			contextMnu.addAction(QIcon(":/images/edit_add24.png"), tr("Add Member"), this, SLOT(addMember()));
	
	}

	contextMnu.exec(QCursor::pos());
}

void CreateCircleDialog::MembershipListCustomPopupMenu( QPoint )
{
	QMenu contextMnu( this );

	QTreeWidgetItem *item = ui.treeWidget_membership->currentItem();
	if (item && !mReadOnly)
			contextMnu.addAction(QIcon(":/images/delete.png"), tr("Remove Member"), this, SLOT(removeMember()));

	contextMnu.exec(QCursor::pos());
}

