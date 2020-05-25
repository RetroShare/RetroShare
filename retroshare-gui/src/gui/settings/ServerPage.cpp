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
#include "util/RsNetUtil.h"
#include "util/misc.h"

#include <iostream>

#include "retroshare/rsbanlist.h"
#include "retroshare/rsconfig.h"
#include "retroshare/rsdht.h"
#include "retroshare/rspeers.h"
#include "retroshare/rsturtle.h"
#include "retroshare/rsinit.h"

#include <QCheckBox>
#include <QMovie>
#include <QMenu>
#include <QTcpSocket>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QTimer>

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
const static uint32_t TAB_HIDDEN_SERVICE_OUTGOING = 0;
const static uint32_t TAB_HIDDEN_SERVICE_INCOMING = 1;
const static uint32_t TAB_HIDDEN_SERVICE_I2P_BOB  = 2;

const static uint32_t TAB_NETWORK                 = 0;
const static uint32_t TAB_HIDDEN_SERVICE          = 1;
const static uint32_t TAB_IP_FILTERS              = 2;
const static uint32_t TAB_RELAYS                  = 3;

//#define SERVER_DEBUG 1

ServerPage::ServerPage(QWidget * parent, Qt::WindowFlags flags)
    : ConfigPage(parent, flags), mIsHiddenNode(false), mHiddenType(RS_HIDDEN_TYPE_NONE)
{
  /* Invoke the Qt Designer generated object setup routine */
  ui.setupUi(this);

  manager = NULL ;
  mOngoingConnectivityCheck = -1;

#ifndef RS_USE_I2P_BOB
  ui.hiddenServiceTab->removeTab(TAB_HIDDEN_SERVICE_I2P_BOB);	// warning: the order of operation here is very important.
#endif

  if(RsAccounts::isHiddenNode())
  {
	  if(RsAccounts::isTorAuto())
	  {
		  // Here we use absolute numbers instead of consts defined above, because the consts correspond to the tab number *after* this tab removal.

		  ui.tabWidget->removeTab(TAB_RELAYS) ;		// remove relays. Not useful in Tor mode.
		  ui.tabWidget->removeTab(TAB_IP_FILTERS) ;	// remove IP filters. Not useful in Tor mode.

		  ui.hiddenpage_proxyAddress_i2p->hide() ;
		  ui.hiddenpage_proxyLabel_i2p->hide() ;
		  ui.hiddenpage_proxyPort_i2p->hide() ;
		  ui.label_i2p_outgoing->hide() ;
		  ui.iconlabel_i2p_outgoing->hide() ;
		  ui.plainTextEdit->hide() ;
		  ui.hiddenpage_configuration->hide() ;
		  ui.l_hiddenpage_configuration->hide() ;
		  ui.hiddenpageInHelpPlainTextEdit->hide() ;

		  ui.hiddenpage_outHeader->setText(tr("Tor has been automatically configured by Retroshare. You shouldn't need to change anything here.")) ;
		  ui.hiddenpage_inHeader->setText(tr("Tor has been automatically configured by Retroshare. You shouldn't need to change anything here.")) ;
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

    QObject::connect(ui.filteredIpsTable,SIGNAL(customContextMenuRequested(const QPoint&)),this,SLOT(ipFilterContextMenu(const QPoint&))) ;
    QObject::connect(ui.whiteListIpsTable,SIGNAL(customContextMenuRequested(const QPoint&)),this,SLOT(ipWhiteListContextMenu(const QPoint&))) ;
    QObject::connect(ui.denyAll_CB,SIGNAL(toggled(bool)),this,SLOT(toggleIpFiltering(bool)));
    QObject::connect(ui.includeFromDHT_CB,SIGNAL(toggled(bool)),this,SLOT(toggleAutoIncludeDHT(bool)));
    QObject::connect(ui.includeFromFriends_CB,SIGNAL(toggled(bool)),this,SLOT(toggleAutoIncludeFriends(bool)));
    QObject::connect(ui.groupIPRanges_CB,SIGNAL(toggled(bool)),this,SLOT(toggleGroupIps(bool)));
    QObject::connect(ui.groupIPRanges_SB,SIGNAL(valueChanged(int)),this,SLOT(setGroupIpLimit(int)));
    QObject::connect(ui.ipInputAddBlackList_PB,SIGNAL(clicked()),this,SLOT(addIpRangeToBlackList()));
    QObject::connect(ui.ipInputAddWhiteList_PB,SIGNAL(clicked()),this,SLOT(addIpRangeToWhiteList()));
    QObject::connect(ui.ipInput_LE,SIGNAL(textChanged(const QString&)),this,SLOT(checkIpRange(const QString&)));
    QObject::connect(ui.filteredIpsTable,SIGNAL(currentCellChanged(int,int,int,int)),this,SLOT(updateSelectedBlackListIP(int,int,int,int)));
    QObject::connect(ui.whiteListIpsTable,SIGNAL(currentCellChanged(int,int,int,int)),this,SLOT(updateSelectedWhiteListIP(int,int,int,int)));

    QObject::connect(ui.pbBobStart,   SIGNAL(clicked()), this, SLOT(startBOB()));
    QObject::connect(ui.pbBobRestart, SIGNAL(clicked()), this, SLOT(restartBOB()));
    QObject::connect(ui.pbBobStop,    SIGNAL(clicked()), this, SLOT(stopBOB()));
    QObject::connect(ui.pbBobGenAddr, SIGNAL(clicked()), this, SLOT(getNewKey()));
    QObject::connect(ui.pbBobLoadKey, SIGNAL(clicked()), this, SLOT(loadKey()));
    QObject::connect(ui.cb_enableBob, SIGNAL(toggled(bool)), this, SLOT(enableBob(bool)));

    QObject::connect(ui.cbBobAdvanced, SIGNAL(toggled(bool)), this, SLOT(toggleBobAdvancedSettings(bool)));

    QObject::connect(ui.sbBobLengthIn,    SIGNAL(valueChanged(int)), this, SLOT(tunnelSettingsChanged(int)));
    QObject::connect(ui.sbBobLengthOut,   SIGNAL(valueChanged(int)), this, SLOT(tunnelSettingsChanged(int)));
    QObject::connect(ui.sbBobQuantityIn,  SIGNAL(valueChanged(int)), this, SLOT(tunnelSettingsChanged(int)));
    QObject::connect(ui.sbBobQuantityOut, SIGNAL(valueChanged(int)), this, SLOT(tunnelSettingsChanged(int)));
    QObject::connect(ui.sbBobVarianceIn,  SIGNAL(valueChanged(int)), this, SLOT(tunnelSettingsChanged(int)));
    QObject::connect(ui.sbBobVarianceOut, SIGNAL(valueChanged(int)), this, SLOT(tunnelSettingsChanged(int)));

    // These two spin boxes are used for the same thing - keep them in sync!
    QObject::connect(ui.hiddenpage_proxyPort_i2p,   SIGNAL(valueChanged(int)), this, SLOT(syncI2PProxyPortNormal(int)));
    QObject::connect(ui.hiddenpage_proxyPort_i2p_2, SIGNAL(valueChanged(int)), this, SLOT(syncI2PProxyPortBob(int)));

    // These two line edits are used for the same thing - keep them in sync!
    QObject::connect(ui.hiddenpage_proxyAddress_i2p,   SIGNAL(textChanged(QString)), this, SLOT(syncI2PProxyAddrNormal(QString)));
    QObject::connect(ui.hiddenpage_proxyAddress_i2p_2, SIGNAL(textChanged(QString)), this, SLOT(syncI2PProxyAddrBob(QString)));

	connect(NotifyQt::getInstance(), SIGNAL(connectionWithoutCert()), this, SLOT(connectionWithoutCert()));

    QObject::connect(ui.localPort,SIGNAL(valueChanged(int)),this,SLOT(saveAddresses()));
    QObject::connect(ui.extPort,SIGNAL(valueChanged(int)),this,SLOT(saveAddresses()));

    connect( ui.netModeComboBox, SIGNAL( activated ( int ) ), this, SLOT( toggleUPnP( ) ) );
    connect( ui.allowIpDeterminationCB, SIGNAL( toggled( bool ) ), this, SLOT( toggleIpDetermination(bool) ) );
    connect( ui.cleanKnownIPs_PB, SIGNAL( clicked( ) ), this, SLOT( clearKnownAddressList() ) );
    connect( ui.testIncoming_PB, SIGNAL( clicked( ) ), this, SLOT( saveAndTestInProxy() ) );

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
	QObject::connect(ui.DhtLineEdit,SIGNAL(textChanged(const QString &)),this,SLOT(checkKey()));

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

    rsAutoProxyMonitor::taskSync(autoProxyType::I2PBOB, autoProxyTask::getSettings, &mBobSettings);

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
            else
                netIndex = 3; // NONE
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
	rsDht->getRelayAllowance(RSDHT_RELAY_CLASS_FRIENDS, count, bandwidth);
	whileBlocking(ui.noFriendSpinBox)->setValue(count);
	whileBlocking(ui.bandFriendSpinBox)->setValue(bandwidth / 1024);

	rsDht->getRelayAllowance(RSDHT_RELAY_CLASS_FOF, count, bandwidth);
	whileBlocking(ui.noFOFSpinBox)->setValue(count);
	whileBlocking(ui.bandFOFSpinBox)->setValue(bandwidth / 1024);

	rsDht->getRelayAllowance(RSDHT_RELAY_CLASS_GENERAL, count, bandwidth);
	whileBlocking(ui.noGeneralSpinBox)->setValue(count);
	whileBlocking(ui.bandGeneralSpinBox)->setValue(bandwidth / 1024);

	updateTotals();


	uint32_t relayMode = rsDht->getRelayMode();
	if (relayMode & RSDHT_RELAY_ENABLED)
	{
		whileBlocking(ui.enableCheckBox)->setCheckState(Qt::Checked);
		if ((relayMode & RSDHT_RELAY_MODE_MASK) == RSDHT_RELAY_MODE_OFF)
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

    QString range0 = RsNetUtil::printAddrRange(addr,0) ;
    QString range1 = RsNetUtil::printAddrRange(addr,1) ;
    QString range2 = RsNetUtil::printAddrRange(addr,2) ;

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

    updateStatusBob();

    // this is used by BOB
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

	ui.label_warningBandwidth->hide();
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
    ui.iconlabel_hiddenMode->setPixmap(QPixmap(":/images/ledon1.png"));
    
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
    for(std::list<std::string>::const_iterator it(detail.ipAddressList.begin());it!=detail.ipAddressList.end();++it)
        whileBlocking(ui.ipAddressList)->addItem(QString::fromStdString(*it));

    ui.iconlabel_upnp->setPixmap(QPixmap(":/images/ledoff1.png"));
    ui.iconlabel_netLimited->setPixmap(QPixmap(":/images/ledoff1.png"));
    ui.iconlabel_ext->setPixmap(QPixmap(":/images/ledoff1.png"));

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
        ui.iconlabel_tor_outgoing->setPixmap(QPixmap(ICON_STATUS_OK)) ;
        ui.iconlabel_tor_outgoing->setToolTip(tr("Proxy seems to work.")) ;
    }
    else
    {
        ui.iconlabel_tor_outgoing->setPixmap(QPixmap(ICON_STATUS_UNKNOWN)) ;
        ui.iconlabel_tor_outgoing->setToolTip(tr("Tor proxy is not enabled")) ;
    }

    // I2P
    socket.connectToHost(ui.hiddenpage_proxyAddress_i2p->text(),ui.hiddenpage_proxyPort_i2p->text().toInt());
    if(socket.waitForConnected(500))
    {
        socket.disconnectFromHost();
        ui.iconlabel_i2p_outgoing->setPixmap(QPixmap(ICON_STATUS_OK)) ;
        ui.iconlabel_i2p_outgoing->setToolTip(tr("Proxy seems to work.")) ;
    }
    else
    {
        ui.iconlabel_i2p_outgoing->setPixmap(QPixmap(ICON_STATUS_UNKNOWN)) ;
        ui.iconlabel_i2p_outgoing->setToolTip(tr("I2P proxy is not enabled")) ;
    }

    // I2P - BOB
    socket.connectToHost(ui.hiddenpage_proxyAddress_i2p_2->text(), 2827);
    if(true == (mBobAccessible = socket.waitForConnected(500)))
    {
        socket.disconnectFromHost();
        ui.iconlabel_i2p_outgoing_2->setPixmap(QPixmap(ICON_STATUS_OK)) ;
        ui.iconlabel_i2p_outgoing_2->setToolTip(tr("BOB is running and accessible")) ;
    }
    else
    {
        ui.iconlabel_i2p_outgoing_2->setPixmap(QPixmap(ICON_STATUS_UNKNOWN)) ;
        ui.iconlabel_i2p_outgoing_2->setToolTip(tr("BOB is not accessible! Is it running?")) ;
    }
}

void ServerPage::updateInProxyIndicator()
{
    // need to find a proper way to do this

    if(!mIsHiddenNode)
        return ;

    //ui.iconlabel_tor_incoming->setPixmap(QPixmap(ICON_STATUS_UNKNOWN)) ;
    //ui.testIncomingTor_PB->setIcon(QIcon(":/loader/circleball-16.gif")) ;
    QMovie *movie = new QMovie(":/images/loader/circleball-16.gif");
    ui.iconlabel_service_incoming->setMovie(movie);
    movie->start();

    if (mHiddenType == RS_HIDDEN_TYPE_I2P && mBobSettings.enable) {

        QTcpSocket tcpSocket;

        const QString host = ui.hiddenpage_proxyAddress_i2p->text();
        qint16 port = ui.hiddenpage_proxyPort_i2p->text().toInt();
        QByteArray addr = ui.leBobB32Addr->text().toUtf8();
        addr.push_back('\n');

        mOngoingConnectivityCheck = 5; // timeout in sec

        tcpSocket.connectToHost(host, port);
        tcpSocket.write(addr); // write addr
        tcpSocket.write(addr); // trigger connection error since RS expects a tls connection
        tcpSocket.close();
        tcpSocket.waitForDisconnected(5 * 1000);

        return;
    }

    if(manager == NULL) {
        manager = new  QNetworkAccessManager(this);
        connect(manager, SIGNAL(finished(QNetworkReply*)),this,SLOT(handleNetworkReply(QNetworkReply*))) ;
    }

    QNetworkProxy proxy ;

    proxy.setType(QNetworkProxy::Socks5Proxy);
    switch (mHiddenType) {
    case RS_HIDDEN_TYPE_I2P:
        proxy.setHostName(ui.hiddenpage_proxyAddress_i2p->text());
        proxy.setPort(ui.hiddenpage_proxyPort_i2p->text().toInt());
        break;
    case RS_HIDDEN_TYPE_TOR:
        proxy.setHostName(ui.hiddenpage_proxyAddress_tor->text());
        proxy.setPort(ui.hiddenpage_proxyPort_tor->text().toInt());
        break;
    default:
        return;
    }
    proxy.setCapabilities(QNetworkProxy::HostNameLookupCapability | proxy.capabilities()) ;

    QNetworkProxy::setApplicationProxy(proxy) ;

    QUrl url("https://"+ui.hiddenpage_serviceAddress->text() + ":" + ui.hiddenpage_servicePort->text());

    std::cerr << "Setting proxy hostname+port to " << std::dec << ui.hiddenpage_proxyAddress_tor->text().toStdString() << ":" << ui.hiddenpage_proxyPort_tor->text().toInt() << std::endl;
    std::cerr << "Connecting to " << url.toString().toStdString() << std::endl;

    manager->get( QNetworkRequest(url) ) ;

    QNetworkProxy::setApplicationProxy(QNetworkProxy::NoProxy) ;
}

void ServerPage::startBOB()
{
    rsAutoProxyMonitor::taskAsync(autoProxyType::I2PBOB, autoProxyTask::start);

    updateStatus();
}

void ServerPage::restartBOB()
{
    rsAutoProxyMonitor::taskAsync(autoProxyType::I2PBOB, autoProxyTask::stop);
    rsAutoProxyMonitor::taskAsync(autoProxyType::I2PBOB, autoProxyTask::start);

    updateStatus();
}

void ServerPage::stopBOB()
{
    rsAutoProxyMonitor::taskAsync(autoProxyType::I2PBOB, autoProxyTask::stop);

    updateStatus();
}

void ServerPage::getNewKey()
{
    bobSettings *bs = new bobSettings();

    rsAutoProxyMonitor::taskAsync(autoProxyType::I2PBOB, autoProxyTask::receiveKey, this, bs);

    updateStatus();
}

void ServerPage::loadKey()
{
	mBobSettings.address.privateKey = ui.pteBobServerKey->toPlainText().toStdString();
	mBobSettings.address.publicKey = i2p::publicKeyFromPrivate(mBobSettings.address.privateKey);
	mBobSettings.address.base32 = i2p::keyToBase32Addr(mBobSettings.address.publicKey);

    rsAutoProxyMonitor::taskSync(autoProxyType::I2PBOB, autoProxyTask::setSettings, &mBobSettings);
}

void ServerPage::enableBob(bool checked)
{
    mBobSettings.enable = checked;

    rsAutoProxyMonitor::taskSync(autoProxyType::I2PBOB, autoProxyTask::setSettings, &mBobSettings);

    setUpBobElements();
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

    mBobSettings.inLength    = fitRange(li, 0, 7);
    mBobSettings.outLength   = fitRange(lo, 0, 7);
    mBobSettings.inQuantity  = fitRange(qi, 1, 16);
    mBobSettings.outQuantity = fitRange(qo, 1, 16);
    mBobSettings.inVariance  = fitRange(vi, -1, 2);
    mBobSettings.outVariance = fitRange(vo, -1, 2);

    rsAutoProxyMonitor::taskSync(autoProxyType::I2PBOB, autoProxyTask::setSettings, &mBobSettings);
}

void ServerPage::toggleBobAdvancedSettings(bool checked)
{
    ui.swBobAdvanced->setCurrentIndex(checked ? 1 : 0);

	if (!mBobSettings.address.privateKey.empty()) {
        if (checked) {
            ui.pbBobGenAddr->show();
        } else {
            ui.pbBobGenAddr->hide();
        }
    }
}

void ServerPage::syncI2PProxyPortNormal(int i)
{
    ui.hiddenpage_proxyPort_i2p_2->setValue(i);
}

void ServerPage::syncI2PProxyPortBob(int i)
{
    ui.hiddenpage_proxyPort_i2p->setValue(i);

    // update port
    saveBob();
    rsAutoProxyMonitor::taskSync(autoProxyType::I2PBOB, autoProxyTask::reloadConfig);
}

void ServerPage::syncI2PProxyAddrNormal(QString t)
{
    ui.hiddenpage_proxyAddress_i2p_2->setText(t);
}

void ServerPage::syncI2PProxyAddrBob(QString t)
{
    ui.hiddenpage_proxyAddress_i2p->setText(t);

    // update addr
    saveBob();
    rsAutoProxyMonitor::taskSync(autoProxyType::I2PBOB, autoProxyTask::reloadConfig);
}

void ServerPage::taskFinished(taskTicket *&ticket)
{
    if (ticket->task == autoProxyTask::receiveKey) {
        bobSettings *s = NULL;
        switch (ticket->types.front()) {
        case autoProxyType::I2PBOB:
            // update settings
            s = (struct bobSettings *)ticket->data;
            mBobSettings = *s;
            delete s;
            s = NULL;
            ticket->data = NULL;
            break;
        default:
            break;
        }
    }

    if (ticket->data)
        std::cerr << "(WW) ServerPage::taskFinished data set. This should NOT happen - check the code!" << std::endl;

    delete ticket;
    ticket = NULL;
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
    whileBlocking(ui.hiddenpage_proxyAddress_i2p_2)->setText(QString::fromStdString(proxyaddr)); // this one is for bob tab
    whileBlocking(ui.hiddenpage_proxyPort_i2p) -> setValue(proxyport);
    whileBlocking(ui.hiddenpage_proxyPort_i2p_2)->setValue(proxyport); // this one is for bob tab

    // don't use whileBlocking here
    ui.cb_enableBob->setChecked(mBobSettings.enable);

	if (!mBobSettings.address.privateKey.empty()) {
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

    saveBob();
}

void ServerPage::saveBob()
{
    std::string orig_proxyaddr, new_proxyaddr;
    uint16_t orig_proxyport, new_proxyport;
    uint32_t status;
    // I2P
    rsPeers->getProxyServer(RS_HIDDEN_TYPE_I2P, orig_proxyaddr, orig_proxyport, status);

    new_proxyaddr = ui.hiddenpage_proxyAddress_i2p -> text().toStdString();
    new_proxyport = ui.hiddenpage_proxyPort_i2p -> value();

    if ((new_proxyaddr != orig_proxyaddr) || (new_proxyport != orig_proxyport)) {
        rsPeers->setProxyServer(RS_HIDDEN_TYPE_I2P, new_proxyaddr, new_proxyport);
    }
}

void ServerPage::updateStatusBob()
{
	QString addr = QString::fromStdString(mBobSettings.address.base32);
    if (ui.leBobB32Addr->text() != addr) {
        ui.leBobB32Addr->setText(addr);
        ui.hiddenpage_serviceAddress->setText(addr);
		ui.pteBobServerKey->setPlainText(QString::fromStdString(mBobSettings.address.privateKey));

		if (!mBobSettings.address.privateKey.empty()) {
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

    bobStates bs;
    rsAutoProxyMonitor::taskSync(autoProxyType::I2PBOB, autoProxyTask::status, &bs);

    QString bobSimpleText = QString();
    bobSimpleText.append(tr("RetroShare uses BOB to set up a %1 tunnel at %2:%3 (named %4)\n\n"
                            "When changing options (e.g. port) use the buttons at the bottom to restart BOB.\n\n").
	                     arg(mBobSettings.address.privateKey.empty() ? tr("client") : tr("server"),
                             ui.hiddenpage_proxyAddress_i2p_2->text(),
                             ui.hiddenpage_proxyPort_i2p_2->text(),
                             bs.tunnelName.empty() ? tr("unknown") :
                                                     QString::fromStdString(bs.tunnelName)));

    // update BOB UI based on state
    std::string errorString;
    switch (bs.cs) {
    case csDoConnect:
    case csConnected:
    case csDoDisconnect:
    case csWaitForBob:
        ui.iconlabel_i2p_bob->setPixmap(QPixmap(ICON_STATUS_WORKING));
        ui.iconlabel_i2p_bob->setToolTip(tr("BOB is processing a request"));

        enableBobElements(false);

        {
            QString s;
            switch (bs.ct) {
            case ctRunCheck:
                s = tr("connectivity check");
                break;
            case ctRunGetKeys:
                s = tr("generating key");
                break;
            case ctRunSetUp:
                s = tr("starting up");
                break;
            case ctRunShutDown:
                s = tr("shuting down");
            default:
                break;
            }
            bobSimpleText.append(tr("BOB is processing a request: %1").arg(s));
        }

        ui.pbBobStart->setEnabled(false);
        ui.pbBobRestart->setEnabled(false);
        ui.pbBobStop->setEnabled(false);
        break;
    case csError:
        // get error msg from bob
        rsAutoProxyMonitor::taskSync(autoProxyType::I2PBOB, autoProxyTask::getErrorInfo, &errorString);

        ui.iconlabel_i2p_bob->setPixmap(QPixmap(ICON_STATUS_ERROR));
        ui.iconlabel_i2p_bob->setToolTip(tr("BOB is broken\n") + QString::fromStdString(errorString));

        enableBobElements(false);

        bobSimpleText.append(tr("BOB encountered an error:\n"));
        bobSimpleText.append(QString::fromStdString(errorString));

        ui.pbBobStart->setEnabled(true);
        ui.pbBobRestart->setEnabled(false);
		ui.pbBobStop->setEnabled(true);
        break;
    case csDisconnected:
    case csIdel:
        switch (bs.ct) {
        case ctRunSetUp:
            ui.iconlabel_i2p_bob->setPixmap(QPixmap(ICON_STATUS_OK));
            ui.iconlabel_i2p_bob->setToolTip(tr("BOB tunnel is running"));

            enableBobElements(false);

            bobSimpleText.append(tr("BOB is working fine: tunnel established"));

            ui.pbBobStart->setEnabled(false);
            ui.pbBobRestart->setEnabled(true);
            ui.pbBobStop->setEnabled(true);
            break;
        case ctRunCheck:
        case ctRunGetKeys:
			ui.iconlabel_i2p_bob->setPixmap(QPixmap(ICON_STATUS_WORKING));
			ui.iconlabel_i2p_bob->setToolTip(tr("BOB is processing a request"));

			enableBobElements(false);

			bobSimpleText.append(tr("BOB is processing a request"));

			ui.pbBobStart->setEnabled(false);
			ui.pbBobRestart->setEnabled(false);
			ui.pbBobStop->setEnabled(false);
			break;
		case ctRunShutDown:
        case ctIdle:
            ui.iconlabel_i2p_bob->setPixmap(QPixmap(ICON_STATUS_UNKNOWN));
            ui.iconlabel_i2p_bob->setToolTip(tr("BOB tunnel is not running"));

            enableBobElements(true);

            bobSimpleText.append(tr("BOB is inactive: tunnel closed"));

            ui.pbBobStart->setEnabled(true);
            ui.pbBobRestart->setEnabled(false);
            ui.pbBobStop->setEnabled(false);
            break;
        }
        break;

    }
    ui.pteBobSimple->setPlainText(bobSimpleText);

    // disable elements when BOB is not accessible
    if (!mBobAccessible) {
        ui.pbBobStart->setEnabled(false);
        ui.pbBobStart->setToolTip("BOB is not accessible");
        ui.pbBobRestart->setEnabled(false);
        ui.pbBobRestart->setToolTip("BOB is not accessible");
        // don't disable the stop button! (in case bob is running you are otherwise unable to stop and disable it)
        ui.pbBobStop->setToolTip("BOB is not accessible");
    } else {
        ui.pbBobStart->setToolTip("");
        ui.pbBobRestart->setToolTip("");
        ui.pbBobStop->setToolTip("");
    }
}

void ServerPage::setUpBobElements()
{
    ui.gbBob->setEnabled(mBobSettings.enable);
    if (mBobSettings.enable) {
        ui.hiddenpage_proxyAddress_i2p->setEnabled(false);
        ui.hiddenpage_proxyAddress_i2p->setToolTip("Use I2P/BOB settings to change this value");
        ui.hiddenpage_proxyPort_i2p->setEnabled(false);
        ui.hiddenpage_proxyPort_i2p->setToolTip("Use I2P/BOB settings to change this value");

		ui.leBobB32Addr->setText(QString::fromStdString(mBobSettings.address.base32));
		ui.pteBobServerKey->setPlainText(QString::fromStdString(mBobSettings.address.privateKey));

        // cast to int to avoid problems
        int li, lo, qi, qo, vi, vo;
        li = mBobSettings.inLength;
        lo = mBobSettings.outLength;
        qi = mBobSettings.inQuantity;
        qo = mBobSettings.outQuantity;
        vi = mBobSettings.inVariance;
        vo = mBobSettings.outVariance;

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

void ServerPage::enableBobElements(bool enable)
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
        ui.pbBobGenAddr->setToolTip(tr("stop BOB tunnel first to generate a new key"));

        ui.pbBobLoadKey->setEnabled(false);
        ui.pbBobLoadKey->setToolTip(tr("stop BOB tunnel first to load a key"));

        ui.cb_enableBob->setEnabled(false);
        ui.cb_enableBob->setToolTip(tr("stop BOB tunnel first to disable BOB"));
    }
}

void ServerPage::updateInProxyIndicatorResult(bool success)
{
    if (success) {
        std::cerr <<"Connected!" << std::endl;

        ui.iconlabel_service_incoming->setPixmap(QPixmap(ICON_STATUS_OK)) ;
        ui.iconlabel_service_incoming->setToolTip(tr("You are reachable through the hidden service.")) ;
        //ui.testIncomingTor_PB->setIcon(QIcon(ICON_STATUS_OK)) ;
    } else {
        std::cerr <<"Failed!" << std::endl;

        //ui.testIncomingTor_PB->setIcon(QIcon(ICON_STATUS_UNKNOWN)) ;
        ui.iconlabel_service_incoming->setPixmap(QPixmap(ICON_STATUS_UNKNOWN)) ;
        ui.iconlabel_service_incoming->setToolTip(tr("The proxy is not enabled or broken.\nAre all services up and running fine??\nAlso check your ports!")) ;
    }
    // delete movie
    delete ui.iconlabel_service_incoming->movie();
}

void ServerPage::handleNetworkReply(QNetworkReply *reply)
{
    int error = reply->error() ;

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

	rsDht->setRelayAllowance(RSDHT_RELAY_CLASS_ALL, total, 0);
	rsDht->setRelayAllowance(RSDHT_RELAY_CLASS_FRIENDS, nFriends, 1024 * friendBandwidth);
	rsDht->setRelayAllowance(RSDHT_RELAY_CLASS_FOF, nFOF, 1024 * fofBandwidth);
	rsDht->setRelayAllowance(RSDHT_RELAY_CLASS_GENERAL, nGeneral, 1024 * genBandwidth);
}

/** Saves the changes on this page */

void ServerPage::updateRelayMode()
{
	uint32_t relayMode = 0;
	if (ui.enableCheckBox->isChecked())
	{
		relayMode |= RSDHT_RELAY_ENABLED;

		if (ui.serverCheckBox->isChecked())
		{
			relayMode |= RSDHT_RELAY_MODE_ON;
		}
		else
		{
			relayMode |= RSDHT_RELAY_MODE_OFF;
		}
	}
	else
	{
		relayMode |= RSDHT_RELAY_MODE_OFF;
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
