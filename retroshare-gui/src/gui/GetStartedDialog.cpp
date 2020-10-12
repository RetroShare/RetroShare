/*******************************************************************************
 * gui/GetStartedDialog.cpp                                                    *
 *                                                                             *
 * Copyright (C) 2011 Robert Fernie   <retroshare.project@gmail.com>           *
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

#include "gui/GetStartedDialog.h"
#include "gui/connect/ConnectFriendWizard.h"
#include "retroshare/rspeers.h"
#include "retroshare/rsdisc.h"
#include "retroshare/rsconfig.h"

#include "retroshare-gui/RsAutoUpdatePage.h"
#include "rshare.h"

#include <QDesktopServices>

#include <iostream>

#define URL_FAQ         "http://retroshare.sourceforge.net/wiki/index.php/Frequently_Asked_Questions"
#define URL_FORUM       "https://github.com/RetroShare/RetroShare/issues"
#define URL_WEBSITE     "http://retroshare.net"
#define URL_DOWNLOAD    "http://retroshare.net/downloads.html"

#define EMAIL_SUBSCRIBE "lists@retroshare.org"

/** Constructor */
GetStartedDialog::GetStartedDialog(QWidget *parent)
: MainPage(parent)
{
	/* Invoke the Qt Designer generated object setup routine */
	ui.setupUi(this);

	mTimer = NULL;
	mInviteTimer = NULL;

	/* we use a flag to setup the GettingStarted Flags, so that RS has a bit of time to initialise itself
	 */

	mFirstShow = true;

	connect(ui.inviteCheckBox, SIGNAL(stateChanged( int )), this, SLOT(tickInviteChanged()));
	connect(ui.addCheckBox, SIGNAL(stateChanged( int )), this, SLOT(tickAddChanged()));
	connect(ui.connectCheckBox, SIGNAL(stateChanged( int )), this, SLOT(tickConnectChanged()));
	connect(ui.firewallCheckBox, SIGNAL(stateChanged( int )), this, SLOT(tickFirewallChanged()));

	connect(ui.pushButton_InviteFriends, SIGNAL(clicked( bool )), this, SLOT(inviteFriends()));
	connect(ui.pushButton_AddFriend, SIGNAL(clicked( bool )), this, SLOT(addFriends()));

	connect(ui.pushButton_FAQ, SIGNAL(clicked( bool )), this, SLOT(OpenFAQ()));
	connect(ui.pushButton_Forums, SIGNAL(clicked( bool )), this, SLOT(OpenForums()));
	connect(ui.pushButton_Website, SIGNAL(clicked( bool )), this, SLOT(OpenWebsite()));
	connect(ui.pushButton_EmailFeedback, SIGNAL(clicked( bool )), this, SLOT(emailFeedback()));
	connect(ui.pushButton_EmailSupport, SIGNAL(clicked( bool )), this, SLOT(emailSupport()));
}

GetStartedDialog::~GetStartedDialog()
{
}

void GetStartedDialog::changeEvent(QEvent *e)
{
	switch (e->type()) {
	case QEvent::LanguageChange:
		ui.retranslateUi(this);
		break;
	default:
		break;
	}
}

void GetStartedDialog::showEvent ( QShowEvent * /*event*/ )
{
	/* do nothing if locked, or not visible */
	if (RsAutoUpdatePage::eventsLocked() == true)
	{
		std::cerr << "GetStartedDialog::showEvent() events Are Locked" << std::endl;
		return;
	}

	if ((mFirstShow) && (rsConfig))
	{
		RsAutoUpdatePage::lockAllEvents();

		updateFromUserLevel();
		mFirstShow = false;

		RsAutoUpdatePage::unlockAllEvents() ;
	}
}

void GetStartedDialog::updateFromUserLevel()
{
	RsConfigUserLvl userLevel = RsConfigUserLvl::NEW;
	userLevel = rsConfig->getUserLevel();

	ui.inviteCheckBox->setChecked(false);
	ui.addCheckBox->setChecked(false);
	ui.connectCheckBox->setChecked(false);
	ui.firewallCheckBox->setChecked(false);

	switch(userLevel)
	{
		// FALLS THROUGH EVERYWHERE.
	    case RsConfigUserLvl::POWER:
	    case RsConfigUserLvl::OVERRIDE:
			ui.firewallCheckBox->setChecked(true);
			/* fallthrough */
	    case RsConfigUserLvl::CASUAL:
			ui.connectCheckBox->setChecked(true);
			/* fallthrough */
	    case RsConfigUserLvl::BASIC:
			ui.addCheckBox->setChecked(true);
			ui.inviteCheckBox->setChecked(true);

		default:
	    case RsConfigUserLvl::NEW:

			break;
	}

	/* will this auto trigger changes? */

}

void GetStartedDialog::tickInviteChanged()
{
	if (ui.inviteCheckBox->isChecked())
	{
		ui.inviteTextBrowser->setVisible(false);
	}
	else
	{
		ui.inviteTextBrowser->setVisible(true);
	}
}

void GetStartedDialog::tickAddChanged()
{
	if (ui.addCheckBox->isChecked())
	{
		ui.addTextBrowser->setVisible(false);
	}
	else
	{
		ui.addTextBrowser->setVisible(true);
	}
}

void GetStartedDialog::tickConnectChanged()
{
	if (ui.connectCheckBox->isChecked())
	{
		ui.connectTextBrowser->setVisible(false);
	}
	else
	{
		ui.connectTextBrowser->setVisible(true);
	}
}

void GetStartedDialog::tickFirewallChanged()
{
	if (ui.firewallCheckBox->isChecked())
	{
		ui.firewallTextBrowser->setVisible(false);
	}
	else
	{
		ui.firewallTextBrowser->setVisible(true);
	}
}

static void sendMail(const QString &address, const QString &subject, QString body)
{
	/* Only under windows do we need to do this! */
#ifdef Q_OS_WIN
	/* search and replace the end of lines with: "%0D%0A" */
	body.replace("\n", "%0D%0A");
#endif

	QString mailstr = "mailto:" + address;
	mailstr += "?subject=" + subject;
	mailstr += "&body=" + body;

	std::cerr << "MAIL STRING:" << mailstr.toStdString() << std::endl;

	/* pass the url directly to QDesktopServices::openUrl */
	QDesktopServices::openUrl(QUrl(mailstr));
}

void GetStartedDialog::addFriends()
{
	ConnectFriendWizard connwiz(this);

	connwiz.show();
	connwiz.next();
	connwiz.exec();
}

void GetStartedDialog::inviteFriends()
{
	if (RsAutoUpdatePage::eventsLocked() == true)
	{	
		std::cerr << "GetStartedDialog::inviteFriends() EventsLocked... waiting";
		std::cerr << std::endl;

		if (!mInviteTimer)
		{
			mInviteTimer = new QTimer(this);
			mInviteTimer->connect(mTimer, SIGNAL(timeout()), this, SLOT(inviteFriends()));
			mInviteTimer->setInterval(100); /* 1/10 second */
			mInviteTimer->setSingleShot(true);
		}

		mInviteTimer->start();
		return;
	}

	std::string cert;
	{
		RsAutoUpdatePage::lockAllEvents();

		cert = rsPeers->GetRetroshareInvite(RsPeerId(),false,false);

		RsAutoUpdatePage::unlockAllEvents() ;
	}

	QString text = QString("%1\n%2\n\n%3\n").arg(GetInviteText()).arg(GetCutBelowText()).arg(QString::fromUtf8(cert.c_str()));

	sendMail("", tr("RetroShare Invitation"), text);
}

QString GetStartedDialog::GetInviteText()
{
	QString text = tr("Your friend has installed RetroShare, and would like you to try it out.") + "\n";
	text += "\n";
	text += tr("You can get RetroShare here: %1").arg(URL_DOWNLOAD) + "\n";
	text += "\n";
	text += tr("RetroShare is a private Friend-2-Friend sharing network.") + "\n";
	text += tr("It has many features, including built-in chat, messaging,") + "\n";
	text += tr("forums and channels, all of which are as secure as the file-sharing.") + "\n";
	text += "\n";
	text += "\n";
	text += tr("Here is your friends ID Certificate.") + "\n";
	text += tr("Cut and paste the text below into your RetroShare client") + "\n";
	text += tr("and send them your ID Certificate to get securely connected.") + "\n";

	return text;
}

QString GetStartedDialog::GetCutBelowText()
{
	return QString("%1 <-------------------------------------------------------------------------------------").arg(tr("Cut Below Here"));
}

void GetStartedDialog::emailSubscribe()
{
	// when translation is needed, replace QString by tr
	QString text = QString("Please let me know when RetroShare has a new release, or exciting news") + "\n";
	text += "\n";
	text += QString("Furthermore, I'd like to say ...") + "\n";
	text += "\n";

	sendMail(EMAIL_SUBSCRIBE, "Subscribe", text);
}


void GetStartedDialog::emailUnsubscribe()
{
	// when translation is needed, replace QString by tr
	QString text = QString("I am no longer interested in RetroShare News.") + "\n";
	text += QString("Please remove me from the Mailing List") + "\n";

	sendMail(EMAIL_SUBSCRIBE, "Unsubscribe", text);
}

void GetStartedDialog::emailFeedback()
{
	// when translation is needed, replace QString by tr
	QString text = QString("Dear RetroShare Developers") + "\n";
	text += "\n";
	text += QString("I've tried out RetroShare and would like provide feedback:") + "\n";
	text += "\n";
	text += QString("To make RetroShare more user friendly, please [ what do you think? ] ") + "\n";
	text += QString("The best feature of RetroShare is [ what do you think? ] ") + "\n";
	text += QString("and the biggest missing feature is [ what do you think? ] ") + "\n";
	text += "\n";
	text += QString("Furthermore, I'd like to say ... ") + "\n";
	text += "\n";

	sendMail("feedback@retroshare.org", tr("RetroShare Feedback"), text);
}

void GetStartedDialog::emailSupport()
{
	if (RsAutoUpdatePage::eventsLocked() == true)
	{	
		std::cerr << "GetStartedDialog::emailSupport() EventsLocked... waiting";
		std::cerr << std::endl;

		if (!mTimer)
		{
			mTimer = new QTimer(this);
			mTimer->connect(mTimer, SIGNAL(timeout()), this, SLOT(emailSupport()));
			mTimer->setInterval(100); /* 1/10 second */
			mTimer->setSingleShot(true);
		}

		mTimer->start();
		return;
	}

	RsConfigUserLvl    userLevel;
	{
		RsAutoUpdatePage::lockAllEvents();

		userLevel = rsConfig->getUserLevel();

		RsAutoUpdatePage::unlockAllEvents() ;
	}

	// when translation is needed, replace QString by tr
	QString text = QString("Hello") + "\n";
	text += "\n";

	QString sysVersion;

#ifdef __APPLE__

  #ifdef Q_OS_MAC
	switch(QSysInfo::MacintoshVersion)
	{
		case QSysInfo::MV_9: 
			sysVersion = "Mac OS 9";
			break;
		case QSysInfo::MV_10_0: 
			sysVersion = "Mac OSX 10.0";
			break;
		case QSysInfo::MV_10_1: 
			sysVersion = "Mac OSX 10.1";
			break;
		case QSysInfo::MV_10_2: 
			sysVersion = "Mac OSX 10.2";
			break;
		case QSysInfo::MV_10_3: 
			sysVersion = "Mac OSX 10.3";
			break;
		case QSysInfo::MV_10_4: 
			sysVersion = "Mac OSX 10.4";
			break;
		case QSysInfo::MV_10_5: 
			sysVersion = "Mac OSX 10.5";
			break;
		case QSysInfo::MV_10_6: 
			sysVersion = "Mac OSX 10.6";
			break;
//		case QSysInfo::MV_10_7: 
//			sysVersion = "Mac OSX 10.7";
//			break;
		default: 
			sysVersion = "Mac Unknown";
			break;
	}
  #else
    sysVersion = "OSX Unknown";
  #endif
#else
  #if defined(_WIN32) || defined(__MINGW32__)
	// Windows
	#ifdef Q_OS_WIN
	switch(QSysInfo::windowsVersion())
	{
		case QSysInfo::WV_32s: 
			sysVersion = "Windows 2.1";
			break;
		case QSysInfo::WV_95: 
			sysVersion = "Windows 95";
			break;
		case QSysInfo::WV_98: 
			sysVersion = "Windows 98";
			break;
		case QSysInfo::WV_Me: 
			sysVersion = "Windows Me";
			break;
		case QSysInfo::WV_NT: 
			sysVersion = "Windows NT";
			break;
		case QSysInfo::WV_2000: 
			sysVersion = "Windows 2000";
			break;
		case QSysInfo::WV_XP:
			sysVersion = "Windows XP";
			break;
		case QSysInfo::WV_2003: 
			sysVersion = "Windows 2003";
			break;
		case QSysInfo::WV_VISTA: 
			sysVersion = "Windows Vista";
			break;
		case QSysInfo::WV_WINDOWS7:
			sysVersion = "Windows 7";
			break;
		default: 
			sysVersion = "Windows";
			break;
	}
	#else
	sysVersion = "Windows Unknown";
	#endif
  #else
	// Linux
	sysVersion = "Linux";
  #endif
#endif
	text += QString("My RetroShare Configuration is: (%1, %2, %3)").arg(Rshare::retroshareVersion(true)).arg(sysVersion).arg(static_cast<typename std::underlying_type<RsConfigUserLvl>::type>(userLevel)) + "\n";
    text += "\n";

	text += QString("I am having trouble with RetroShare.");
	text += QString(" Can you help me with....") + "\n";
	text += "\n";

	sendMail("support@retroshare.org", tr("RetroShare Support"), text);
}

void GetStartedDialog::OpenFAQ()
{
	/* pass the url directly to QDesktopServices::openUrl */
	QDesktopServices::openUrl(QUrl(URL_FAQ));
}

void GetStartedDialog::OpenForums()
{
	/* pass the url directly to QDesktopServices::openUrl */
	QDesktopServices::openUrl(QUrl(URL_FORUM));
}

void GetStartedDialog::OpenWebsite()
{
	/* pass the url directly to QDesktopServices::openUrl */
	QDesktopServices::openUrl(QUrl(URL_WEBSITE));
}
