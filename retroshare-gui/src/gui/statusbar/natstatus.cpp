/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2009 RetroShare Team
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

#include <QHBoxLayout>
#include <QLabel>

#include "natstatus.h"

#include "retroshare/rsiface.h"
#include "retroshare/rsconfig.h"

NATStatus::NATStatus(QWidget *parent)
 : QWidget(parent)
{
    QHBoxLayout *hbox = new QHBoxLayout(this);
    hbox->setMargin(0);
    hbox->setSpacing(6);
       
    statusNAT = new QLabel( tr("<strong>NAT:</strong>"), this );
//    statusDHT->setMinimumSize( statusPeers->frameSize().width() + 0, 0 );
    hbox->addWidget(statusNAT);
    
    iconLabel = new QLabel(this);
    iconLabel->setPixmap(QPixmap(":/images/grayled.png"));
    // iconLabel doesn't change over time, so we didn't need a minimum size
    hbox->addWidget(iconLabel);

    hbox->addSpacing(2);

    setLayout(hbox);
}

void NATStatus::getNATStatus()
{
	uint32_t netState = rsConfig -> getNetState();

	switch(netState)
	{
		default:
		case RSNET_NETSTATE_BAD_UNKNOWN:
		{
        		iconLabel->setPixmap(QPixmap(":/images/yellowled.png"));
        		iconLabel->setToolTip(tr("Network Status Unknown"));
		}
			break;

		case RSNET_NETSTATE_BAD_OFFLINE:
		{
        		iconLabel->setPixmap(QPixmap(":/images/grayled.png"));
        		iconLabel->setToolTip(tr("Offline"));
		}
			break;

// BAD. (RED)
		case RSNET_NETSTATE_BAD_NATSYM:
		{
        		iconLabel->setPixmap(QPixmap(":/images/redled.png"));
        		iconLabel->setToolTip(tr("Nasty Firewall"));
		}
			break;

		case RSNET_NETSTATE_BAD_NODHT_NAT:
		{
        		iconLabel->setPixmap(QPixmap(":/images/redled.png"));
        		iconLabel->setToolTip(tr("DHT Disabled and Firewalled"));
		}
			break;


// CAUTION. (ORANGE)
		case RSNET_NETSTATE_WARNING_RESTART:
		{
        		iconLabel->setPixmap(QPixmap(":/images/yellowled.png"));
        		iconLabel->setToolTip(tr("Network Restarting"));
		}
			break;

		case RSNET_NETSTATE_WARNING_NATTED:
		{
        		iconLabel->setPixmap(QPixmap(":/images/yellowled.png"));
        		iconLabel->setToolTip(tr("Behind Firewall"));
		}
			break;

		case RSNET_NETSTATE_WARNING_NODHT:
		{
        		iconLabel->setPixmap(QPixmap(":/images/yellowled.png"));
        		iconLabel->setToolTip(tr("DHT Disabled"));
		}
			break;

// GOOD (GREEN)
		case RSNET_NETSTATE_GOOD:
		{
        		iconLabel->setPixmap(QPixmap(":/images/greenled.png"));
        		iconLabel->setToolTip(tr("OK | RetroShare Server"));
		}
			break;
	}
}

#if 0

void NATStatus::getNATStatus()
{
	uint32_t netMode = rsConfig -> getNetworkMode();
	uint32_t natType = rsConfig -> getNatTypeMode();
	uint32_t natHole = rsConfig -> getNatHoleMode();

    rsiface->lockData(); /* Lock Interface */

    /* now the extra bit .... switch on check boxes */
    const RsConfig &config = rsiface->getConfig();
    
//    if(config.netUpnpOk)
//    {
//      iconLabel->setPixmap(QPixmap::QPixmap(":/images/greenled.png"));
//      iconLabel->setToolTip(tr("UPNP is active."));
//    }
//    else
//    {
//      iconLabel->setPixmap(QPixmap::QPixmap(":/images/yellowled.png"));
//      iconLabel->setToolTip(tr("UPNP NOT FOUND."));
//    }
//
//    if(config.netExtOk)
//    {
//      iconLabel->setPixmap(QPixmap::QPixmap(":/images/greenled.png"));
//      iconLabel->setToolTip(tr("Stable External IP Address"));
//    }
//    else
//    {
//      iconLabel->setPixmap(QPixmap::QPixmap(":/images/yellowled.png"));
//      iconLabel->setToolTip(tr("Not Found External Address"));
//    }
//
//    if(config.netUdpOk)
//    {
//      iconLabel->setPixmap(QPixmap::QPixmap(":/images/yellowled.png"));
//      iconLabel->setToolTip(tr("UDP Port is reachable"));
//    }
//    else
//    {
//      iconLabel->setPixmap(QPixmap::QPixmap(":/images/grayled.png"));
//      iconLabel->setToolTip(tr("UDP Port is not reachable"));
//    }
    
    if (config.netUpnpOk)
    {
        iconLabel->setPixmap(QPixmap(":/images/greenled.png"));
        iconLabel->setToolTip(tr("OK | RetroShare Server"));
    }
    else if (config.netStunOk || config.netExtraAddressOk)
    {
        iconLabel->setPixmap(QPixmap(":/images/greenled.png"));
        iconLabel->setToolTip(tr("Internet connection"));
    }
    else if (config.netLocalOk)
    {
        iconLabel->setPixmap(QPixmap(":/images/grayled.png"));
        iconLabel->setToolTip(tr("No internet connection"));
    }
    else
    {
        iconLabel->setPixmap(QPixmap(":/images/redled.png"));
        iconLabel->setToolTip(tr("No local network"));
    }

    rsiface->unlockData(); /* UnLock Interface */

}

#endif
