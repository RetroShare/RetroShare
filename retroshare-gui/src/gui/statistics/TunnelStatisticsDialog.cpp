/*******************************************************************************
 * gui/statistics/TunnelStatisticsDialog.cpp                                   *
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

#include <QObject>
#include <util/rsprint.h>
#include "retroshare/rsconfig.h"
#include <retroshare/rsturtle.h>
#include <retroshare/rspeers.h>
#include <retroshare/rsgxstunnel.h>
#include <retroshare/rsservicecontrol.h>
#include <retroshare/rsidentity.h>
#include <retroshare/rsgxsdistsync.h>

#include "TunnelStatisticsDialog.h"
#include <QPainter>
#include <QStylePainter>
#include <algorithm> // for sort
#include <time.h>

#include "util/misc.h"
#include "util/RsQtVersion.h"
#include "gui/settings/rsharesettings.h"

// Defines for Tunnel list columns
#define COLUMN_TUNNELID           0
#define COLUMN_FROM               1
#define COLUMN_DESTINATION        2
#define COLUMN_STATUS             3
#define COLUMN_TOTALSENT          4
#define COLUMN_TOTALRECEIVED      5

TunnelStatisticsDialog::TunnelStatisticsDialog(QWidget *parent)
	: RsAutoUpdatePage(2000,parent)
{
	setupUi(this) ;

	m_bProcessSettings = false;

	float FS = QFontMetricsF(font()).height();
	float fact = FS/14.0 ;

	QHeaderView * _header = authenticatedTunnels_TW->header () ;
	_header->resizeSection (COLUMN_TUNNELID, 270*fact);
	_header->resizeSection (COLUMN_FROM, 270*fact);
	_header->resizeSection (COLUMN_DESTINATION, 270*fact);

	// load settings
    processSettings(true);
}

TunnelStatisticsDialog::~TunnelStatisticsDialog()
{
    // save settings
    processSettings(false);
}

void TunnelStatisticsDialog::processSettings(bool bLoad)
{
    m_bProcessSettings = true;

    Settings->beginGroup(QString("TunnelStatisticsDialog"));

    if (bLoad) {
        // load settings
    } else {
        // save settings
    }

    Settings->endGroup();
    
    m_bProcessSettings = false;
}

void TunnelStatisticsDialog::updateDisplay()
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

QString TunnelStatisticsDialog::getPeerName(const RsPeerId &peer_id)
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

QString TunnelStatisticsDialog::getPeerName(const RsGxsId& gxs_id)
{
    static std::map<RsGxsId, QString> names ;

    std::map<RsGxsId,QString>::const_iterator it = names.find(gxs_id) ;

	if( it != names.end())
		return it->second ;
	else
	{
		RsIdentityDetails detail ;

		if(!rsIdentity->getIdDetails(gxs_id,detail))
			return tr("Unknown Peer");

		return (names[gxs_id] = QString::fromUtf8(detail.mNickname.c_str())) ;
	}
}

QString TunnelStatisticsDialog::speedString(float f)
{
	if(f < 1.0f)
		return QString("0 B/s") ;
	if(f < 1024.0f)
		return QString::number((int)f)+" B/s" ;

	return QString::number(f/1024.0,'f',2) + " KB/s";
}

void TunnelStatisticsDialog::updateTunnels()
{
	//    // Tunnel information
 	//           
	//    GxsTunnelId  tunnel_id ;        // GXS Id we're talking to
	//    RsGxsId  destination_gxs_id ;   // GXS Id we're talking to
	//    RsGxsId  source_gxs_id ;	          // GXS Id we're using to talk
	//    uint32_t tunnel_status ;	          // active, requested, DH pending, etc.
	//    uint32_t total_size_sent ;	          // total bytes sent through that tunnel since openned (including management). 
	//    uint32_t total_size_received ;	  // total bytes received through that tunnel since openned (including management). 

	//    // Data packets

	//    uint32_t pending_data_packets;         // number of packets not acknowledged by other side, still on their way. Should be 0 unless something bad happens.
	//    uint32_t total_data_packets_sent ;     // total number of data packets sent (does not include tunnel management)
	//    uint32_t total_data_packets_received ; // total number of data packets received (does not include tunnel management)

    QTreeWidget *tunnelsTreeWidget = authenticatedTunnels_TW;

    // Clear the existing tree content
    tunnelsTreeWidget->clear();

    // Request info about ongoing tunnels
    std::vector<RsGxsTunnelService::GxsTunnelInfo> tunnel_infos;
    if(rsGxsTunnel) {
        rsGxsTunnel->getTunnelsInfo(tunnel_infos);
    }

    // Populate the tree
    for(uint32_t i = 0; i < tunnel_infos.size(); ++i)
    {
        // Create a top-level item for the Tunnel ID's
        QTreeWidgetItem *tunnelItem = new QTreeWidgetItem(tunnelsTreeWidget);

        // Use local variables to avoid multiple calls to the same function
        QString srcName = getPeerName(tunnel_infos[i].source_gxs_id);
        QString dstName = getPeerName(tunnel_infos[i].destination_gxs_id);

        // Populate Tunnel Data
        tunnelItem->setData(COLUMN_TUNNELID,          Qt::DisplayRole, QString::fromStdString(tunnel_infos[i].tunnel_id.toStdString()));
        tunnelItem->setData(COLUMN_FROM,              Qt::DisplayRole, QString::fromStdString(tunnel_infos[i].source_gxs_id.toStdString())+ " (" + srcName + ")");
        tunnelItem->setData(COLUMN_DESTINATION,       Qt::DisplayRole, QString::fromStdString(tunnel_infos[i].destination_gxs_id.toStdString()) + " (" + dstName + ")");
        tunnelItem->setData(COLUMN_STATUS,            Qt::DisplayRole, QString::number(tunnel_infos[i].tunnel_status));
        tunnelItem->setData(COLUMN_TOTALSENT,         Qt::DisplayRole, misc::friendlyUnit(tunnel_infos[i].total_size_sent ));
        tunnelItem->setData(COLUMN_TOTALRECEIVED,     Qt::DisplayRole, misc::friendlyUnit(tunnel_infos[i].total_size_received));

    }
}

void TunnelStatisticsDialog::hideEvent(QHideEvent *event)
{
    authenticatedTunnels_TW->clear();
    QWidget::hideEvent(event);
}
