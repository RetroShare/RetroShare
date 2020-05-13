/*******************************************************************************
 * retroshare-gui/src/gui/Identity/IdDetailsDialog.cpp                         *
 *                                                                             *
 * Copyright (C) 2014 - 2010 RetroShare Team <retroshare.project@gmail.com>    *
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
#include <QDateTime>

#include "IdDetailsDialog.h"
#include "ui_IdDetailsDialog.h"
#include "gui/gxs/GxsIdDetails.h"
#include "util/qtthreadsutils.h"
#include "gui/settings/rsharesettings.h"
#include "gui/common/UIStateHelper.h"
#include "gui/msgs/MessageComposer.h"
#include "gui/RetroShareLink.h"

#include <retroshare/rspeers.h>

// Data Requests.
#define IDDETAILSDIALOG_IDDETAILS  1
#define IDDETAILSDIALOG_REPLIST    2
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
	mStateHelper->addWidget(IDDETAILSDIALOG_IDDETAILS, ui->lineEdit_Created);
	mStateHelper->addWidget(IDDETAILSDIALOG_IDDETAILS, ui->lineEdit_LastUsed);
	mStateHelper->addWidget(IDDETAILSDIALOG_IDDETAILS, ui->ownOpinion_CB);
	mStateHelper->addWidget(IDDETAILSDIALOG_IDDETAILS, ui->overallOpinion_TF);
	mStateHelper->addWidget(IDDETAILSDIALOG_IDDETAILS, ui->neighborNodesOpinion_TF);

	mStateHelper->addLoadPlaceholder(IDDETAILSDIALOG_IDDETAILS, ui->lineEdit_Nickname);
	mStateHelper->addLoadPlaceholder(IDDETAILSDIALOG_IDDETAILS, ui->lineEdit_GpgName);
	mStateHelper->addLoadPlaceholder(IDDETAILSDIALOG_IDDETAILS, ui->lineEdit_KeyId);
	mStateHelper->addLoadPlaceholder(IDDETAILSDIALOG_IDDETAILS, ui->lineEdit_GpgId);
	mStateHelper->addLoadPlaceholder(IDDETAILSDIALOG_IDDETAILS, ui->lineEdit_Type);
	mStateHelper->addLoadPlaceholder(IDDETAILSDIALOG_IDDETAILS, ui->lineEdit_Created);
	mStateHelper->addLoadPlaceholder(IDDETAILSDIALOG_IDDETAILS, ui->lineEdit_LastUsed);
	mStateHelper->addLoadPlaceholder(IDDETAILSDIALOG_IDDETAILS, ui->lineEdit_GpgName);

	mStateHelper->addClear(IDDETAILSDIALOG_IDDETAILS, ui->lineEdit_Nickname);
	mStateHelper->addClear(IDDETAILSDIALOG_IDDETAILS, ui->lineEdit_KeyId);
	mStateHelper->addClear(IDDETAILSDIALOG_IDDETAILS, ui->lineEdit_GpgId);
	mStateHelper->addClear(IDDETAILSDIALOG_IDDETAILS, ui->lineEdit_Type);
	mStateHelper->addClear(IDDETAILSDIALOG_IDDETAILS, ui->lineEdit_Created);
	mStateHelper->addClear(IDDETAILSDIALOG_IDDETAILS, ui->lineEdit_LastUsed);
	mStateHelper->addClear(IDDETAILSDIALOG_IDDETAILS, ui->lineEdit_GpgName);

	mStateHelper->setActive(IDDETAILSDIALOG_REPLIST, false);

	Settings->loadWidgetInformation(this);

	ui->headerFrame->setHeaderImage(QPixmap(":/icons/png/person.png"));
	ui->headerFrame->setHeaderText(tr("Person Details"));

	//connect(ui.buttonBox, SIGNAL(accepted()), this, SLOT(changeGroup()));
	connect(ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
	connect(ui->ownOpinion_CB, SIGNAL(currentIndexChanged(int)), this, SLOT(modifyReputation()));
	connect(ui->autoBanIdentities_CB, SIGNAL(toggled(bool)), this, SLOT(toggleAutoBanIdentities(bool)));
	
  connect(ui->inviteButton, SIGNAL(clicked()), this, SLOT(sendInvite()));
	
	loadIdentity();
}

/** Destructor. */
IdDetailsDialog::~IdDetailsDialog()
{
	Settings->saveWidgetInformation(this);

	delete(ui);
}

void IdDetailsDialog::toggleAutoBanIdentities(bool b)
{
    RsPgpId id(ui->lineEdit_GpgId->text().left(16).toStdString());

    if(!id.isNull())
    {
        rsReputations->banNode(id,b) ;
        //requestIdList();
    }
}

static QString getHumanReadableDuration(uint32_t seconds)
{
    if(seconds < 60)
        return QString(QObject::tr("%1 seconds ago")).arg(seconds) ;
    else if(seconds < 120)
        return QString(QObject::tr("%1 minute ago")).arg(seconds/60) ;
    else if(seconds < 3600)
        return QString(QObject::tr("%1 minutes ago")).arg(seconds/60) ;
    else if(seconds < 7200)
        return QString(QObject::tr("%1 hour ago")).arg(seconds/3600) ;
    else if(seconds < 24*3600)
        return QString(QObject::tr("%1 hours ago")).arg(seconds/3600) ;
    else if(seconds < 2*24*3600)
        return QString(QObject::tr("%1 day ago")).arg(seconds/86400) ;
    else 
        return QString(QObject::tr("%1 days ago")).arg(seconds/86400) ;
}

void IdDetailsDialog::loadIdentity(RsGxsIdGroup data)
{
	/* get GPG Details from rsPeers */
	RsPgpId ownPgpId  = rsPeers->getGPGOwnId();

	ui->lineEdit_Nickname->setText(QString::fromUtf8(data.mMeta.mGroupName.c_str()));
	ui->lineEdit_KeyId->setText(QString::fromStdString(data.mMeta.mGroupId.toStdString()));
    if(data.mPgpKnown)
	    ui->lineEdit_GpgId->setText(QString::fromStdString(data.mPgpId.toStdString()));
    else
	    ui->lineEdit_GpgId->setText(QString::fromStdString(data.mPgpId.toStdString()) + tr(" [unverified]"));

    ui->autoBanIdentities_CB->setVisible(!data.mPgpId.isNull()) ;
    ui->banoption_label->setVisible(!data.mPgpId.isNull()) ;
	
	ui->lineEdit_Created->setText(QDateTime::fromMSecsSinceEpoch(qint64(1000)*data.mMeta.mPublishTs).toString(Qt::SystemLocaleShortDate));
	
	time_t now = time(NULL) ;
	ui->lineEdit_LastUsed->setText(getHumanReadableDuration(now - data.mLastUsageTS)) ;
	
	QPixmap pixmap;
	
	if(data.mImage.mSize > 0 && GxsIdDetails::loadPixmapFromData(data.mImage.mData, data.mImage.mSize, pixmap, GxsIdDetails::LARGE))
		ui->avatarLabel->setPixmap(pixmap);
	else
	{
		pixmap = GxsIdDetails::makeDefaultIcon(RsGxsId(data.mMeta.mGroupId),GxsIdDetails::LARGE) ;
		ui->avatarLabel->setPixmap(pixmap) ; // we need to use the default pixmap here, generated from the ID
	}

#ifdef ID_DEBUG
	std::cerr << "Setting header frame image : " << pix.width() << " x " << pix.height() << std::endl;
#endif

	if (data.mPgpKnown)
	{
		RsPeerDetails details;
		rsPeers->getGPGDetails(data.mPgpId, details);
		ui->lineEdit_GpgName->setText(QString::fromUtf8(details.name.c_str()));
	}
	else
	{
		if(data.mMeta.mGroupFlags & RSGXSID_GROUPFLAG_REALID_kept_for_compatibility)
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
    {
		if (data.mPgpKnown)
			ui->lineEdit_Type->setText(tr("Identity owned by you, linked to your Retroshare node")) ;
		else
			ui->lineEdit_Type->setText(tr("Anonymous identity, owned by you")) ;
    }
	else if (data.mMeta.mGroupFlags & RSGXSID_GROUPFLAG_REALID_kept_for_compatibility)
	{
		if(data.mPgpKnown)
			if (rsPeers->isGPGAccepted(data.mPgpId))
				ui->lineEdit_Type->setText(tr("Owned by a friend Retroshare node")) ;
			else
				ui->lineEdit_Type->setText(tr("Owned by 2-hops Retroshare node")) ;
		else
			ui->lineEdit_Type->setText(tr("Owned by unknown Retroshare node")) ;
	}
	else
		ui->lineEdit_Type->setText(tr("Anonymous identity")) ;
		
		
	if (isOwnId)
	{
		mStateHelper->setWidgetEnabled(ui->ownOpinion_CB, false);
	}
	else
	{
		// No Reputation yet!
		mStateHelper->setWidgetEnabled(ui->ownOpinion_CB, true);
	}
	
	ui->autoBanIdentities_CB->setChecked(rsReputations->isNodeBanned(data.mPgpId));

/* now fill in the reputation information */

#ifdef SUSPENDED
	if (data.mPgpKnown)
	{
		ui->line_RatingImplicit->setText(tr("+50 Known PGP"));
	}
	else if (data.mMeta.mGroupFlags & RSGXSID_GROUPFLAG_REALID)
	{
		ui->line_RatingImplicit->setText(tr("+10 UnKnown PGP"));
	}
	else
	{
		ui->line_RatingImplicit->setText(tr("+5 Anon Id"));
	}
	{
		QString rating = QString::number(data.mReputation.mIdScore);
		ui->line_RatingImplicit->setText(rating);
	}

#endif

	RsReputationInfo info;
    rsReputations->getReputationInfo(RsGxsId(data.mMeta.mGroupId),data.mPgpId,info) ;
    
#warning (csoler) Do we need to do this? This code is apparently not used.

    QString frep_string ;
    if(info.mFriendsPositiveVotes > 0) frep_string += QString::number(info.mFriendsPositiveVotes) + tr(" positive ") ;
    if(info.mFriendsNegativeVotes > 0) frep_string += QString::number(info.mFriendsNegativeVotes) + tr(" negative ") ;

    if(info.mFriendsPositiveVotes==0 && info.mFriendsNegativeVotes==0)
        frep_string = tr("No votes from friends") ;

    ui->neighborNodesOpinion_TF->setText(frep_string) ;
    
    ui->label_positive->setText(QString::number(info.mFriendsPositiveVotes));
    ui->label_negative->setText(QString::number(info.mFriendsNegativeVotes));

	switch(info.mOverallReputationLevel)
	{
	case RsReputationLevel::LOCALLY_POSITIVE:
		ui->overallOpinion_TF->setText(tr("Positive")); break;
	case RsReputationLevel::LOCALLY_NEGATIVE:
		ui->overallOpinion_TF->setText(tr("Negative (Banned by you)")); break;
	case RsReputationLevel::REMOTELY_POSITIVE:
		ui->overallOpinion_TF->setText(
		            tr("Positive (according to your friends)"));
		break;
	case RsReputationLevel::REMOTELY_NEGATIVE:
		ui->overallOpinion_TF->setText(
		            tr("Negative (according to your friends)"));
		break;
	case RsReputationLevel::NEUTRAL: // fallthrough
	default:
		ui->overallOpinion_TF->setText(tr("Neutral")); break;
	}

	switch(info.mOwnOpinion)
	{
	case RsOpinion::NEGATIVE: ui->ownOpinion_CB->setCurrentIndex(0); break;
	case RsOpinion::NEUTRAL : ui->ownOpinion_CB->setCurrentIndex(1); break;
	case RsOpinion::POSITIVE: ui->ownOpinion_CB->setCurrentIndex(2); break;
	default:
		std::cerr << "Unexpected value in own opinion: "
		          << static_cast<uint32_t>(info.mOwnOpinion) << std::endl;
		break;
	}
}

void IdDetailsDialog::modifyReputation()
{
#ifdef ID_DEBUG
	std::cerr << "IdDialog::modifyReputation()";
	std::cerr << std::endl;
#endif

	RsGxsId id(ui->lineEdit_KeyId->text().toStdString());

	RsOpinion op;

	switch(ui->ownOpinion_CB->currentIndex())
	{
	case 0: op = RsOpinion::NEGATIVE; break;
	case 1: op = RsOpinion::NEUTRAL ; break;
	case 2: op = RsOpinion::POSITIVE; break;
	default:
		std::cerr << "Wrong value from opinion combobox. Bug??" << std::endl;
		return;
	}
	rsReputations->setOwnOpinion(id,op);

	// trigger refresh when finished.
	// basic / anstype are not needed.
	loadIdentity();

	return;
}

void IdDetailsDialog::loadIdentity()
{
    if (mId.isNull())
	{
		mStateHelper->setActive(IDDETAILSDIALOG_IDDETAILS, false);
		mStateHelper->setLoading(IDDETAILSDIALOG_IDDETAILS, false);
		mStateHelper->clear(IDDETAILSDIALOG_IDDETAILS);

		return;
	}

	mStateHelper->setLoading(IDDETAILSDIALOG_IDDETAILS, true);

	RsThread::async([this]()
	{
#ifdef ID_DEBUG
        std::cerr << "Retrieving post data for identity " << mThreadId << std::endl;
#endif

        std::set<RsGxsId> ids( { RsGxsId(mId) } ) ;
        std::vector<RsGxsIdGroup> ids_data;

		if(!rsIdentity->getIdentitiesInfo(ids,ids_data))
		{
			std::cerr << __PRETTY_FUNCTION__ << " failed to retrieve identities group info for id " << mId << std::endl;
			return;
        }

        if(ids_data.size() != 1)
		{
			std::cerr << __PRETTY_FUNCTION__ << " failed to retrieve exactly one group info for id " << mId << std::endl;
			return;
        }
        RsGxsIdGroup group(ids_data[0]);

        RsQThreadUtils::postToObject( [group,this]()
		{
			/* Here it goes any code you want to be executed on the Qt Gui
			 * thread, for example to update the data model with new information
			 * after a blocking call to RetroShare API complete */

            loadIdentity(group);

		}, this );
	});

}


QString IdDetailsDialog::inviteMessage()
{
    return tr("Hi,<br>I want to be friends with you on RetroShare.<br>");
}

void IdDetailsDialog::sendInvite()
{
    /* create a message */
    MessageComposer *composer = MessageComposer::newMsg();

    composer->setTitleText(tr("You have a friend invite"));
    
    RsPeerId ownId = rsPeers->getOwnId();
    RetroShareLink link = RetroShareLink::createCertificate(ownId);
    
    RsGxsId keyId(ui->lineEdit_KeyId->text().toStdString());
    
    QString sMsgText = inviteMessage();
    sMsgText += "<br><br>";
    sMsgText += tr("Respond now:") + "<br>";
    sMsgText += link.toHtml() + "<br>";
    sMsgText += "<br>";
    sMsgText += tr("Thanks, <br>") + QString::fromUtf8(rsPeers->getGPGName(rsPeers->getGPGOwnId()).c_str());
    composer->setMsgText(sMsgText);
    composer->addRecipient(MessageComposer::TO,  RsGxsId(keyId));

    composer->show();

}
