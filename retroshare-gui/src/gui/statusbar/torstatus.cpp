/*******************************************************************************
 * gui/statusbar/torstatus.cpp                                                 *
 *                                                                             *
 * Copyright (c) 2012 Retroshare Team <retroshare.project@gmail.com>           *
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

#include "torstatus.h"

#include <QLayout>
#include <QLabel>
#include <QIcon>
#include <QPixmap>

#include "retroshare/rsconfig.h"
#include "retroshare/rsinit.h"
#include "retroshare/rspeers.h"
#include "retroshare/rstor.h"
#include <QTcpSocket>
#include "util/misc.h"

#include "gui/common/FilesDefs.h"

#include <iomanip>

TorStatus::TorStatus(QWidget *parent)
 : QWidget(parent)
{
    QHBoxLayout *hbox = new QHBoxLayout();
    hbox->setMargin(0);
    hbox->setSpacing(6);
       
    statusTor = new QLabel("<strong>" + tr("Tor") + ":</strong>", this );
	statusTor->setToolTip(tr("<p>This version of Retroshare uses Tor to connect to your trusted nodes.</p>")) ;
    hbox->addWidget(statusTor);
    
    torstatusLabel = new QLabel( this );
    torstatusLabel->setPixmap(FilesDefs::getPixmapFromQtResourcePath(":/icons/no-tor.png"));
    hbox->addWidget(torstatusLabel);
    
    _compactMode = false;
    _updated = false;

    setLayout( hbox );
}

void TorStatus::getTorStatus()
{
	statusTor->setVisible(!_compactMode);
	QString text = _compactMode?statusTor->text():"";

	/* check local network state. We cannot make sure that Tor is running yet. */
	RsNetState netState = rsConfig -> getNetState();
	bool online ;

	switch(netState)
	{
		default:
	    case RsNetState::BAD_UNKNOWN:
	    case RsNetState::BAD_OFFLINE: online = false ;
										 break ;

	    case RsNetState::WARNING_RESTART:

	    case RsNetState::BAD_NATSYM:
	    case RsNetState::BAD_NODHT_NAT:
	    case RsNetState::WARNING_NATTED:
	    case RsNetState::WARNING_NODHT:
	    case RsNetState::GOOD:
	    case RsNetState::ADV_FORWARD: online = true ;
										 break ;
	}

	/* now the extra bit .... switch on check boxes */

	int S = QFontMetricsF(torstatusLabel->font()).height();

    if(RsAccounts::isTorAuto())
	{
		// get Tor status
        RsTorControlStatus tor_control_status = RsTor::torControlStatus();
        RsTorStatus torstatus = RsTor::torStatus();

		QString tor_control_status_str,torstatus_str ;
		bool tor_control_ok ;

		switch(tor_control_status)
		{
		default:
        case RsTorControlStatus::ERROR :			tor_control_ok = false ; tor_control_status_str = "Error" ; break ;
        case RsTorControlStatus::NOT_CONNECTED:		tor_control_ok = false ; tor_control_status_str = "Not connected" ; break ;
        case RsTorControlStatus::CONNECTING:		tor_control_ok = false ; tor_control_status_str = "Connecting" ; break ;
        case RsTorControlStatus::AUTHENTICATING:	tor_control_ok = false ; tor_control_status_str = "Authenticating" ; break ;
        case RsTorControlStatus::CONNECTED:			tor_control_ok = true  ; tor_control_status_str = "Connected" ; break ;
		}

		switch(torstatus)
		{
		default:
        case RsTorStatus::UNKNOWN: 	torstatus_str = "Unknown" ; break ;
        case RsTorStatus::OFFLINE: 	torstatus_str = "Tor offline" ; break ;
        case RsTorStatus::READY: 	torstatus_str = "Tor ready" ; break ;
		}

#define MIN_RS_NET_SIZE		10

        if(torstatus == RsTorStatus::OFFLINE || !online || !tor_control_ok)
		{
			// RED - some issue.
            torstatusLabel->setPixmap(FilesDefs::getPixmapFromQtResourcePath(":/icons/tor-stopping.png").scaledToHeight(1.5*S,Qt::SmoothTransformation));
			torstatusLabel->setToolTip( text + tr("Tor is currently offline"));
		}
        else if(torstatus == RsTorStatus::READY && online && tor_control_ok)
		{
            torstatusLabel->setPixmap(FilesDefs::getPixmapFromQtResourcePath(":/icons/tor-on.png").scaledToHeight(1.5*S,Qt::SmoothTransformation));
			torstatusLabel->setToolTip( text + tr("Tor is OK"));
		}
		else // torstatus == Tor::TorControl::TorUnknown
		{
			// GRAY.
            torstatusLabel->setPixmap(FilesDefs::getPixmapFromQtResourcePath(":/icons/no-tor.png").scaledToHeight(1.5*S,Qt::SmoothTransformation));
			torstatusLabel->setToolTip( text + tr("No tor configuration"));
		}
	}
	else
	{
		if(!_updated)
		{
			RsPeerDetails pd;
			uint32_t hiddentype;
			if (rsPeers->getPeerDetails(rsPeers->getOwnId(), pd)) {
				if(pd.netMode == RS_NETMODE_HIDDEN)
				{
					hiddentype = pd.hiddenType;
				}
			}
			std::string proxyaddr;
			uint16_t proxyport;
			uint32_t status ;
			QTcpSocket socket ;
			if(hiddentype == RS_HIDDEN_TYPE_TOR)
			{
				rsPeers->getProxyServer(RS_HIDDEN_TYPE_TOR, proxyaddr, proxyport, status);
				socket.connectToHost(QString::fromStdString(proxyaddr),quint16(proxyport));
				if(socket.waitForConnected(500))
				{
					socket.disconnectFromHost();
					torstatusLabel->setPixmap(FilesDefs::getPixmapFromQtResourcePath(":/icons/tor-on.png").scaledToHeight(S,Qt::SmoothTransformation));
					torstatusLabel->setToolTip( text + tr("Tor proxy is OK"));
				}
				else
				{
					torstatusLabel->setPixmap(FilesDefs::getPixmapFromQtResourcePath(":/icons/tor-off.png").scaledToHeight(S,Qt::SmoothTransformation));
					torstatusLabel->setToolTip( text + tr("Tor proxy is not available"));
				}
			}
			if(hiddentype == RS_HIDDEN_TYPE_I2P)
			{
				statusTor->setText("<strong>" + tr("I2P") + ":</strong>");
				rsPeers->getProxyServer(RS_HIDDEN_TYPE_I2P, proxyaddr, proxyport, status);
				socket.connectToHost(QString::fromStdString(proxyaddr),quint16(proxyport));
				if(socket.waitForConnected(500))
				{
					socket.disconnectFromHost();
					torstatusLabel->setPixmap(FilesDefs::getPixmapFromQtResourcePath(":/icons/i2p-128.png").scaledToHeight(S,Qt::SmoothTransformation));
					torstatusLabel->setToolTip( text + tr("i2p proxy is OK"));
				}
				else
				{
					torstatusLabel->setPixmap(FilesDefs::getPixmapFromQtResourcePath(":/icons/tile_downloading_48.png").scaledToHeight(S,Qt::SmoothTransformation));
					torstatusLabel->setToolTip( text + tr("i2p proxy is not available"));
				}
			}
			_updated = true;
		}
	}
}
