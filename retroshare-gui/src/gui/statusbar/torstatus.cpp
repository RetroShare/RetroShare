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
#include "util/misc.h"

#include "TorControl/TorManager.h"
#include "TorControl/TorControl.h"
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
		int tor_control_status = Tor::TorManager::instance()->control()->status();
		int torstatus = Tor::TorManager::instance()->control()->torStatus();

		QString tor_control_status_str,torstatus_str ;
		bool tor_control_ok ;

		switch(tor_control_status)
		{
		default:
		case Tor::TorControl::Error :			tor_control_ok = false ; tor_control_status_str = "Error" ; break ;
		case Tor::TorControl::NotConnected:		tor_control_ok = false ; tor_control_status_str = "Not connected" ; break ;
		case Tor::TorControl::Connecting:		tor_control_ok = false ; tor_control_status_str = "Connecting" ; break ;
		case Tor::TorControl::Authenticating:	tor_control_ok = false ; tor_control_status_str = "Authenticating" ; break ;
		case Tor::TorControl::Connected:		tor_control_ok = true  ; tor_control_status_str = "Connected" ; break ;
		}

		switch(torstatus)
		{
		default:
		case Tor::TorControl::TorUnknown: 	torstatus_str = "Unknown" ; break ;
		case Tor::TorControl::TorOffline: 	torstatus_str = "Tor offline" ; break ;
		case Tor::TorControl::TorReady: 	torstatus_str = "Tor ready" ; break ;
		}

#define MIN_RS_NET_SIZE		10

		if(torstatus == Tor::TorControl::TorOffline || !online || !tor_control_ok)
		{
			// RED - some issue.
            torstatusLabel->setPixmap(FilesDefs::getPixmapFromQtResourcePath(":/icons/tor-stopping.png").scaledToHeight(1.5*S,Qt::SmoothTransformation));
			torstatusLabel->setToolTip( text + tr("Tor is currently offline"));
		}
		else if(torstatus == Tor::TorControl::TorReady && online && tor_control_ok)
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
        torstatusLabel->setPixmap(FilesDefs::getPixmapFromQtResourcePath(":/icons/tor-stopping.png").scaledToHeight(S,Qt::SmoothTransformation));
		torstatusLabel->setToolTip( text + tr("Tor is currently offline"));
	}
}
