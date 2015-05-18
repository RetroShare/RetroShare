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

#include "rshare.h"
#include "rsharesettings.h"

#include <iostream>

#include <retroshare/rsconfig.h>
#include <retroshare/rspeers.h>
#include <retroshare/rsturtle.h>

#include <QTcpSocket>
#include <QNetworkProxy>
#include <QTimer>

#define ICON_STATUS_UNKNOWN ":/images/ledoff1.png"
#define ICON_STATUS_OK      ":/images/ledon1.png"

//#define SERVER_DEBUG 1

ServerPage::ServerPage(QWidget * parent, Qt::WindowFlags flags)
    : ConfigPage(parent, flags), mIsHiddenNode(false)
{
  /* Invoke the Qt Designer generated object setup routine */
  ui.setupUi(this);

  connect( ui.netModeComboBox, SIGNAL( activated ( int ) ), this, SLOT( toggleUPnP( ) ) );
  connect( ui.allowIpDeterminationCB, SIGNAL( toggled( bool ) ), this, SLOT( toggleIpDetermination(bool) ) );
  connect( ui.cleanKnownIPs_PB, SIGNAL( clicked( ) ), this, SLOT( clearKnownAddressList() ) );
  //connect( ui.allowTunnelConnectionCB, SIGNAL( toggled( bool ) ), this, SLOT( toggleTunnelConnection(bool) ) );
  //connect( ui._turtle_enabled_CB, SIGNAL( toggled( bool ) ), this, SLOT( toggleTurtleRouting(bool) ) );

   QTimer *timer = new QTimer(this);
   timer->connect(timer, SIGNAL(timeout()), this, SLOT(updateStatus()));
   timer->start(1000);


	//load();
	updateStatus();

	bool b = rsPeers->getAllowServerIPDetermination() ;
	ui.allowIpDeterminationCB->setChecked(b) ;
	ui.IPServersLV->setEnabled(b) ;

    //ui.allowTunnelConnectionCB->hide();
    //this->toggleTunnelConnection(false);

    //b = rsPeers->getAllowTunnelConnection() ;
    //ui.allowTunnelConnectionCB->setChecked(b) ;

	std::list<std::string> ip_servers ;
	rsPeers->getIPServersList(ip_servers) ;

	for(std::list<std::string>::const_iterator it(ip_servers.begin());it!=ip_servers.end();++it)
		ui.IPServersLV->addItem(QString::fromStdString(*it)) ;

	ui.torpage_incoming->setVisible(false);

  /* Hide platform specific features */
#ifdef Q_WS_WIN

#endif

#ifdef SERVER_DEBUG
	std::cerr << "ServerPage::ServerPage() called";
	std::cerr << std::endl;
#endif
}

void ServerPage::clearKnownAddressList()
{
    rsPeers->resetOwnExternalAddressList() ;

    load() ;
}

void ServerPage::toggleIpDetermination(bool b)
{
	rsPeers->allowServerIPDetermination(b) ;
	ui.IPServersLV->setEnabled(b) ;
}

void ServerPage::toggleTunnelConnection(bool b)
{
        std::cerr << "ServerPage::toggleTunnelConnection() set tunnel to : " << b << std::endl;
        //rsPeers->allowTunnelConnection(b) ;
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
#ifdef SERVER_DEBUG
	std::cerr << "ServerPage::load() called";
	std::cerr << std::endl;
#endif

	/* load up configuration from rsPeers */
	RsPeerDetails detail;
	if (!rsPeers->getPeerDetails(rsPeers->getOwnId(), detail))
	{
		return;
	}

	mIsHiddenNode = (detail.netMode == RS_NETMODE_HIDDEN);
	if (mIsHiddenNode)
	{
		loadHiddenNode();
		return;
	}

	ui.netModeComboBox->show() ;
	ui.hiddenMode_LB->hide() ;

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
	if (detail.vs_dht != RS_VS_DISC_OFF)
	{
		if (detail.vs_disc != RS_VS_DISC_OFF)
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
		if (detail.vs_disc != RS_VS_DISC_OFF)
		{
			netIndex = 1; // PRIVATE
		}
		else
		{
			netIndex = 3; // NONE
		}
	}

	ui.discComboBox->setCurrentIndex(netIndex);

	int dlrate = 0;
	int ulrate = 0;
	rsConfig->GetMaxDataRates(dlrate, ulrate);
	ui.totalDownloadRate->setValue(dlrate);
	ui.totalUploadRate->setValue(ulrate);

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

		ui.ipAddressList->clear();
		for(std::list<std::string>::const_iterator it(detail.ipAddressList.begin());it!=detail.ipAddressList.end();++it)
			ui.ipAddressList->addItem(QString::fromStdString(*it));

	/* TOR PAGE SETTINGS - only Proxy (outgoing) */
	std::string proxyaddr;
    uint16_t proxyport;
    uint32_t status ;
    rsPeers->getProxyServer(proxyaddr, proxyport, status);
	ui.torpage_proxyAddress -> setText(QString::fromStdString(proxyaddr));
    ui.torpage_proxyPort -> setValue(proxyport);

    updateTorOutProxyIndicator();

    if(mIsHiddenNode)
        updateTorInProxyIndicator();
}

//void ServerPage::toggleTurtleRouting(bool b)
//{
//	ui._max_tr_up_per_sec_SB->setEnabled(b) ;
//
//	rsTurtle->setEnabled(b) ;
//}

/** Loads the settings for this page */
void ServerPage::updateStatus()
{
#ifdef SERVER_DEBUG
	std::cerr << "ServerPage::updateStatus() called";
	std::cerr << std::endl;
#endif

	if(RsAutoUpdatePage::eventsLocked())
		return ;

	if(!isVisible())
		return ;

	if (mIsHiddenNode)
	{
		updateStatusHiddenNode();
		return;
	}

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


	// Now update network bits.
	RsConfigNetStatus net_status;
	rsConfig->getConfigNetStatus(net_status);

	/******* Network Status Tab *******/

	if(net_status.netUpnpOk)
		ui.iconlabel_upnp->setPixmap(QPixmap(":/images/ledon1.png"));
	else
		ui.iconlabel_upnp->setPixmap(QPixmap(":/images/ledoff1.png"));

	if (net_status.netLocalOk)
		ui.iconlabel_netLimited->setPixmap(QPixmap(":/images/ledon1.png"));
	else
		ui.iconlabel_netLimited->setPixmap(QPixmap(":/images/ledoff1.png"));

	if (net_status.netExtAddressOk)
		ui.iconlabel_ext->setPixmap(QPixmap(":/images/ledon1.png"));
	else
        ui.iconlabel_ext->setPixmap(QPixmap(":/images/ledoff1.png"));

    // check for TOR
    updateTorOutProxyIndicator();

    if(mIsHiddenNode)
    updateTorInProxyIndicator();
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

	if (mIsHiddenNode)
	{
		saveAddressesHiddenNode();
		return;
	}

	RsPeerDetails detail;
    RsPeerId ownId = rsPeers->getOwnId();

	if (!rsPeers->getPeerDetails(ownId, detail))
		return;

	int netIndex = ui.netModeComboBox->currentIndex();

	/* Check if netMode has changed */
	uint32_t netMode = 0;
	switch(netIndex)
	{
		case 3:
			netMode = RS_NETMODE_HIDDEN;
			break;
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

	uint16_t vs_disc = 0;
	uint16_t vs_dht = 0;
	/* Check if vis has changed */
	switch(ui.discComboBox->currentIndex())
	{
		case 0:
			vs_disc = RS_VS_DISC_FULL;
			vs_dht = RS_VS_DHT_FULL;
			break;
		case 1:
			vs_disc = RS_VS_DISC_FULL;
			vs_dht = RS_VS_DHT_OFF;
			break;
		case 2:
			vs_disc = RS_VS_DISC_OFF;
			vs_dht = RS_VS_DHT_FULL;
			break;
		case 3:
		default:
			vs_disc = RS_VS_DISC_OFF;
			vs_dht = RS_VS_DHT_OFF;
			break;
	}

	if ((vs_disc != detail.vs_disc) || (vs_dht != detail.vs_dht))
		rsPeers->setVisState(ownId, vs_disc, vs_dht);

	if (0 != netIndex)
		saveAddr = true;

	if (saveAddr)
	{
		rsPeers->setLocalAddress(ownId, ui.localAddress->text().toStdString(), ui.localPort->value());
		rsPeers->setExtAddress(ownId, ui.extAddress->text().toStdString(), ui.extPort->value());
	}

	rsPeers->setDynDNS(ownId, ui.dynDNS->text().toStdString());
	rsConfig->SetMaxDataRates( ui.totalDownloadRate->value(), ui.totalUploadRate->value() );

	// HANDLE PROXY SERVER.
	std::string orig_proxyaddr;
    uint16_t orig_proxyport;
    uint32_t status ;
    rsPeers->getProxyServer(orig_proxyaddr, orig_proxyport,status);

	std::string new_proxyaddr = ui.torpage_proxyAddress -> text().toStdString();
	uint16_t new_proxyport = ui.torpage_proxyPort -> value();

	if ((new_proxyaddr != orig_proxyaddr) || (new_proxyport != orig_proxyport))
	{
		rsPeers->setProxyServer(new_proxyaddr, new_proxyport);
	}

	load();
}


/***********************************************************************************/
/***********************************************************************************/
/******* ALTERNATIVE VERSION IF HIDDEN NODE ***************************************/
/***********************************************************************************/
/***********************************************************************************/

/** Loads the settings for this page */
void ServerPage::loadHiddenNode()
{
#ifdef SERVER_DEBUG
	std::cerr << "ServerPage::loadHiddenNode() called";
	std::cerr << std::endl;
#endif

	/* load up configuration from rsPeers */
	RsPeerDetails detail;
	if (!rsPeers->getPeerDetails(rsPeers->getOwnId(), detail))
	{
		return;
	}

	/* At this point we want to force the Configuration Page to look different
	 * We will be called multiple times - so cannot just delete bad items.
	 *
	 * We want:
	 *  NETMODE: HiddenNode FIXED.
	 *  Disc/DHT: Discovery / No Discovery.
	 *  Local Address: 127.0.0.1, Port: Listening Port. (listening port changable)
	 *  External Address ==> TOR Address: 17621376587.onion + PORT.
	 *
	 *  Known / Previous IPs: empty / removed.
	 *  Ask about IP: Disabled.
	 */

	// FIXED.
	//ui.netModeComboBox->setCurrentIndex(3);
	ui.netModeComboBox->hide();
	ui.hiddenMode_LB->show();

	// CHANGE OPTIONS ON 
	ui.discComboBox->removeItem(3);
	ui.discComboBox->removeItem(2);
	ui.discComboBox->removeItem(1);
	ui.discComboBox->removeItem(0);
	ui.discComboBox->insertItem (0, tr("Discovery On (recommended)"));
	ui.discComboBox->insertItem (1, tr("Discovery Off"));

	int netIndex = 1; // OFF.
	if (detail.vs_disc != RS_VS_DISC_OFF)
	{
		netIndex = 0; // DISC ON;
	}
	ui.discComboBox->setCurrentIndex(netIndex);

	// Download Rates - Stay the same as before.
	int dlrate = 0;
	int ulrate = 0;
	rsConfig->GetMaxDataRates(dlrate, ulrate);
	ui.totalDownloadRate->setValue(dlrate);
	ui.totalUploadRate->setValue(ulrate);

	// Addresses.
	ui.localAddress->setEnabled(false);
	ui.localPort  -> setEnabled(false);
	ui.extAddress -> setEnabled(false);
	ui.extPort    -> setVisible(false);
	ui.label_dynDNS->setVisible(false);
	ui.dynDNS      ->setVisible(false);

	ui.torpage_incoming->setVisible(true);

	/* Addresses must be set here - otherwise can't edit it */
		/* set local address */
	ui.localAddress->setText(QString::fromStdString(detail.localAddr));
	ui.localPort -> setValue(detail.localPort);
		/* set the server address */

	ui.extAddress->setText(tr("Hidden - See TOR Config"));

	ui.showDiscStatusBar->setChecked(Settings->getStatusBarFlags() & STATUSBAR_DISC);
    ui.showDiscStatusBar->hide() ;	// hidden because not functional at the moment.

    //ui._turtle_enabled_CB->setChecked(rsTurtle->enabled()) ;

	// show what we have in ipAddresses. (should be nothing!)
	ui.ipAddressList->clear();
	for(std::list<std::string>::const_iterator it(detail.ipAddressList.begin());it!=detail.ipAddressList.end();++it)
		ui.ipAddressList->addItem(QString::fromStdString(*it));

	ui.iconlabel_upnp->setPixmap(QPixmap(":/images/ledoff1.png"));
	ui.iconlabel_netLimited->setPixmap(QPixmap(":/images/ledoff1.png"));
	ui.iconlabel_ext->setPixmap(QPixmap(":/images/ledoff1.png"));

	ui.allowIpDeterminationCB->setChecked(false);
	ui.allowIpDeterminationCB->setEnabled(false);
	ui.IPServersLV->setEnabled(false);

	/* TOR PAGE SETTINGS */

	/* set local address */
	ui.torpage_localAddress->setEnabled(false);
	ui.torpage_localAddress->setText(QString::fromStdString(detail.localAddr));
	ui.torpage_localPort -> setValue(detail.localPort);

	/* set the server address */
	ui.torpage_onionAddress->setText(QString::fromStdString(detail.hiddenNodeAddress));
	ui.torpage_onionPort -> setValue(detail.hiddenNodePort);

	std::string proxyaddr;
    uint16_t proxyport;
    uint32_t proxy_state_flags;
    rsPeers->getProxyServer(proxyaddr, proxyport, proxy_state_flags);
	ui.torpage_proxyAddress -> setText(QString::fromStdString(proxyaddr));
	ui.torpage_proxyPort -> setValue(proxyport);

    updateTorOutProxyIndicator();

if(mIsHiddenNode)
    updateTorInProxyIndicator();

	QString expected = "HiddenServiceDir </your/path/to/hidden/directory/service>\n";
	expected += "HiddenServicePort ";
	expected += QString::number(detail.hiddenNodePort);
	expected += " ";
	expected += QString::fromStdString(detail.localAddr);
	expected += ":";
	expected += QString::number(detail.localPort);

	ui.torpage_configuration->setPlainText(expected);
}

/** Loads the settings for this page */
void ServerPage::updateStatusHiddenNode()
{
#ifdef SERVER_DEBUG
	std::cerr << "ServerPage::updateStatusHiddenNode() called";
	std::cerr << std::endl;
#endif

// THIS IS DISABLED FOR NOW.
#if 0

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


	// Now update network bits.
	RsConfigNetStatus net_status;
	rsConfig->getConfigNetStatus(net_status);

	/******* Network Status Tab *******/

	if(net_status.netUpnpOk)
		ui.iconlabel_upnp->setPixmap(QPixmap(":/images/ledon1.png"));
	else
		ui.iconlabel_upnp->setPixmap(QPixmap(":/images/ledoff1.png"));

	if (net_status.netLocalOk)
		ui.iconlabel_netLimited->setPixmap(QPixmap(":/images/ledon1.png"));
	else
		ui.iconlabel_netLimited->setPixmap(QPixmap(":/images/ledoff1.png"));

	if (net_status.netExtAddressOk)
		ui.iconlabel_ext->setPixmap(QPixmap(":/images/ledon1.png"));
	else
		ui.iconlabel_ext->setPixmap(QPixmap(":/images/ledoff1.png"));

#endif

    updateTorOutProxyIndicator();
    if(mIsHiddenNode)
    updateTorInProxyIndicator();
}

void ServerPage::saveAddressesHiddenNode()
{
	RsPeerDetails detail;
    RsPeerId ownId = rsPeers->getOwnId();

	if (!rsPeers->getPeerDetails(ownId, detail))
		return;

	// NETMODE IS UNCHANGABLE
	uint16_t vs_disc = 0;
	uint16_t vs_dht = 0;
	/* Check if vis has changed */
	switch(ui.discComboBox->currentIndex())
	{
		default:
		case 0:
			vs_disc = RS_VS_DISC_FULL;
			vs_dht = RS_VS_DHT_OFF;
			break;
		case 1:
			vs_disc = RS_VS_DISC_OFF;
			vs_dht = RS_VS_DHT_OFF;
			break;
	}

	if ((vs_disc != detail.vs_disc) || (vs_dht != detail.vs_dht))
		rsPeers->setVisState(ownId, vs_disc, vs_dht);

	if (detail.localPort != ui.torpage_localPort->value())
	{
		// Set Local Address - force to 127.0.0.1
		rsPeers->setLocalAddress(ownId, "127.0.0.1", ui.torpage_localPort->value());
	}

	std::string hiddenAddr = ui.torpage_onionAddress->text().toStdString();
	uint16_t    hiddenPort = ui.torpage_onionPort->value();
	if ((hiddenAddr != detail.hiddenNodeAddress) || (hiddenPort != detail.hiddenNodePort))
	{
		rsPeers->setHiddenNode(ownId, hiddenAddr, hiddenPort);
	}

	// HANDLE PROXY SERVER.
	std::string orig_proxyaddr;
    uint16_t orig_proxyport;
    uint32_t state_flags ;
    rsPeers->getProxyServer(orig_proxyaddr, orig_proxyport,state_flags);

	std::string new_proxyaddr = ui.torpage_proxyAddress -> text().toStdString();
	uint16_t new_proxyport = ui.torpage_proxyPort -> value();

	if ((new_proxyaddr != orig_proxyaddr) || (new_proxyport != orig_proxyport))
	{
		rsPeers->setProxyServer(new_proxyaddr, new_proxyport);
	}

	rsConfig->SetMaxDataRates( ui.totalDownloadRate->value(), ui.totalUploadRate->value() );
	load();
}
void ServerPage::updateTorOutProxyIndicator()
{
    QTcpSocket socket ;
    socket.connectToHost(ui.torpage_proxyAddress->text(),ui.torpage_proxyPort->text().toInt());

    if(socket.waitForConnected(500))
    {
        socket.disconnectFromHost();
        ui.iconlabel_tor_outgoing->setPixmap(QPixmap(ICON_STATUS_OK)) ;
        ui.iconlabel_tor_outgoing->setToolTip(tr("Proxy seems to work.")) ;
    }
    else
    {
        ui.iconlabel_tor_outgoing->setPixmap(QPixmap(ICON_STATUS_UNKNOWN)) ;
        ui.iconlabel_tor_outgoing->setToolTip(tr("TOR proxy is not enabled")) ;
    }
}
void ServerPage::updateLocInProxyIndicator()
{
    QTcpSocket socket ;
    socket.connectToHost(ui.torpage_localAddress->text(),ui.torpage_localPort->text().toInt());

    if(socket.waitForConnected(1000))
    {
        socket.disconnectFromHost();
        ui.iconlabel_local_incoming->setPixmap(QPixmap(ICON_STATUS_OK)) ;
        ui.iconlabel_local_incoming->setToolTip(tr("You are reachable through TOR.")) ;
    }
    else
    {
        ui.iconlabel_local_incoming->setPixmap(QPixmap(ICON_STATUS_UNKNOWN)) ;
        ui.iconlabel_local_incoming->setToolTip(tr("TOR proxy is not enabled or broken.\nAre you running a TOR hidden service?\nCheck your ports!")) ;
    }
}
void ServerPage::updateTorInProxyIndicator()
{
    // need to find a proper way to do this
#ifdef SUSPENDED
    if(!mIsHiddenNode)
        return ;

    QTcpSocket socket ;

    QNetworkProxy proxy ;

    proxy.setType(QNetworkProxy::DefaultProxy);
    proxy.setHostName(ui.torpage_proxyAddress->text());
    proxy.setPort(ui.torpage_proxyPort->text().toInt());
    proxy.setCapabilities(QNetworkProxy::HostNameLookupCapability | proxy.capabilities()) ;

    std::cerr << "Setting proxy hostname+port to " << std::dec << ui.torpage_proxyAddress->text().toStdString() << ":" << ui.torpage_proxyPort->text().toInt() << std::endl;
    socket.setProxy(proxy) ;

    std::cerr << "Connecting to " << ui.torpage_onionAddress->text().toStdString() << ":" << ui.torpage_onionPort->text().toInt() << std::endl;

    socket.connectToHost(ui.torpage_onionAddress->text(),ui.torpage_onionPort->text().toInt());
    //socket.connectToHost("www.showip.com",80);

    if(socket.waitForConnected(5000))
    {
        std::cerr <<"Connected!" << std::endl;
		socket.disconnectFromHost();
        ui.iconlabel_tor_incoming->setPixmap(QPixmap(ICON_STATUS_OK)) ;
        ui.iconlabel_tor_incoming->setToolTip(tr("You are reachable through TOR.")) ;
	}
	else
	{
        std::cerr <<"Failed!" << std::endl;
    std::cerr << "Error: " << socket.errorString().toStdString() << std::endl;
        ui.iconlabel_tor_incoming->setPixmap(QPixmap(ICON_STATUS_UNKNOWN)) ;
        ui.iconlabel_tor_incoming->setToolTip(tr("TOR proxy is not enabled or broken.\nAre you running a TOR hidden service?\nCheck your ports!")) ;
    }
#endif
}

