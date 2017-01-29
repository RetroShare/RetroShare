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
#include "util/RsNetUtil.h"

#include <iostream>

#include <retroshare/rsconfig.h>
#include <retroshare/rspeers.h>
#include <retroshare/rsturtle.h>
#include <retroshare/rsbanlist.h>

#include <QMovie>
#include <QMenu>
#include <QTcpSocket>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QNetworkAccessManager>
#include <QTimer>

#define ICON_STATUS_UNKNOWN ":/images/ledoff1.png"
#define ICON_STATUS_OK      ":/images/ledon1.png"

#define COLUMN_RANGE   0
#define COLUMN_STATUS  1
#define COLUMN_ORIGIN  2
#define COLUMN_REASON  3
#define COLUMN_COMMENT 4

//#define SERVER_DEBUG 1

ServerPage::ServerPage(QWidget * parent, Qt::WindowFlags flags)
	: ConfigPage(parent, flags), mIsHiddenNode(false), mHiddenType(RS_HIDDEN_TYPE_NONE)
{
  /* Invoke the Qt Designer generated object setup routine */
  ui.setupUi(this);

  manager = NULL ;

    ui.filteredIpsTable->setHorizontalHeaderItem(COLUMN_RANGE,new QTableWidgetItem(tr("IP Range"))) ;
    ui.filteredIpsTable->setHorizontalHeaderItem(COLUMN_STATUS,new QTableWidgetItem(tr("Status"))) ;
    ui.filteredIpsTable->setHorizontalHeaderItem(COLUMN_ORIGIN,new QTableWidgetItem(tr("Origin"))) ;
    ui.filteredIpsTable->setHorizontalHeaderItem(COLUMN_COMMENT,new QTableWidgetItem(tr("Comment"))) ;

    ui.filteredIpsTable->setColumnHidden(COLUMN_STATUS,true) ;
    ui.filteredIpsTable->verticalHeader()->hide() ;
    ui.whiteListIpsTable->setColumnHidden(COLUMN_STATUS,true) ;
    ui.whiteListIpsTable->verticalHeader()->hide() ;

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

	ui.hiddenpage_incoming->setVisible(false);

	connect( ui.netModeComboBox, SIGNAL( activated ( int ) ), this, SLOT( toggleUPnP( ) ) );
	connect( ui.allowIpDeterminationCB, SIGNAL( toggled( bool ) ), this, SLOT( toggleIpDetermination(bool) ) );
	connect( ui.cleanKnownIPs_PB, SIGNAL( clicked( ) ), this, SLOT( clearKnownAddressList() ) );
	connect( ui.testIncoming_PB, SIGNAL( clicked( ) ), this, SLOT( updateInProxyIndicator() ) );
    connect( ui.showDiscStatusBar,SIGNAL(toggled(bool)),this,SLOT(updateShowDiscStatusBar())) ;

#ifdef SERVER_DEBUG
	std::cerr << "ServerPage::ServerPage() called";
	std::cerr << std::endl;
#endif

	connect(ui.netModeComboBox,SIGNAL(currentIndexChanged(int)),this,SLOT(saveAddresses()));
	connect(ui.discComboBox,   SIGNAL(currentIndexChanged(int)),this,SLOT(saveAddresses()));
	connect(ui.localAddress,   SIGNAL(textChanged(QString)),this,SLOT(saveAddresses()));
	connect(ui.extAddress,     SIGNAL(textChanged(QString)),this,SLOT(saveAddresses()));
	connect(ui.dynDNS,         SIGNAL(textChanged(QString)),this,SLOT(saveAddresses()));

	connect(ui.hiddenpage_proxyAddress_tor, SIGNAL(textChanged(QString)),this,SLOT(saveAddresses()));
	connect(ui.hiddenpage_proxyPort_tor,    SIGNAL(textChanged(QString)),this,SLOT(saveAddresses()));
	connect(ui.hiddenpage_proxyAddress_i2p, SIGNAL(textChanged(QString)),this,SLOT(saveAddresses()));
	connect(ui.hiddenpage_proxyPort_i2p,    SIGNAL(textChanged(QString)),this,SLOT(saveAddresses()));

	connect(ui.totalDownloadRate,SIGNAL(valueChanged(int)),this,SLOT(saveRates()));
	connect(ui.totalUploadRate,  SIGNAL(valueChanged(int)),this,SLOT(saveRates()));
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

void ServerPage::updateShowDiscStatusBar() { Settings->setStatusBarFlag(STATUSBAR_DISC, ui.showDiscStatusBar->isChecked()); }

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
		mHiddenType = detail.hiddenType;
        ui.tabWidget->setTabEnabled(1,false) ;
		loadHiddenNode();
		return;
	}

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
	ui.netModeComboBox->setCurrentIndex(netIndex);

	/* DHT + Discovery: (public)
	 * Discovery only:  (private)
	 * DHT only: (inverted)
	 * None: (dark net)
	 */

	netIndex = 3; // NONE.
    if (detail.vs_dht != RS_VS_DHT_OFF)
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

	/* HIDDEN PAGE SETTINGS - only Proxy (outgoing) */
	std::string proxyaddr;
    uint16_t proxyport;
    uint32_t status ;
	// Tor
	rsPeers->getProxyServer(RS_HIDDEN_TYPE_TOR, proxyaddr, proxyport, status);
	ui.hiddenpage_proxyAddress_tor -> setText(QString::fromStdString(proxyaddr));
	ui.hiddenpage_proxyPort_tor -> setValue(proxyport);
	// I2P
	rsPeers->getProxyServer(RS_HIDDEN_TYPE_I2P, proxyaddr, proxyport, status);
	ui.hiddenpage_proxyAddress_i2p -> setText(QString::fromStdString(proxyaddr));
	ui.hiddenpage_proxyPort_i2p -> setValue(proxyport);

	updateOutProxyIndicator();
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
    if(rsBanList->ipFilteringEnabled())
    {
        ui.denyAll_CB->setChecked(true) ;
        ui.filteredIpsTable->setEnabled(true) ;
        ui.includeFromFriends_CB->setEnabled(true) ;
        ui.includeFromDHT_CB->setEnabled(true) ;
        ui.ipInput_LE->setEnabled(true) ;
        ui.ipInputRange_SB->setEnabled(true) ;
        ui.ipInputComment_LE->setEnabled(true) ;
        ui.ipInputAddBlackList_PB->setEnabled(true) ;
        ui.ipInputAddWhiteList_PB->setEnabled(true) ;
        ui.groupIPRanges_CB->setEnabled(true) ;
        ui.groupIPRanges_SB->setEnabled(true) ;
    }
    else
    {
        ui.denyAll_CB->setChecked(false) ;
        ui.filteredIpsTable->setEnabled(false) ;
        ui.includeFromFriends_CB->setEnabled(false) ;
        ui.includeFromDHT_CB->setEnabled(false) ;
        ui.ipInput_LE->setEnabled(false) ;
        ui.ipInputRange_SB->setEnabled(false) ;
        ui.ipInputComment_LE->setEnabled(false) ;
        ui.ipInputAddBlackList_PB->setEnabled(false) ;
        ui.ipInputAddWhiteList_PB->setEnabled(true) ;
        ui.groupIPRanges_CB->setEnabled(false) ;
        ui.groupIPRanges_SB->setEnabled(false) ;
    }

    ui.includeFromFriends_CB->setChecked(rsBanList->IPsFromFriendsEnabled()) ;
    ui.includeFromDHT_CB->setChecked(rsBanList->iPsFromDHTEnabled()) ;
    ui.groupIPRanges_CB->setChecked(rsBanList->autoRangeEnabled()) ;
    ui.groupIPRanges_SB->setValue(rsBanList->autoRangeLimit()) ;

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

    // check for Tor
	updateOutProxyIndicator();
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

	// HANDLE PROXY SERVER.
	std::string orig_proxyaddr, new_proxyaddr;
	uint16_t orig_proxyport, new_proxyport;
	uint32_t status ;
	// Tor
	rsPeers->getProxyServer(RS_HIDDEN_TYPE_TOR, orig_proxyaddr, orig_proxyport,status);

	new_proxyaddr = ui.hiddenpage_proxyAddress_tor -> text().toStdString();
	new_proxyport = ui.hiddenpage_proxyPort_tor -> value();

	if ((new_proxyaddr != orig_proxyaddr) || (new_proxyport != orig_proxyport))
	{
		rsPeers->setProxyServer(RS_HIDDEN_TYPE_TOR, new_proxyaddr, new_proxyport);
	}

	// I2P
	rsPeers->getProxyServer(RS_HIDDEN_TYPE_I2P, orig_proxyaddr, orig_proxyport,status);

	new_proxyaddr = ui.hiddenpage_proxyAddress_i2p -> text().toStdString();
	new_proxyport = ui.hiddenpage_proxyPort_i2p -> value();

	if ((new_proxyaddr != orig_proxyaddr) || (new_proxyport != orig_proxyport))
	{
		rsPeers->setProxyServer(RS_HIDDEN_TYPE_I2P, new_proxyaddr, new_proxyport);
	}

	load();
}

void ServerPage::saveRates()
{
	rsConfig->SetMaxDataRates( ui.totalDownloadRate->value(), ui.totalUploadRate->value() );
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
	
	ui.textlabel_hiddenMode->show();
	ui.iconlabel_hiddenMode->show() ;
	ui.iconlabel_hiddenMode->setPixmap(QPixmap(":/images/ledon1.png"));

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

	ui.hiddenpage_incoming->setVisible(true);

	/* Addresses must be set here - otherwise can't edit it */
		/* set local address */
	ui.localAddress->setText(QString::fromStdString(detail.localAddr));
	ui.localPort -> setValue(detail.localPort);
		/* set the server address */

	ui.extAddress->setText(tr("Hidden - See Config"));

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
	ui.hiddenpage_localAddress->setEnabled(false);
	ui.hiddenpage_localAddress->setText(QString::fromStdString(detail.localAddr));
	ui.hiddenpage_localPort -> setValue(detail.localPort);

	/* set the server address */
	ui.hiddenpage_serviceAddress->setText(QString::fromStdString(detail.hiddenNodeAddress));
	ui.hiddenpage_servicePort -> setValue(detail.hiddenNodePort);
	/* in I2P there is no port - there is only the address */
	ui.hiddenpage_servicePort->setEnabled(detail.hiddenType != RS_HIDDEN_TYPE_I2P);

	/* out proxy settings */
	std::string proxyaddr;
	uint16_t proxyport;
	uint32_t status ;
	// Tor
	rsPeers->getProxyServer(RS_HIDDEN_TYPE_TOR, proxyaddr, proxyport, status);
	ui.hiddenpage_proxyAddress_tor -> setText(QString::fromStdString(proxyaddr));
	ui.hiddenpage_proxyPort_tor -> setValue(proxyport);
	// I2P
	rsPeers->getProxyServer(RS_HIDDEN_TYPE_I2P, proxyaddr, proxyport, status);
	ui.hiddenpage_proxyAddress_i2p -> setText(QString::fromStdString(proxyaddr));
	ui.hiddenpage_proxyPort_i2p -> setValue(proxyport);

	updateOutProxyIndicator();

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
	ui.hiddenpage_configuration->setPlainText(expected);
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

	updateOutProxyIndicator();
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

	// HANDLE PROXY SERVER.
	std::string orig_proxyaddr,new_proxyaddr;
	uint16_t orig_proxyport, new_proxyport;
	uint32_t status ;
	// Tor
	rsPeers->getProxyServer(RS_HIDDEN_TYPE_TOR, orig_proxyaddr, orig_proxyport,status);

	new_proxyaddr = ui.hiddenpage_proxyAddress_tor -> text().toStdString();
	new_proxyport = ui.hiddenpage_proxyPort_tor -> value();

	if ((new_proxyaddr != orig_proxyaddr) || (new_proxyport != orig_proxyport))
	{
		rsPeers->setProxyServer(RS_HIDDEN_TYPE_TOR, new_proxyaddr, new_proxyport);
	}

	// I2P
	rsPeers->getProxyServer(RS_HIDDEN_TYPE_I2P, orig_proxyaddr, orig_proxyport,status);

	new_proxyaddr = ui.hiddenpage_proxyAddress_i2p -> text().toStdString();
	new_proxyport = ui.hiddenpage_proxyPort_i2p -> value();

	if ((new_proxyaddr != orig_proxyaddr) || (new_proxyport != orig_proxyport))
	{
		rsPeers->setProxyServer(RS_HIDDEN_TYPE_I2P, new_proxyaddr, new_proxyport);
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
}

void ServerPage::updateInProxyIndicator()
{
    // need to find a proper way to do this

    if(!mIsHiddenNode)
        return ;

    if(manager == NULL)
        manager = new  QNetworkAccessManager(this);

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

	//ui.iconlabel_tor_incoming->setPixmap(QPixmap(ICON_STATUS_UNKNOWN)) ;
	//ui.testIncomingTor_PB->setIcon(QIcon(":/loader/circleball-16.gif")) ;
	QMovie *movie = new QMovie(":/images/loader/circleball-16.gif");
	ui.iconlabel_service_incoming->setMovie(movie);
	movie->start() ;

    QNetworkProxy::setApplicationProxy(proxy) ;

	QUrl url("https://"+ui.hiddenpage_serviceAddress->text() + ":" + ui.hiddenpage_servicePort->text()) ;

	std::cerr << "Setting proxy hostname+port to " << std::dec << ui.hiddenpage_proxyAddress_tor->text().toStdString() << ":" << ui.hiddenpage_proxyPort_tor->text().toInt() << std::endl;
    std::cerr << "Connecting to " << url.toString().toStdString() << std::endl;

    connect(manager, SIGNAL(finished(QNetworkReply*)),this,SLOT(handleNetworkReply(QNetworkReply*))) ;
    manager->get( QNetworkRequest(url) ) ;

    QNetworkProxy::setApplicationProxy(QNetworkProxy::NoProxy) ;
}

void ServerPage::handleNetworkReply(QNetworkReply *reply)
{
    int error = reply->error() ;

    if(reply->isOpen() &&  error == QNetworkReply::SslHandshakeFailedError)
    {
        std::cerr <<"Connected!" << std::endl;
		ui.iconlabel_service_incoming->setPixmap(QPixmap(ICON_STATUS_OK)) ;
		ui.iconlabel_service_incoming->setToolTip(tr("You are reachable through the hidden service.")) ;
        //ui.testIncomingTor_PB->setIcon(QIcon(ICON_STATUS_OK)) ;
    }
    else
    {
        std::cerr <<"Failed!" << std::endl;

        //ui.testIncomingTor_PB->setIcon(QIcon(ICON_STATUS_UNKNOWN)) ;
		ui.iconlabel_service_incoming->setPixmap(QPixmap(ICON_STATUS_UNKNOWN)) ;
		ui.iconlabel_service_incoming->setToolTip(tr("The proxy is not enabled or broken.\nAre all services up and running fine??\nAlso check your ports!")) ;
    }

    reply->close();
}

