/*******************************************************************************
 * gui/statistics/GxsNetTunnelsDialog.cpp                                      *
 *                                                                             *
 * Copyright (c) 2025 Retroshare Team <retroshare.project@gmail.com>           *
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

#include "retroshare/rsconfig.h"
#include <retroshare/rsturtle.h>
#include <retroshare/rspeers.h>
#include <retroshare/rsgxstunnel.h>
#include <retroshare/rsservicecontrol.h>
#include <retroshare/rsidentity.h>
#include <retroshare/rsgxsdistsync.h>

#include "GxsNetTunnelsDialog.h"

#include <QObject>
#include <QModelIndex>
#include <QHeaderView>
#include <QPainter>
#include <QStylePainter>
#include <algorithm> // for sort
#include <time.h>

#include <util/rsprint.h>
#include "util/RsQtVersion.h"
#include "gui/settings/rsharesettings.h"

class NetTunnelsListDelegate: public QAbstractItemDelegate
{
public:
    NetTunnelsListDelegate(QObject *parent=0);
    virtual ~NetTunnelsListDelegate();
    void paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const;
    QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const;
};

NetTunnelsListDelegate::NetTunnelsListDelegate(QObject *parent) : QAbstractItemDelegate(parent)
{
}

NetTunnelsListDelegate::~NetTunnelsListDelegate(void)
{
}

void NetTunnelsListDelegate::paint(QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index) const
{
	QString strNA = tr("N/A");
	QStyleOptionViewItem opt = option;

	QString temp ;
	float flValue;
	qint64 qi64Value;

	// prepare
	painter->save();
	painter->setClipRect(opt.rect);

	//set text color
	QVariant value = index.data(Qt::ForegroundRole);
	if(value.isValid() && qvariant_cast<QColor>(value).isValid()) {
		opt.palette.setColor(QPalette::Text, qvariant_cast<QColor>(value));
	}
	QPalette::ColorGroup cg = option.state & QStyle::State_Enabled ? QPalette::Normal : QPalette::Disabled;
	if(option.state & QStyle::State_Selected){
		painter->setPen(opt.palette.color(cg, QPalette::HighlightedText));
	} else {
		painter->setPen(opt.palette.color(cg, QPalette::Text));
	}

	// draw the background color
	if(option.showDecorationSelected && (option.state & QStyle::State_Selected)) {
		if(cg == QPalette::Normal && !(option.state & QStyle::State_Active)) {
			cg = QPalette::Inactive;
		}
		painter->fillRect(option.rect, option.palette.brush(cg, QPalette::Highlight));
	} else {
		value = index.data(Qt::BackgroundRole);
		if(value.isValid() && qvariant_cast<QColor>(value).isValid()) {
			painter->fillRect(option.rect, qvariant_cast<QColor>(value));
		}
	}

	switch(index.column()) {
	case COLUMN_SERVICE :
		painter->drawText(option.rect, Qt::AlignLeft, index.data().toString());
		break;
	case COLUMN_GROUPID:
		painter->drawText(option.rect, Qt::AlignLeft, index.data().toString());
		break;
	case COLUMN_POLICY:
		painter->drawText(option.rect, Qt::AlignLeft, index.data().toString());
		break;
	case COLUMN_STATUS:
		painter->drawText(option.rect, Qt::AlignHCenter, index.data().toString());
		break;
	case COLUMN_LASTCONTACT:
		painter->drawText(option.rect, Qt::AlignLeft, index.data().toString());
		break;
	default:
		painter->drawText(option.rect, Qt::AlignLeft, index.data().toString());
	}

	// done
	painter->restore();
}

QSize NetTunnelsListDelegate::sizeHint(const QStyleOptionViewItem & option/*option*/, const QModelIndex & index) const
{
    float FS = QFontMetricsF(option.font).height();
    //float fact = FS/14.0 ;

    float w = QFontMetrics_horizontalAdvance(QFontMetricsF(option.font), index.data(Qt::DisplayRole).toString());

    return QSize(w,FS*1.2);
    //return QSize(50*fact,17*fact);
}

GxsNetTunnelsDialog::GxsNetTunnelsDialog(QWidget *parent)
	: RsAutoUpdatePage(2000,parent)
{
	setupUi(this) ;

	m_ProcessSettings = false;

	TunnelDelegate = new NetTunnelsListDelegate();
	groups_treeWidget->setItemDelegate(TunnelDelegate);

	float FS = QFontMetricsF(font()).height();
	float fact = FS/14.0 ;

	QHeaderView * _header = groups_treeWidget->header () ;
	_header->resizeSection (COLUMN_SERVICE, 270*fact);
	_header->resizeSection (COLUMN_GROUPID, 270*fact);
	_header->resizeSection (COLUMN_STATUS, 140*fact);

	// load settings
	processSettings(true);
}

GxsNetTunnelsDialog::~GxsNetTunnelsDialog()
{
    // save settings
    processSettings(false);
}

void GxsNetTunnelsDialog::processSettings(bool bLoad)
{
    m_ProcessSettings = true;

    Settings->beginGroup(QString("GxsNetTunnelsDialog"));

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
    
    m_ProcessSettings = false;

}

static QString getGroupStatusString(RsGxsNetTunnelGroupInfo::GroupStatus group_status)
{
    switch(group_status)
    {
    default:
	   	    case RsGxsNetTunnelGroupInfo::RS_GXS_NET_TUNNEL_GRP_STATUS_UNKNOWN            : return QObject::tr("Unknown") ;
	   	    case RsGxsNetTunnelGroupInfo::RS_GXS_NET_TUNNEL_GRP_STATUS_IDLE               : return QObject::tr("Idle");
	   	    case RsGxsNetTunnelGroupInfo::RS_GXS_NET_TUNNEL_GRP_STATUS_VPIDS_AVAILABLE    : return QObject::tr("Virtual peers available");
    }
    return QString();
}

static QString getGroupPolicyString(RsGxsNetTunnelGroupInfo::GroupPolicy group_policy)
{
    switch(group_policy)
    {
    default:
	 		case RsGxsNetTunnelGroupInfo::RS_GXS_NET_TUNNEL_GRP_POLICY_UNKNOWN            : return QObject::tr("Unknown") ;
	 		case RsGxsNetTunnelGroupInfo::RS_GXS_NET_TUNNEL_GRP_POLICY_PASSIVE            : return QObject::tr("Passive") ;
	 		case RsGxsNetTunnelGroupInfo::RS_GXS_NET_TUNNEL_GRP_POLICY_ACTIVE             : return QObject::tr("Active") ;
	 		case RsGxsNetTunnelGroupInfo::RS_GXS_NET_TUNNEL_GRP_POLICY_REQUESTING         : return QObject::tr("Requesting peers") ;
    }
    return QString();
}

static QString getLastContactString(time_t last_contact)
{
    time_t now = time(NULL);

    if(last_contact == 0)
        return QObject::tr("Never");

    return QString::number(now - last_contact) + " secs ago" ;
}

static QString getServiceNameString(uint16_t service_id)
{
	static RsPeerServiceInfo ownServices;

    if(ownServices.mServiceList.find(service_id) == ownServices.mServiceList.end())
		rsServiceControl->getOwnServices(ownServices);

    return QString::fromUtf8(ownServices.mServiceList[RsServiceInfo::RsServiceInfoUIn16ToFullServiceId(service_id)].mServiceName.c_str()) ;
}

static QString getVirtualPeerStatusString(uint8_t status)
{
	switch(status)
	{
	default:
	case RsGxsNetTunnelVirtualPeerInfo::RS_GXS_NET_TUNNEL_VP_STATUS_UNKNOWN   : return QObject::tr("Unknown") ;
	case RsGxsNetTunnelVirtualPeerInfo::RS_GXS_NET_TUNNEL_VP_STATUS_TUNNEL_OK : return QObject::tr("Tunnel OK") ;
	case RsGxsNetTunnelVirtualPeerInfo::RS_GXS_NET_TUNNEL_VP_STATUS_ACTIVE    : return QObject::tr("Tunnel active") ;
	}
	return QString();
}

static QString getSideString(uint8_t side)
{
    return side?QObject::tr("Client"):QObject::tr("Server") ;
}

static QString getMasterKeyString(const uint8_t *key,uint32_t size)
{
    return QString::fromStdString(RsUtil::BinToHex(key,size,10));
}

void GxsNetTunnelsDialog::updateDisplay()
{
	/* do nothing if locked, or not visible */
	if (RsAutoUpdatePage::eventsLocked() == true) 
	{
		return;
	}

	if (!rsConfig)
	{
		return;
	}

	updateTunnels();
}

void GxsNetTunnelsDialog::updateTunnels()
{
	// RsGxsNetTunnelGroupInfo:
	//
	//   enum GroupStatus {
	//   	    RS_GXS_NET_TUNNEL_GRP_STATUS_UNKNOWN            = 0x00,	// unknown status
	//   	    RS_GXS_NET_TUNNEL_GRP_STATUS_IDLE               = 0x01,	// no virtual peers requested, just waiting
	//   	    RS_GXS_NET_TUNNEL_GRP_STATUS_VPIDS_AVAILABLE    = 0x02	// some virtual peers are available. Data can be read/written
	//   };
	//   enum GroupPolicy {
	//   	    RS_GXS_NET_TUNNEL_GRP_POLICY_UNKNOWN            = 0x00,	// nothing has been set
	//   	    RS_GXS_NET_TUNNEL_GRP_POLICY_PASSIVE            = 0x01,	// group is available for server side tunnels, but does not explicitely request tunnels
	//   	    RS_GXS_NET_TUNNEL_GRP_POLICY_ACTIVE             = 0x02,	// group will only explicitely request tunnels if none available
	//   	    RS_GXS_NET_TUNNEL_GRP_POLICY_REQUESTING         = 0x03,	// group explicitely requests tunnels
	//      };
    //
	//   RsGxsNetTunnelGroupInfo() : group_policy(RS_GXS_NET_TUNNEL_GRP_POLICY_PASSIVE),group_status(RS_GXS_NET_TUNNEL_GRP_STATUS_IDLE),last_contact(0) {}
    //
	//   GroupPolicy        group_policy ;
	//   GroupStatus        group_status ;
	//   time_t             last_contact ;
	//   RsFileHash         hash ;
	//   uint16_t           service_id ;
	//   std::set<RsPeerId> virtual_peers ;
    //
 	// struct RsGxsNetTunnelVirtualPeerInfo:
	//
	// 	enum {   RS_GXS_NET_TUNNEL_VP_STATUS_UNKNOWN   = 0x00,		// unknown status.
	// 		     RS_GXS_NET_TUNNEL_VP_STATUS_TUNNEL_OK = 0x01,		// tunnel has been established and we're waiting for virtual peer id
	// 		     RS_GXS_NET_TUNNEL_VP_STATUS_ACTIVE    = 0x02		// virtual peer id is known. Data can transfer.
	// 	     };
	//
	// 	RsGxsNetTunnelVirtualPeerInfo() : vpid_status(RS_GXS_NET_TUNNEL_VP_STATUS_UNKNOWN), last_contact(0),side(0) { memset(encryption_master_key,0,32) ; }
	// 	virtual ~RsGxsNetTunnelVirtualPeerInfo(){}
	//
	// 	uint8_t vpid_status ;					// status of the peer
	// 	time_t  last_contact ;					// last time some data was sent/recvd
	// 	uint8_t side ;	                        // client/server
	// 	uint8_t encryption_master_key[32];
	//
	// 	RsPeerId      turtle_virtual_peer_id ;  // turtle peer to use when sending data to this vpid.
	//
	// 	RsGxsGroupId group_id ;					// group that virtual peer is providing
	// 	uint16_t service_id ; 					// this is used for checkng consistency of the incoming data

    QTreeWidget *peerTreeWidget = groups_treeWidget;

    // Clear existing items to prevent duplicates on refresh
    peerTreeWidget->clear();

    // Request info about ongoing tunnels
    std::map<RsGxsGroupId, RsGxsNetTunnelGroupInfo> groups;                             // groups on the client and server side
    std::map<TurtleVirtualPeerId, RsGxsNetTunnelVirtualPeerId> turtle2gxsnettunnel;     // convertion table from turtle to net tunnel virtual peer id
    std::map<RsGxsNetTunnelVirtualPeerId, RsGxsNetTunnelVirtualPeerInfo> virtual_peers;  // current virtual peers, which group they provide, and how to talk to them through turtle
    Bias20Bytes bias;

    // Fetch data from rsGxsDistSync
    if (rsGxsDistSync) {
        rsGxsDistSync->getStatistics(groups, virtual_peers, turtle2gxsnettunnel, bias);
    }

    randombias->setText(tr("Random Bias: %1").arg(getMasterKeyString(bias.toByteArray(),20))) ;
    peerTreeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    
    for(auto it = groups.begin(); it != groups.end(); ++it)
    {
        // Create the Parent (Group) Item
        QTreeWidgetItem *groups_item = new QTreeWidgetItem(peerTreeWidget);
        groups_item->setExpanded(true); // This prevents it from "closing"

        // Populate Group Data
        groups_item->setData(COLUMN_SERVICE,     Qt::DisplayRole, "0x" + QString::number(it->second.service_id, 16) + " (" + getServiceNameString(it->second.service_id) + ")");
        groups_item->setData(COLUMN_GROUPID,     Qt::DisplayRole, QString::fromStdString(it->first.toStdString()));
        groups_item->setData(COLUMN_POLICY,      Qt::DisplayRole, getGroupPolicyString(it->second.group_policy));
        groups_item->setData(COLUMN_STATUS,      Qt::DisplayRole, getGroupStatusString(it->second.group_status)); 
        groups_item->setData(COLUMN_LASTCONTACT, Qt::DisplayRole, getLastContactString(it->second.last_contact));

        // Iterate through Virtual Peers for this group
        for(auto it2 = it->second.virtual_peers.begin(); it2 != it->second.virtual_peers.end(); ++it2)
        {
            auto it4 = turtle2gxsnettunnel.find(*it2);
            QTreeWidgetItem *peers_item = new QTreeWidgetItem(groups_item); // Set parent immediately

            if(it4 != turtle2gxsnettunnel.end())
            {
                auto it3 = virtual_peers.find(it4->second);

                if(it3 != virtual_peers.end())
                {
                    // Populate Peer Data
                    peers_item->setData(COLUMN_SERVICE,        Qt::DisplayRole, QString::fromStdString((*it2).toStdString()));
                    peers_item->setData(COLUMN_GROUPID,        Qt::DisplayRole, getMasterKeyString(it3->second.encryption_master_key, 32));
                    peers_item->setData(COLUMN_STATUS,         Qt::DisplayRole, getVirtualPeerStatusString(it3->second.vpid_status) + " (" + getSideString(it3->second.side) + ")");
                    peers_item->setData(COLUMN_LASTCONTACT,    Qt::DisplayRole, getLastContactString(it3->second.last_contact));
                }
                else 
                {
                    peers_item->setData(COLUMN_SERVICE, Qt::DisplayRole, QString::fromStdString((*it2).toStdString()) + " (No info)");
                }
            }
            else
            {
                peers_item->setData(COLUMN_SERVICE, Qt::DisplayRole, QString::fromStdString((*it2).toStdString()) );
                peers_item->setData(COLUMN_GROUPID, Qt::DisplayRole, tr(" (No Peer information available)"));
            }
        }
    }
}
