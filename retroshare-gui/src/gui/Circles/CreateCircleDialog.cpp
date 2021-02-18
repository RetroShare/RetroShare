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
#include <QCloseEvent>
#include <QMenu>

#include <algorithm>
#include <memory>

#include <retroshare/rspeers.h>
#include <retroshare/rsidentity.h>

#include "gui/common/AvatarDefs.h"
#include "gui/common/FilesDefs.h"
#include "util/qtthreadsutils.h"
#include "util/misc.h"
#include "gui/Circles/CreateCircleDialog.h"
#include "gui/gxs/GxsIdDetails.h"
#include "gui/Identity/IdDialog.h"
#include "gui/Identity/IdEditDialog.h"

//#define DEBUG_CREATE_CIRCLE_DIALOG 1

#define CREATECIRCLEDIALOG_CIRCLEINFO 2
#define CREATECIRCLEDIALOG_IDINFO     3

#define RSCIRCLEID_COL_NICKNAME       0
#define RSCIRCLEID_COL_IDTYPE         1
#define RSCIRCLEID_COL_KEYID          2

/** Constructor */
CreateCircleDialog::CreateCircleDialog()
	: QDialog(NULL, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint)
{
    mIdentitiesLoading = false;
    mCircleLoading = false;
    mCloseRequested = false;

    /* Invoke the Qt Designer generated object setup routine */
	ui.setupUi(this);

    setAttribute(Qt::WA_DeleteOnClose, false);

    /* Setup Queue */
    ui.headerFrame->setHeaderImage(FilesDefs::getPixmapFromQtResourcePath(":/icons/png/circles.png"));

	//connect(ui.toolButton_NewId, SIGNAL(clicked()), this, SLOT(createNewGxsId()));

	/* Add filter actions */
	QTreeWidgetItem *headerItem = ui.treeWidget_IdList->headerItem();
	QString headerText = headerItem->text(RSCIRCLEID_COL_NICKNAME);
	ui.IdFilter->addFilter(QIcon(), headerText, RSCIRCLEID_COL_NICKNAME, QString("%1 %2").arg(tr("Search"), headerText));
	headerText = headerItem->text(RSCIRCLEID_COL_KEYID);
	ui.IdFilter->addFilter(QIcon(), headerText, RSCIRCLEID_COL_KEYID, QString("%1 %2").arg(tr("Search"), headerText));
	
	/* Set initial column width */
	int fontWidth = QFontMetricsF(ui.treeWidget_IdList->font()).width("W");
	ui.treeWidget_IdList->setColumnWidth(RSCIRCLEID_COL_NICKNAME, 17 * fontWidth);
	ui.treeWidget_membership->setColumnWidth(RSCIRCLEID_COL_NICKNAME, 17 * fontWidth);
	
	ui.removeButton->setEnabled(false);
	ui.addButton->setEnabled(false);
	ui.radioButton_ListAll->setChecked(true);
	ui.radioButton_Public->setChecked(true) ;

	mIsExistingCircle = false;
	mIsExternalCircle = true;
	mClearList = true;
#if QT_VERSION >= 0x040700
	ui.circleName->setPlaceholderText(QApplication::translate("CreateCircleDialog", "Circle name", 0));
#endif
        
	ui.treeWidget_IdList->setColumnHidden(RSCIRCLEID_COL_KEYID,true); // no need to show this. the tooltip will do it.
    ui.circleComboBox->loadCircles(RsGxsCircleId());
    ui.circleComboBox->hide();

    // connect up the buttons.
    connect(ui.addButton, SIGNAL(clicked()), this, SLOT(addMember()));
    connect(ui.removeButton, SIGNAL(clicked()), this, SLOT(removeMember()));

    connect(ui.createButton, SIGNAL(clicked()), this, SLOT(createCircle()));
    connect(ui.cancelButton, SIGNAL(clicked()), this, SLOT(close()));

    connect(ui.treeWidget_membership, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(selectedMember(QTreeWidgetItem*, QTreeWidgetItem*)));
    connect(ui.treeWidget_IdList, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(selectedId(QTreeWidgetItem*, QTreeWidgetItem*)));

    connect(ui.treeWidget_IdList, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(IdListCustomPopupMenu(QPoint)));
    connect(ui.treeWidget_membership, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(MembershipListCustomPopupMenu(QPoint)));

    connect(ui.IdFilter, SIGNAL(textChanged(QString)), this, SLOT(filterChanged(QString)));


    QObject::connect(ui.radioButton_ListAll, SIGNAL(toggled(bool)), this, SLOT(idTypeChanged())) ;
    QObject::connect(ui.radioButton_ListAllPGP, SIGNAL(toggled(bool)), this, SLOT(idTypeChanged())) ;
    QObject::connect(ui.radioButton_ListFriendPGP, SIGNAL(toggled(bool)), this, SLOT(idTypeChanged())) ;

    QObject::connect(ui.radioButton_Public, SIGNAL(toggled(bool)), this, SLOT(updateCircleType(bool))) ;
    QObject::connect(ui.radioButton_Self, SIGNAL(toggled(bool)), this, SLOT(updateCircleType(bool))) ;
    QObject::connect(ui.radioButton_Restricted, SIGNAL(toggled(bool)), this, SLOT(updateCircleType(bool))) ;

}

CreateCircleDialog::~CreateCircleDialog()
{
}

bool CreateCircleDialog::tryClose()
{
    if(mIdentitiesLoading || mCircleLoading)
    {
        std::cerr << "Close() called. Identities or circle currently loading => not actually closing." << std::endl;
        mCloseRequested = true;
        return false;
    }
    else
    {
        std::cerr << "Close() called. Identities not currently loading => closing." << std::endl;
        return true;
    }
}

void CreateCircleDialog::accept()
{
    if(tryClose())
        QDialog::accept();
}
void CreateCircleDialog::reject()
{
    if(tryClose())
        QDialog::reject();
}

void CreateCircleDialog::keyPressEvent(QKeyEvent *e)
{
    if(e->key() != Qt::Key_Escape || tryClose())
        QDialog::keyPressEvent(e);
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
    
    ui.createButton->setText(tr("Update"));
    
	ui.addButton->setEnabled(!readonly) ;
	ui.removeButton->setEnabled(!readonly) ;
    
    if(readonly)
	{
		ui.createButton->hide() ;
		ui.cancelButton->setText(tr("Close"));
		ui.peersSelection_GB->hide() ;
		ui.addButton->hide() ;
		ui.removeButton->hide() ;
	}

	loadCircle(circleId);
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
		ui.createButton->setText(tr("Create"));
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
	
	loadIdentities();
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
	QString  idtype = tr("[Anonymous Id]");

	QPixmap pixmap ;

	if(idGroup.mImage.mSize == 0 || !GxsIdDetails::loadPixmapFromData(idGroup.mImage.mData, idGroup.mImage.mSize, pixmap, GxsIdDetails::SMALL))
		pixmap = GxsIdDetails::makeDefaultIcon(RsGxsId(idGroup.mMeta.mGroupId),GxsIdDetails::SMALL);

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
			QString  idtype = tr("[Anonymous Id]");

			QPixmap pixmap ;

			if(gxs_details.mAvatar.mSize == 0 || !GxsIdDetails::loadPixmapFromData(gxs_details.mAvatar.mData, gxs_details.mAvatar.mSize, pixmap, GxsIdDetails::SMALL))
				pixmap = GxsIdDetails::makeDefaultIcon(gxs_details.mId,GxsIdDetails::SMALL);

			addMember(keyId, idtype, nickname, QIcon(pixmap));

		}
	}

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

		}
	}
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
    }

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

    whileBlocking(ui.circleName)->setText(QString::fromUtf8(mCircleGroup.mMeta.mGroupName.c_str()));

	bool isExternal = true;
#ifdef DEBUG_CREATE_CIRCLE_DIALOG 
	std::cerr << "CreateCircleDialog::updateCircleGUI() : CIRCLETYPE: " << mCircleGroup.mMeta.mCircleType;
	std::cerr << std::endl;
#endif

    whileBlocking(ui.radioButton_Public)->setChecked(false);
    whileBlocking(ui.radioButton_Self)->setChecked(false);
    whileBlocking(ui.radioButton_Restricted)->setChecked(false);
            
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

            whileBlocking(ui.radioButton_Public)->setChecked(true);
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
                whileBlocking(ui.radioButton_Self)->setChecked(true);
            else
                whileBlocking(ui.radioButton_Restricted)->setChecked(true);

            whileBlocking(ui.circleComboBox)->loadCircles(mCircleGroup.mMeta.mCircleId);
			
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

void CreateCircleDialog::loadCircle(const RsGxsGroupId& groupId)
{
	QTreeWidget *tree = ui.treeWidget_membership;
	if (mClearList) tree->clear();

    std::cerr << "Loading circle..."<< std::endl;
    mCircleLoading = true;

	RsThread::async([groupId,this]()
	{
		std::vector<RsGxsCircleGroup> circlesInfo ;

        if(! rsGxsCircles->getCirclesInfo(std::list<RsGxsGroupId>({ groupId }), circlesInfo) || circlesInfo.size() != 1)
		{
			std::cerr << __PRETTY_FUNCTION__ << " failed to retrieve circle info for circle " << groupId << std::endl;
			return;
        }

        RsGxsCircleGroup grp(circlesInfo[0]);

		RsQThreadUtils::postToObject( [grp,this]()
		{
			/* Here it goes any code you want to be executed on the Qt Gui
			 * thread, for example to update the data model with new information
			 * after a blocking call to RetroShare API complete */

			mCircleGroup = grp;

#ifdef DEBUG_CREATE_CIRCLE_DIALOG
			std::cerr << "CreateCircleDialog::loadCircle() LoadedGroup.meta: " << mCircleGroup.mMeta << std::endl;
#endif
			updateCircleGUI();

            mCircleLoading = false;

            std::cerr << "finished loading circle..."<< std::endl;

            if(mCloseRequested && !mIdentitiesLoading)
            {
                std::cerr << "Close() previously called, so closing now." << std::endl;
                close();
            }

        }, this );
	});

}

void CreateCircleDialog::loadIdentities()
{
    std::cerr << "Loading identities..." << std::endl;

    mIdentitiesLoading = true;

	RsThread::async([this]()
	{
		std::list<RsGroupMetaData> ids_meta;

		if(!rsIdentity->getIdentitiesSummaries(ids_meta))
		{
			RS_ERR("failed to retrieve identities ids for all identities");
			return;
		}

		std::set<RsGxsId> ids;
        for(auto& meta:ids_meta)
            ids.insert(RsGxsId(meta.mGroupId));

        // Needs a pointer on the heap, to pass to postToObject, otherwise it will get deleted before
        // the posted method will actually run. Memory ownership is left to the posted method.

        auto id_groups = new std::vector<RsGxsIdGroup>();

		if(!rsIdentity->getIdentitiesInfo(ids, *id_groups))
		{
			RS_ERR("failed to retrieve identities group info for all identities");
            delete id_groups;
			return;
		}

        RsQThreadUtils::postToObject( [id_groups, this]()
		{
			/* Here it goes any code you want to be executed on the Qt Gui
			 * thread, for example to update the data model with new information
			 * after a blocking call to RetroShare API complete */

			fillIdentitiesList(*id_groups);

            delete id_groups;

            std::cerr << "Identities finished loading." << std::endl;
            mIdentitiesLoading = false;

            if(mCloseRequested && !mCircleLoading)
            {
                std::cerr << "Close() previously called, so closing now." << std::endl;
                close();
            }

        }, this );
	});

}

void CreateCircleDialog::fillIdentitiesList(const std::vector<RsGxsIdGroup>& id_groups)
{
#ifdef DEBUG_CREATE_CIRCLE_DIALOG 
	std::cerr << "CreateCircleDialog::loadIdentities(" << token << ")";
	std::cerr << std::endl;
#endif

	QTreeWidget *tree = ui.treeWidget_IdList;
	tree->clear();

	bool acceptAll = ui.radioButton_ListAll->isChecked();
	bool acceptOnlySignedIdentities = ui.radioButton_ListAllPGP->isChecked();
	bool acceptOnlyIdentitiesSignedByFriend = ui.radioButton_ListFriendPGP->isChecked();

	for(const auto& idGroup:id_groups)
	{
        //usleep(20*1000);

		bool isSigned = !idGroup.mPgpId.isNull();
		bool isSignedByFriendNode = isSigned && rsPeers->isPgpFriend(idGroup.mPgpId);

		/* do filtering */
		bool ok = false;

		if(!(acceptAll ||(acceptOnlySignedIdentities && isSigned) ||(acceptOnlyIdentitiesSignedByFriend && isSignedByFriendNode)))
		{
#ifdef DEBUG_CREATE_CIRCLE_DIALOG 
			std::cerr << "CreateCircleDialog::insertIdentities() Skipping ID: " << data.mMeta.mGroupId;
			std::cerr << std::endl;
#endif
			continue;
		}

		QString  keyId = QString::fromStdString(idGroup.mMeta.mGroupId.toStdString());
		QString  nickname = QString::fromUtf8(idGroup.mMeta.mGroupName.c_str());
		QString  idtype ;

		QPixmap pixmap ;

		if(idGroup.mImage.mSize == 0 || !GxsIdDetails::loadPixmapFromData(idGroup.mImage.mData, idGroup.mImage.mSize, pixmap, GxsIdDetails::SMALL))
			pixmap = GxsIdDetails::makeDefaultIcon(RsGxsId(idGroup.mMeta.mGroupId),GxsIdDetails::SMALL) ;

		if (!idGroup.mPgpId.isNull())
		{
			RsPeerDetails details;

			if(rsPeers->getGPGDetails(idGroup.mPgpId, details))
				idtype = QString::fromUtf8(details.name.c_str());
            else
				idtype = tr("[Unknown]");
		}

		QTreeWidgetItem *item = new QTreeWidgetItem();
		item->setText(RSCIRCLEID_COL_NICKNAME, nickname);
		item->setIcon(RSCIRCLEID_COL_NICKNAME, QIcon(pixmap));
		item->setText(RSCIRCLEID_COL_KEYID, keyId);
		item->setText(RSCIRCLEID_COL_IDTYPE, idtype);
		tree->addTopLevelItem(item);

        RsIdentityDetails det;

        if(rsIdentity->getIdDetails(RsGxsId(idGroup.mMeta.mGroupId),det))
			item->setToolTip(RSCIRCLEID_COL_NICKNAME,GxsIdDetails::getComment(det));

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

void CreateCircleDialog::idTypeChanged()
{
	loadIdentities();
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

