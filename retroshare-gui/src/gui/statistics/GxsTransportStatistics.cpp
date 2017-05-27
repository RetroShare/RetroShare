/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 20011, RetroShare Team
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

#include <iostream>
#include <QTimer>
#include <QObject>
#include <QFontMetrics>
#include <QWheelEvent>
#include <time.h>

#include <QMenu>
#include <QPainter>
#include <QStylePainter>
#include <QLayout>
#include <QHeaderView>

#include <retroshare/rsgxstrans.h>
#include <retroshare/rspeers.h>
#include <retroshare/rsidentity.h>
#include <retroshare/rsgxstrans.h>

#include "GxsTransportStatistics.h"

#include "gui/Identity/IdDetailsDialog.h"
#include "gui/settings/rsharesettings.h"
#include "util/QtVersion.h"
#include "gui/common/UIStateHelper.h"
#include "util/misc.h"

#define COL_PENDING_ID                  0
#define COL_PENDING_DESTINATION         1
#define COL_PENDING_NICKNAME            2
#define COL_PENDING_DATASTATUS          3
#define COL_PENDING_DATASIZE            4
#define COL_PENDING_DATAHASH            5
#define COL_PENDING_SEND                6
#define COL_PENDING_GROUP_ID            7

#define COL_GROUP_GRP_ID                  0
#define COL_GROUP_NUM_MSGS                1
#define COL_GROUP_SIZE_MSGS               2

static const int PARTIAL_VIEW_SIZE                           =  9 ;
static const int MAX_TUNNEL_REQUESTS_DISPLAY                 = 10 ;
static const int GXSTRANS_STATISTICS_DELAY_BETWEEN_GROUP_REQ = 30 ;	// never request more than every 30 secs.

#define GXSTRANS_GROUP_META  0x01
#define GXSTRANS_GROUP_DATA  0x02
#define GXSTRANS_GROUP_STAT  0x03

// static QColor colorScale(float f)
// {
// 	if(f == 0)
// 		return QColor::fromHsv(0,0,192) ;
// 	else
// 		return QColor::fromHsv((int)((1.0-f)*280),200,255) ;
// }

GxsTransportStatistics::GxsTransportStatistics(QWidget *parent)
    : RsAutoUpdatePage(2000,parent)
{
	setupUi(this) ;
	
	mStateHelper = new UIStateHelper(this);
	mStateHelper->addWidget(GXSTRANS_GROUP_META, treeWidget);

	mTransQueue = new TokenQueue(rsGxsTrans->getTokenService(), this);

	m_bProcessSettings = false;
    mLastGroupReqTS = 0 ;

	//_router_F->setWidget( _tst_CW = new GxsTransportStatisticsWidget() ) ;

	/* Set header resize modes and initial section sizes Uploads TreeView*/
	QHeaderView_setSectionResizeMode(treeWidget->header(), QHeaderView::ResizeToContents);

	connect(treeWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(CustomPopupMenu(QPoint)));


	// load settings
	processSettings(true);
}

GxsTransportStatistics::~GxsTransportStatistics()
{

    // save settings
    processSettings(false);
}

void GxsTransportStatistics::processSettings(bool bLoad)
{
    m_bProcessSettings = true;

    Settings->beginGroup(QString("GxsTransportStatistics"));

    if (bLoad) {
        // load settings

        // state of splitter
        //splitter->restoreState(Settings->value("Splitter").toByteArray());
    } else {
        // save settings

        // state of splitter
        //Settings->setValue("Splitter", splitter->saveState());

    }

    Settings->endGroup();
    
    m_bProcessSettings = false;
}

void GxsTransportStatistics::CustomPopupMenu( QPoint )
{
	QMenu contextMnu( this );
	
	QTreeWidgetItem *item = treeWidget->currentItem();
	if (item) {
	contextMnu.addAction(QIcon(":/images/info16.png"), tr("Details"), this, SLOT(personDetails()));

  }

	contextMnu.exec(QCursor::pos());
}

void GxsTransportStatistics::updateDisplay()
{
    time_t now = time(NULL) ;

    if(mLastGroupReqTS + GXSTRANS_STATISTICS_DELAY_BETWEEN_GROUP_REQ < now)
    {
        requestGroupMeta();
        mLastGroupReqTS = now ;
    }

	//_tst_CW->updateContent() ;

	updateContent();
}

QString GxsTransportStatistics::getPeerName(const RsPeerId &peer_id)
{
	static std::map<RsPeerId, QString> names ;

	std::map<RsPeerId,QString>::const_iterator it = names.find(peer_id) ;

	if( it != names.end())
		return it->second ;
	else
	{
		RsPeerDetails detail ;
		if(!rsPeers->getPeerDetails(peer_id,detail))
			return tr("Unknown Peer");

		return (names[peer_id] = QString::fromUtf8(detail.name.c_str())) ;
	}
}

static QString getStatusString(GxsTransSendStatus status)
{
    switch(status)
	{
    case GxsTransSendStatus::PENDING_PROCESSING         :  return QObject::tr("Processing") ;
	case GxsTransSendStatus::PENDING_PREFERRED_GROUP    :  return QObject::tr("Choosing group") ;
	case GxsTransSendStatus::PENDING_RECEIPT_CREATE     :  return QObject::tr("Creating receipt") ;
	case GxsTransSendStatus::PENDING_RECEIPT_SIGNATURE  :  return QObject::tr("Signing receipt") ;
	case GxsTransSendStatus::PENDING_SERIALIZATION      :  return QObject::tr("Serializing") ;
	case GxsTransSendStatus::PENDING_PAYLOAD_CREATE     :  return QObject::tr("Creating payload") ;
	case GxsTransSendStatus::PENDING_PAYLOAD_ENCRYPT    :  return QObject::tr("Encrypting payload") ;
	case GxsTransSendStatus::PENDING_PUBLISH            :  return QObject::tr("Publishing") ;
	case GxsTransSendStatus::PENDING_RECEIPT_RECEIVE    :  return QObject::tr("Waiting for receipt") ;
	case GxsTransSendStatus::RECEIPT_RECEIVED           :  return QObject::tr("Receipt received") ;
	case GxsTransSendStatus::FAILED_RECEIPT_SIGNATURE   :  return QObject::tr("Receipt signature failed") ;
	case GxsTransSendStatus::FAILED_ENCRYPTION          :  return QObject::tr("Encryption failed") ;
	case GxsTransSendStatus::UNKNOWN                    :
	default                                             :  return QObject::tr("Unknown") ;
	}
}

void GxsTransportStatistics::updateContent()
{
    RsGxsTrans::GxsTransStatistics transinfo ;

    rsGxsTrans->getStatistics(transinfo) ;

    treeWidget->clear();
    time_t now = time(NULL) ;
    
    // 1 - fill the table for pending packets

    groupBox->setTitle(tr("Pending data items")+": " + QString::number(transinfo.outgoing_records.size()) );

    for(uint32_t i=0;i<transinfo.outgoing_records.size();++i)
    {
        const RsGxsTransOutgoingRecord& rec(transinfo.outgoing_records[i]) ;

        QTreeWidgetItem *item = new QTreeWidgetItem();
        treeWidget->addTopLevelItem(item);
        
        RsIdentityDetails details ;
        rsIdentity->getIdDetails(rec.recipient,details);
        QString nickname = QString::fromUtf8(details.mNickname.c_str());
        
        if(nickname.isEmpty())
          nickname = tr("Unknown");

        item -> setData(COL_PENDING_ID,           Qt::DisplayRole, QString::number(rec.trans_id,16).rightJustified(8,'0'));
        item -> setData(COL_PENDING_NICKNAME,     Qt::DisplayRole, nickname ) ;
        item -> setData(COL_PENDING_DESTINATION,  Qt::DisplayRole, QString::fromStdString(rec.recipient.toStdString()));
        item -> setData(COL_PENDING_DATASTATUS,   Qt::DisplayRole, getStatusString(rec.status));
        item -> setData(COL_PENDING_DATASIZE,     Qt::DisplayRole, misc::friendlyUnit(rec.data_size));
        item -> setData(COL_PENDING_DATAHASH,     Qt::DisplayRole, QString::fromStdString(rec.data_hash.toStdString()));
        item -> setData(COL_PENDING_SEND,         Qt::DisplayRole, QString::number(now - rec.send_TS));
        item -> setData(COL_PENDING_GROUP_ID,     Qt::DisplayRole, QString::fromStdString(rec.group_id.toStdString()));
    }

    // 2 - fill the table for pending group data

    groupTreeWidget->clear();

    for(std::map<RsGxsGroupId,GxsGroupStatistic>::const_iterator it(mGroupStats.begin());it!=mGroupStats.end();++it)
    {
        const GxsGroupStatistic& stat(it->second) ;

		QTreeWidgetItem *item = new QTreeWidgetItem();
        groupTreeWidget->addTopLevelItem(item);

        item->setData(COL_GROUP_GRP_ID,   Qt::DisplayRole,  QString::fromStdString(stat.mGrpId.toStdString())) ;
        item->setData(COL_GROUP_NUM_MSGS, Qt::DisplayRole,  QString::number(stat.mNumMsgs)) ;
        item->setData(COL_GROUP_SIZE_MSGS,Qt::DisplayRole,  QString::number(stat.mTotalSizeOfMsgs)) ;
    }
}

void GxsTransportStatistics::personDetails()
{
    QTreeWidgetItem *item = treeWidget->currentItem();
    std::string id = item->text(COL_PENDING_DESTINATION).toStdString();

    if (id.empty()) {
        return;
    }
    
    IdDetailsDialog *dialog = new IdDetailsDialog(RsGxsGroupId(id));
    dialog->show();
}

void GxsTransportStatistics::loadRequest(const TokenQueue *queue, const TokenRequest &req)
{
	std::cerr << "GxsTransportStatistics::loadRequest() UserType: " << req.mUserType << std::endl;

	if (queue != mTransQueue)
	{
		std::cerr << "Wrong queue!" << std::endl;
		return ;
	}

	/* now switch on req */
	switch(req.mUserType)
	{
	case GXSTRANS_GROUP_META: loadGroupMeta(req.mToken);
		break;

	case GXSTRANS_GROUP_DATA: loadGroupData(req.mToken);
		break;

	case GXSTRANS_GROUP_STAT: loadGroupStat(req.mToken);
		break;

	default:
		std::cerr << "GxsTransportStatistics::loadRequest() ERROR: INVALID TYPE";
		std::cerr << std::endl;
		break;
	}
}

void GxsTransportStatistics::requestGroupMeta()
{
	mStateHelper->setLoading(GXSTRANS_GROUP_META, true);

	std::cerr << "GxsTransportStatisticsWidget::requestGroupMeta()";
	std::cerr << std::endl;

	mTransQueue->cancelActiveRequestTokens(GXSTRANS_GROUP_META);

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_META;

	uint32_t token;
	mTransQueue->requestGroupInfo(token,  RS_TOKREQ_ANSTYPE_SUMMARY, opts, GXSTRANS_GROUP_META);
}
void GxsTransportStatistics::requestGroupStat(const RsGxsGroupId &groupId)
{
	mTransQueue->cancelActiveRequestTokens(GXSTRANS_GROUP_STAT);
	uint32_t token;
	rsGxsTrans->getTokenService()->requestGroupStatistic(token, groupId);
	mTransQueue->queueRequest(token, 0, RS_TOKREQ_ANSTYPE_ACK, GXSTRANS_GROUP_STAT);
}

void GxsTransportStatistics::loadGroupStat(const uint32_t &token)
{
    std::cerr << "GxsTransportStatistics::loadGroupStat." << std::endl;
	GxsGroupStatistic stats;
	rsGxsTrans->getGroupStatistic(token, stats);

    mGroupStats[stats.mGrpId] = stats ;
}

void GxsTransportStatistics::loadGroupMeta(const uint32_t& token)
{
	mStateHelper->setLoading(GXSTRANS_GROUP_META, false);

	std::cerr << "GxsTransportStatisticsWidget::loadGroupMeta()";
	std::cerr << std::endl;

//	ui.treeWidget_membership->clear();

	std::list<RsGroupMetaData> groupInfo;
	std::list<RsGroupMetaData>::iterator vit;

	if (!rsGxsTrans->getGroupSummary(token,groupInfo))
	{
		std::cerr << "GxsTransportStatistics::loadGroupMeta() Error getting GroupMeta";
		std::cerr << std::endl;
		mStateHelper->setActive(GXSTRANS_GROUP_META, false);
		return;
	}

	mStateHelper->setActive(GXSTRANS_GROUP_META, true);

//	/* add the top level item */
//	QTreeWidgetItem *personalCirclesItem = new QTreeWidgetItem();
//	personalCirclesItem->setText(0, tr("Personal Circles"));
//	ui.treeWidget_membership->addTopLevelItem(personalCirclesItem);
//
//	QTreeWidgetItem *externalAdminCirclesItem = new QTreeWidgetItem();
//	externalAdminCirclesItem->setText(0, tr("External Circles (Admin)"));
//	ui.treeWidget_membership->addTopLevelItem(externalAdminCirclesItem);
//
//	QTreeWidgetItem *externalSubCirclesItem = new QTreeWidgetItem();
//	externalSubCirclesItem->setText(0, tr("External Circles (Subscribed)"));
//	ui.treeWidget_membership->addTopLevelItem(externalSubCirclesItem);
//
//	QTreeWidgetItem *externalOtherCirclesItem = new QTreeWidgetItem();
//	externalOtherCirclesItem->setText(0, tr("External Circles (Other)"));
//	ui.treeWidget_membership->addTopLevelItem(externalOtherCirclesItem);

    std::set<RsGxsGroupId> existing_groups ;

	for(vit = groupInfo.begin(); vit != groupInfo.end(); ++vit)
	{
        existing_groups.insert(vit->mGroupId) ;

		/* Add Widget, and request Pages */
		std::cerr << "GxsTransportStatisticsWidget::loadGroupMeta() GroupId: " << vit->mGroupId << " Group: " << vit->mGroupName << std::endl;

        requestGroupStat(vit->mGroupId) ;
	}

    // remove group stats for group that do not exist anymore

    for(std::map<RsGxsGroupId,GxsGroupStatistic>::iterator it(mGroupStats.begin());it!=mGroupStats.end();)
        if(existing_groups.find(it->first) == existing_groups.end())
			it = mGroupStats.erase(it);
		else
			++it;
}

void GxsTransportStatistics::loadGroupData(const uint32_t& token)
{
    std::cerr << __PRETTY_FUNCTION__ << ": not implemented." << std::endl;
}
