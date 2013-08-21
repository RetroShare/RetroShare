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
#include "dhtstatus.h"

#include <QLayout>
#include <QLabel>
#include <QIcon>
#include <QPixmap>

#include "retroshare/rsconfig.h"
#include "retroshare/rspeers.h"

#include "util/misc.h"

#include <iomanip>

DHTStatus::DHTStatus(QWidget *parent)
 : QWidget(parent)
{
    QHBoxLayout *hbox = new QHBoxLayout();
    hbox->setMargin(0);
    hbox->setSpacing(6);
       
    statusDHT = new QLabel("<strong>" + tr("DHT") + ":</strong>", this );
    hbox->addWidget(statusDHT);
    
    dhtstatusLabel = new QLabel( this );
    dhtstatusLabel->setPixmap(QPixmap(":/images/grayled.png"));
    hbox->addWidget(dhtstatusLabel);
    
    spaceLabel = new QLabel( "|", this );
    spaceLabel->setVisible(false);
    hbox->addWidget(spaceLabel);

    dhtnetworkLabel = new QLabel( this );
    dhtnetworkLabel->setVisible(false);
    dhtnetworkLabel->setPixmap(QPixmap(":/images/dht16.png"));
    hbox->addWidget(dhtnetworkLabel);

    dhtnetworksizeLabel = new QLabel( "0 (0) ",this );
    dhtnetworksizeLabel->setVisible(false);
    hbox->addWidget(dhtnetworksizeLabel);

    hbox->addSpacing(2);

    setLayout( hbox );
}

void DHTStatus::getDHTStatus()
{

	/* now the extra bit .... switch on check boxes */
	RsConfigNetStatus config;
	rsConfig->getConfigNetStatus(config);

	if (!(config.DHTActive))
	{
		// GRAY.
		dhtstatusLabel->setPixmap(QPixmap(":/images/grayled.png"));
		dhtstatusLabel->setToolTip(tr("DHT Off"));
		
		spaceLabel->setVisible(false);
		dhtnetworkLabel->setVisible(false);
		dhtnetworksizeLabel->setVisible(false);
		
		dhtnetworksizeLabel->setText("");
		dhtnetworksizeLabel->setToolTip("");
	}
	else
	{
		if (config.netDhtOk)
		{
#define MIN_RS_NET_SIZE		10
			// YELLOW or GREEN.
			if (config.netDhtRsNetSize < MIN_RS_NET_SIZE)
			{
				dhtstatusLabel->setPixmap(QPixmap(":/images/yellowled.png"));
				dhtstatusLabel->setToolTip(tr("DHT Searching for RetroShare Peers"));
				
				spaceLabel->setVisible(true);
				dhtnetworkLabel->setVisible(true);
				dhtnetworksizeLabel->setVisible(true);
				
				dhtnetworksizeLabel->setText(QString("%1 (%2)").arg(misc::userFriendlyUnit(config.netDhtRsNetSize, 1)).arg(misc::userFriendlyUnit(config.netDhtNetSize, 1)));
				dhtnetworksizeLabel->setToolTip(tr("RetroShare users in DHT (Total DHT users)") );
			}
			else
			{
				dhtstatusLabel->setPixmap(QPixmap(":/images/greenled.png"));
				dhtstatusLabel->setToolTip(tr("DHT Good"));
				
				spaceLabel->setVisible(true);
				dhtnetworkLabel->setVisible(true);
				dhtnetworksizeLabel->setVisible(true);
				
				dhtnetworksizeLabel->setText(QString("%1 (%2)").arg(misc::userFriendlyUnit(config.netDhtRsNetSize, 1)).arg(misc::userFriendlyUnit(config.netDhtNetSize, 1)));
				dhtnetworksizeLabel->setToolTip(tr("RetroShare users in DHT (Total DHT users)") );
			}
		}
		else
		{
			// RED - some issue.
			dhtstatusLabel->setPixmap(QPixmap(":/images/redled.png"));
			dhtstatusLabel->setToolTip(tr("DHT Error"));
			
			spaceLabel->setVisible(false);
			dhtnetworkLabel->setVisible(false);
			dhtnetworksizeLabel->setVisible(false);
			
			dhtnetworksizeLabel->setText("");
			dhtnetworksizeLabel->setToolTip("");
		}
	}
}
