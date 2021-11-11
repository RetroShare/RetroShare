/*******************************************************************************
 * gui/settings/ServerPage.cpp                                                 *
 *                                                                             *
 * Copyright (c) 2006 Crypton         <retroshare.project@gmail.com>           *
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

#include "ServerPage.h"

#include <gui/notifyqt.h>
#include "rshare.h"
#include "rsharesettings.h"
#include "util/i2pcommon.h"
#include "util/misc.h"
#include "util/qtthreadsutils.h"
#include "util/RsNetUtil.h"

#include <iostream>

#include "retroshare/rsbanlist.h"
#include "retroshare/rsconfig.h"
#include "retroshare/rsdht.h"
#include "retroshare/rsinit.h"
#include "retroshare/rspeers.h"
#include "retroshare/rsturtle.h"

#include <QCheckBox>
#include <QMovie>
#include <QMenu>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QTcpSocket>
#include <QTimer>

#include <libsam3.h>

#define ICON_STATUS_UNKNOWN ":/images/ledoff1.png"
#define ICON_STATUS_WORKING ":/images/yellowled.png"
#define ICON_STATUS_OK      ":/images/ledon1.png"
#define ICON_STATUS_ERROR   ":/images/redled.png"

#define COLUMN_RANGE   0
#define COLUMN_STATUS  1
#define COLUMN_ORIGIN  2
#define COLUMN_REASON  3
#define COLUMN_COMMENT 4

///
/// \brief hiddenServiceIncomingTab index of hidden serice incoming tab
///
///

// Tabs numbers *after* non relevant tabs are removed. So do not use them to add/remove tabs!!
//nst static uint32_t TAB_HIDDEN_SERVICE_OUTGOING = 0;
const static uint32_t TAB_HIDDEN_SERVICE_INCOMING = 1;
const static uint32_t TAB_HIDDEN_SERVICE_I2P	  = 2;

//nst static uint32_t TAB_NETWORK                 = 0;
const static uint32_t TAB_HIDDEN_SERVICE          = 1;
const static uint32_t TAB_IP_FILTERS              = 2;
const static uint32_t TAB_RELAYS                  = 3;

//#define SERVER_DEBUG 1

ServerPage::ServerPage(QWidget * parent, Qt::WindowFlags flags)
    : ConfigPage(parent, flags)
    , manager(NULL), mOngoingConnectivityCheck(-1)
    , mIsHiddenNode(false), mHiddenType(RS_HIDDEN_TYPE_NONE)
    , mSamAccessible(false)
    , mEventHandlerId(0)
{
  /* Invoke the Qt Designer generated object setup routine */
  ui.setupUi(this);

#ifndef RS_USE_I2P_SAM3
  ui.hiddenServiceTab->removeTab(TAB_HIDDEN_SERVICE_I2P);	// warning: the order of operation here is very important.
#endif

  if(RsAccounts::isHiddenNode())
  {
      ui.tabWidget->removeTab(TAB_RELAYS) ;		// remove relays. Not useful in Tor mode.
      ui.tabWidget->removeTab(TAB_IP_FILTERS) ;	// remove IP filters. Not useful in Tor mode.

      if(RsAccounts::isTorAuto())
	  {
		  // Here we use absolute numbers instead of consts defined above, because the consts correspond to the tab number *after* this tab removal.

		  ui.hiddenpage_proxyAddress_i2p->hide() ;
		  ui.hiddenpage_proxyLabel_i2p->hide() ;
		  ui.hiddenpage_proxyPort_i2p->hide() ;
		  ui.label_i2p_outgoing->hide() ;
		  ui.iconlabel_i2p_outgoing->hide() ;
		  ui.info_SocksProxy->hide() ;
		  ui.hiddenpage_configuration->hide() ;
		  ui.l_hiddenpage_configuration->hide() ;
		  ui.info_HiddenPageInHelp->hide() ;

          ui.hiddenpage_outHeader->setText(tr("Tor has been automatically configured by Retroshare. You shouldn't need to change anything here.")) ;
		  ui.hiddenpage_inHeader->setText(tr("Tor has been automatically configured by Retroshare. You shouldn't need to change anything here.")) ;

		  ui.hiddenServiceTab->removeTab(TAB_HIDDEN_SERVICE_I2P);	// warning: the order of operation here is very important.
      }
  }
  else
  {
	  ui.hiddenServiceTab->removeTab(TAB_HIDDEN_SERVICE_INCOMING);	// warning: the order of operation here is very important.
  }

    ui.filteredIpsTable->setHorizontalHeaderItem(COLUMN_RANGE,new QTableWidgetItem(tr("IP Range"))) ;
    ui.filteredIpsTable->setHorizontalHeaderItem(COLUMN_STATUS,new QTableWidgetItem(tr("Status"))) ;
    ui.filteredIpsTable->setHorizontalHeaderItem(COLUMN_ORIGIN,new QTableWidgetItem(tr("Origin"))) ;
    ui.filteredIpsTable->setHorizontalHeaderItem(COLUMN_COMMENT,new QTableWidgetItem(tr("Comment"))) ;

    ui.filteredIpsTable->setColumnHidden(COLUMN_STATUS,true) ;
    ui.filteredIpsTable->verticalHeader()->hide() ;
    ui.whiteListIpsTable->setColumnHidden(COLUMN_STATUS,true) ;
    ui.whiteListIpsTable->verticalHeader()->hide() ;

   QTimer *timer = new QTimer(this);
   timer->connect(timer, SIGNAL(timeout()), this, SLOT(updateStatus()));
   timer->start(1000);

    //load();
    updateStatus();

    bool b = rsPeers->getAllowServerIPDetermination() ;
    ui.allowIpDeterminationCB->setChecked(b) ;
    ui.IPServersLV->setEnabled(b) ;

    std::list<std::string> ip_servers ;
    rsPeers->getIPServersList(ip_servers) ;

    for(std::list<std::string>::const_iterator it(ip_servers.begin());it!=ip_servers.end();++it)
        ui.IPServersLV->addItem(QString::fromStdString(*it)) ;

    ui.gbBob->setEnabled(false);
    ui.swBobAdvanced->setCurrentIndex(0);

    ui.lBobB32Addr->hide();
    ui.leBobB32Addr->hide();
    ui.pbBobGenAddr->hide();

    QObject::connect(ui.filteredIpsTable,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(ipFilterContextMenu(QPoint))) ;
    QObject::connect(ui.whiteListIpsTable,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(ipWhiteListContextMenu(QPoint))) ;
    QObject::connect(ui.denyAll_CB,SIGNAL(toggled(bool)),this,SLOT(toggleIpFiltering(bool)));
    QObject::connect(ui.includeFromDHT_CB,SIGNAL(toggled(bool)),this,SLOT(toggleAutoIncludeDHT(bool)));
    QObject::connect(ui.includeFromFriends_CB,SIGNAL(toggled(bool)),this,SLOT(toggleAutoIncludeFriends(bool)));
    QObject::connect(ui.groupIPRanges_CB,SIGNAL(toggled(bool)),this,SLOT(toggleGroupIps(bool)));
    QObject::connect(ui.groupIPRanges_SB,SIGNAL(valueChanged(int)),this,SLOT(setGroupIpLimit(int)));
    QObject::connect(ui.ipInputAddBlackList_PB,SIGNAL(clicked()),this,SLOT(addIpRangeToBlackList()));
    QObject::connect(ui.ipInputAddWhiteList_PB,SIGNAL(clicked()),this,SLOT(addIpRangeToWhiteList()));
    QObject::connect(ui.ipInput_LE,SIGNAL(textChanged(QString)),this,SLOT(checkIpRange(QString)));
    QObject::connect(ui.filteredIpsTable,SIGNAL(currentCellChanged(int,int,int,int)),this,SLOT(updateSelectedBlackListIP(int,int,int,int)));
    QObject::connect(ui.whiteListIpsTable,SIGNAL(currentCellChanged(int,int,int,int)),this,SLOT(updateSelectedWhiteListIP(int,int,int,int)));

	QObject::connect(ui.pbBobStart,   SIGNAL(clicked()), this, SLOT(startSam()));
	QObject::connect(ui.pbBobRestart, SIGNAL(clicked()), this, SLOT(restartSam()));
	QObject::connect(ui.pbBobStop,    SIGNAL(clicked()), this, SLOT(stopSam()));
    QObject::connect(ui.pbBobGenAddr, SIGNAL(clicked()), this, SLOT(getNewKey()));
    QObject::connect(ui.pbBobLoadKey, SIGNAL(clicked()), this, SLOT(loadKey()));
	QObject::connect(ui.cb_enableBob, SIGNAL(toggled(bool)), this, SLOT(enableSam(bool)));

	QObject::connect(ui.cbBobAdvanced, SIGNAL(toggled(bool)), this, SLOT(toggleSamAdvancedSettings(bool)));

    QObject::connect(ui.sbBobLengthIn,    SIGNAL(valueChanged(int)), this, SLOT(tunnelSettingsChanged(int)));
    QObject::connect(ui.sbBobLengthOut,   SIGNAL(valueChanged(int)), this, SLOT(tunnelSettingsChanged(int)));
    QObject::connect(ui.sbBobQuantityIn,  SIGNAL(valueChanged(int)), this, SLOT(tunnelSettingsChanged(int)));
    QObject::connect(ui.sbBobQuantityOut, SIGNAL(valueChanged(int)), this, SLOT(tunnelSettingsChanged(int)));
    QObject::connect(ui.sbBobVarianceIn,  SIGNAL(valueChanged(int)), this, SLOT(tunnelSettingsChanged(int)));
    QObject::connect(ui.sbBobVarianceOut, SIGNAL(valueChanged(int)), this, SLOT(tunnelSettingsChanged(int)));

    // These two line edits are used for the same thing - keep them in sync!
    QObject::connect(ui.hiddenpage_proxyAddress_i2p,   SIGNAL(textChanged(QString)), this, SLOT(syncI2PProxyAddrNormal(QString)));
	QObject::connect(ui.hiddenpage_proxyAddress_i2p_2, SIGNAL(textChanged(QString)), this, SLOT(syncI2PProxyAddrSam(QString)));

	connect(NotifyQt::getInstance(), SIGNAL(connectionWithoutCert()), this, SLOT(connectionWithoutCert()));

    QObject::connect(ui.localPort,SIGNAL(valueChanged(int)),this,SLOT(saveAddresses()));
    QObject::connect(ui.extPort,SIGNAL(valueChanged(int)),this,SLOT(saveAddresses()));

    connect( ui.netModeComboBox, SIGNAL( activated(int) ), this, SLOT( toggleUPnP() ) );
    connect( ui.allowIpDeterminationCB, SIGNAL( toggled(bool) ), this, SLOT( toggleIpDetermination(bool) ) );
    connect( ui.cleanKnownIPs_PB, SIGNAL( clicked() ), this, SLOT( clearKnownAddressList() ) );
    connect( ui.testIncoming_PB, SIGNAL( clicked() ), this, SLOT( saveAndTestInProxy() ) );

#ifdef SERVER_DEBUG
    std::cerr << "ServerPage::ServerPage() called";
    std::cerr << std::endl;
#endif

    connect(ui.discComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(saveAddresses()));
    connect(ui.netModeComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(saveAddresses()));
    connect(ui.localAddress,   SIGNAL(textChanged(QString)),this,SLOT(saveAddresses()));
    connect(ui.extAddress,     SIGNAL(textChanged(QString)),this,SLOT(saveAddresses()));
    connect(ui.dynDNS,         SIGNAL(textChanged(QString)),this,SLOT(saveAddresses()));

    connect(ui.tabWidget, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));
    connect(ui.hiddenpage_proxyAddress_tor, SIGNAL(editingFinished()),this,SLOT(saveAddresses()));
    connect(ui.hiddenpage_proxyPort_tor,    SIGNAL(editingFinished()),this,SLOT(saveAddresses()));
    connect(ui.hiddenpage_proxyAddress_i2p, SIGNAL(editingFinished()),this,SLOT(saveAddresses()));
    connect(ui.hiddenpage_proxyPort_i2p,    SIGNAL(editingFinished()),this,SLOT(saveAddresses()));

    connect(ui.totalDownloadRate,SIGNAL(valueChanged(int)),this,SLOT(saveRates()));
    connect(ui.totalUploadRate,  SIGNAL(valueChanged(int)),this,SLOT(saveRates()));

	//Relay Tab
	QObject::connect(ui.noFriendSpinBox,SIGNAL(valueChanged(int)),this,SLOT(updateRelayOptions()));
	QObject::connect(ui.noFOFSpinBox,SIGNAL(valueChanged(int)),this,SLOT(updateRelayOptions()));
	QObject::connect(ui.noGeneralSpinBox,SIGNAL(valueChanged(int)),this,SLOT(updateRelayOptions()));
	QObject::connect(ui.bandFriendSpinBox,SIGNAL(valueChanged(int)),this,SLOT(updateRelayOptions()));
	QObject::connect(ui.bandFOFSpinBox,SIGNAL(valueChanged(int)),this,SLOT(updateRelayOptions()));
	QObject::connect(ui.bandGeneralSpinBox,SIGNAL(valueChanged(int)),this,SLOT(updateRelayOptions()));

	QObject::connect(ui.addPushButton,SIGNAL(clicked()),this,SLOT(addServer()));
	QObject::connect(ui.removePushButton,SIGNAL(clicked()),this,SLOT(removeServer()));
	QObject::connect(ui.DhtLineEdit,SIGNAL(textChanged(QString)),this,SLOT(checkKey()));

	QObject::connect(ui.enableCheckBox,SIGNAL(stateChanged(int)),this,SLOT(updateEnabled()));
	QObject::connect(ui.serverCheckBox,SIGNAL(stateChanged(int)),this,SLOT(updateEnabled()));

	QObject::connect(ui.noFriendSpinBox,SIGNAL(valueChanged(int)),this,SLOT(updateTotals()));
	QObject::connect(ui.bandFriendSpinBox,SIGNAL(valueChanged(int)),this,SLOT(updateTotals()));
	QObject::connect(ui.noFOFSpinBox,SIGNAL(valueChanged(int)),this,SLOT(updateTotals()));
	QObject::connect(ui.bandFOFSpinBox,SIGNAL(valueChanged(int)),this,SLOT(updateTotals()));
	QObject::connect(ui.noGeneralSpinBox,SIGNAL(valueChanged(int)),this,SLOT(updateTotals()));
	QObject::connect(ui.bandGeneralSpinBox,SIGNAL(valueChanged(int)),this,SLOT(updateTotals()));

	QObject::connect(ui.enableCheckBox,SIGNAL(toggled(bool)),this,SLOT(updateRelayMode()));
	QObject::connect(ui.serverCheckBox,SIGNAL(toggled(bool)),this,SLOT(updateRelayMode()));

	// when the network menu is opened and the hidden service tab is already selected updateOutProxyIndicator() won't be called and thus resulting in wrong proxy indicators.
	if (ui.tabWidget->currentIndex() == TAB_HIDDEN_SERVICE)
		updateOutProxyIndicator();

	rsEvents->registerEventsHandler( [this](std::shared_ptr<const RsEvent> event) { handleEvent(event); }, mEventHandlerId, RsEventType::NETWORK );

}

void ServerPage::handleEvent(std::shared_ptr<const RsEvent> e)
{
	if(e->mType != RsEventType::NETWORK)
		return;

	const RsNetworkEvent *ne = dynamic_cast<const RsNetworkEvent*>(e.get());

	if(!ne)
		return;

	// in any case we update the IPs

	switch(ne->mNetworkEventCode)
	{
		case RsNetworkEventCode::LOCAL_IP_UPDATED:  // [fallthrough]
		case RsNetworkEventCode::EXTERNAL_IP_UPDATED:  // [fallthrough]
			RsQThreadUtils::postToObject( [=]()
			{
				updateStatus();
			},this);
		break;
		default:
		break;
	}
}


void ServerPage::saveAndTestInProxy()
{
    saveAddresses();
    updateInProxyIndicator() ;
}

void ServerPage::checkIpRange(const QString& ipstr)
{
    QColor color;
    struct sockaddr_storage addr;
    int bytes ;

    if(!RsNetUtil::parseAddrFromQString(ipstr,addr,bytes) || bytes != 0)
    {
        std::cout << "setting palette 1" << std::endl ;
        color = QApplication::palette().color(QPalette::Disabled, QPalette::Base);
    }
    else
    {
        std::cout << "setting palette 2" << std::endl ;
        color = QApplication::palette().color(QPalette::Active, QPalette::Base);
    }
    /* unpolish widget to clear the stylesheet's palette cache */
    ui.ipInput_LE->style()->unpolish(ui.ipInput_LE);

    QPalette palette = ui.ipInput_LE->palette();
    palette.setColor(ui.ipInput_LE->backgroundRole(), color);
    ui.ipInput_LE->setPalette(palette);
}

void ServerPage::addIpRangeToBlackList()
{
    QString ipstr = ui.ipInput_LE->text() ;
    sockaddr_storage addr ;
    int bytes = 0 ;

    if(!RsNetUtil::parseAddrFromQString(ipstr,addr,bytes) || bytes != 0)
        return ;

    bytes = 4 - ui.ipInputRange_SB->value()/8;

    rsBanList->addIpRange(addr,bytes, RSBANLIST_TYPE_BLACKLIST,ui.ipInputComment_LE->text().toStdString());
}

void ServerPage::addIpRangeToWhiteList()
{
    QString ipstr = ui.ipInput_LE->text() ;
    sockaddr_storage addr ;
    int bytes = 0 ;

    if(!RsNetUtil::parseAddrFromQString(ipstr,addr,bytes) || bytes != 0)
        return ;

    bytes = 4 - ui.ipInputRange_SB->value()/8;

    rsBanList->addIpRange(addr,bytes, RSBANLIST_TYPE_WHITELIST,ui.ipInputComment_LE->text().toStdString());
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

	rsAutoProxyMonitor::taskSync(autoProxyType::I2PSAM3, autoProxyTask::getSettings, &mSamSettings);

    loadCommon();
    updateStatus();

    if (mIsHiddenNode)
    {
        mHiddenType = detail.hiddenType;
        //ui.tabWidget->setTabEnabled(TAB_IP_FILTERS,false) ; // ip filter
		//ui.tabWidget->setTabEnabled(TAB_RELAYS,false) ; // relay
        loadHiddenNode();
        return;
    }

    // (csoler) Disabling some signals in this block in order to avoid
    // some nasty feedback.
    {
        loadFilteredIps() ;

        ui.netModeComboBox->show() ;
        ui.textlabel_upnp->show();
        ui.iconlabel_upnp->show();
        ui.label_nat->show();

        ui.textlabel_hiddenMode->hide() ;
        ui.iconlabel_hiddenMode->hide() ;

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
        whileBlocking(ui.netModeComboBox)->setCurrentIndex(netIndex);

        /* DHT + Discovery: (public)
     * Discovery only:  (private)
     * DHT only: (inverted)
     * None: (dark net)
     */

        netIndex = 3; // NONE.
        if (detail.vs_dht != RS_VS_DHT_OFF)
        {
            if (detail.vs_disc != RS_VS_DISC_OFF)
                netIndex = 0; // PUBLIC
            else
                netIndex = 2; // INVERTED
        }
        else
        {
            if (detail.vs_disc != RS_VS_DISC_OFF)
                netIndex = 1; // PRIVATE
            //else //Use default value
            //    netIndex = 3; // NONE
        }

        whileBlocking(ui.discComboBox)->setCurrentIndex(netIndex);

        int dlrate = 0;
        int ulrate = 0;
        rsConfig->GetMaxDataRates(dlrate, ulrate);
        whileBlocking(ui.totalDownloadRate)->setValue(dlrate);
        whileBlocking(ui.totalUploadRate)->setValue(ulrate);

        toggleUPnP();

        /* Addresses must be set here - otherwise can't edit it */
        /* set local address */
        whileBlocking(ui.localAddress)->setText(QString::fromStdString(detail.localAddr));
        whileBlocking(ui.localPort )-> setValue(detail.localPort);
        /* set the server address */
        whileBlocking(ui.extAddress)->setText(QString::fromStdString(detail.extAddr));
        whileBlocking(ui.extPort) -> setValue(detail.extPort);
        /* set DynDNS */
        whileBlocking(ui.dynDNS) -> setText(QString::fromStdString(detail.dyndns));


        whileBlocking(ui.ipAddressList)->clear();
        detail.ipAddressList.sort();
        for(std::list<std::string>::const_iterator it(detail.ipAddressList.begin());it!=detail.ipAddressList.end();++it)
            whileBlocking(ui.ipAddressList)->addItem(QString::fromStdString(*it));

        /* HIDDEN PAGE SETTINGS - only Proxy (outgoing) */
        std::string proxyaddr;
        uint16_t proxyport;
        uint32_t status ;
        // Tor
        rsPeers->getProxyServer(RS_HIDDEN_TYPE_TOR, proxyaddr, proxyport, status);
        whileBlocking(ui.hiddenpage_proxyAddress_tor) -> setText(QString::fromStdString(proxyaddr));
        whileBlocking(ui.hiddenpage_proxyPort_tor) -> setValue(proxyport);
        // I2P
        rsPeers->getProxyServer(RS_HIDDEN_TYPE_I2P, proxyaddr, proxyport, status);
        whileBlocking(ui.hiddenpage_proxyAddress_i2p) -> setText(QString::fromStdString(proxyaddr));
        whileBlocking(ui.hiddenpage_proxyPort_i2p) -> setValue(proxyport);

    }

	//Relay Tab
	uint32_t count;
	uint32_t bandwidth;
	rsDht->getRelayAllowance(RsDhtRelayClass::FRIENDS, count, bandwidth);
	whileBlocking(ui.noFriendSpinBox)->setValue(count);
	whileBlocking(ui.bandFriendSpinBox)->setValue(bandwidth / 1024);

	rsDht->getRelayAllowance(RsDhtRelayClass::FOF, count, bandwidth);
	whileBlocking(ui.noFOFSpinBox)->setValue(count);
	whileBlocking(ui.bandFOFSpinBox)->setValue(bandwidth / 1024);

	rsDht->getRelayAllowance(RsDhtRelayClass::GENERAL, count, bandwidth);
	whileBlocking(ui.noGeneralSpinBox)->setValue(count);
	whileBlocking(ui.bandGeneralSpinBox)->setValue(bandwidth / 1024);

	updateTotals();


	RsDhtRelayMode relayMode = rsDht->getRelayMode();
	if (!!(relayMode & RsDhtRelayMode::ENABLED))
	{
		whileBlocking(ui.enableCheckBox)->setCheckState(Qt::Checked);
		if ((relayMode & RsDhtRelayMode::MASK) == RsDhtRelayMode::OFF)
		{
			whileBlocking(ui.serverCheckBox)->setCheckState(Qt::Unchecked);
		}
		else
		{
			whileBlocking(ui.serverCheckBox)->setCheckState(Qt::Checked);
		}
	}
	else
	{
		whileBlocking(ui.enableCheckBox)->setCheckState(Qt::Unchecked);
		whileBlocking(ui.serverCheckBox)->setCheckState(Qt::Unchecked);
	}

	loadServers();
	updateRelayOptions();
	updateEnabled();
	checkKey();

}

void ServerPage::toggleAutoIncludeFriends(bool b)
{
    rsBanList->enableIPsFromFriends(b) ;
}

void ServerPage::toggleAutoIncludeDHT(bool b)
{
    rsBanList->enableIPsFromDHT(b) ;
}

void ServerPage::toggleIpFiltering(bool b)
{
    rsBanList->enableIPFiltering(b) ;
    loadFilteredIps() ;
}

void ServerPage::loadFilteredIps()
{
	if(rsBanList == NULL)
		return ;

    if(rsBanList->ipFilteringEnabled())
    {
        whileBlocking(ui.denyAll_CB)->setChecked(true) ;
        whileBlocking(ui.filteredIpsTable)->setEnabled(true) ;
        whileBlocking(ui.includeFromFriends_CB)->setEnabled(true) ;
        whileBlocking(ui.includeFromDHT_CB)->setEnabled(true) ;
        whileBlocking(ui.ipInput_LE)->setEnabled(true) ;
        whileBlocking(ui.ipInputRange_SB)->setEnabled(true) ;
        whileBlocking(ui.ipInputComment_LE)->setEnabled(true) ;
        whileBlocking(ui.ipInputAddBlackList_PB)->setEnabled(true) ;
        whileBlocking(ui.ipInputAddWhiteList_PB)->setEnabled(true) ;
        whileBlocking(ui.groupIPRanges_CB)->setEnabled(true) ;
        whileBlocking(ui.groupIPRanges_SB)->setEnabled(true) ;
    }
    else
    {
        whileBlocking(ui.denyAll_CB)->setChecked(false) ;
        whileBlocking(ui.filteredIpsTable)->setEnabled(false) ;
        whileBlocking(ui.includeFromFriends_CB)->setEnabled(false) ;
        whileBlocking(ui.includeFromDHT_CB)->setEnabled(false) ;
        whileBlocking(ui.ipInput_LE)->setEnabled(false) ;
        whileBlocking(ui.ipInputRange_SB)->setEnabled(false) ;
        whileBlocking(ui.ipInputComment_LE)->setEnabled(false) ;
        whileBlocking(ui.ipInputAddBlackList_PB)->setEnabled(false) ;
        whileBlocking(ui.ipInputAddWhiteList_PB)->setEnabled(true) ;
        whileBlocking(ui.groupIPRanges_CB)->setEnabled(false) ;
        whileBlocking(ui.groupIPRanges_SB)->setEnabled(false) ;
    }

    whileBlocking(ui.includeFromFriends_CB)->setChecked(rsBanList->IPsFromFriendsEnabled()) ;
    whileBlocking(ui.includeFromDHT_CB)->setChecked(rsBanList->iPsFromDHTEnabled()) ;
    whileBlocking(ui.groupIPRanges_CB)->setChecked(rsBanList->autoRangeEnabled()) ;
    whileBlocking(ui.groupIPRanges_SB)->setValue(rsBanList->autoRangeLimit()) ;

    ui.whiteListIpsTable->setColumnHidden(COLUMN_STATUS,true);
    ui.filteredIpsTable->setColumnHidden(COLUMN_STATUS,true);

    std::list<BanListPeer> lst ;
    rsBanList->getBannedIps(lst) ;

    ui.filteredIpsTable->setRowCount(lst.size()) ;

    int row = 0 ;
    for(std::list<BanListPeer>::const_iterator it(lst.begin());it!=lst.end();++it,++row)
        addPeerToIPTable(ui.filteredIpsTable,row,*it) ;

    rsBanList->getWhiteListedIps(lst) ;
    ui.whiteListIpsTable->setRowCount(lst.size()) ;

    row = 0;
    for(std::list<BanListPeer>::const_iterator it(lst.begin());it!=lst.end();++it,++row)
        addPeerToIPTable(ui.whiteListIpsTable,row,*it) ;
}

void ServerPage::updateSelectedBlackListIP(int row,int,int,int)
{
    QTableWidgetItem *item = ui.filteredIpsTable->item(row,COLUMN_RANGE);
    if (!item) {
        return;
    }
    QString addr_string = item->text() ;

    sockaddr_storage addr ;
    int masked_bytes ;

    if(!RsNetUtil::parseAddrFromQString(addr_string,addr,masked_bytes))
    {
        std::cerr <<"Cannot parse IP \"" << addr_string.toStdString() << "\"" << std::endl;
        return ;
    }

    ui.ipInput_LE->setText(QString::fromStdString(sockaddr_storage_iptostring(addr))) ;
    ui.ipInputRange_SB->setValue(32 - 8*masked_bytes) ;
    ui.ipInputComment_LE->setText(ui.filteredIpsTable->item(row,COLUMN_COMMENT)->text()) ;
}

void ServerPage::updateSelectedWhiteListIP(int row, int,int,int)
{
    QTableWidgetItem *item = ui.whiteListIpsTable->item(row,COLUMN_RANGE);
    if (!item) {
        return;
    }

    QString addr_string = item->text() ;

    sockaddr_storage addr ;
    int masked_bytes ;

    if(!RsNetUtil::parseAddrFromQString(addr_string,addr,masked_bytes))
    {
        std::cerr <<"Cannot parse IP \"" << addr_string.toStdString() << "\"" << std::endl;
        return ;
    }

    ui.ipInput_LE->setText(QString::fromStdString(sockaddr_storage_iptostring(addr))) ;
    ui.ipInputRange_SB->setValue(32 - 8*masked_bytes) ;
    ui.ipInputComment_LE->setText(ui.whiteListIpsTable->item(row,COLUMN_COMMENT)->text()) ;
}

void ServerPage::addPeerToIPTable(QTableWidget *table,int row,const BanListPeer& blp)
{
    table->setItem(row,COLUMN_RANGE,new QTableWidgetItem(RsNetUtil::printAddrRange(blp.addr,blp.masked_bytes))) ;

    if( blp.state )
        table->setItem(row,COLUMN_STATUS,new QTableWidgetItem(QString("active"))) ;
    else
        table->setItem(row,COLUMN_STATUS,new QTableWidgetItem(QString(""))) ;

    switch(blp.level)
    {
    case RSBANLIST_ORIGIN_FOF:  table->setItem(row,COLUMN_ORIGIN,new QTableWidgetItem(QString("From friend of a friend"))) ;
        break ;
    case RSBANLIST_ORIGIN_FRIEND:  table->setItem(row,COLUMN_ORIGIN,new QTableWidgetItem(QString("From friend"))) ;
        break ;
    case RSBANLIST_ORIGIN_SELF:  table->setItem(row,COLUMN_ORIGIN,new QTableWidgetItem(QString("Local"))) ;
        break ;
    default:
    case RSBANLIST_ORIGIN_UNKNOWN:  table->setItem(row,COLUMN_ORIGIN,new QTableWidgetItem(QString("Unknown"))) ;
        break ;
    }

    switch( blp.reason )
    {
    case RSBANLIST_REASON_DHT:  table->setItem(row,COLUMN_REASON,new QTableWidgetItem(QString("Masquerading peer"))) ;
                    table->setItem(row,COLUMN_COMMENT,new QTableWidgetItem(tr("Reported by DHT for IP masquerading"))) ;
        break ;
    case RSBANLIST_REASON_USER:  table->setItem(row,COLUMN_REASON,new QTableWidgetItem(QString("Home-made rule"))) ;
                    table->setItem(row,COLUMN_COMMENT,new QTableWidgetItem(QString::fromStdString(blp.comment))) ;
        break ;
    case RSBANLIST_REASON_AUTO_RANGE:  table->setItem(row,COLUMN_REASON,new QTableWidgetItem(QString("Auto-generated range"))) ;
                        table->setItem(row,COLUMN_COMMENT,new QTableWidgetItem(tr("Range made from %1 collected addresses").arg(QString::number(blp.connect_attempts)))) ;
        break ;
    default:
    case RSBANLIST_REASON_UNKNOWN:  table->setItem(row,COLUMN_REASON,new QTableWidgetItem(QString("Unknown"))) ;
                    table->setItem(row,COLUMN_COMMENT,new QTableWidgetItem(QString::fromStdString(blp.comment))) ;
        break ;
    }
    table->item(row,COLUMN_STATUS)->setData(Qt::UserRole,QVariant( blp.state )) ;
    table->item(row,COLUMN_REASON)->setData(Qt::UserRole,QVariant( blp.reason )) ;
    table->item(row,COLUMN_ORIGIN)->setData(Qt::UserRole,QVariant( blp.level )) ;

}

void ServerPage::toggleGroupIps(bool b) { rsBanList->enableAutoRange(b) ; }

void ServerPage::setGroupIpLimit(int n) { rsBanList->setAutoRangeLimit(n) ; }

void ServerPage::ipFilterContextMenu(const QPoint& /*point*/)
{
    QMenu contextMenu(this) ;
    int row = ui.filteredIpsTable->currentRow();

    QTableWidgetItem *item = ui.filteredIpsTable->item(row, COLUMN_STATUS);

    if(item == NULL)
        return ;

    //bool status = item->data(Qt::UserRole).toBool();

    uint32_t reason = ui.filteredIpsTable->item(row,COLUMN_REASON)->data(Qt::UserRole).toUInt();

    QString addr_string = ui.filteredIpsTable->item(row,COLUMN_RANGE)->text() ;

    sockaddr_storage addr ;
    int masked_bytes ;

    if(!RsNetUtil::parseAddrFromQString(addr_string,addr,masked_bytes))
    {
        std::cerr <<"Cannot parse IP \"" << addr_string.toStdString() << "\"" << std::endl;
        return ;
    }

    QString range0 = RsNetUtil::printAddrRange(addr,0) ;
    QString range1 = RsNetUtil::printAddrRange(addr,1) ;
    QString range2 = RsNetUtil::printAddrRange(addr,2) ;

    if(reason == RSBANLIST_REASON_USER)
        contextMenu.addAction(tr("Remove"),this,SLOT(removeBannedIp()));

    contextMenu.addAction(QObject::tr("Move IP %1 to whitelist"  ).arg(range0),this,SLOT(moveToWhiteList0())) ;
    contextMenu.addAction(QObject::tr("Whitelist entire range %1").arg(range1),this,SLOT(moveToWhiteList1())) ;
    contextMenu.addAction(QObject::tr("whitelist entire range %1").arg(range2),this,SLOT(moveToWhiteList2())) ;

    contextMenu.exec(QCursor::pos()) ;
}

bool ServerPage::removeCurrentRowFromBlackList(sockaddr_storage& collected_addr,int &masked_bytes)
{
    int row = ui.filteredIpsTable->currentRow();
    QTableWidgetItem *item = ui.filteredIpsTable->item(row, COLUMN_STATUS);

    if(item == NULL)
        return false;

    QString addr_string = ui.filteredIpsTable->item(row,COLUMN_RANGE)->text() ;

    if(!RsNetUtil::parseAddrFromQString(addr_string,collected_addr,masked_bytes))
    {
        std::cerr <<"Cannot parse IP \"" << addr_string.toStdString() << "\"" << std::endl;
        return false;
    }
    rsBanList->removeIpRange(collected_addr,masked_bytes,RSBANLIST_TYPE_BLACKLIST);
    return true ;
}

bool ServerPage::removeCurrentRowFromWhiteList(sockaddr_storage& collected_addr,int &masked_bytes)
{
    int row = ui.whiteListIpsTable->currentRow();
    QTableWidgetItem *item = ui.whiteListIpsTable->item(row, COLUMN_STATUS);

    if(item == NULL)
        return false;

    QString addr_string = ui.whiteListIpsTable->item(row,COLUMN_RANGE)->text() ;

    if(!RsNetUtil::parseAddrFromQString(addr_string,collected_addr,masked_bytes))
    {
        std::cerr <<"Cannot parse IP \"" << addr_string.toStdString() << "\"" << std::endl;
        return false;
    }
    rsBanList->removeIpRange(collected_addr,masked_bytes,RSBANLIST_TYPE_WHITELIST);
    return true ;
}

void ServerPage::moveToWhiteList0()
{
    sockaddr_storage addr ;
    int bytes ;

    if(!removeCurrentRowFromBlackList(addr,bytes))
        return ;

    rsBanList->addIpRange(addr,0,RSBANLIST_TYPE_WHITELIST, tr("Added by you").toStdString());
}

void ServerPage::moveToWhiteList1()
{
    sockaddr_storage addr ;
    int bytes ;

    if(!removeCurrentRowFromBlackList(addr,bytes))
        return ;

    rsBanList->addIpRange(addr,1,RSBANLIST_TYPE_WHITELIST, tr("Added by you").toStdString());
}

void ServerPage::moveToWhiteList2()
{
    sockaddr_storage addr ;
    int bytes ;

    if(!removeCurrentRowFromBlackList(addr,bytes))
        return ;

    rsBanList->addIpRange(addr,2,RSBANLIST_TYPE_WHITELIST, tr("Added by you").toStdString());
}

void ServerPage::ipWhiteListContextMenu(const QPoint& /* point */)
{
    QMenu contextMenu(this) ;
    int row = ui.whiteListIpsTable->currentRow();

    QTableWidgetItem *item = ui.whiteListIpsTable->item(row, COLUMN_STATUS);

    if(item == NULL)
        return ;

    //bool status = item->data(Qt::UserRole).toBool();

    contextMenu.addAction(tr("Remove"),this,SLOT(removeWhiteListedIp()));

    QString addr_string = ui.whiteListIpsTable->item(row,COLUMN_RANGE)->text() ;

    sockaddr_storage addr ;
    int masked_bytes ;

    if(!RsNetUtil::parseAddrFromQString(addr_string,addr,masked_bytes))
    {
        std::cerr <<"Cannot parse IP \"" << addr_string.toStdString() << "\"" << std::endl;
        return ;
    }

//    QString range0 = RsNetUtil::printAddrRange(addr,0) ;
//    QString range1 = RsNetUtil::printAddrRange(addr,1) ;
//    QString range2 = RsNetUtil::printAddrRange(addr,2) ;
//
//    contextMenu.addAction(QObject::tr("Whitelist only IP "          )+range0,this,SLOT(enableBannedIp()))->setEnabled(false) ;
//#warning UNIMPLEMENTED CODE
//    contextMenu.addAction(QObject::tr("Whitelist entire range ")+range1,this,SLOT(enableBannedIp()))->setEnabled(false) ;
//    contextMenu.addAction(QObject::tr("Whitelist entire range ")+range2,this,SLOT(enableBannedIp()))->setEnabled(false) ;

    contextMenu.exec(QCursor::pos()) ;
}

void ServerPage::removeBannedIp()
{
    sockaddr_storage addr;
    int bytes ;

    removeCurrentRowFromBlackList(addr,bytes) ;
}

void ServerPage::removeWhiteListedIp()
{
    sockaddr_storage addr;
    int bytes ;

    removeCurrentRowFromWhiteList(addr,bytes) ;
}

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

    loadFilteredIps() ;

	updateStatusSam();

	// this is used by SAM
    if (mOngoingConnectivityCheck > 0) {
        mOngoingConnectivityCheck--;

        if (mOngoingConnectivityCheck == 0) {
            updateInProxyIndicatorResult(false);
            mOngoingConnectivityCheck = -1;
        }
    }

    if (mIsHiddenNode) {
        updateStatusHiddenNode();
        return;
    }

	/* load up configuration from rsPeers */
	RsPeerDetails detail;
	if (!rsPeers->getPeerDetails(rsPeers->getOwnId(), detail))
	{
		std::cerr << __PRETTY_FUNCTION__ << " getPeerDetails(...) failed!"
		          << " This is unexpected report to developers please."
		          << std::endl;
		return;
	}

    /* only update if can't edit */
    if (!ui.localPort->isEnabled())
    {
        /* set local address */
        whileBlocking(ui.localPort) -> setValue(detail.localPort);
        whileBlocking(ui.extPort) -> setValue(detail.extPort);
    }

    /* set local address */
    whileBlocking(ui.localAddress)->setText(QString::fromStdString(detail.localAddr));
    /* set the server address */
    whileBlocking(ui.extAddress)->setText(QString::fromStdString(detail.extAddr));


    // Now update network bits.
    RsConfigNetStatus net_status;
    rsConfig->getConfigNetStatus(net_status);

    /******* Network Status Tab *******/

    if(net_status.netUpnpOk)
        ui.iconlabel_upnp->setPixmap(FilesDefs::getPixmapFromQtResourcePath(":/images/ledon1.png"));
    else
        ui.iconlabel_upnp->setPixmap(FilesDefs::getPixmapFromQtResourcePath(":/images/ledoff1.png"));

    if (net_status.netLocalOk)
        ui.iconlabel_netLimited->setPixmap(FilesDefs::getPixmapFromQtResourcePath(":/images/ledon1.png"));
    else
        ui.iconlabel_netLimited->setPixmap(FilesDefs::getPixmapFromQtResourcePath(":/images/ledoff1.png"));

    if (net_status.netExtAddressOk)
        ui.iconlabel_ext->setPixmap(FilesDefs::getPixmapFromQtResourcePath(":/images/ledon1.png"));
    else
        ui.iconlabel_ext->setPixmap(FilesDefs::getPixmapFromQtResourcePath(":/images/ledoff1.png"));

    if (ui.ipAddressList->isEnabled() )
	{
		whileBlocking(ui.ipAddressList)->clear();
		detail.ipAddressList.sort();
		for(auto& it : detail.ipAddressList)
			whileBlocking(ui.ipAddressList)->addItem(QString::fromStdString(it).replace("sec",tr("sec")).replace("loc",tr("local")).replace("ext",tr("external")));
	}

		QString toolTip = tr("List of OpenDns servers used.");
	if (ui.IPServersLV->isEnabled() )
	{
		std::list<std::string> ip_list;
		rsPeers->getCurrentExtIPList(ip_list);
		if ( !ip_list.empty() )
		{
			toolTip += tr("\n\nList of found external IP:\n");
			for(std::list<std::string>::const_iterator it(ip_list.begin());it!=ip_list.end();++it)
				toolTip += "  " + QString::fromStdString(*it) +"\n" ;
		}
	}
	if(ui.IPServersLV->toolTip() != toolTip)
		ui.IPServersLV->setToolTip(toolTip);
}

void ServerPage::toggleUPnP()
{
    /* switch on the radioButton */
    bool settingChangeable = false;
    if (0 != ui.netModeComboBox->currentIndex())
    {
        settingChangeable = true;
    }

	// Shouldn't we use readOnly instead of enabled??

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
    bool saveAddr = false;

    saveCommon();

	if(ui.tabWidget->currentIndex() == TAB_HIDDEN_SERVICE) // hidden services tab
		updateOutProxyIndicator();

    if (mIsHiddenNode) {
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

    load();
}

void ServerPage::saveRates()
{
    rsConfig->SetMaxDataRates( ui.totalDownloadRate->value(), ui.totalUploadRate->value() );
}

void ServerPage::tabChanged(int page)
{
	if(page == TAB_HIDDEN_SERVICE)
		updateOutProxyIndicator();
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
     *  External Address ==> Tor Address: 17621376587.onion + PORT.
     *
     *  Known / Previous IPs: empty / removed.
     *  Ask about IP: Disabled.
     */

    // FIXED.
    //ui.netModeComboBox->setCurrentIndex(3);
    ui.netModeComboBox->hide();
    ui.textlabel_upnp->hide();
    ui.iconlabel_upnp->hide();
    ui.label_nat->hide();

	ui.info_warningBandwidth->hide();
	ui.iconlabel_netLimited->hide();
	ui.textlabel_netLimited->hide();
	ui.iconlabel_ext->hide();
	ui.textlabel_ext->hide();
	ui.extPortLabel->hide();
	
	ui.ipAddressLabel->hide();
	ui.cleanKnownIPs_PB->hide();
	
	ui.ipAddressList->hide();
	ui.allowIpDeterminationCB->hide();
	ui.IPServersLV->hide();
	
    ui.textlabel_hiddenMode->show();
    ui.iconlabel_hiddenMode->show() ;
    ui.iconlabel_hiddenMode->setPixmap(FilesDefs::getPixmapFromQtResourcePath(":/images/ledon1.png"));
    
    // CHANGE OPTIONS ON
    whileBlocking(ui.discComboBox)->removeItem(3);
    whileBlocking(ui.discComboBox)->removeItem(2);
    whileBlocking(ui.discComboBox)->removeItem(1);
    whileBlocking(ui.discComboBox)->removeItem(0);
    whileBlocking(ui.discComboBox)->insertItem (0, tr("Discovery On (recommended)"));
    whileBlocking(ui.discComboBox)->insertItem (1, tr("Discovery Off"));

    int netIndex = 1; // OFF.
    if (detail.vs_disc != RS_VS_DISC_OFF)
    {
        netIndex = 0; // DISC ON;
    }
    whileBlocking(ui.discComboBox)->setCurrentIndex(netIndex);

    // Download Rates - Stay the same as before.
    int dlrate = 0;
    int ulrate = 0;
    rsConfig->GetMaxDataRates(dlrate, ulrate);
    whileBlocking(ui.totalDownloadRate)->setValue(dlrate);
    whileBlocking(ui.totalUploadRate)->setValue(ulrate);

    // Addresses.
    ui.localAddress->setEnabled(false);
    ui.localPort  -> setEnabled(false);
    ui.extAddress -> setEnabled(false);
    ui.extPort    -> setVisible(false);
    ui.label_dynDNS->setVisible(false);
    ui.dynDNS      ->setVisible(false);

    ui.hiddenServiceTab->setTabEnabled(TAB_HIDDEN_SERVICE_INCOMING, true);

    /* Addresses must be set here - otherwise can't edit it */
        /* set local address */
    whileBlocking(ui.localAddress)->setText(QString::fromStdString(detail.localAddr));
    whileBlocking(ui.localPort )-> setValue(detail.localPort);
        /* set the server address */

    whileBlocking(ui.extAddress)->setText(tr("Hidden - See Config"));

    //ui._turtle_enabled_CB->setChecked(rsTurtle->enabled()) ;

    // show what we have in ipAddresses. (should be nothing!)
    ui.ipAddressList->clear();
    detail.ipAddressList.sort();
    for(std::list<std::string>::const_iterator it(detail.ipAddressList.begin());it!=detail.ipAddressList.end();++it)
        whileBlocking(ui.ipAddressList)->addItem(QString::fromStdString(*it));

    ui.iconlabel_upnp->setPixmap(FilesDefs::getPixmapFromQtResourcePath(":/images/ledoff1.png"));
    ui.iconlabel_netLimited->setPixmap(FilesDefs::getPixmapFromQtResourcePath(":/images/ledoff1.png"));
    ui.iconlabel_ext->setPixmap(FilesDefs::getPixmapFromQtResourcePath(":/images/ledoff1.png"));

    whileBlocking(ui.allowIpDeterminationCB)->setChecked(false);
    whileBlocking(ui.allowIpDeterminationCB)->setEnabled(false);
    whileBlocking(ui.IPServersLV)->setEnabled(false);

    /* TOR PAGE SETTINGS */

    /* set local address */
    ui.hiddenpage_localAddress->setEnabled(false);
    whileBlocking(ui.hiddenpage_localAddress)->setText(QString::fromStdString(detail.localAddr));
    whileBlocking(ui.hiddenpage_localPort) -> setValue(detail.localPort);

    /* set the server address */
    whileBlocking(ui.hiddenpage_serviceAddress)->setText(QString::fromStdString(detail.hiddenNodeAddress));
    whileBlocking(ui.hiddenpage_servicePort) -> setValue(detail.hiddenNodePort);
    /* in I2P there is no port - there is only the address */
    whileBlocking(ui.hiddenpage_servicePort)->setHidden(detail.hiddenType == RS_HIDDEN_TYPE_I2P);

    /* out proxy settings */
    std::string proxyaddr;
    uint16_t proxyport;
    uint32_t status ;
    // Tor
    rsPeers->getProxyServer(RS_HIDDEN_TYPE_TOR, proxyaddr, proxyport, status);
    whileBlocking(ui.hiddenpage_proxyAddress_tor) -> setText(QString::fromStdString(proxyaddr));
    whileBlocking(ui.hiddenpage_proxyPort_tor) -> setValue(proxyport);
    // I2P
    rsPeers->getProxyServer(RS_HIDDEN_TYPE_I2P, proxyaddr, proxyport, status);
    whileBlocking(ui.hiddenpage_proxyAddress_i2p) -> setText(QString::fromStdString(proxyaddr));
    whileBlocking(ui.hiddenpage_proxyPort_i2p) -> setValue(proxyport);

    QString expected = "";
    switch (mHiddenType) {
    case RS_HIDDEN_TYPE_I2P:
        ui.l_serviceAddress->setText(tr("I2P Address"));
        ui.l_incomingTestResult->setText(tr("I2P incoming ok"));

        expected += "http://127.0.0.1:7657/i2ptunnelmgr - I2P Hidden Services\n";
        expected += tr("Points at: ");
        expected += QString::fromStdString(detail.localAddr);
        expected += ":";
        expected += QString::number(detail.localPort);
        break;
    case RS_HIDDEN_TYPE_TOR:
        ui.l_serviceAddress->setText(tr("Onion Address"));
        ui.l_incomingTestResult->setText(tr("Tor incoming ok"));

        expected += "HiddenServiceDir </your/path/to/hidden/directory/service>\n";
        expected += "HiddenServicePort ";
        expected += QString::number(detail.hiddenNodePort);
        expected += " ";
        expected += QString::fromStdString(detail.localAddr);
        expected += ":";
        expected += QString::number(detail.localPort);
        break;
    default:
        ui.l_serviceAddress->setText(tr("Service Address"));
        ui.l_incomingTestResult->setText(tr("incoming ok"));

        expected += "Please fill in a service address";

        break;
    }
    whileBlocking(ui.hiddenpage_configuration)->setPlainText(expected);
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
        ui.iconlabel_upnp->setPixmap(FilesDefs::getPixmapFromQtResourcePath(":/images/ledon1.png"));
    else
        ui.iconlabel_upnp->setPixmap(FilesDefs::getPixmapFromQtResourcePath(":/images/ledoff1.png"));

    if (net_status.netLocalOk)
        ui.iconlabel_netLimited->setPixmap(FilesDefs::getPixmapFromQtResourcePath(":/images/ledon1.png"));
    else
        ui.iconlabel_netLimited->setPixmap(FilesDefs::getPixmapFromQtResourcePath(":/images/ledoff1.png"));

    if (net_status.netExtAddressOk)
        ui.iconlabel_ext->setPixmap(FilesDefs::getPixmapFromQtResourcePath(":/images/ledon1.png"));
    else
        ui.iconlabel_ext->setPixmap(FilesDefs::getPixmapFromQtResourcePath(":/images/ledoff1.png"));

#endif
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

    if (detail.localPort != ui.hiddenpage_localPort->value())
    {
        // Set Local Address - force to 127.0.0.1
        rsPeers->setLocalAddress(ownId, "127.0.0.1", ui.hiddenpage_localPort->value());
    }

    std::string hiddenAddr = ui.hiddenpage_serviceAddress->text().toStdString();
    uint16_t    hiddenPort = ui.hiddenpage_servicePort->value();
    if ((hiddenAddr != detail.hiddenNodeAddress) || (hiddenPort != detail.hiddenNodePort))
    {
        rsPeers->setHiddenNode(ownId, hiddenAddr, hiddenPort);
    }

    rsConfig->SetMaxDataRates( ui.totalDownloadRate->value(), ui.totalUploadRate->value() );
    load();
}

void ServerPage::updateOutProxyIndicator()
{
    QTcpSocket socket ;

    // Tor
    socket.connectToHost(ui.hiddenpage_proxyAddress_tor->text(),ui.hiddenpage_proxyPort_tor->text().toInt());
    if(socket.waitForConnected(500))
    {
        socket.disconnectFromHost();
        ui.iconlabel_tor_outgoing->setPixmap(FilesDefs::getPixmapFromQtResourcePath(ICON_STATUS_OK)) ;
        ui.iconlabel_tor_outgoing->setToolTip(tr("Proxy seems to work.")) ;
    }
    else
    {
        ui.iconlabel_tor_outgoing->setPixmap(FilesDefs::getPixmapFromQtResourcePath(ICON_STATUS_UNKNOWN)) ;
        ui.iconlabel_tor_outgoing->setToolTip(tr("Tor proxy is not enabled")) ;
    }

	// I2P - SAM
	// Note: there is only "the SAM port", there is no additional proxy port!
	samStatus ss;
	rsAutoProxyMonitor::taskSync(autoProxyType::I2PSAM3, autoProxyTask::status, &ss);
	if(ss.state == samStatus::samState::online)
	{
		socket.disconnectFromHost();
		ui.iconlabel_i2p_outgoing->setPixmap(FilesDefs::getPixmapFromQtResourcePath(ICON_STATUS_OK)) ;
		ui.iconlabel_i2p_outgoing->setToolTip(tr("Proxy seems to work.")) ;
	}
	else
	{
		ui.iconlabel_i2p_outgoing->setPixmap(FilesDefs::getPixmapFromQtResourcePath(ICON_STATUS_UNKNOWN)) ;
		ui.iconlabel_i2p_outgoing->setToolTip(tr("I2P proxy is not enabled")) ;
	}

	socket.connectToHost(ui.hiddenpage_proxyAddress_i2p_2->text(), 7656);
	if(true == (mSamAccessible = socket.waitForConnected(1000)))
    {
        socket.disconnectFromHost();
        ui.iconlabel_i2p_outgoing_2->setPixmap(FilesDefs::getPixmapFromQtResourcePath(ICON_STATUS_OK)) ;
		ui.iconlabel_i2p_outgoing_2->setToolTip(tr("SAMv3 is running and accessible")) ;
    }
    else
    {
        ui.iconlabel_i2p_outgoing_2->setPixmap(FilesDefs::getPixmapFromQtResourcePath(ICON_STATUS_UNKNOWN)) ;
		ui.iconlabel_i2p_outgoing_2->setToolTip(tr("SAMv3 is not accessible! Is i2p running and SAM enabled?")) ;
    }
}

void ServerPage::updateInProxyIndicator()
{
    // need to find a proper way to do this

    if(!mIsHiddenNode)
        return ;

	//ui.iconlabel_tor_incoming->setPixmap(FilesDefs::getPixmapFromQtResourcePath(ICON_STATUS_UNKNOWN)) ;
    //ui.testIncomingTor_PB->setIcon(FilesDefs::getIconFromQtResourcePath(":/loader/circleball-16.gif")) ;
    QMovie *movie = new QMovie(":/images/loader/circleball-16.gif");
    ui.iconlabel_service_incoming->setMovie(movie);
    movie->start();

	if (mHiddenType == RS_HIDDEN_TYPE_I2P && mSamSettings.enable) {
		// there is no inproxy for SAMv3, since every connection goes through sam itself
		auto secw = new samEstablishConnectionWrapper();
		secw->address = mSamSettings.address;
		secw->connection = nullptr;
		rsAutoProxyMonitor::taskAsync(autoProxyType::I2PSAM3, autoProxyTask::establishConnection, this, secw);

        return;
    }

    if(manager == NULL) {
        manager = new  QNetworkAccessManager(this);
        connect(manager, SIGNAL(finished(QNetworkReply*)),this,SLOT(handleNetworkReply(QNetworkReply*))) ;
    }

    QNetworkProxy proxy ;

    proxy.setType(QNetworkProxy::Socks5Proxy);

	proxy.setHostName(ui.hiddenpage_proxyAddress_tor->text());
	proxy.setPort(ui.hiddenpage_proxyPort_tor->text().toInt());

    proxy.setCapabilities(QNetworkProxy::HostNameLookupCapability | proxy.capabilities()) ;

    QNetworkProxy::setApplicationProxy(proxy) ;

    QUrl url("https://"+ui.hiddenpage_serviceAddress->text() + ":" + ui.hiddenpage_servicePort->text());

    std::cerr << "Setting proxy hostname+port to " << std::dec << ui.hiddenpage_proxyAddress_tor->text().toStdString() << ":" << ui.hiddenpage_proxyPort_tor->text().toInt() << std::endl;
    std::cerr << "Connecting to " << url.toString().toStdString() << std::endl;

    manager->get( QNetworkRequest(url) ) ;

    QNetworkProxy::setApplicationProxy(QNetworkProxy::NoProxy) ;
}

void ServerPage::startSam()
{
	rsAutoProxyMonitor::taskAsync(autoProxyType::I2PSAM3, autoProxyTask::start);

    updateStatus();
}

void ServerPage::restartSam()
{
	rsAutoProxyMonitor::taskAsync(autoProxyType::I2PSAM3, autoProxyTask::stop);
	rsAutoProxyMonitor::taskAsync(autoProxyType::I2PSAM3, autoProxyTask::start);

    updateStatus();
}

void ServerPage::stopSam()
{
	rsAutoProxyMonitor::taskAsync(autoProxyType::I2PSAM3, autoProxyTask::stop);

    updateStatus();
}

void ServerPage::getNewKey()
{
	i2p::address *addr = new i2p::address();
	rsAutoProxyMonitor::taskAsync(autoProxyType::I2PSAM3, autoProxyTask::receiveKey, this, addr);
}

void ServerPage::loadKey()
{
	auto priv = ui.pteBobServerKey->toPlainText().toStdString();
	auto pub = i2p::publicKeyFromPrivate(priv);
	if (pub.empty()) {
		// something went wrong!
		ui.pteBobServerKey->setPlainText("FAILED! Something went wrong while parsing the key!");
		return;
	}

	mSamSettings.address.privateKey = priv;
	mSamSettings.address.publicKey = pub;
	mSamSettings.address.base32 = i2p::keyToBase32Addr(mSamSettings.address.publicKey);

	rsAutoProxyMonitor::taskSync(autoProxyType::I2PSAM3, autoProxyTask::setSettings, &mSamSettings);
}

void ServerPage::enableSam(bool checked)
{
	mSamSettings.enable = checked;

	rsAutoProxyMonitor::taskSync(autoProxyType::I2PSAM3, autoProxyTask::setSettings, &mSamSettings);

	setUpSamElements();
}

int8_t fitRange(int i, int min, int max) {
    if (i < min)
        i = min;
    else if (i > max)
        i = max;

    return (int8_t)i;
}

void ServerPage::tunnelSettingsChanged(int)
{
    int li, lo, qi, qo, vi, vo;
    li = ui.sbBobLengthIn->value();
    lo = ui.sbBobLengthOut->value();
    qi = ui.sbBobQuantityIn->value();
    qo = ui.sbBobQuantityOut->value();
    vi = ui.sbBobVarianceIn->value();
    vo = ui.sbBobVarianceOut->value();

	mSamSettings.inLength    = fitRange(li, 0, 7);
	mSamSettings.outLength   = fitRange(lo, 0, 7);
	mSamSettings.inQuantity  = fitRange(qi, 1, 16);
	mSamSettings.outQuantity = fitRange(qo, 1, 16);
	mSamSettings.inVariance  = fitRange(vi, -1, 2);
	mSamSettings.outVariance = fitRange(vo, -1, 2);

	rsAutoProxyMonitor::taskSync(autoProxyType::I2PSAM3, autoProxyTask::setSettings, &mSamSettings);
}

void ServerPage::toggleSamAdvancedSettings(bool checked)
{
    ui.swBobAdvanced->setCurrentIndex(checked ? 1 : 0);

	if (!mSamSettings.address.privateKey.empty()) {
        if (checked) {
            ui.pbBobGenAddr->show();
        } else {
            ui.pbBobGenAddr->hide();
        }
    }
}

void ServerPage::syncI2PProxyAddrNormal(QString t)
{
    ui.hiddenpage_proxyAddress_i2p_2->setText(t);
}

void ServerPage::syncI2PProxyAddrSam(QString t)
{
    ui.hiddenpage_proxyAddress_i2p->setText(t);

    // update addr
	saveSam();
	rsAutoProxyMonitor::taskSync(autoProxyType::I2PSAM3, autoProxyTask::reloadConfig);
}

void ServerPage::taskFinished(taskTicket *&ticket)
{
	switch (ticket->task) {
	case autoProxyTask::receiveKey:
	{
		i2p::address *addr = nullptr;
		addr = static_cast<i2p::address *>(ticket->data);

		if (ticket->types.front() != autoProxyType::I2PSAM3)
			RS_WARN("auto proxy task finished but not for SMA, not exptected! Also not a serious problem.");
		else {
			// update settings
			auto copy = *addr;
			RsQThreadUtils::postToObject(
			            [this, copy]()
			{
				mSamSettings.address = copy;
				rsAutoProxyMonitor::taskSync(autoProxyType::I2PSAM3, autoProxyTask::setSettings, &mSamSettings);
				updateStatusSam();

			});
		}

		delete addr;
		addr = nullptr;
		ticket->data = nullptr;


		break;
	}
	case autoProxyTask::establishConnection:
	{
		samEstablishConnectionWrapper *secw = nullptr;
		secw = static_cast<samEstablishConnectionWrapper *>(ticket->data);

		if (ticket->types.front() != autoProxyType::I2PSAM3)
			RS_WARN("auto proxy task finished but not for SMA, not exptected! Also not a serious problem.");
		else {
			bool res = ticket->result == autoProxyStatus::ok && !!secw->connection->ses;
			// update settings
			if (res) {
				sam3CloseConnection(secw->connection);
				secw->connection = nullptr; // freed by above call
			}

			RsQThreadUtils::postToObject(
			            [this, res]()
			{
				updateInProxyIndicatorResult(res);

			});
		}

		if (secw->connection)
			delete secw->connection;
		delete secw;
		secw = nullptr;
		ticket->data = nullptr;
		break;
	}
	default:
		RS_DBG("unsupported task!", ticket->task);
	}

    if (ticket->data)
        std::cerr << "(WW) ServerPage::taskFinished data set. This should NOT happen - check the code!" << std::endl;

    delete ticket;
	ticket = nullptr;
}

void ServerPage::connectionWithoutCert()
{
    if (mOngoingConnectivityCheck > 0) {
        mOngoingConnectivityCheck = -1;
        updateInProxyIndicatorResult(true);
    }
}

void ServerPage::loadCommon()
{
    /* HIDDEN PAGE SETTINGS - only Proxy (outgoing) */
    /* out proxy settings */
    std::string proxyaddr;
    uint16_t proxyport;
    uint32_t status ;

    // Tor
    rsPeers->getProxyServer(RS_HIDDEN_TYPE_TOR, proxyaddr, proxyport, status);
    whileBlocking(ui.hiddenpage_proxyAddress_tor)->setText(QString::fromStdString(proxyaddr));
    whileBlocking(ui.hiddenpage_proxyPort_tor)->setValue(proxyport);

    // I2P
    rsPeers->getProxyServer(RS_HIDDEN_TYPE_I2P, proxyaddr, proxyport, status);
    whileBlocking(ui.hiddenpage_proxyAddress_i2p) -> setText(QString::fromStdString(proxyaddr));
	whileBlocking(ui.hiddenpage_proxyAddress_i2p_2)->setText(QString::fromStdString(proxyaddr)); // this one is for sam tab
    whileBlocking(ui.hiddenpage_proxyPort_i2p) -> setValue(proxyport);

    // don't use whileBlocking here
	ui.cb_enableBob->setChecked(mSamSettings.enable);

	if (!mSamSettings.address.privateKey.empty()) {
        ui.lBobB32Addr->show();
        ui.leBobB32Addr->show();
    }
}

void ServerPage::saveCommon()
{
    // HANDLE PROXY SERVER.
    std::string orig_proxyaddr, new_proxyaddr;
    uint16_t orig_proxyport, new_proxyport;
    uint32_t status ;
    // Tor
    rsPeers->getProxyServer(RS_HIDDEN_TYPE_TOR, orig_proxyaddr, orig_proxyport, status);

    new_proxyaddr = ui.hiddenpage_proxyAddress_tor -> text().toStdString();
    new_proxyport = ui.hiddenpage_proxyPort_tor -> value();

    if ((new_proxyaddr != orig_proxyaddr) || (new_proxyport != orig_proxyport)) {
        rsPeers->setProxyServer(RS_HIDDEN_TYPE_TOR, new_proxyaddr, new_proxyport);
    }

	saveSam();
}

void ServerPage::saveSam()
{
    std::string orig_proxyaddr, new_proxyaddr;
    uint16_t orig_proxyport, new_proxyport;
    uint32_t status;
    // I2P
    rsPeers->getProxyServer(RS_HIDDEN_TYPE_I2P, orig_proxyaddr, orig_proxyport, status);

    new_proxyaddr = ui.hiddenpage_proxyAddress_i2p -> text().toStdString();
    new_proxyport = ui.hiddenpage_proxyPort_i2p -> value();

	// SAMv3 has no proxy port, everything goes through the SAM port.
	if ((new_proxyaddr != orig_proxyaddr) /* || (new_proxyport != orig_proxyport) */) {
        rsPeers->setProxyServer(RS_HIDDEN_TYPE_I2P, new_proxyaddr, new_proxyport);
    }
}

void ServerPage::updateStatusSam()
{
	QString addr = QString::fromStdString(mSamSettings.address.base32);
    if (ui.leBobB32Addr->text() != addr) {
        ui.leBobB32Addr->setText(addr);
        ui.hiddenpage_serviceAddress->setText(addr);
		ui.pteBobServerKey->setPlainText(QString::fromStdString(mSamSettings.address.privateKey));

		std::string signingKeyType, cryptoKeyType;
		if (i2p::getKeyTypes(mSamSettings.address.publicKey, signingKeyType, cryptoKeyType))
			ui.samKeyInfo->setText(tr("Your key uses the following algorithms: %1 and %2").
			                       arg(QString::fromStdString(signingKeyType)).
			                       arg(QString::fromStdString(cryptoKeyType)));
		else
			ui.samKeyInfo->setText(tr("unkown key type"));

		if (!mSamSettings.address.privateKey.empty()) {
            // we have an addr -> show fields
            ui.lBobB32Addr->show();
            ui.leBobB32Addr->show();

            if (ui.cbBobAdvanced->checkState() == Qt::Checked) {
                ui.pbBobGenAddr->show();
            } else {
                ui.pbBobGenAddr->hide();
            }
        } else {
            // we don't have an addr -> hide fields
            ui.lBobB32Addr->hide();
            ui.leBobB32Addr->hide();
            ui.pbBobGenAddr->hide();
        }

		saveAddresses();
    }

	samStatus ss;
	rsAutoProxyMonitor::taskSync(autoProxyType::I2PSAM3, autoProxyTask::status, &ss);

    QString bobSimpleText = QString();
	bobSimpleText.append(tr("RetroShare uses SAMv3 to set up a %1 tunnel at %2:%3\n(id: %4)\n\n"
	                        "When changing options use the buttons at the bottom to restart SAMv3.\n\n").
	                     arg(mSamSettings.address.privateKey.empty() ? tr("client") : tr("server"),
                             ui.hiddenpage_proxyAddress_i2p_2->text(),
	                         "7656",
	                         ss.sessionName.empty() ? tr("unknown") :
	                                                  QString::fromStdString(ss.sessionName)));

	// update SAM UI based on state
	QString s;
	QString icon;
	switch (ss.state) {
	case samStatus::samState::offline:
		enableSamElements(false);

		ui.pbBobStart->setEnabled(true);
		ui.pbBobRestart->setEnabled(false);
		ui.pbBobStop->setEnabled(false);

		s = tr("Offline, no SAM session is established yet.\n");
		icon = ICON_STATUS_ERROR;
		break;
	case samStatus::samState::connectSession:
		enableSamElements(false);

		ui.pbBobStart->setEnabled(false);
		ui.pbBobRestart->setEnabled(false);
		ui.pbBobStop->setEnabled(true);

		s = tr("SAM is trying to establish a session ... this can take some time.\n");
		icon = ICON_STATUS_WORKING;
		break;
	case samStatus::samState::connectForward:
		enableSamElements(false);

		ui.pbBobStart->setEnabled(false);
		ui.pbBobRestart->setEnabled(false);
		ui.pbBobStop->setEnabled(true);

		s = tr("SAM session established! Now setting up a forward session ...\n");
		icon = ICON_STATUS_WORKING;
		break;
	case samStatus::samState::online:
		enableSamElements(true);

		ui.pbBobStart->setEnabled(false);
		ui.pbBobRestart->setEnabled(true);
		ui.pbBobStop->setEnabled(true);

		s = tr("Online, SAM is working as exptected\n");
		icon = ICON_STATUS_OK;
		break;
	}
	ui.iconlabel_i2p_bob->setPixmap(icon);
	ui.iconlabel_i2p_bob->setToolTip(s);
	bobSimpleText.append(s);


	ui.info_BobSimple->setPlainText(bobSimpleText);


    // disable elements when BOB is not accessible
	if (!mSamAccessible) {
        ui.pbBobStart->setEnabled(false);
		ui.pbBobStart->setToolTip("SAMv3 is not accessible");
        ui.pbBobRestart->setEnabled(false);
		ui.pbBobRestart->setToolTip("SAMv3 is not accessible");
		// don't disable the stop button! (in case SAM is running you are otherwise unable to stop and disable it)
		ui.pbBobStop->setToolTip("SAMv3 is not accessible");
    } else {
        ui.pbBobStart->setToolTip("");
        ui.pbBobRestart->setToolTip("");
        ui.pbBobStop->setToolTip("");
    }
}

void ServerPage::setUpSamElements()
{
	ui.gbBob->setEnabled(mSamSettings.enable);
	if (mSamSettings.enable) {
        ui.hiddenpage_proxyAddress_i2p->setEnabled(false);
		ui.hiddenpage_proxyAddress_i2p->setToolTip("Use I2P settings to change this value");
        ui.hiddenpage_proxyPort_i2p->setEnabled(false);
		ui.hiddenpage_proxyPort_i2p->setToolTip("Use I2P settings to change this value");

		ui.leBobB32Addr->setText(QString::fromStdString(mSamSettings.address.base32));
		ui.pteBobServerKey->setPlainText(QString::fromStdString(mSamSettings.address.privateKey));

		std::string signingKeyType, cryptoKeyType;
		if (i2p::getKeyTypes(mSamSettings.address.publicKey, signingKeyType, cryptoKeyType))
			ui.samKeyInfo->setText(tr("You key uses %1 for signing and %2 for crypto").
			                       arg(QString::fromStdString(signingKeyType)).
			                       arg(QString::fromStdString(cryptoKeyType)));
		else
			ui.samKeyInfo->setText(tr("unkown key type"));

        // cast to int to avoid problems
        int li, lo, qi, qo, vi, vo;
		li = mSamSettings.inLength;
		lo = mSamSettings.outLength;
		qi = mSamSettings.inQuantity;
		qo = mSamSettings.outQuantity;
		vi = mSamSettings.inVariance;
		vo = mSamSettings.outVariance;

        ui.sbBobLengthIn   ->setValue(li);
        ui.sbBobLengthOut  ->setValue(lo);
        ui.sbBobQuantityIn ->setValue(qi);
        ui.sbBobQuantityOut->setValue(qo);
        ui.sbBobVarianceIn ->setValue(vi);
        ui.sbBobVarianceOut->setValue(vo);
    } else {
        ui.hiddenpage_proxyAddress_i2p->setEnabled(true);
        ui.hiddenpage_proxyAddress_i2p->setToolTip(QString());
        ui.hiddenpage_proxyPort_i2p->setEnabled(true);
        ui.hiddenpage_proxyPort_i2p->setToolTip(QString());
    }
}

void ServerPage::enableSamElements(bool enable)
{
    if (enable) {
        ui.pbBobGenAddr->setEnabled(true);
        ui.pbBobGenAddr->setToolTip(tr("request a new server key"));

        ui.pbBobLoadKey->setEnabled(true);
        ui.pbBobLoadKey->setToolTip(tr("load server key from base64"));

        ui.cb_enableBob->setEnabled(true);
        ui.cb_enableBob->setToolTip(tr(""));
    } else {
        ui.pbBobGenAddr->setEnabled(false);
		ui.pbBobGenAddr->setToolTip(tr("stop SAM tunnel first to generate a new key"));

        ui.pbBobLoadKey->setEnabled(false);
		ui.pbBobLoadKey->setToolTip(tr("stop SAM tunnel first to load a key"));

        ui.cb_enableBob->setEnabled(false);
		ui.cb_enableBob->setToolTip(tr("stop SAM tunnel first to disable SAM"));
    }
}

void ServerPage::updateInProxyIndicatorResult(bool success)
{
    if (success) {
        std::cerr <<"Connected!" << std::endl;

        ui.iconlabel_service_incoming->setPixmap(FilesDefs::getPixmapFromQtResourcePath(ICON_STATUS_OK)) ;
        ui.iconlabel_service_incoming->setToolTip(tr("You are reachable through the hidden service.")) ;
        //ui.testIncomingTor_PB->setIcon(FilesDefs::getIconFromQtResourcePath(ICON_STATUS_OK)) ;
    } else {
        std::cerr <<"Failed!" << std::endl;

        //ui.testIncomingTor_PB->setIcon(FilesDefs::getIconFromQtResourcePath(ICON_STATUS_UNKNOWN)) ;
        ui.iconlabel_service_incoming->setPixmap(FilesDefs::getPixmapFromQtResourcePath(ICON_STATUS_UNKNOWN)) ;
        ui.iconlabel_service_incoming->setToolTip(tr("The proxy is not enabled or broken.\nAre all services up and running fine??\nAlso check your ports!")) ;
    }
    // delete movie
    delete ui.iconlabel_service_incoming->movie();
}

void ServerPage::handleNetworkReply(QNetworkReply *reply)
{
    int error = reply->error() ;

	RS_INFO("error:", error);
    if(reply->isOpen() &&  error == QNetworkReply::SslHandshakeFailedError)
        updateInProxyIndicatorResult(true);
    else
        updateInProxyIndicatorResult(false);

    reply->close();
}

//#####################################################################
//## Relay Tab
//#####################################################################

void ServerPage::updateTotals()
{
	int nFriends = ui.noFriendSpinBox->value();
	int friendBandwidth = ui.bandFriendSpinBox->value();

	int nFOF = ui.noFOFSpinBox->value();
	int fofBandwidth = ui.bandFOFSpinBox->value();

	int nGeneral = ui.noGeneralSpinBox->value();
	int genBandwidth = ui.bandGeneralSpinBox->value();

	int total = nFriends + nFOF + nGeneral;

	rsDht->setRelayAllowance(RsDhtRelayClass::ALL, total, 0);
	rsDht->setRelayAllowance(RsDhtRelayClass::FRIENDS, nFriends, 1024 * friendBandwidth);
	rsDht->setRelayAllowance(RsDhtRelayClass::FOF, nFOF, 1024 * fofBandwidth);
	rsDht->setRelayAllowance(RsDhtRelayClass::GENERAL, nGeneral, 1024 * genBandwidth);
}

/** Saves the changes on this page */

void ServerPage::updateRelayMode()
{
	RsDhtRelayMode relayMode = static_cast<RsDhtRelayMode>(0);
	if (ui.enableCheckBox->isChecked())
	{
		relayMode |= RsDhtRelayMode::ENABLED;

		if (ui.serverCheckBox->isChecked())
		{
			relayMode |= RsDhtRelayMode::ON;
		}
		else
		{
			relayMode |= RsDhtRelayMode::OFF;
		}
	}
	else
	{
		relayMode |= RsDhtRelayMode::OFF;
	}

	rsDht->setRelayMode(relayMode);
}

void ServerPage::loadServers()
{
	std::list<std::string> servers;
	std::list<std::string>::iterator it;

	rsDht->getRelayServerList(servers);

	ui.serverTreeWidget->clear();
	for(it = servers.begin(); it != servers.end(); ++it)
	{
		QTreeWidgetItem *item = new QTreeWidgetItem();
		item->setData(0, Qt::DisplayRole, QString::fromStdString(*it));
		ui.serverTreeWidget->addTopLevelItem(item);
	}
}

void ServerPage::updateRelayOptions()
{
	int nFriends = ui.noFriendSpinBox->value();
	int friendBandwidth = ui.bandFriendSpinBox->value();

	int nFOF = ui.noFOFSpinBox->value();
	int fofBandwidth = ui.bandFOFSpinBox->value();

	int nGeneral = ui.noGeneralSpinBox->value();
	int genBandwidth = ui.bandGeneralSpinBox->value();

	ui.totalFriendLineEdit->setText(QString::number(nFriends * friendBandwidth * 2));
	ui.totalFOFLineEdit->setText(QString::number(nFOF * fofBandwidth * 2));
	ui.totalGeneralLineEdit->setText(QString::number(nGeneral * genBandwidth * 2));
	ui.totalBandwidthLineEdit->setText(QString::number((nFriends * friendBandwidth + nFOF * fofBandwidth + nGeneral * genBandwidth) * 2));
	ui.noTotalLineEdit->setText(QString::number(nFriends + nFOF + nGeneral));
}

void ServerPage::updateEnabled()
{
	std::cerr << "RelayPage::updateEnabled()" << std::endl;

	if (ui.enableCheckBox->isChecked())
	{
		ui.relayOptionGBox->setEnabled(true);
		if (ui.serverCheckBox->isChecked())
		{
			std::cerr << "RelayPage::updateEnabled() Both Enabled" << std::endl;
			ui.serverGroupBox->setEnabled(true);
		}
		else
		{
			std::cerr << "RelayPage::updateEnabled() Options Only Enabled" << std::endl;
			ui.serverGroupBox->setEnabled(false);
		}
	}
	else
	{
		std::cerr << "RelayPage::updateEnabled() Both Disabled" << std::endl;
		ui.relayOptionGBox->setEnabled(false);
		ui.serverGroupBox->setEnabled(false);
	}

}

void ServerPage::checkKey()
{

	std::string server = ui.DhtLineEdit->text().toStdString();
	std::cerr << "RelayPage::checkKey() length: " << server.length();
	std::cerr << std::endl;
	if (server.length() == 40)
	{
		ui.keyOkBox->setChecked(true);
	}
	else
	{
		ui.keyOkBox->setChecked(false);
	}
}

void ServerPage::addServer()
{
	std::cerr << "RelayPage::addServer()";
	std::cerr << std::endl;

	if (!ui.keyOkBox->isChecked())
	{
		return;
	}

	std::string server = ui.DhtLineEdit->text().toStdString();

	bool ok = rsDht->addRelayServer(server);
	if (ok)
	{
		ui.DhtLineEdit->setText(QString(""));
	}
	loadServers();
}

void ServerPage::removeServer()
{
	QTreeWidgetItem *item = ui.serverTreeWidget->currentItem();
	if (item)
	{
		std::string server = item->data(0, Qt::DisplayRole).toString().toStdString();
		rsDht->removeRelayServer(server);
	}

	loadServers();
}
