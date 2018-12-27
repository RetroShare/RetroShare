/*******************************************************************************
 * gui/statusbar/dhtstatus.cpp                                                 *
 *                                                                             *
 * Copyright (c) 2009 Retroshare Team <retroshare.project@gmail.com>           *
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
	 statusDHT->setToolTip(tr("<p>Retroshare uses Bittorrent's DHT as a proxy for connexions. It does not \"store\" your IP in the DHT. \
				                        Instead the DHT is used by your trusted nodes to reach you while processing standard DHT requests. \
												The status bullet will turn green as soon as Retroshare gets a DHT response from one of your trusted nodes.</p>")) ;
    hbox->addWidget(statusDHT);
    
    dhtstatusLabel = new QLabel( this );
    dhtstatusLabel->setPixmap(QPixmap(":/icons/bullet_grey_128.png"));
    hbox->addWidget(dhtstatusLabel);
    
    spaceLabel = new QLabel( "|", this );
    spaceLabel->setVisible(false);
    hbox->addWidget(spaceLabel);

    dhtnetworkLabel = new QLabel( this );
    dhtnetworkLabel->setVisible(false);
    int S = QFontMetricsF(dhtnetworkLabel->font()).height();
    dhtnetworkLabel->setPixmap(QPixmap(":/images/dht32.png").scaledToHeight(S,Qt::SmoothTransformation));
    hbox->addWidget(dhtnetworkLabel);

    dhtnetworksizeLabel = new QLabel( "0 (0) ",this );
    dhtnetworksizeLabel->setVisible(false);
    dhtnetworksizeLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    hbox->addWidget(dhtnetworksizeLabel);

    hbox->addSpacing(2);

    _compactMode = false;

    setLayout( hbox );
}

void DHTStatus::getDHTStatus()
{
	statusDHT->setVisible(!_compactMode);
	QString text = _compactMode?statusDHT->text():"";

	/* now the extra bit .... switch on check boxes */
	RsConfigNetStatus config;
	rsConfig->getConfigNetStatus(config);

    int S = QFontMetricsF(dhtstatusLabel->font()).height();
	if (!(config.DHTActive))
	{
		// GRAY.
        dhtstatusLabel->setPixmap(QPixmap(":/icons/bullet_grey_128.png").scaledToHeight(S,Qt::SmoothTransformation));
		dhtstatusLabel->setToolTip( text + tr("DHT Off"));

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
                dhtstatusLabel->setPixmap(QPixmap(":/icons/bullet_yellow_128.png").scaledToHeight(S,Qt::SmoothTransformation));
                dhtstatusLabel->setToolTip( text + tr("DHT Searching for RetroShare Peers"));

				spaceLabel->setVisible(true);
				dhtnetworkLabel->setVisible(true);
				dhtnetworksizeLabel->setVisible(true);

				dhtnetworksizeLabel->setText(QString("%1 (%2)").arg(misc::userFriendlyUnit(config.netDhtRsNetSize, 1)).arg(misc::userFriendlyUnit(config.netDhtNetSize, 1)));
				dhtnetworksizeLabel->setToolTip( text + tr("RetroShare users in DHT (Total DHT users)") );
			}
			else
			{
                dhtstatusLabel->setPixmap(QPixmap(":/icons/bullet_green_128.png").scaledToHeight(S,Qt::SmoothTransformation));
				dhtstatusLabel->setToolTip( text + tr("DHT Good"));

				spaceLabel->setVisible(true);
				dhtnetworkLabel->setVisible(true);
				dhtnetworksizeLabel->setVisible(true);

				dhtnetworksizeLabel->setText(QString("%1 (%2)").arg(misc::userFriendlyUnit(config.netDhtRsNetSize, 1)).arg(misc::userFriendlyUnit(config.netDhtNetSize, 1)));
				dhtnetworksizeLabel->setToolTip( text + tr("RetroShare users in DHT (Total DHT users)") );
			}
		}
		else
		{
			// RED - some issue.
                dhtstatusLabel->setPixmap(QPixmap(":/icons/bullet_red_128.png").scaledToHeight(S,Qt::SmoothTransformation));
            dhtstatusLabel->setToolTip( text + tr("No peer found in DHT"));

			spaceLabel->setVisible(false);
			dhtnetworkLabel->setVisible(false);
			dhtnetworksizeLabel->setVisible(false);
			
			dhtnetworksizeLabel->setText("");
			dhtnetworksizeLabel->setToolTip("");
		}
	}
}
