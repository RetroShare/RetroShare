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

#include <iostream>
#include <sstream>

#include "rsiface/rsiface.h"
#include "rsiface/rspeers.h"

#include <QTimer>

ServerPage::ServerPage(QWidget * parent, Qt::WFlags flags)
    : ConfigPage(parent, flags)
{
  /* Invoke the Qt Designer generated object setup routine */
  ui.setupUi(this);

  connect( ui.netModeComboBox, SIGNAL( activated ( int ) ), this, SLOT( toggleUPnP( ) ) );
  connect( ui.allowIpDeterminationCB, SIGNAL( toggled( bool ) ), this, SLOT( toggleIpDetermination(bool) ) );
  connect( ui.allowTunnelConnectionCB, SIGNAL( toggled( bool ) ), this, SLOT( toggleTunnelConnection(bool) ) );
  connect( ui._showTurtleDialogPB,SIGNAL(clicked()),this,SLOT( showTurtleRouterDialog() )) ;

	  ui._enableTurtleCB->setChecked(true) ;
	  ui._enableTurtleCB->setEnabled(false) ;

   QTimer *timer = new QTimer(this);
   timer->connect(timer, SIGNAL(timeout()), this, SLOT(updateStatus()));
   timer->start(1000);

	//load();
	updateStatus();

	bool b = rsPeers->getAllowServerIPDetermination() ;
	ui.allowIpDeterminationCB->setChecked(b) ;
	ui.IPServersLV->setEnabled(b) ;

        b = rsPeers->getAllowTunnelConnection() ;
        ui.allowTunnelConnectionCB->setChecked(b) ;

	std::list<std::string> ip_servers ;
	rsPeers->getIPServersList(ip_servers) ;

	for(std::list<std::string>::const_iterator it(ip_servers.begin());it!=ip_servers.end();++it)
		ui.IPServersLV->addItem(QString::fromStdString(*it)) ;

  /* Hide platform specific features */
#ifdef Q_WS_WIN

#endif
}

void ServerPage::showTurtleRouterDialog()
{
	TurtleRouterDialog::showUp() ;
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

void
ServerPage::closeEvent (QCloseEvent * event)
{
    QWidget::closeEvent(event);
}


/** Saves the changes on this page */
bool
ServerPage::save(QString &errmsg)
{

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
	switch(detail.tryNetMode)
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

	/* set dht/disc */
	netIndex = 1;
	if (detail.visState & RS_VS_DHT_ON)
	{
		netIndex = 0;
	}
	ui.dhtComboBox->setCurrentIndex(netIndex);

	netIndex = 1;
	if (detail.visState & RS_VS_DISC_ON)
	{
		netIndex = 0;
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
}

/** Loads the settings for this page */
void ServerPage::updateStatus()
{

	/* load up configuration from rsPeers */
	RsPeerDetails detail;
	if (!rsPeers->getPeerDetails(rsPeers->getOwnId(), detail))
	{
		return;
	}

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

#if 0
	/* set status */
	std::ostringstream out;
	out << "Attempted Network Mode: ";
	switch(detail.tryNetMode)
	{
		case RS_NETMODE_EXT:
			out << "External Forwarded Port (UltraPEER Mode)";
			break;
		case RS_NETMODE_UDP:
			out << "Firewalled";
			break;
		default:
		case RS_NETMODE_UPNP:

			out << "Automatic: UPnP Forwarded Port";
			break;
	}
	out << std::endl;
	out << "Actual Network Mode: ";
	switch(detail.netMode)
	{
		case RS_NETMODE_EXT:
			out << "External Forwarded Port (UltraPEER Mode)";
			break;
		case RS_NETMODE_UDP:
			out << "Firewalled";
			break;
		case RS_NETMODE_UPNP:
			out << "Automatic: UPnP Forwarded Port";
			break;
		default:
		case RS_NETMODE_UNREACHABLE:
			out << "Unreachable: Firewalled by Symmetric NAT";
			break;
	}
	out << std::endl;
	out << "\tLocal Address: " << detail.localAddr;
	out << ":" << detail.localPort;
	out << std::endl;
	out << "\tExternal Address: " << detail.extAddr;
	out << ":" << detail.extPort;
	out << std::endl;

	out << "UPnP Status: ";
	out << std::endl;

	out << "DHT Status: ";
	if (detail.visState & RS_VS_DHT_ON)
		out << " Enabled";
	else
		out << " Disabled";
	out << std::endl;

	out << "Discovery Status: ";
	if (detail.visState & RS_VS_DISC_ON)
		out << " Enabled";
	else
		out << " Disabled";
	out << std::endl;


	if (detail.netMode == RS_NETMODE_UNREACHABLE)
	{
		ui.netStatusBox->setTextColor( Qt::red );
	}
	else
	{
		ui.netStatusBox->setTextColor( Qt::black );
	}

	ui.netStatusBox->setText(QString::fromStdString(out.str()));
	ui.netStatusBox ->setReadOnly(true);
#endif




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
		ui.dhtComboBox->setEnabled(true);
		// disabled until we've got it all working.
		//ui.discComboBox->setEnabled(true);
		ui.discComboBox->setEnabled(false);

		ui.localAddress->setEnabled(false);
		ui.localPort  -> setEnabled(true);
		ui.extAddress -> setEnabled(false);
		ui.extPort    -> setEnabled(true);
	}
	else
	{
		ui.dhtComboBox->setEnabled(false);
		ui.discComboBox->setEnabled(false);

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
	{
		return;
	}

	int netIndex = ui.netModeComboBox->currentIndex();

	/* Check if netMode has changed */
	int netMode = 0;
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

	if (detail.tryNetMode != netMode)
	{
		rsPeers->setNetworkMode(ownId, netMode);
	}

	int visState = 0;
	/* Check if vis has changed */
	if (0 == ui.discComboBox->currentIndex())
	{
		visState |= RS_VS_DISC_ON;
	}

	if (0 == ui.dhtComboBox->currentIndex())
	{
		visState |= RS_VS_DHT_ON;
	}

	if (visState != detail.visState)
	{
		rsPeers->setVisState(ownId, visState);
	}

	if (0 != netIndex)
	{
		saveAddr = true;
	}

	if (saveAddr)
	{
	  rsPeers->setLocalAddress(rsPeers->getOwnId(), ui.localAddress->text().toStdString(), ui.localPort->value());
	  rsPeers->setExtAddress(rsPeers->getOwnId(), ui.extAddress->text().toStdString(), ui.extPort->value());
	}

	rsicontrol->ConfigSetDataRates( ui.totalDownloadRate->value(), ui.totalUploadRate->value() );
	load();
}

