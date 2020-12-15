/*******************************************************************************
 * gui/settings/ServerPage.h                                                   *
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

#ifndef SERVERPAGE_H
#define SERVERPAGE_H

#include "ui_ServerPage.h"

#include <inttypes.h>
/* get OS-specific definitions for:
 * 	struct sockaddr_storage
 */
#ifndef WINDOWS_SYS
    #include <sys/socket.h>
#else
    #include <winsock2.h>
#endif

#include <services/autoproxy/rsautoproxymonitor.h>
#include <services/autoproxy/p3i2psam3.h>

#include "retroshare-gui/configpage.h"
#include "retroshare-gui/RsAutoUpdatePage.h"
#include "gui/common/FilesDefs.h"

#include "retroshare/rsevents.h"


class QNetworkReply;
class QNetworkAccessManager;
class BanListPeer;

class ServerPage: public ConfigPage, public autoProxyCallback
{
    Q_OBJECT

public:
    ServerPage(QWidget * parent = 0, Qt::WindowFlags flags = 0);
    ~ServerPage() {}

    /** Loads the settings for this page */
    virtual void load();

    virtual QPixmap iconPixmap() const { return FilesDefs::getPixmapFromQtResourcePath(":/icons/png/network.png") ; }
    virtual QString pageName() const { return tr("Network") ; }
    virtual QString helpText() const { return ""; }

public slots:
    void updateStatus();

private slots:
    // ban list
    void updateSelectedBlackListIP(int row, int, int, int);
    void updateSelectedWhiteListIP(int row,int,int,int);
	void saveAndTestInProxy();
    void addIpRangeToBlackList();
    void addIpRangeToWhiteList();
    void moveToWhiteList0();
    void moveToWhiteList1();
    void moveToWhiteList2();
    void removeWhiteListedIp();
    void checkIpRange(const QString &);
    void setGroupIpLimit(int n);
    void toggleGroupIps(bool b);
    void toggleAutoIncludeDHT(bool b);
    void toggleAutoIncludeFriends(bool b);
    void toggleIpFiltering(bool b);
    void ipFilterContextMenu(const QPoint &);
    void ipWhiteListContextMenu(const QPoint &point);
    void removeBannedIp();

    void tabChanged(int page);
    // server
    void saveAddresses();
    void saveRates();
    void toggleUPnP();
    void toggleIpDetermination(bool) ;
    void toggleTunnelConnection(bool) ;
    void clearKnownAddressList() ;
    void handleNetworkReply(QNetworkReply *reply);
    void updateInProxyIndicator();

	// i2p SAMv3
	void startSam();
	void restartSam();
	void stopSam();
	void getNewKey();
	void loadKey();
	void enableSam(bool checked);
	void tunnelSettingsChanged(int);

	void toggleSamAdvancedSettings(bool checked);

	void syncI2PProxyPortNormal(int i);
	void syncI2PProxyPortSam(int i);

	void syncI2PProxyAddrNormal(QString);
	void syncI2PProxyAddrSam(QString);

	void connectionWithoutCert();

	//Relay Tab
	void updateRelayOptions();
	void updateEnabled();
	void checkKey();
	void addServer();
	void removeServer();
	void loadServers();
	void updateTotals();
	void updateRelayMode();

	// autoProxyCallback interface
public:
	void taskFinished(taskTicket *&ticket);

private:
	void loadCommon();
	void saveCommon();
	void saveSam();
	void updateStatusSam();

	void setUpSamElements();
	void enableSamElements(bool enable);

	void updateInProxyIndicatorResult(bool success);

    // ban list
    void addPeerToIPTable(QTableWidget *table, int row, const BanListPeer &blp);
    bool removeCurrentRowFromBlackList(sockaddr_storage& collected_addr,int& masked_bytes);
    bool removeCurrentRowFromWhiteList(sockaddr_storage &collected_addr, int &masked_bytes);

    // Alternative Versions for HiddenNode Mode.
    void loadHiddenNode();
    void updateStatusHiddenNode();
    void saveAddressesHiddenNode();
    void updateOutProxyIndicator();
    void loadFilteredIps() ;

    Ui::ServerPage ui;

    QNetworkAccessManager *manager ;
	int mOngoingConnectivityCheck;

	bool mIsHiddenNode;
	uint32_t mHiddenType;
	samSettings mSamSettings;
	bool mSamAccessible; // keeps track wether SAM is accessable or not to en/disable the corresponding buttons

	RsEventsHandlerId_t mEventHandlerId;
	void handleEvent(std::shared_ptr<const RsEvent> event);
};

#endif // !SERVERPAGE_H

