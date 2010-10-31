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

#include "retroshare/rsiface.h"
#include "retroshare/rspeers.h"

#include <sstream>
#include <iomanip>

DHTStatus::DHTStatus(QWidget *parent)
 : QWidget(parent)
{
    QHBoxLayout *hbox = new QHBoxLayout();
    hbox->setMargin(0);
    hbox->setSpacing(6);
       
    statusDHT = new QLabel( tr("<strong>DHT:</strong>"), this );
	  //statusDHT->setMinimumSize( statusPeers->frameSize().width() + 0, 0 );
    hbox->addWidget(statusDHT);
    
    dhtstatusLabel = new QLabel( this );
    dhtstatusLabel->setPixmap(QPixmap(":/images/grayled.png"));
    hbox->addWidget(dhtstatusLabel);
    
    spaceLabel = new QLabel( " | ", this );
    hbox->addWidget(spaceLabel);
    
    dhtnetworkLabel = new QLabel( this );
    dhtnetworkLabel->setPixmap(QPixmap(":/images/dht16.png"));
    hbox->addWidget(dhtnetworkLabel);
    
    dhtnetworksizeLabel = new QLabel( "0 (0) ",this );
    hbox->addWidget(dhtnetworksizeLabel);
    
    setLayout( hbox );
    



}

void DHTStatus::getDHTStatus()
{

    rsiface->lockData(); /* Lock Interface */

    /* now the extra bit .... switch on check boxes */
    const RsConfig &config = rsiface->getConfig();


    if (config.netDhtOk)
    {
        dhtstatusLabel->setPixmap(QPixmap(":/images/greenled.png"));
        dhtstatusLabel->setToolTip(tr("DHT On"));
    }
    else
    {
        dhtstatusLabel->setPixmap(QPixmap(":/images/redled.png"));
        dhtstatusLabel->setToolTip(tr("DHT Off"));
    }

    QString dhtsize;
    {
	std::ostringstream out;
	out << (int) config.netDhtRsNetSize << " ( " << (int) config.netDhtNetSize << " )";
	dhtsize = QString::fromStdString(out.str());
    }

    dhtnetworksizeLabel->setText(dhtsize);
		
    rsiface->unlockData(); /* UnLock Interface */

}


