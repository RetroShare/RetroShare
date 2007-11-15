/****************************************************************
 *  RShare is distributed under the following license:
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


#include <rshare.h>
#include "ServerDialog.h"
#include <iostream>

#include "rsiface/rsiface.h"



/** Constructor */
ServerDialog::ServerDialog(QWidget *parent)
: ConfigPage(parent)
{
  /* Invoke the Qt Designer generated object setup routine */
  ui.setupUi(this);

 /* Create RshareSettings object */
  _settings = new RshareSettings();

  connect( ui.ManualButton, SIGNAL( toggled( bool ) ), this, SLOT( toggleUPnP( ) ) );
  connect( ui.UPnPButton, SIGNAL( toggled( bool ) ), this, SLOT( toggleUPnP( ) ) );


  /* Hide platform specific features */
#ifdef Q_WS_WIN

#endif
}

/** Saves the changes on this page */
bool
ServerDialog::save(QString &errmsg)
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
void
ServerDialog::load()
{
	/* get the shared directories */
	rsiface->lockData(); /* Lock Interface */

/* set local address */
	ui.localAddress->setText(QString::fromStdString(rsiface->getConfig().localAddr));
	ui.localPort -> setValue(rsiface->getConfig().localPort);
/* set the server address */
	ui.extAddress->setText(QString::fromStdString(rsiface->getConfig().extAddr));
	ui.extPort -> setValue(rsiface->getConfig().extPort);
/* set the flags */
	ui.chkFirewall  ->setChecked(rsiface->getConfig().firewalled);
	ui.chkForwarded ->setChecked(rsiface->getConfig().forwardPort);

	/* now handle networking options */
	if (rsiface->getConfig().DHTActive)
	{
		ui.DHTButton -> setChecked(true);
	}
	else
	{
		ui.noDHTButton -> setChecked(true);
	}

	int dhtPeers = rsiface->getConfig().DHTPeers;
	if (!dhtPeers)
	{
		ui.dhtStatus -> setText("DHT Off/Unavailable");
	}
	else if (dhtPeers < 20)
	{
		ui.dhtStatus -> setText("DHT Initialising");
	}
	else
	{
		ui.dhtStatus -> setText("DHT Active");
	}
		
	switch(rsiface->getConfig().uPnPState)
	{
		case UPNP_STATE_ACTIVE:
			ui.upnpStatus -> setText("Forwarding Active");
			break;
		case UPNP_STATE_FAILED_UDP:
			ui.upnpStatus -> setText("TCP Active/UDP Failed");
			break;
		case UPNP_STATE_FAILED_TCP:
			ui.upnpStatus -> setText("Forwarding Failed");
			break;
		case UPNP_STATE_READY:
			ui.upnpStatus -> setText("uPnP Ready");
			break;
		case UPNP_STATE_UNAVAILABILE:
			ui.upnpStatus -> setText("uPnP Unavailable");
			break;
		case UPNP_STATE_UNINITIALISED:
		default:
			ui.upnpStatus -> setText("uPnP Uninitialised");
			break;

	}
	ui.upnpStatus->setReadOnly(true);
	ui.dhtStatus ->setReadOnly(true);
      
	if (rsiface->getConfig().uPnPActive)
	{
		/* flag uPnP */
		ui.UPnPButton->setChecked(true);
		/* shouldn't fiddle with port */
	}
	else
	{
		/* noobie */
		ui.ManualButton->setChecked(true);

	}

	ui.totalRate->setValue(rsiface->getConfig().maxDataRate);
	ui.indivRate->setValue(rsiface->getConfig().maxIndivDataRate);

	rsiface->unlockData(); /* UnLock Interface */

	toggleUPnP();
}

void ServerDialog::toggleUPnP()
{
	/* switch on the radioButton */
	bool settingChangeable = false;
	if (ui.ManualButton->isChecked())
	{
		settingChangeable = true;
	}

	if (settingChangeable)
	{
		ui.localAddress->setEnabled(true);
		ui.localPort  -> setEnabled(true);
		ui.extAddress -> setEnabled(true);
		ui.extPort    -> setEnabled(true);
		ui.chkFirewall-> setEnabled(true);
		ui.chkForwarded->setEnabled(true);
	}
	else
	{
		ui.localAddress->setEnabled(false);
		ui.localPort  -> setEnabled(false);
		ui.extAddress -> setEnabled(false);
		ui.extPort    -> setEnabled(false);
		ui.chkFirewall-> setEnabled(false);
		ui.chkForwarded->setEnabled(false);
	}
}

void ServerDialog::saveAddresses()
{
	QString str;

	bool saveAddr = false;
	rsicontrol -> NetworkDHTActive(ui.DHTButton->isChecked());
	rsicontrol -> NetworkUPnPActive(ui.UPnPButton->isChecked());

	if (ui.ManualButton->isChecked())
	{
		saveAddr = true;
	}

	if (saveAddr)
	{
	  rsicontrol->ConfigSetLocalAddr(ui.localAddress->text().toStdString(), ui.localPort->value());
	  rsicontrol->ConfigSetLanConfig(ui.chkFirewall->isChecked(), ui.chkForwarded->isChecked());
	  rsicontrol->ConfigSetExtAddr(ui.extAddress->text().toStdString(), ui.extPort->value());
	}

	rsicontrol->ConfigSetDataRates( ui.totalRate->value(),  ui.indivRate->value() );
	load();
}

