/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006, crypton
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

#include "ServerPage.h"
#include <gui/TurtleRouterDialog.h>

#include "rshare.h"
#include "rsharesettings.h"

#include <iostream>

#include <retroshare/rsiface.h>
#include <retroshare/rspeers.h>
#include <retroshare/rsturtle.h>

#include <QTimer>

ServerPage::ServerPage(QWidget * parent, Qt::WFlags flags)
    : ConfigPage(parent, flags)
{
  /* Invoke the Qt Designer generated object setup routine */
  ui.setupUi(this);

  connect( ui.netModeComboBox, SIGNAL( activated ( int ) ), this, SLOT( toggleUPnP( ) ) );
  connect( ui.allowIpDeterminationCB, SIGNAL( toggled( bool ) ), this, SLOT( toggleIpDetermination(bool) ) );
  connect( ui.allowTunnelConnectionCB, SIGNAL( toggled( bool ) ), this, SLOT( toggleTunnelConnection(bool) ) );
  connect( ui._max_tr_up_per_sec_SB, SIGNAL( valueChanged( int ) ), this, SLOT( updateMaxTRUpRate(int) ) );
  connect( ui._turtle_enabled_CB, SIGNAL( toggled( bool ) ), this, SLOT( toggleTurtleRouting(bool) ) );

   QTimer *timer = new QTimer(this);
   timer->connect(timer, SIGNAL(timeout()), this, SLOT(updateStatus()));
   timer->start(1000);

	//load();
	updateStatus();

	bool b = rsPeers->getAllowServerIPDetermination() ;
	ui.allowIpDeterminationCB->setChecked(b) ;
	ui.IPServersLV->setEnabled(b) ;

#ifdef RS_RELEASE_VERSION
    ui.allowTunnelConnectionCB->hide();
    this->toggleTunnelConnection(false);
#else
    b = rsPeers->getAllowTunnelConnection() ;
    ui.allowTunnelConnectionCB->setChecked(b) ;
#endif

	std::list<std::string> ip_servers ;
	rsPeers->getIPServersList(ip_servers) ;

	for(std::list<std::string>::const_iterator it(ip_servers.begin());it!=ip_servers.end();++it)
		ui.IPServersLV->addItem(QString::fromStdString(*it)) ;

  /* Hide platform specific features */
#ifdef Q_WS_WIN

#endif
}

void ServerPage::updateMaxTRUpRate(int b)
{
	rsTurtle->setMaxTRForwardRate(b) ;
}

void ServerPage::toggleIpDetermination(bool b)
{
	rsPeers->allowServerIPDetermination(b) ;
	ui.IPServersLV->setEnabled(b) ;
}

void ServerPage::toggleTunnelConnection(bool b)
{
        std::cerr << "ServerPage::toggleTunnelConnection() set tunnel to : " << b << std::endl;
        rsPeers->allowTunnelConnection(b) ;
}

/** Saves the changes on this page */
bool
ServerPage::save(QString &/*errmsg*/)
{
    Settings->setStatusBarFlag(STATUSBAR_DISC, ui.showDiscStatusBar->isChecked());

/* save the server address */
/* save local address */
/* save the url for DNS access */

/* restart server */

/* save all? */
   saveAddresses();
 return true;
}

/** Loads the settings for this page */
void ServerPage::load()
{

	/* load up configuration from rsPeers */
	RsPeerDetails detail;
	if (!rsPeers->getPeerDetails(rsPeers->getOwnId(), detail))
	{
		return;
	}

	/* set net mode */
	int netIndex = 0;
	switch(detail.netMode)
	{
		case RS_NETMODE_EXT:
			netIndex = 2;
			break;
		case RS_NETMODE_UDP:
			netIndex = 1;
			break;
		default:
		case RS_NETMODE_UPNP:
			netIndex = 0;
			break;
	}
	ui.netModeComboBox->setCurrentIndex(netIndex);

	/* DHT + Discovery: (public)
	 * Discovery only:  (private)
	 * DHT only: (inverted)
	 * None: (dark net)
	 */

	netIndex = 3; // NONE.
	if (detail.visState & RS_VS_DHT_ON)
	{
		if (detail.visState & RS_VS_DISC_ON)
		{
			netIndex = 0; // PUBLIC
		}
		else
		{
			netIndex = 2; // INVERTED
		}
	}
	else
	{
		if (detail.visState & RS_VS_DISC_ON)
		{
			netIndex = 1; // PRIVATE
		}
		else
		{
			netIndex = 3; // NONE
		}
	}

	ui.discComboBox->setCurrentIndex(netIndex);

	rsiface->lockData(); /* Lock Interface */

	ui.totalDownloadRate->setValue(rsiface->getConfig().maxDownloadDataRate);
	ui.totalUploadRate->setValue(rsiface->getConfig().maxUploadDataRate);

	rsiface->unlockData(); /* UnLock Interface */


	toggleUPnP();


	/* Addresses must be set here - otherwise can't edit it */
		/* set local address */
	ui.localAddress->setText(QString::fromStdString(detail.localAddr));
	ui.localPort -> setValue(detail.localPort);
		/* set the server address */
	ui.extAddress->setText(QString::fromStdString(detail.extAddr));
	ui.extPort -> setValue(detail.extPort);
	/* set DynDNS */
	ui.dynDNS -> setText(QString::fromStdString(detail.dyndns));

	ui.showDiscStatusBar->setChecked(Settings->getStatusBarFlags() & STATUSBAR_DISC);

	ui._max_tr_up_per_sec_SB->setValue(rsTurtle->getMaxTRForwardRate()) ;

	ui._turtle_enabled_CB->setChecked(rsTurtle->enabled()) ;
}

void ServerPage::toggleTurtleRouting(bool b)
{
	rsTurtle->setEnabled(b) ;
}

/** Loads the settings for this page */
void ServerPage::updateStatus()
{
	if(!isVisible())
		return ;

	/* load up configuration from rsPeers */
	RsPeerDetails detail;
	if (!rsPeers->getPeerDetails(rsPeers->getOwnId(), detail))
		return;

	/* only update if can't edit */
	if (!ui.localPort->isEnabled())
	{
		/* set local address */
		ui.localPort -> setValue(detail.localPort);
		ui.extPort -> setValue(detail.extPort);
	}

	/* set local address */
	ui.localAddress->setText(QString::fromStdString(detail.localAddr));
	/* set the server address */
	ui.extAddress->setText(QString::fromStdString(detail.extAddr));
}

void ServerPage::toggleUPnP()
{
	/* switch on the radioButton */
	bool settingChangeable = false;
	if (0 != ui.netModeComboBox->currentIndex())
	{
		settingChangeable = true;
	}

	if (settingChangeable)
	{
		ui.localAddress->setEnabled(false);
		ui.localPort  -> setEnabled(true);
		ui.extAddress -> setEnabled(false);
		ui.extPort    -> setEnabled(true);
	}
	else
	{
		ui.localAddress->setEnabled(false);
		ui.localPort  -> setEnabled(false);
		ui.extAddress -> setEnabled(false);
		ui.extPort    -> setEnabled(false);
	}
}

void ServerPage::saveAddresses()
{
	QString str;

	bool saveAddr = false;

	RsPeerDetails detail;
	std::string ownId = rsPeers->getOwnId();

	if (!rsPeers->getPeerDetails(ownId, detail))
		return;

	int netIndex = ui.netModeComboBox->currentIndex();

	/* Check if netMode has changed */
	uint32_t netMode = 0;
	switch(netIndex)
	{
		case 2:
			netMode = RS_NETMODE_EXT;
			break;
		case 1:
			netMode = RS_NETMODE_UDP;
			break;
		default:
		case 0:
			netMode = RS_NETMODE_UPNP;
			break;
	}

	if (detail.netMode != netMode)
		rsPeers->setNetworkMode(ownId, netMode);

	uint32_t visState = 0;
	/* Check if vis has changed */
	switch(ui.discComboBox->currentIndex())
	{
		case 0:
			visState |= (RS_VS_DISC_ON | RS_VS_DHT_ON);
			break;
		case 1:
			visState |= RS_VS_DISC_ON;
			break;
		case 2:
			visState |= RS_VS_DHT_ON;
			break;
		case 3:
		default:
			break;
	}

	if (visState != detail.visState)
		rsPeers->setVisState(ownId, visState);

	if (0 != netIndex)
		saveAddr = true;

	if (saveAddr)
	{
		rsPeers->setLocalAddress(ownId, ui.localAddress->text().toStdString(), ui.localPort->value());
		rsPeers->setExtAddress(ownId, ui.extAddress->text().toStdString(), ui.extPort->value());
	}

	rsPeers->setDynDNS(ownId, ui.dynDNS->text().toStdString());

	rsicontrol->ConfigSetDataRates( ui.totalDownloadRate->value(), ui.totalUploadRate->value() );

	load();
}

