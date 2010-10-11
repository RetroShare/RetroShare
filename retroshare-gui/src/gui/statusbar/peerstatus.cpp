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

#include <QHBoxLayout>
#include <QLabel>

#include "peerstatus.h"

#include <sstream>

PeerStatus::PeerStatus(QWidget *parent)
 : QWidget(parent)
{
    QHBoxLayout *hbox = new QHBoxLayout(this);
    hbox->setMargin(0);
    hbox->setSpacing(6);
    
    iconLabel = new QLabel( this );
    iconLabel->setPixmap(QPixmap(":/images/user/identitygray16.png"));
    // iconLabel doesn't change over time, so we didn't need a minimum size
    hbox->addWidget(iconLabel);
    
    statusPeers = new QLabel( tr("Online: 0  | Friends: 0  | Network: 0 "), this );
//    statusPeers->setMinimumSize( statusPeers->frameSize().width() + 0, 0 );
    hbox->addWidget(statusPeers);
    
    setLayout(hbox);

}

void PeerStatus::getPeerStatus(unsigned int nFriendCount, unsigned int nOnlineCount)
{
    /* set users/friends/network */

    std::ostringstream out;
    out << nFriendCount << " ";
    
    std::ostringstream out2;
    out2 << nOnlineCount << " ";


    if (statusPeers)
        statusPeers -> setText( "<strong>" + tr("Friends") + ":</strong> " + QString::fromStdString(out.str()) + " | <span style=\"color:#0000FF\"><strong>" + tr("Online") + ":</strong></span> " + QString::fromStdString(out2.str()) );
    		
    if (nOnlineCount > 0)
    {
        iconLabel->setPixmap(QPixmap(":/images/user/identity16.png"));
    }
    else
    {
        iconLabel->setPixmap(QPixmap(":/images/user/identitygray16.png"));
    }
   		

}


