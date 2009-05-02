/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2008 RetroShare Team
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
#include "peerstatus.h"

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

PeerStatus::PeerStatus(QWidget *parent)
 : QWidget(parent)
{
    QHBoxLayout *hbox = new QHBoxLayout();
    hbox->setMargin(0);
    hbox->setSpacing(6);
    
    iconLabel = new QLabel( this );
    iconLabel->setPixmap(QPixmap::QPixmap(":/images/user/identitygray16.png"));
    // iconLabel doesn't change over time, so we didn't need a minimum size
    hbox->addWidget(iconLabel);
    
    statusPeers = new QLabel( tr("Online: 0  | Friends: 0  | Network: 0 "), this );
	  //statusPeers->setMinimumSize( statusPeers->frameSize().width() + 0, 0 );
    hbox->addWidget(statusPeers);
    
    setLayout( hbox );

}

PeerStatus::~PeerStatus()
{
}

void PeerStatus::getPeerStatus()
{
    /* set users/friends/network */

    std::list<std::string> ids;
    rsPeers->getOnlineList(ids);
    int online = ids.size();

    ids.clear();
    rsPeers->getFriendList(ids);
    int friends = ids.size();

    ids.clear();
    rsPeers->getOthersList(ids);
    int others = 1 + ids.size();

    std::ostringstream out2;
    out2 << "<span style=\"color:#008000\"><strong>Online: </strong></span>" << online << " | <span style=\"color:#0000FF\"><strong>Friends: </strong></span>" << friends << " | <strong>Network: </strong>" << others << " ";


    if (statusPeers)
          statusPeers -> setText(QString::fromStdString(out2.str()));
    		
    if (online > 0) 
    {
        iconLabel->setPixmap(QPixmap::QPixmap(":/images/user/identity16.png"));
    }
    else
    {
        iconLabel->setPixmap(QPixmap::QPixmap(":/images/user/identitygray16.png"));
    }
   		

}


