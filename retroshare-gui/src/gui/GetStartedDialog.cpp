/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2011, drbob
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

#include "gui/GetStartedDialog.h"
#include "gui/connect/ConnectFriendWizard.h"
#include "retroshare/rspeers.h"
#include "retroshare/rsdisc.h"
#include "retroshare/rsconfig.h"

#include "gui/RsAutoUpdatePage.h"

#include <QDesktopServices>

#include <iostream>
#include <sstream>

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

/* Hide platform specific features */
#ifdef Q_WS_WIN

#endif
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
	uint32_t userLevel = RSCONFIG_USER_LEVEL_NEW;
	userLevel = rsConfig->getUserLevel();

	ui.inviteCheckBox->setChecked(false);
	ui.addCheckBox->setChecked(false);
	ui.connectCheckBox->setChecked(false);
	ui.firewallCheckBox->setChecked(false);

	switch(userLevel)
	{
		// FALLS THROUGH EVERYWHERE.
		case RSCONFIG_USER_LEVEL_POWER:
		case RSCONFIG_USER_LEVEL_OVERRIDE:
			ui.firewallCheckBox->setChecked(true);

		case RSCONFIG_USER_LEVEL_CASUAL:
			ui.connectCheckBox->setChecked(true);

		case RSCONFIG_USER_LEVEL_BASIC:
			ui.addCheckBox->setChecked(true);
			ui.inviteCheckBox->setChecked(true);

		default:
		case RSCONFIG_USER_LEVEL_NEW:

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





static void sendMail(std::string sAddress, std::string sSubject, std::string sBody)
{
	/* Only under windows do we need to do this! */
#ifdef Q_WS_WIN
    /* search and replace the end of lines with: "%0D%0A" */
    size_t loc;
    while ((loc = sBody.find("\n")) != sBody.npos) {
        sBody.replace(loc, 1, "%0D%0A");
    }
#endif

    std::string mailstr = "mailto:" + sAddress;
    mailstr += "?subject=" + sSubject;
    mailstr += "&body=" + sBody;

    std::cerr << "MAIL STRING:" << mailstr.c_str() << std::endl;

    /* pass the url directly to QDesktopServices::openUrl */
    QDesktopServices::openUrl (QUrl (QString::fromUtf8(mailstr.c_str())));
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

    		bool shouldAddSignatures = false;
    		cert = rsPeers->GetRetroshareInvite(shouldAddSignatures);

        	RsAutoUpdatePage::unlockAllEvents() ;
	}

	std::ostringstream out;







    	QString trstr;
	trstr = tr("Your friend has installed Retroshare, and would like you to try it out.");
	out << trstr.toStdString() << std::endl;
	out << std::endl;
	trstr = tr("You can get Retroshare here: http://retroshare.sourceforge.net/download");
	out << trstr.toStdString() << std::endl;
	out << std::endl;
	trstr = tr("Retroshare is a private Friend-2-Friend sharing network.");
	out << trstr.toStdString() << std::endl;
	trstr = tr("It has an many features, including built-in chat, messaging, "); 
	out << trstr.toStdString() << std::endl;
	trstr = tr("forums and channels, all of which are as secure as the file-sharing.");
	out << trstr.toStdString() << std::endl;
	out << std::endl;
	out << std::endl;
	trstr = tr("Below is your friends ID Certificate. Cut and paste this into your Retroshare client");
	out << trstr.toStdString() << std::endl;
	trstr = tr("and send them your ID Certificate to enable the secure connection");
	out << trstr.toStdString() << std::endl;
	out << std::endl;
	out << cert;
	out << std::endl;

	sendMail("", tr("Retroshare Invitation").toStdString(), out.str());

}


void GetStartedDialog::emailSubscribe()
{
	std::ostringstream out;
	out << "Please let me know when Retroshare has a new release, or exciting news";
	out << std::endl;
	out << std::endl;
	out << "Furthermore, I'd like to say ... ";
	out << std::endl;
	out << std::endl;

	sendMail("lists@retroshare.org", "Subscribe", out.str());
}


void GetStartedDialog::emailUnsubscribe()
{
	std::ostringstream out;
	out << "I am no longer interested in Retroshare News.";
	out << std::endl;
	out << "Please remove me from the Mailing List";
	out << std::endl;

	sendMail("lists@retroshare.org", "Unsubscribe", out.str());
}


void GetStartedDialog::emailFeedback()
{
	std::ostringstream out;
	out << "Dear Retroshare Developers";
	out << std::endl;
	out << std::endl;
	out << "I've tried out Retroshare and would like provide feedback:";
	out << std::endl;
	out << std::endl;
	out << "To make Retroshare more user friendly, please [ what do you think? ] ";
	out << std::endl;
	out << "The best feature of Retroshare is [ what do you think? ] ";
	out << std::endl;
	out << "and the biggest missing feature is [ what do you think? ] ";
	out << std::endl;
	out << std::endl;
	out << "Furthermore, I'd like to say ... ";
	out << std::endl;
	out << std::endl;

	sendMail("feedback@retroshare.org", tr("RetroShare Feedback").toStdString(), out.str());
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

	std::string versionString;
	uint32_t    userLevel;
	{
        	RsAutoUpdatePage::lockAllEvents();

		/* set retroshare version */
    		std::map<std::string, std::string>::iterator vit;
    		std::map<std::string, std::string> versions;
        	bool retv = rsDisc->getDiscVersions(versions);
			std::string id = rsPeers->getOwnId();
        	if (retv && versions.end() != (vit = versions.find(id)))
        	{
			versionString = vit->second;
        	}
		userLevel = rsConfig->getUserLevel();

        	RsAutoUpdatePage::unlockAllEvents() ;
	}

	std::ostringstream out;
	out << "Hello";
	out << std::endl;
	out << std::endl;

	out << "My Retroshare Configuration is: (" << versionString;
	out << ", ";

#ifdef __APPLE__

  #ifdef Q_WS_MAC
	switch(QSysInfo::MacintoshVersion)
	{
  		case QSysInfo::MV_9: 
			out << "Mac OS 9";
			break;
  		case QSysInfo::MV_10_0: 
			out << "Mac OSX 10.0";
			break;
  		case QSysInfo::MV_10_1: 
			out << "Mac OSX 10.1";
			break;
  		case QSysInfo::MV_10_2: 
			out << "Mac OSX 10.2";
			break;
  		case QSysInfo::MV_10_3: 
			out << "Mac OSX 10.3";
			break;
  		case QSysInfo::MV_10_4: 
			out << "Mac OSX 10.4";
			break;
  		case QSysInfo::MV_10_5: 
			out << "Mac OSX 10.5";
			break;
  		case QSysInfo::MV_10_6: 
			out << "Mac OSX 10.6";
			break;
//  		case QSysInfo::MV_10_7: 
//			out << "Mac OSX 10.7";
//			break;
  		default: 
			out << "Mac Unknown";
			break;
	}
  #else
	out << "OSX Unknown";
  #endif
#else
  #if defined(_WIN32) || defined(__MINGW32__)
    // Windows
    #ifdef Q_WS_WIN
	switch(QSysInfo::windowsVersion())
	{
  		case QSysInfo::WV_32s: 
			out << "Windows 2.1";
			break;
  		case QSysInfo::WV_95: 
			out << "Windows 95";
			break;
  		case QSysInfo::WV_98: 
			out << "Windows 98";
			break;
  		case QSysInfo::WV_Me: 
			out << "Windows Me";
			break;
  		case QSysInfo::WV_NT: 
			out << "Windows NT";
			break;
  		case QSysInfo::WV_2000: 
			out << "Windows 2000";
			break;
  		case QSysInfo::WV_XP:
			out << "Windows XP";
			break;
  		case QSysInfo::WV_2003: 
			out << "Windows 2003";
			break;
  		case QSysInfo::WV_VISTA: 
			out << "Windows Vista";
			break;
		case QSysInfo::WV_WINDOWS7:
			out << "Windows 7";
			break;
  		default: 
			out << "Windows";
			break;
	}
    #else
	out << "Windows Unknown";
    #endif
  #else
	// Linux
	out << "Linux";
  #endif
#endif
	out << ", 0x60" << std::hex << userLevel << std::dec;
	out << ")" << std::endl;
	out << std::endl;

	out << "I am having trouble with Retroshare.";
	out << " Can you help me with....";
	out << std::endl;
	out << std::endl;

	sendMail("support@retroshare.org", tr("RetroShare Support").toStdString(), out.str());
}


void GetStartedDialog::OpenFAQ()
{
    /* pass the url directly to QDesktopServices::openUrl */
    std::string faq_url("http://retroshare.sourceforge.net/wiki/index.php/Frequently_Asked_Questions");
    QDesktopServices::openUrl (QUrl (QString::fromStdString(faq_url)));
}

void GetStartedDialog::OpenForums()
{
    /* pass the url directly to QDesktopServices::openUrl */
    std::string forum_url("http://retroshare.sourceforge.net/forum/");
    QDesktopServices::openUrl (QUrl (QString::fromStdString(forum_url)));
}

void GetStartedDialog::OpenWebsite()
{
    /* pass the url directly to QDesktopServices::openUrl */
    std::string website_url("http://retroshare.org");
    QDesktopServices::openUrl (QUrl (QString::fromStdString(website_url)));
}



