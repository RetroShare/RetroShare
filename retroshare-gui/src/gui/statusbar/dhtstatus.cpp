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

#include <QtGui>
#include <QString>

#include <QLayout>
#include <QLabel>
#include <QIcon>
#include <QPainter>
#include <QPixmap>

#include "rsiface/rsiface.h"
#include "rsiface/rspeers.h"

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
    
    iconLabel = new QLabel( this );
    iconLabel->setPixmap(QPixmap::QPixmap(":/images/grayled.png"));
    // iconLabel doesn't change over time, so we didn't need a minimum size
    hbox->addWidget(iconLabel);
    
    setLayout( hbox );
    



}

DHTStatus::~DHTStatus()
{
}

void DHTStatus::getDHTStatus()
{

    rsiface->lockData(); /* Lock Interface */

    /* now the extra bit .... switch on check boxes */
    const RsConfig &config = rsiface->getConfig();


    if (config.netDhtOk)
    {
    iconLabel->setPixmap(QPixmap::QPixmap(":/images/greenled.png"));
    iconLabel->setToolTip(tr("DHT OK"));
		}
		else
		{
    iconLabel->setPixmap(QPixmap::QPixmap(":/images/redled.png"));
    iconLabel->setToolTip(tr("DHT DOWN"));
		}
		
		rsiface->unlockData(); /* UnLock Interface */

}


