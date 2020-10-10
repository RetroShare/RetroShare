/*******************************************************************************
 * gui/connect/ConnectProgressDialog.cpp                                       *
 *                                                                             *
 * Copyright 2012-2013 by Robert Fernie <retroshare.project@gmail.com>         *
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

#include "gui/connect/ConnectProgressDialog.h"

#include <QTimer>

#include <map>
#include <time.h>
#include <retroshare/rspeers.h>
#include <retroshare/rsconfig.h>
#include <retroshare/rsdht.h>

#include "gui/common/StatusDefs.h"
#include "gui/common/FilesDefs.h"

/* maintain one static dialog per SSL ID */

static std::map<RsPeerId, ConnectProgressDialog *> instances;

int calcProgress(time_t now, time_t start, int period50, int period75, int period100);

ConnectProgressDialog *ConnectProgressDialog::instance(const RsPeerId& peer_id)
{
	std::map<RsPeerId, ConnectProgressDialog *>::iterator it;
	it = instances.find(peer_id);
	if (it != instances.end())
	{
		return it->second;
	}

	ConnectProgressDialog *d = new ConnectProgressDialog(peer_id);
	instances[peer_id] = d;
	return d;
}

ConnectProgressDialog::ConnectProgressDialog(const RsPeerId& id, QWidget *parent, Qt::WindowFlags flags)
	:QDialog(parent, flags), mId(id), ui(new Ui::ConnectProgressDialog)
{
	ui->setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose, true);

    ui->headerFrame->setHeaderImage(FilesDefs::getPixmapFromQtResourcePath(":/images/user/identityinfo64.png"));
	ui->headerFrame->setHeaderText(tr("Connection Assistant"));

	connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(stopAndClose()));

	mAmIHiddenNode = rsPeers->isHiddenNode(rsPeers->getOwnId()) ;
	mIsPeerHiddenNode = rsPeers->isHiddenNode(id) ;
}

ConnectProgressDialog::~ConnectProgressDialog()
{
	std::map<RsPeerId, ConnectProgressDialog *>::iterator it;
	it = instances.find(mId);
	if (it != instances.end())
	{
		instances.erase(it);
	}
}

void ConnectProgressDialog::showProgress(const RsPeerId& peer_id)
{
    ConnectProgressDialog *d = instance(peer_id);

    d->initDialog();
    d->show();
    d->raise();
    d->activateWindow();

    /* window will destroy itself! */
}

const uint32_t CONNECT_STATE_INIT		= 0;
const uint32_t CONNECT_STATE_PROGRESS		= 1;
const uint32_t CONNECT_STATE_CONNECTED		= 2;
const uint32_t CONNECT_STATE_DENIED		= 3;
const uint32_t CONNECT_STATE_CLOSED		= 4;
const uint32_t CONNECT_STATE_FAILED		= 5;

const uint32_t CONNECT_DHT_INIT		= 0;
const uint32_t CONNECT_DHT_OKAY		= 2;

const uint32_t CONNECT_DHT_FAIL		= 3;
const uint32_t CONNECT_DHT_DISABLED	= 4;


const uint32_t CONNECT_LOOKUP_INIT		= 0;
const uint32_t CONNECT_LOOKUP_SEARCH		= 1;
const uint32_t CONNECT_LOOKUP_OFFLINE		= 2;
const uint32_t CONNECT_LOOKUP_UNREACHABLE	= 3;
const uint32_t CONNECT_LOOKUP_ONLINE		= 4;
const uint32_t CONNECT_LOOKUP_NODHTCONFIG	= 5;
const uint32_t CONNECT_LOOKUP_FAIL		= 6;

const uint32_t CONNECT_UDP_INIT			= 0;
const uint32_t CONNECT_UDP_PROGRESS		= 1;
const uint32_t CONNECT_UDP_FAIL			= 2;

const uint32_t CONNECT_CONTACT_NONE		= 0;
const uint32_t CONNECT_CONTACT_CONNECTED	= 1;

/* at a minimum allow 30 secs for TCP connections */
const int MINIMUM_CONNECT_PERIOD = 30;

/* overall timeout */
const int CONNECT_TIMEOUT_PERIOD = 1800;

/* assume it will take 5 minute to complete DHT load (should take 30 secs, and 2-3 minutes from cold start) */
const int CONNECT_DHT_TYPICAL = 30;
const int CONNECT_DHT_SLOW   = 120;
const int CONNECT_DHT_PERIOD = 300;

/* need to shorten this time (DHT work) */
const int CONNECT_LOOKUP_TYPICAL = 200;
const int CONNECT_LOOKUP_SLOW    = 300;
const int CONNECT_LOOKUP_PERIOD  = 600;

/* Udp can take ages ... */
const int CONNECT_UDP_TYPICAL = 150;
const int CONNECT_UDP_SLOW   = 300;
const int CONNECT_UDP_PERIOD = 600;

/* connection must stay up for 5 seconds to be considered okay */
const int REQUIRED_CONTACT_PERIOD = 5;

void ConnectProgressDialog::initDialog()
{
	mTimer = new QTimer(this);
	connect(mTimer, SIGNAL(timeout()), this, SLOT(updateStatus()));
     	mTimer->start(250);

	mState = CONNECT_STATE_PROGRESS;
	mInitTS = time(NULL);
	mDhtStatus = CONNECT_DHT_INIT;

	mLookupTS = 0;
	mLookupStatus = CONNECT_LOOKUP_INIT;

	mContactTS = 0;
	mContactState = CONNECT_CONTACT_NONE;

	mUdpTS = 0;
	mUdpStatus = CONNECT_UDP_INIT;

	ui->progressFrame->setEnabled(true);
	/* initialise GUI data */
	ui->NetResult->setText(tr("N/A"));
	ui->ContactResult->setText(tr("N/A"));

#ifdef RS_USE_BITDHT
	if(!mIsPeerHiddenNode && !mAmIHiddenNode)
	{
	ui->DhtResult->setText(tr("DHT Startup"));
	ui->DhtProgressBar->setValue(0);
	}
#else
	if(mIsPeerHiddenNode || mAmIHiddenNode)
	{
		ui->DhtResult->hide();
		ui->DhtLabel->hide();
		ui->DhtProgressBar->hide();
	}
#endif
	if(mIsPeerHiddenNode || mAmIHiddenNode)
	{
		ui->UdpResult->hide();
		ui->UdpProgressBar->hide();
		ui->UdpLabel->hide();

        ui->DhtLabel->hide();
        ui->DhtProgressBar->hide();
        ui->DhtResult->hide();

        ui->LookupLabel->hide();
        ui->LookupProgressBar->hide();
        ui->LookupResult->hide();
	}
	else
	{
		ui->UdpResult->setText(tr("N/A"));
		ui->UdpProgressBar->setValue(0);
	}

	ui->LookupResult->setText(tr("N/A"));
	ui->LookupProgressBar->setValue(0);

	sayInProgress();

	if (rsPeers->isFriend(mId))
	{
		/* okay */
		std::string name = rsPeers->getPeerName(mId);
		QString connectName = QString::fromUtf8(name.c_str());
		connectName += " (";
        connectName += QString::fromStdString(mId.toStdString()).left(10);
		connectName += "...)";
		ui->connectId->setText(connectName);
	}
	else
	{
		ui->connectId->setText(tr("Invalid Peer ID"));

		/* list Error */
		mState = CONNECT_STATE_FAILED;
		sayInvalidPeer();
		return;
	}

	if (rsPeers->isOnline(mId))
	{
		mState = CONNECT_STATE_CONNECTED;
		sayConnected();
		return;
	}
}

void ConnectProgressDialog::updateStatus()
{

	if (time(NULL) > mInitTS + CONNECT_TIMEOUT_PERIOD)
	{
		sayConnectTimeout();
		mState = CONNECT_STATE_FAILED;
	}

	switch(mState)
	{
		case CONNECT_STATE_PROGRESS:

			updateNetworkStatus();
			updateContactStatus();
#ifdef RS_USE_BITDHT
			updateDhtStatus();
#endif
			updateLookupStatus();
			updateUdpStatus();

			return;
			break;

		default:
		case CONNECT_STATE_CLOSED:
		case CONNECT_STATE_FAILED:
		case CONNECT_STATE_DENIED:
		case CONNECT_STATE_CONNECTED:
			break;
	}

	/* shutdown actions */
	ui->progressFrame->setEnabled(false);
	mTimer->stop();
}


void ConnectProgressDialog::stopAndClose()
{
	mState = CONNECT_STATE_CLOSED;
	if (mTimer)
	{
		mTimer->stop();
	}
	close();
}

void ConnectProgressDialog::updateNetworkStatus()
{
	RsNetState netState = rsConfig->getNetState();

	QLabel *label = ui->NetResult;
	switch(netState)
	{
	    case RsNetState::BAD_UNKNOWN:
			label->setText(tr("Unknown State"));
			break;
	    case RsNetState::BAD_OFFLINE:
			label->setText(tr("Offline"));
			break;
	    case RsNetState::BAD_NATSYM:
			label->setText(tr("Behind Symmetric NAT"));
			break;
	    case RsNetState::BAD_NODHT_NAT:
			label->setText(tr("Behind NAT & No DHT"));
			break;
	    case RsNetState::WARNING_RESTART:
			label->setText(tr("NET Restart"));
			break;
	    case RsNetState::WARNING_NATTED:
			label->setText(tr("Behind NAT"));
			break;
	    case RsNetState::WARNING_NODHT:
			label->setText(tr("No DHT"));
			break;
	    case RsNetState::GOOD:
			label->setText(tr("NET STATE GOOD!"));
			break;
	    case RsNetState::ADV_FORWARD:
			label->setText(tr("UNVERIFIABLE FORWARD!"));
			break;
	    case RsNetState::ADV_DARK_FORWARD:
			label->setText(tr("UNVERIFIABLE FORWARD & NO DHT"));
			break;
	}
}

void ConnectProgressDialog::updateContactStatus()
{
	/* lookup peer details - and try those addresses */

	RsPeerDetails details;
	if (!rsPeers->getPeerDetails(mId, details))
		return;

	QString status = StatusDefs::connectStateString(details);
	ui->ContactResult->setText(status);

	time_t now = time(NULL);

	/* now if it says connected - Alter overall state, after REQUIRED CONTACT PERIOD */
	if (rsPeers->isOnline(mId))
	{
		switch(mContactState)
		{
			default:
			case CONNECT_CONTACT_NONE:
				{
					mContactTS = now;
					mContactState = CONNECT_CONTACT_CONNECTED;
				}
				break;
			case CONNECT_CONTACT_CONNECTED:
				if (now > mContactTS + REQUIRED_CONTACT_PERIOD)
				{
					/* still connected, flag as SUCCESS */
					mState = CONNECT_STATE_CONNECTED;
					sayConnected();
				}
				break;
		}
	}
	else if (details.wasDeniedConnection)
	{
		if (details.deniedTS > mInitTS)
		{
			/* connection dropped, flag as DENIED */
			mState = CONNECT_STATE_DENIED;
			sayDenied();
		}
	}
}


void ConnectProgressDialog::updateDhtStatus()
{
	/* if DHT is disabled -> mark bad */
	/* if DHT is red -> mark bad, probable cause missing bdboot.txt */
	/* if DHT is starting, run progress bar */

	time_t now = time(NULL);
	switch(mDhtStatus)
	{
		case CONNECT_DHT_OKAY:
		case CONNECT_DHT_FAIL:
			return;
			break;

		case CONNECT_DHT_DISABLED:

			/* Ensure minimum time for direct TCP connections */
			if (now > mInitTS + MINIMUM_CONNECT_PERIOD)
			{
				mState = CONNECT_STATE_FAILED;
				sayDHTOffline();
			}
			return;
			break;

		default: /* still in progress */
		case CONNECT_DHT_INIT:
			if (now > mInitTS + CONNECT_DHT_PERIOD)
			{
				ui->DhtProgressBar->setValue(100);
				ui->DhtResult->setText(tr("DHT Failed"));
				mDhtStatus = CONNECT_DHT_FAIL;
				mState = CONNECT_STATE_FAILED;
		
				sayDHTFailed();
				return;
			}
			break;
	}



	RsConfigNetStatus status;
	rsConfig->getConfigNetStatus(status);

	if (!status.DHTActive)
	{
		mDhtStatus = CONNECT_DHT_DISABLED;
		
		/* ERROR message */
		ui->DhtProgressBar->setValue(100);
		ui->DhtResult->setText(tr("DHT Disabled"));

		return;
	}

	if (status.netDhtOk && (status.netDhtNetSize > 100))
	{
		if (status.netDhtRsNetSize > 10)
		{
			ui->DhtProgressBar->setValue(100);
			ui->DhtResult->setText(tr("DHT Okay"));
			mDhtStatus = CONNECT_DHT_OKAY;
			return;
		}
		ui->DhtResult->setText(tr("Finding RS Peers"));
	}

	ui->DhtProgressBar->setValue(calcProgress(now, mInitTS, CONNECT_DHT_TYPICAL, CONNECT_DHT_SLOW, CONNECT_DHT_PERIOD));

	return;
}


void ConnectProgressDialog::updateLookupStatus()
{
	switch(mLookupStatus)
	{
		case CONNECT_LOOKUP_OFFLINE:
		case CONNECT_LOOKUP_NODHTCONFIG:

			/* Ensure minimum time for direct TCP connections */
			if (time(NULL) > mInitTS + MINIMUM_CONNECT_PERIOD)
			{
				mState = CONNECT_STATE_FAILED;

				if (mLookupStatus == CONNECT_LOOKUP_NODHTCONFIG)
				{
					sayPeerNoDhtConfig();
				}
				else
				{
					sayPeerOffline();
				}
			}

		case CONNECT_LOOKUP_FAIL:
		case CONNECT_LOOKUP_ONLINE:
		case CONNECT_LOOKUP_UNREACHABLE:
			return;
			break;

		default:
			break;
	}

#ifdef RS_USE_BITDHT
	time_t now = time(NULL);
	switch(mDhtStatus)
	{
		case CONNECT_DHT_DISABLED:
		case CONNECT_DHT_FAIL:
			mLookupStatus = CONNECT_LOOKUP_FAIL;
			ui->LookupProgressBar->setValue(0);
			ui->LookupResult->setText(tr("Lookup requires DHT"));

		case CONNECT_DHT_INIT:

			return;
			break;

		case CONNECT_DHT_OKAY:
	
			if (mLookupStatus == CONNECT_LOOKUP_INIT)
			{
				mLookupStatus = CONNECT_LOOKUP_SEARCH;
				ui->LookupResult->setText(tr("Searching DHT"));
				mLookupTS = now;
			}
	}




	if (now > mLookupTS + CONNECT_LOOKUP_PERIOD)
	{
		ui->LookupProgressBar->setValue(100);
		ui->LookupResult->setText(tr("Lookup Timeout"));
		mLookupStatus = CONNECT_LOOKUP_FAIL;

		mState = CONNECT_STATE_FAILED;
		sayLookupTimeout();

		return;
	}

	ui->LookupProgressBar->setValue(calcProgress(now, mLookupTS, CONNECT_LOOKUP_TYPICAL, CONNECT_LOOKUP_SLOW, CONNECT_LOOKUP_PERIOD));

	/* now actually look at the DHT Details */
	RsDhtNetPeer status;
	rsDht->getNetPeerStatus(mId, status);

	switch(status.mDhtState)
	{
		default:
	    case RsDhtPeerDht::NOT_ACTIVE:
			ui->LookupProgressBar->setValue(0);
			ui->LookupResult->setText(tr("Peer DHT NOT ACTIVE"));
			mLookupStatus = CONNECT_LOOKUP_NODHTCONFIG;
			break;
	    case RsDhtPeerDht::SEARCHING:
			ui->LookupResult->setText(tr("Searching"));
			break;
	    case RsDhtPeerDht::FAILURE:
			ui->LookupProgressBar->setValue(0);
			ui->LookupResult->setText(tr("Lookup Failure"));
			mLookupStatus = CONNECT_LOOKUP_FAIL;
			break;
	    case RsDhtPeerDht::OFFLINE:
			ui->LookupProgressBar->setValue(100);
			ui->LookupResult->setText(tr("Peer Offline"));
			mLookupStatus = CONNECT_LOOKUP_OFFLINE;
			break;
	    case RsDhtPeerDht::UNREACHABLE:
			ui->LookupProgressBar->setValue(100);
			ui->LookupResult->setText(tr("Peer Firewalled"));
			mLookupStatus = CONNECT_LOOKUP_UNREACHABLE;
			break;
	    case RsDhtPeerDht::ONLINE:
			ui->LookupProgressBar->setValue(100);
			ui->LookupResult->setText(tr("Peer Online"));
			mLookupStatus = CONNECT_LOOKUP_ONLINE;
			break;
	}
#endif
}



void ConnectProgressDialog::updateUdpStatus()
{

	switch(mLookupStatus)
	{
		default:
		case CONNECT_LOOKUP_OFFLINE:
		case CONNECT_LOOKUP_NODHTCONFIG:
		case CONNECT_LOOKUP_FAIL:
			return;
			break;

		case CONNECT_LOOKUP_ONLINE:
		case CONNECT_LOOKUP_UNREACHABLE:
			break;
	}

	switch(mUdpStatus)
	{
		case CONNECT_UDP_FAIL:
			return;
			break;
		case CONNECT_UDP_INIT:
			mUdpTS = time(NULL);
			mUdpStatus = CONNECT_UDP_PROGRESS;
			ui->UdpResult->setText(tr("UDP Setup"));
			break;
		default:
		case CONNECT_UDP_PROGRESS:
			break;
	}


	time_t now = time(NULL);
	if (now > mUdpTS + CONNECT_UDP_PERIOD)
	{
		ui->UdpProgressBar->setValue(100);
		ui->UdpResult->setText(tr("UDP Connect Timeout"));
		mUdpStatus = CONNECT_UDP_FAIL;

		mState = CONNECT_STATE_FAILED;
		sayUdpTimeout();

		return;
	}

	ui->UdpProgressBar->setValue(calcProgress(now, mUdpTS, CONNECT_UDP_TYPICAL, CONNECT_UDP_SLOW, CONNECT_UDP_PERIOD));

#ifdef RS_USE_BITDHT
	/* now lookup details from Dht */
	RsDhtNetPeer status;
	rsDht->getNetPeerStatus(mId, status);

	QString fullString = QString::fromStdString(status.mConnectState);
	QString connectString = fullString.section(':',0,0);
	ui->UdpResult->setText(connectString);

	if (connectString == "Failed Wait")
	{
		/* Ensure minimum time for direct TCP connections */
		if (now > mInitTS + MINIMUM_CONNECT_PERIOD)
		{
			if (status.mCbPeerMsg == "ERROR : END:AUTH_DENIED")
			{
				/* disaster - connection not allowed */
				mState = CONNECT_STATE_DENIED;
				sayUdpDenied();
			}
			else
			{
				/* disaster - udp failed */
				mState = CONNECT_STATE_FAILED;
				sayUdpFailed();
			}
		}
	}
#endif
}


int calcProgress(time_t now, time_t start, int period50, int period75, int period100)
{
	int dt = now - start;
	if (dt < 0)
		return 0;
	if (dt > period100)
		return 100;

	if (dt < period50)
	{
		return 50 * (dt / (float) period50);
	}

	if (dt < period75)
	{
		return 50  + 25 * ((dt - period50) / (float) (period75 - period50));
	}
	return 75  + 25 * ((dt - period75) / (float) (period100 - period75));
}



	













/*********** Status ***********/


const uint32_t MESSAGE_STATUS_INPROGRESS	= 1;
const uint32_t MESSAGE_STATUS_SUCCESS		= 2;
const uint32_t MESSAGE_STATUS_CONFIG_ERROR	= 3;
const uint32_t MESSAGE_STATUS_PEER_ERROR	= 4;
const uint32_t MESSAGE_STATUS_UNKNOWN_ERROR	= 5;

void ConnectProgressDialog::sayInProgress()
{
	QString title = tr("Connection In Progress");
	QString message = tr("Initial connections can take a while, please be patient");
	message += "\n\n";
	message += tr("If an error is detected it will be displayed here");
	message += "\n\n";
	message += tr("You can close this dialog at any time");
	message += "\n";
	message += tr("Retroshare will continue connecting in the background");

	setStatusMessage(MESSAGE_STATUS_INPROGRESS, title, message);
}


void ConnectProgressDialog::sayConnectTimeout()
{
	QString title = tr("Connection Timeout");
	QString message = tr("Connection Attempt has taken too long");
	message += "\n";
	message += tr("But no error has been detected");
	message += "\n\n";
	message += tr("Try again shortly, Retroshare will continue connecting in the background");
	message += "\n\n";
	message += tr("If you continue to get this message, please contact developers");

	setStatusMessage(MESSAGE_STATUS_UNKNOWN_ERROR, title, message);
}


void ConnectProgressDialog::sayLookupTimeout()
{
	QString title = tr("DHT Lookup Timeout");
	QString message = tr("DHT Lookup has taken too long");
	message += "\n\n";
	message += tr("Try again shortly, Retroshare will continue connecting in the background");
	message += "\n\n";
	message += tr("If you continue to get this message, please contact developers");

	setStatusMessage(MESSAGE_STATUS_UNKNOWN_ERROR, title, message);
}

void ConnectProgressDialog::sayUdpTimeout()
{
	QString title = tr("UDP Connection Timeout");
	QString message = tr("UDP Connection has taken too long");
	message += "\n\n";
	message += tr("Try again shortly, Retroshare will continue connecting in the background");
	message += "\n\n";
	message += tr("If you continue to get this message, please contact developers");

	setStatusMessage(MESSAGE_STATUS_UNKNOWN_ERROR, title, message);
}


void ConnectProgressDialog::sayUdpFailed()
{
	QString title = tr("UDP Connection Failed");
	QString message = tr("We are continually working to improve connectivity.");
	message += "\n";
	message += tr("In this case the UDP connection attempt has failed.");
	message += "\n\n";
	message += tr("Improve connectivity by opening a Port in your Firewall.");
	message += "\n\n";
	message += tr("Retroshare will continue connecting in the background");
	message += "\n";
	message += tr("If you continue to get this message, please contact developers");

	setStatusMessage(MESSAGE_STATUS_UNKNOWN_ERROR, title, message);
}



void ConnectProgressDialog::sayConnected()
{
	QString title = tr("Connected");
	QString message = tr("Congratulations, you are connected");

	setStatusMessage(MESSAGE_STATUS_SUCCESS, title, message);
}

void ConnectProgressDialog::sayDHTFailed()
{
	QString title = tr("DHT startup Failed");
	QString message = tr("Your DHT has not started properly");
	message += "\n\n";
	message += tr("Common causes of this problem are:");
	message += "\n";
	message += tr("     - You are not connected to the Internet");
	message += "\n";
	message += tr("     - You have a missing or out-of-date DHT bootstrap file (bdboot.txt)");

	setStatusMessage(MESSAGE_STATUS_CONFIG_ERROR, title, message);
}

void ConnectProgressDialog::sayDHTOffline()
{
	QString title = tr("DHT is Disabled");
	QString message = tr("The DHT is OFF, so Retroshare cannot find your Friends.");
	message += "\n\n";
	message += tr("Retroshare has tried All Known Addresses, with no success");
	message += "\n";
	message += tr("The DHT is needed if your friends have Dynamic IP Addresses.");
	message += "\n";
	message += tr("Only Advanced Retroshare users should switch off the DHT.");
	message += "\n\n";
	message += tr("Go to Settings->Server and change config to \"Public: DHT and Discovery\"");

	setStatusMessage(MESSAGE_STATUS_CONFIG_ERROR, title, message);
}

void ConnectProgressDialog::sayDenied()
{
	QString title = tr("Peer Denied Connection");
	QString message = tr("We successfully reached your Friend.");
	message += "\n";
	message += tr("but they have not added you as a Friend.");
	message += "\n\n";
	message += tr("Please contact them to add your Certificate");
	message += "\n\n";
	message += tr("Your Retroshare Node is configured Okay");

	setStatusMessage(MESSAGE_STATUS_PEER_ERROR, title, message);
}


void ConnectProgressDialog::sayUdpDenied()
{
	QString title = tr("Peer Denied Connection");
	QString message = tr("We successfully reached your Friend via UDP.");
	message += "\n";
	message += tr("but they have not added you as a Friend.");
	message += "\n\n";
	message += tr("Please contact them to add your Full Certificate");
	message += "\n";
	message += tr("They need a Certificate + Node for UDP connections to succeed");
	message += "\n\n";
	message += tr("Your Retroshare Node is configured Okay");

	setStatusMessage(MESSAGE_STATUS_PEER_ERROR, title, message);
}


void ConnectProgressDialog::sayPeerOffline()
{
	QString title = tr("Peer Offline");
	QString message = tr("We Cannot find your Friend.");
	message += "\n\n";
	message += tr("They are either offline or their DHT is Off");
	message += "\n\n";
	message += tr("Your Retroshare Node is configured Okay");

	setStatusMessage(MESSAGE_STATUS_PEER_ERROR, title, message);
}


void ConnectProgressDialog::sayPeerNoDhtConfig()
{
	QString title = tr("Peer DHT is Disabled");
	QString message = tr("Your Friend has configured Retroshare with DHT Disabled.");
	message += "\n\n";
	message += tr("You have previously connected to this Friend");
	message += "\n";
	message += tr("Retroshare has determined that they have DHT switched off");
	message += "\n";
	message += tr("Without the DHT it is hard for Retroshare to locate your friend");
	message += "\n\n";
	message += tr("Try importing a fresh Certificate to get up-to-date connection information");
	message += "\n\n";
	message += tr("If you continue to get this message, please contact developers");

	setStatusMessage(MESSAGE_STATUS_PEER_ERROR, title, message);
}


void ConnectProgressDialog::sayInvalidPeer()
{
	QString title = tr("Incomplete Friend Details");
	QString message = tr("You have imported an incomplete Certificate");
	message += "\n\n";
	message += tr("Retroshare cannot connect without this information");
	message += "\n\n";
	message += tr("Please retry importing the full Certificate");

	setStatusMessage(MESSAGE_STATUS_CONFIG_ERROR, title, message);
}


void ConnectProgressDialog::setStatusMessage(uint32_t status, const QString &title, const QString &message)
{
	switch(status)
	{
		case MESSAGE_STATUS_INPROGRESS:
			{
				QPixmap pm(":/images/graph-blue.png");
				ui->statusIcon->setPixmap(pm.scaledToWidth(40));
			}
			break;
		case MESSAGE_STATUS_SUCCESS:
			{
				QPixmap pm(":/images/graph-downloaded.png");
				ui->statusIcon->setPixmap(pm.scaledToWidth(40));
			}
			break;
		case MESSAGE_STATUS_CONFIG_ERROR:
			{
				QPixmap pm(":/images/graph-downloading.png");
				ui->statusIcon->setPixmap(pm.scaledToWidth(40));
			}
			break;
		case MESSAGE_STATUS_PEER_ERROR:
			{
				QPixmap pm(":/images/graph-checking.png");
				ui->statusIcon->setPixmap(pm.scaledToWidth(40));
			}
			break;
		case MESSAGE_STATUS_UNKNOWN_ERROR:
			{
				QPixmap pm(":/images/graph-checking.png");
				ui->statusIcon->setPixmap(pm.scaledToWidth(40));
			}
			break;
	}

	ui->statusTitle->setText(title);
	ui->textBrowser->setText(message);
}



