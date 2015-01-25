/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2015 RetroShare Team
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

#include "IdDetailsDialog.h"
#include "ui_IdDetailsDialog.h"
#include "gui/gxs/GxsIdDetails.h"
#include "gui/settings/rsharesettings.h"
#include "gui/common/UIStateHelper.h"

#include <retroshare/rspeers.h>

#define IDDETAILSDIALOG_IDDETAILS  1

/******
 * #define ID_DEBUG 1
 *****/

/** Default constructor */
IdDetailsDialog::IdDetailsDialog(const RsGxsGroupId& id, QWidget *parent) :
    QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint),
    mId(id),
    ui(new Ui::IdDetailsDialog)
{
	/* Invoke Qt Designer generated QObject setup routine */
	ui->setupUi(this);

	setAttribute (Qt::WA_DeleteOnClose,true);

	/* Setup UI helper */
	mStateHelper = new UIStateHelper(this);
	mStateHelper->addWidget(IDDETAILSDIALOG_IDDETAILS, ui->lineEdit_Nickname);
	mStateHelper->addWidget(IDDETAILSDIALOG_IDDETAILS, ui->lineEdit_KeyId);
	mStateHelper->addWidget(IDDETAILSDIALOG_IDDETAILS, ui->lineEdit_GpgId);
	mStateHelper->addWidget(IDDETAILSDIALOG_IDDETAILS, ui->lineEdit_GpgName);
	mStateHelper->addWidget(IDDETAILSDIALOG_IDDETAILS, ui->lineEdit_Type);
	mStateHelper->addWidget(IDDETAILSDIALOG_IDDETAILS, ui->toolButton_Reputation);
	mStateHelper->addWidget(IDDETAILSDIALOG_IDDETAILS, ui->line_RatingOverall);
	mStateHelper->addWidget(IDDETAILSDIALOG_IDDETAILS, ui->line_RatingImplicit);
	mStateHelper->addWidget(IDDETAILSDIALOG_IDDETAILS, ui->line_RatingOwn);
	mStateHelper->addWidget(IDDETAILSDIALOG_IDDETAILS, ui->line_RatingPeers);
	mStateHelper->addWidget(IDDETAILSDIALOG_IDDETAILS, ui->repModButton);
	mStateHelper->addWidget(IDDETAILSDIALOG_IDDETAILS, ui->repMod_Accept);
	mStateHelper->addWidget(IDDETAILSDIALOG_IDDETAILS, ui->repMod_Ban);
	mStateHelper->addWidget(IDDETAILSDIALOG_IDDETAILS, ui->repMod_Negative);
	mStateHelper->addWidget(IDDETAILSDIALOG_IDDETAILS, ui->repMod_Positive);
	mStateHelper->addWidget(IDDETAILSDIALOG_IDDETAILS, ui->repMod_Custom);
	mStateHelper->addWidget(IDDETAILSDIALOG_IDDETAILS, ui->repMod_spinBox);

	mStateHelper->addLoadPlaceholder(IDDETAILSDIALOG_IDDETAILS, ui->lineEdit_Nickname);
	mStateHelper->addLoadPlaceholder(IDDETAILSDIALOG_IDDETAILS, ui->lineEdit_GpgName);
	mStateHelper->addLoadPlaceholder(IDDETAILSDIALOG_IDDETAILS, ui->lineEdit_KeyId);
	mStateHelper->addLoadPlaceholder(IDDETAILSDIALOG_IDDETAILS, ui->lineEdit_GpgId);
	mStateHelper->addLoadPlaceholder(IDDETAILSDIALOG_IDDETAILS, ui->lineEdit_Type);
	mStateHelper->addLoadPlaceholder(IDDETAILSDIALOG_IDDETAILS, ui->lineEdit_GpgName);
	mStateHelper->addLoadPlaceholder(IDDETAILSDIALOG_IDDETAILS, ui->line_RatingOverall);
	mStateHelper->addLoadPlaceholder(IDDETAILSDIALOG_IDDETAILS, ui->line_RatingImplicit);
	mStateHelper->addLoadPlaceholder(IDDETAILSDIALOG_IDDETAILS, ui->line_RatingOwn);
	mStateHelper->addLoadPlaceholder(IDDETAILSDIALOG_IDDETAILS, ui->line_RatingPeers);

	mStateHelper->addClear(IDDETAILSDIALOG_IDDETAILS, ui->lineEdit_Nickname);
	mStateHelper->addClear(IDDETAILSDIALOG_IDDETAILS, ui->lineEdit_KeyId);
	mStateHelper->addClear(IDDETAILSDIALOG_IDDETAILS, ui->lineEdit_GpgId);
	mStateHelper->addClear(IDDETAILSDIALOG_IDDETAILS, ui->lineEdit_Type);
	mStateHelper->addClear(IDDETAILSDIALOG_IDDETAILS, ui->lineEdit_GpgName);
	mStateHelper->addClear(IDDETAILSDIALOG_IDDETAILS, ui->line_RatingOverall);
	mStateHelper->addClear(IDDETAILSDIALOG_IDDETAILS, ui->line_RatingImplicit);
	mStateHelper->addClear(IDDETAILSDIALOG_IDDETAILS, ui->line_RatingOwn);
	mStateHelper->addClear(IDDETAILSDIALOG_IDDETAILS, ui->line_RatingPeers);

	/* Create token queue */
	mIdQueue = new TokenQueue(rsIdentity->getTokenService(), this);

	Settings->loadWidgetInformation(this);

	ui->headerFrame->setHeaderImage(QPixmap(":/images/identity/identity_64.png"));
	ui->headerFrame->setHeaderText(tr("Person Details"));

	//connect(ui.buttonBox, SIGNAL(accepted()), this, SLOT(changeGroup()));
	connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
	
	// Hiding Rep Btn until that part is finished.
	ui->toolButton_Reputation->setVisible(false);
	
	requestIdDetails();
}

/** Destructor. */
IdDetailsDialog::~IdDetailsDialog()
{
	Settings->saveWidgetInformation(this);

	delete(ui);
	delete(mIdQueue);
}

void IdDetailsDialog::insertIdDetails(uint32_t token)
{
	mStateHelper->setLoading(IDDETAILSDIALOG_IDDETAILS, false);

	/* get details from libretroshare */
	std::vector<RsGxsIdGroup> datavector;
	if (!rsIdentity->getGroupData(token, datavector))
	{
		mStateHelper->setActive(IDDETAILSDIALOG_IDDETAILS, false);
		mStateHelper->clear(IDDETAILSDIALOG_IDDETAILS);

		ui->lineEdit_KeyId->setText("ERROR GETTING KEY!");

		return;
	}

	if (datavector.size() != 1)
	{
#ifdef ID_DEBUG
		std::cerr << "IdDetailsDialog::insertIdDetails() Invalid datavector size";
#endif

		mStateHelper->setActive(IDDETAILSDIALOG_IDDETAILS, false);
		mStateHelper->clear(IDDETAILSDIALOG_IDDETAILS);

		ui->lineEdit_KeyId->setText("INVALID DV SIZE");

		return;
	}

	mStateHelper->setActive(IDDETAILSDIALOG_IDDETAILS, true);

	RsGxsIdGroup &data = datavector[0];

	/* get GPG Details from rsPeers */
	RsPgpId ownPgpId  = rsPeers->getGPGOwnId();

	ui->lineEdit_Nickname->setText(QString::fromUtf8(data.mMeta.mGroupName.c_str()));
	ui->lineEdit_KeyId->setText(QString::fromStdString(data.mMeta.mGroupId.toStdString()));
	//ui->lineEdit_GpgHash->setText(QString::fromStdString(data.mPgpIdHash.toStdString()));
	ui->lineEdit_GpgId->setText(QString::fromStdString(data.mPgpId.toStdString()));
	
	QPixmap pix = QPixmap::fromImage(GxsIdDetails::makeDefaultIcon(RsGxsId(data.mMeta.mGroupId))) ;
#ifdef ID_DEBUG
	std::cerr << "Setting header frame image : " << pix.width() << " x " << pix.height() << std::endl;
#endif
	ui->headerFrame->setHeaderImage(QPixmap(pix));


	if (data.mPgpKnown)
	{
		RsPeerDetails details;
		rsPeers->getGPGDetails(data.mPgpId, details);
		ui->lineEdit_GpgName->setText(QString::fromUtf8(details.name.c_str()));
	}
	else
	{
		if (data.mMeta.mGroupFlags & RSGXSID_GROUPFLAG_REALID)
		{
			ui->lineEdit_GpgName->setText(tr("Unknown real name"));
		}
		else
		{
			ui->lineEdit_GpgName->setText(tr("Anonymous Id"));
		}
	}

	if(data.mPgpId.isNull())
	{
		ui->lineEdit_GpgId->hide() ;
		ui->lineEdit_GpgName->hide() ;
		ui->PgpId_LB->hide() ;
		ui->PgpName_LB->hide() ;
	}
	else
	{
		ui->lineEdit_GpgId->show() ;
		ui->lineEdit_GpgName->show() ;
		ui->PgpId_LB->show() ;
		ui->PgpName_LB->show() ;
	}

	bool isOwnId = (data.mPgpKnown && (data.mPgpId == ownPgpId)) || (data.mMeta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_ADMIN);

	if (isOwnId)
		if (data.mPgpKnown)
			ui->lineEdit_Type->setText(tr("Identity owned by you, linked to your Retroshare node")) ;
		else
			ui->lineEdit_Type->setText(tr("Anonymous identity, owned by you")) ;
	else if (data.mMeta.mGroupFlags & RSGXSID_GROUPFLAG_REALID)
	{
		if (data.mPgpKnown)
			if (rsPeers->isGPGAccepted(data.mPgpId))
				ui->lineEdit_Type->setText(tr("Owned by a friend Retroshare node")) ;
			else
				ui->lineEdit_Type->setText(tr("Owned by 2-hops Retroshare node")) ;
		else
			ui->lineEdit_Type->setText(tr("Owned by unknown Retroshare node")) ;
	}
	else
		ui->lineEdit_Type->setText(tr("Anonymous identity")) ;

//	if (isOwnId)
//	{
//		ui->radioButton_IdYourself->setChecked(true);
//	}
//	else if (data.mMeta.mGroupFlags & RSGXSID_GROUPFLAG_REALID)
//	{
//		if (data.mPgpKnown)
//		{
//			if (rsPeers->isGPGAccepted(data.mPgpId))
//			{
//				ui->radioButton_IdFriend->setChecked(true);
//			}
//			else
//			{
//				ui->radioButton_IdFOF->setChecked(true);
//			}
//		}
//		else
//		{
//			ui->radioButton_IdOther->setChecked(true);
//		}
//	}
//	else
//	{
//		ui->radioButton_IdPseudo->setChecked(true);
//	}

	if (isOwnId)
	{
		//mStateHelper->setWidgetEnabled(ui->toolButton_Reputation, false);
	}
	else
	{
		// No Reputation yet!
		//mStateHelper->setWidgetEnabled(ui->toolButton_Reputation, /*true*/ false);
	}

	/* now fill in the reputation information */
	ui->line_RatingOverall->setText("Overall Rating TODO");
	ui->line_RatingOwn->setText("Own Rating TODO");

	if (data.mPgpKnown)
	{
		ui->line_RatingImplicit->setText("+50 Known PGP");
	}
	else if (data.mMeta.mGroupFlags & RSGXSID_GROUPFLAG_REALID)
	{
		ui->line_RatingImplicit->setText("+10 UnKnown PGP");
	}
	else
	{
		ui->line_RatingImplicit->setText("+5 Anon Id");
	}

	{
		QString rating = QString::number(data.mReputation.mOverallScore);
		ui->line_RatingOverall->setText(rating);
	}

	{
		QString rating = QString::number(data.mReputation.mIdScore);
		ui->line_RatingImplicit->setText(rating);
	}

	{
		QString rating = QString::number(data.mReputation.mOwnOpinion);
		ui->line_RatingOwn->setText(rating);
	}

	{
		QString rating = QString::number(data.mReputation.mPeerOpinion);
		ui->line_RatingPeers->setText(rating);
	}
}

void IdDetailsDialog::requestIdDetails()
{
	mIdQueue->cancelActiveRequestTokens(IDDETAILSDIALOG_IDDETAILS);

	if (mId.isNull())
	{
		mStateHelper->setActive(IDDETAILSDIALOG_IDDETAILS, false);
		mStateHelper->setLoading(IDDETAILSDIALOG_IDDETAILS, false);
		mStateHelper->clear(IDDETAILSDIALOG_IDDETAILS);

		return;
	}

	mStateHelper->setLoading(IDDETAILSDIALOG_IDDETAILS, true);

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;

	uint32_t token;
	std::list<RsGxsGroupId> groupIds;
	groupIds.push_back(mId);

	mIdQueue->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, groupIds, IDDETAILSDIALOG_IDDETAILS);
}

void IdDetailsDialog::loadRequest(const TokenQueue *queue, const TokenRequest &req)
{
	if (queue != mIdQueue) {
		return;
	}

#ifdef ID_DEBUG
	std::cerr << "IdDetailsDialog::loadRequest() UserType: " << req.mUserType;
	std::cerr << std::endl;
#endif

	switch (req.mUserType)
	{
	case IDDETAILSDIALOG_IDDETAILS:
		insertIdDetails(req.mToken);
		break;

	default:
		std::cerr << "IdDetailsDialog::loadRequest() ERROR";
		std::cerr << std::endl;
	}
}
