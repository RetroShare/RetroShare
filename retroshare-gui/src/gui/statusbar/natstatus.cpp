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

    _compactMode = false;

    setLayout(hbox);
}

void NATStatus::getNATStatus()
{
	uint32_t netState = rsConfig -> getNetState();

	statusNAT->setVisible(!_compactMode);
	QString text = _compactMode?statusNAT->text():"";
    int S = QFontMetricsF(iconLabel->font()).height() ;

	switch(netState)
	{
		default:
		case RSNET_NETSTATE_BAD_UNKNOWN:
		{
            iconLabel->setPixmap(QPixmap(":/icons/bullet_yellow_64.png").scaledToHeight(S,Qt::SmoothTransformation)) ;
            iconLabel->setToolTip( text + tr("Network Status Unknown")) ;
		}
		break ;

		case RSNET_NETSTATE_BAD_OFFLINE:
		{
            iconLabel->setPixmap(QPixmap(":/icons/bullet_grey_64.png").scaledToHeight(S,Qt::SmoothTransformation)) ;
            iconLabel->setToolTip( text + tr("Offline")) ;
		}
		break ;

// BAD. (RED)
		case RSNET_NETSTATE_BAD_NATSYM:
		{
            iconLabel->setPixmap(QPixmap(":/icons/bullet_red_64.png").scaledToHeight(S,Qt::SmoothTransformation)) ;
            iconLabel->setToolTip( text + tr("Nasty Firewall")) ;
		}
		break ;

		case RSNET_NETSTATE_BAD_NODHT_NAT:
		{
            iconLabel->setPixmap(QPixmap(":/icons/bullet_red_64.png").scaledToHeight(S,Qt::SmoothTransformation)) ;
            iconLabel->setToolTip( text + tr("DHT Disabled and Firewalled")) ;
		}
		break ;

// CAUTION. (ORANGE)
		case RSNET_NETSTATE_WARNING_RESTART:
		{
            iconLabel->setPixmap(QPixmap(":/icons/bullet_yellow_64.png").scaledToHeight(S,Qt::SmoothTransformation)) ;
            iconLabel->setToolTip( text + tr("Network Restarting")) ;
		}
		break ;

		case RSNET_NETSTATE_WARNING_NATTED:
		{
            iconLabel->setPixmap(QPixmap(":/icons/bullet_yellow_64.png").scaledToHeight(S,Qt::SmoothTransformation)) ;
            iconLabel->setToolTip( text + tr("Behind Firewall")) ;
		}
		break ;

		case RSNET_NETSTATE_WARNING_NODHT:
		{
            iconLabel->setPixmap(QPixmap(":/icons/bullet_yellow_64.png").scaledToHeight(S,Qt::SmoothTransformation)) ;
			iconLabel->setToolTip( text + tr("DHT Disabled")) ;
		}
		break ;

// GOOD (GREEN)
		case RSNET_NETSTATE_GOOD:
		{
            iconLabel->setPixmap(QPixmap(":/icons/bullet_green_64.png").scaledToHeight(S,Qt::SmoothTransformation)) ;
			iconLabel->setToolTip( text + tr("RetroShare Server")) ;
		}
		break ;

		case RSNET_NETSTATE_ADV_FORWARD:
		{
            iconLabel->setPixmap(QPixmap(":/icons/bullet_green_64.png").scaledToHeight(S,Qt::SmoothTransformation)) ;
			iconLabel->setToolTip( text + tr("Forwarded Port")) ;
		}
		break ;
	}
}

