/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2009 The RetroShare Team, Oleksiy Bilyanskyy
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

#include "ConnectFriendWizard.h"

#include "rsiface/rspeers.h" //for rsPeers variable
#include "rsiface/rsiface.h"

#include <QLineEdit>
#include <QTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QCheckBox>
#include <QGroupBox>
#include <QComboBox>
#include <QtGui>
#include <QClipboard>
#include <QTableWidget>
#include <QHeaderView>

#include <QFileDialog>

#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include <QMessageBox>

#include <QRegExpValidator>
#include <QRegExp>

#include <QDebug>
#include <sstream>
#include <iostream>
#include <set>

//============================================================================
//! 
ConnectFriendWizard::ConnectFriendWizard(QWidget *parent)
                    : QWizard(parent)
{
    setPage(Page_Intro, new IntroPage);
    setPage(Page_Text, new TextPage);
    setPage(Page_Cert, new CertificatePage);
    setPage(Page_Foff, new FofPage);
    setPage(Page_ErrorMessage, new ErrorMessagePage);
    setPage(Page_Conclusion, new ConclusionPage);

    setStartId(Page_Intro);

// this define comes from Qt example. I don't have mac, so it wasn't tested
#ifndef Q_WS_MAC
    setWizardStyle(ModernStyle);
#endif

// at this moment I don't know, what information should be in help
//    setOption(HaveHelpButton, true); 
//    connect(this, SIGNAL(helpRequested()), this, SLOT(showHelp()));
    
    setPixmap(QWizard::LogoPixmap,
              QPixmap(":/images/connect/connectFriendLogo.png"));
// we have no good pictures for watermarks
//    setPixmap(QWizard::WatermarkPixmap,
//              QPixmap(":/images/connectFriendWatermark.png"));
    setPixmap(QWizard::BannerPixmap,
              QPixmap(":/images/connect/connectFriendBanner.png")) ;
              
    setWindowTitle(tr("Connect Friend Wizard"));

}

//============================================================================

void
ConnectFriendWizard::accept()
{
    if ( hasVisitedPage(Page_Conclusion) )
    {
        std::string authId = field("idField").toString().toStdString();
        std::string authCode = field("authCode").toString().toStdString();

        rsPeers->AuthCertificate(authId, authCode );
        rsPeers->addFriend(authId);
        rsicontrol->getNotify().notifyListChange(NOTIFY_LIST_NEIGHBOURS,1) ;
    }

    QDialog::accept();
}

//============================================================================
//============================================================================
//============================================================================
//
IntroPage::IntroPage(QWidget *parent)
    : QWizardPage(parent)
{
    QString titleStr("<span style=\"font-size:14pt; font-weight:500;"
                               "color:#32cd32;\">%1</span>");
    setTitle( titleStr.arg( tr("Add a new Friend") ) ) ;
             
    setSubTitle(tr("This wizard will help you to connect to your friend(s) "
                   "to RetroShare network. There are three possible ways "
                   "to do this:")) ;

    textRadioButton = new QRadioButton(tr("&Enter the certificate manually"));
    certRadioButton = new QRadioButton(tr("&Use *.pqi files with certificates" ));
    foffRadioButton = new QRadioButton(tr("&Make friend with selected friends of my friends" ));
    textRadioButton->setChecked(true);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(textRadioButton);
    layout->addWidget(certRadioButton);
    layout->addWidget(foffRadioButton);
    setLayout(layout);
}
//
//============================================================================
//
int IntroPage::nextId() const
{
    if (textRadioButton->isChecked()) return ConnectFriendWizard::Page_Text;
    if (certRadioButton->isChecked()) return ConnectFriendWizard::Page_Cert;
    if (foffRadioButton->isChecked()) return ConnectFriendWizard::Page_Foff;
}
//
//============================================================================
//============================================================================
//============================================================================
//
TextPage::TextPage(QWidget *parent)
    : QWizardPage(parent)
{
    QString titleStr("<span style=\"font-size:14pt; font-weight:500;"
                               "color:#32cd32;\">%1</span>");
    setTitle( titleStr.arg( tr("Text certificate") ) ) ;
    
    setSubTitle(tr("Use text representation of the XPGP certificates."));

    userCertLabel = new QLabel(tr("The text below is your XPGP certificate. "
                                  "You have to provide it to your friend "));

    std::cerr << "TextPage() getting Invite" << std::endl;
    userCertEdit = new QTextEdit;
    std::string invite = rsPeers->GetRetroshareInvite();

    //add the ip local and external address after the signature
    RsPeerDetails ownDetail;
    rsPeers->getPeerDetails(rsPeers->getOwnId(), ownDetail);
    invite += LOCAL_IP;
    invite += ownDetail.localAddr + ":";
    std::ostringstream out;
    out << ownDetail.localPort;
    invite += out.str() + ";";
    invite += "\n";
    invite += EXT_IP;
    invite += ownDetail.extAddr + ":";
    std::ostringstream out2;
    out2 << ownDetail.extPort;
    invite += out2.str() + ";";

    userCertEdit->setText(QString::fromStdString(invite));
    userCertEdit->setReadOnly(true);
    userCertEdit->setMinimumHeight(200);

    std::cerr << "TextPage() getting Invite: " << invite << std::endl;

    userCertHelpButton = new QPushButton;
    userCertHelpButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    userCertHelpButton->setFixedSize(20,20);
    userCertHelpButton->setFlat(true);
    userCertHelpButton->setIcon( QIcon(":images/connect/info16.png") );
    connect (userCertHelpButton,  SIGNAL( clicked()),
             this,                SLOT(   showHelpUserCert()) );
    
    userCertCopyButton = new QPushButton;
    userCertCopyButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    userCertCopyButton->setFixedSize(20,20);
    userCertCopyButton->setFlat(true);
    userCertCopyButton->setIcon( QIcon(":images/copyrslink.png") );
    userCertCopyButton->setToolTip(tr("Copy your Cert to Clipboard"));
    connect (userCertCopyButton,  SIGNAL( clicked()),
             this,                SLOT(   copyCert()) );         
             
#if defined(Q_OS_WIN)
    userCertMailButton = new QPushButton;
    userCertMailButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    userCertMailButton->setFixedSize(20,20);
    userCertMailButton->setFlat(true);
    userCertMailButton->setIcon( QIcon(":images/connect/mail_send.png") );
    userCertMailButton->setToolTip(tr("Run Email program"));
    connect (userCertMailButton,  SIGNAL( clicked()),
             this,                SLOT(   runEmailClient()) );
#endif
    userCertButtonsLayout = new QVBoxLayout();
    userCertButtonsLayout->addWidget(userCertHelpButton);
    userCertButtonsLayout->addWidget(userCertCopyButton);
#if defined(Q_OS_WIN)
    userCertButtonsLayout->addWidget(userCertMailButton);
#endif
    userCertLayout = new QHBoxLayout();
    userCertLayout->addWidget(userCertEdit);
    userCertLayout->addLayout(userCertButtonsLayout);

    friendCertLabel = new QLabel(tr("Please, paste your friends XPGP "
                                    "certificate into the box below" )) ;
    
    friendCertEdit = new QTextEdit;
    registerField("aaabbb", friendCertEdit, "plainText");//, "textChanged");

    //=== add all widgets to one layout
    textPageLayout = new QVBoxLayout();
    textPageLayout->addWidget(userCertLabel);
    textPageLayout->addLayout(userCertLayout);
    textPageLayout->addWidget(friendCertLabel);
    textPageLayout->addWidget(friendCertEdit);
//
    setLayout(textPageLayout);
}
//
//============================================================================
//
#if defined(Q_OS_WIN)

#include <iostream>
#include <windows.h>

void 
TextPage::runEmailClient()
{
	std::string mailstr = "mailto:";
    mailstr += "?subject=RetroShare Invite";

	mailstr += "&body=";

	mailstr += (userCertEdit->toPlainText()).toStdString();

	/* search and replace the end of lines with: "%0D%0A" */
	std::cerr << "MAIL STRING:" << mailstr.c_str() << std::endl;
	size_t loc;
	while((loc = mailstr.find("\n")) != mailstr.npos)
	{
		/* sdfkasdflkjh */
		mailstr.replace(loc, 1, "%0D%0A");
	}

	HINSTANCE hInst = ShellExecuteA(0, "open", mailstr.c_str(), 
		                            NULL, NULL, SW_SHOW);

    if(reinterpret_cast<int>(hInst) <= 32)
    {
	/* error */
	std::cerr << "ShellExecute Error: " << reinterpret_cast<int>(hInst);
	std::cerr << std::endl;
    }
}
#endif
//
//============================================================================
//
void
TextPage::showHelpUserCert()
{
    QMessageBox::information(this,
                             tr("Connect Friend Help"),
                             tr("You can copy this text and send it to your "
                                "friend via email or some other way"));                          

}
//
//============================================================================
//
void
TextPage::copyCert()
{
    QMessageBox::information(this,
                             tr("RetroShare"),
                             tr("Your Cert is copied to Clipbard, paste and send it to your"
                                "friend via email or some other way"));
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(userCertEdit->toPlainText());                            

}
//
//============================================================================
//

int
TextPage::nextId() const
{
    std::string id;
    std::string certstr;
    
    certstr = friendCertEdit->toPlainText().toStdString();

    if ( rsPeers->LoadCertificateFromString(certstr, id) )
    {
	//parse the text to get ip address
	try {
#ifdef FRIEND_WIZARD_DEBUG
	    std::cerr << "Paring cert for ip detection : " << certstr << std::endl;
#endif
	    int parsePosition = certstr.find(LOCAL_IP);
#ifdef FRIEND_WIZARD_DEBUG
	    std::cerr << "local ip position : " << parsePosition << std::endl;
#endif
	    if (parsePosition != std::string::npos) {
		//let's parse ip local address
		parsePosition += LOCAL_IP.length();
		std::string subCert = certstr.substr(parsePosition);
		parsePosition = subCert.find(":");
		std::string local_ip = subCert.substr(0, parsePosition);
#ifdef FRIEND_WIZARD_DEBUG
		std::cerr << "Local Ip : " << local_ip << std::endl;
#endif

		//let's parse local port
		subCert = subCert.substr(parsePosition + 1);
		parsePosition = subCert.find(";");
		std::string local_port_string = subCert.substr(0, parsePosition);
#ifdef FRIEND_WIZARD_DEBUG
		std::cerr << "Local port : " << local_port_string << std::endl;
#endif
		std::istringstream iss(local_port_string);
		int local_port;
		iss >> local_port;

#ifdef FRIEND_WIZARD_DEBUG
		std::cerr << "ConnectFriendWizard : saving ip local address." << std::endl;
#endif
		rsPeers->setLocalAddress(id, local_ip, local_port);

		//let's parse ip ext address
		parsePosition = certstr.find(EXT_IP);
#ifdef FRIEND_WIZARD_DEBUG
		std::cerr << "local ip position : " << parsePosition << std::endl;
#endif
		if (parsePosition != std::string::npos) {
		    parsePosition = parsePosition + EXT_IP.length();
		    subCert = certstr.substr(parsePosition);
		    parsePosition = subCert.find(":");
		    std::string ext_ip = subCert.substr(0, parsePosition);
    #ifdef FRIEND_WIZARD_DEBUG
		    std::cerr << "Ext Ip : " << ext_ip << std::endl;
    #endif

		    //let's parse ext port
		    subCert = subCert.substr(parsePosition + 1);
		    parsePosition = subCert.find(";");
		    std::string ext_port_string = subCert.substr(0, parsePosition);
    #ifdef FRIEND_WIZARD_DEBUG
		    std::cerr << "Ext port : " << ext_port_string << std::endl;
    #endif
		    std::istringstream iss2(ext_port_string);
		    int ext_port;
		    iss2 >> ext_port;

#ifdef FRIEND_WIZARD_DEBUG
		    std::cerr << "ConnectFriendWizard : saving ip ext address." << std::endl;
#endif
		    rsPeers->setExtAddress(id, ext_ip, ext_port);
		}

	    }
	} catch (...) {
	    std::cerr << "ConnectFriendWizard : Parse ip address error." << std::endl;
	}
	wizard()->setField("idField", QString::fromStdString(id));
        return ConnectFriendWizard::Page_Conclusion ;
    }
    else
    {
        // error message 
        wizard()->setField("errorMessage",  tr("Certificate Load Failed") );
        return ConnectFriendWizard::Page_ErrorMessage;
    }

    return ConnectFriendWizard::Page_ErrorMessage;
}
//
//============================================================================
//============================================================================
//============================================================================
//
FofPage::FofPage(QWidget *parent)
    : QWizardPage(parent)
{
	_friends_signed = false ;
    QString titleStr("<span style=\"font-size:14pt; font-weight:500;" "color:#32cd32;\">%1</span>");
    setTitle( titleStr.arg( tr("Friends of friends") ) ) ;

    setSubTitle(tr("Select now who you want to make friends with."));

    userFileLabel = new QLabel(tr("Show me: ")) ;
	 userSelectionCB = new QComboBox ;
	 userSelectionCB->addItem(tr("Any peer I've not signed")) ;
	 userSelectionCB->addItem(tr("Friends of my friends who already trust me")) ;
	 userSelectionCB->addItem(tr("Signed peers showing as denied")) ;

	 selectedPeersTW = new QTableWidget(0,4,NULL) ;
	 selectedPeersTW->setHorizontalHeaderItem(0,new QTableWidgetItem(tr(""))) ;
	 selectedPeersTW->setHorizontalHeaderItem(1,new QTableWidgetItem(tr("Peer name"))) ;
	 selectedPeersTW->setHorizontalHeaderItem(2,new QTableWidgetItem(tr("Also signed by"))) ;
	 selectedPeersTW->setHorizontalHeaderItem(3,new QTableWidgetItem(tr("Peer id"))) ;

	 makeFriendButton = new QPushButton(tr("Make friend with these peers")) ;

    userFileLayout = new QVBoxLayout;
    userFileLayout->addWidget(userFileLabel);
    userFileLayout->addWidget(userSelectionCB);
    userFileLayout->addWidget(selectedPeersTW);
    userFileLayout->addWidget(makeFriendButton);

//    userFileFrame = new QGroupBox;
//    userFileFrame->setFlat(true);
//    userFileFrame->setTitle("toto");
//    userFileFrame->setLayout(userFileLayout);

	 setLayout(userFileLayout) ;

	 connect(makeFriendButton,SIGNAL(clicked()),this,SLOT(signAllSelectedUsers())) ;
	 connect(userSelectionCB,SIGNAL(activated(int)),this,SLOT(updatePeersList(int))) ;

	 updatePeersList(0) ;
}
 
void FofPage::updatePeersList(int e) 
{
	rsiface->unlockData(); /* UnLock Interface */
	std::cout << "updating peers list with e=" << e << std::endl ;

	selectedPeersTW->clearContents() ;
	selectedPeersTW->setRowCount(0) ;

	std::list<std::string> ids ;
	rsPeers->getOthersList(ids) ;

	int row = 0 ;

	_id_boxes.clear() ;

	// We have to use this trick because signers are given by their names instead of their ids. That's a cause
	// for some confusion when two peers have the same name. 
	//
	std::set<std::string> my_friends_names ;

	std::list<std::string> friends_ids ;
	rsPeers->getFriendList(friends_ids) ;

	for(std::list<std::string>::const_iterator it(friends_ids.begin());it!=friends_ids.end();++it)
		my_friends_names.insert(rsPeers->getPeerName(*it)) ;

	// Now fill in the table of selected peers.
	//
	for(std::list<std::string>::const_iterator it(ids.begin());it!=ids.end();++it)
	{
		std::cerr << "examining peer " << *it << " (name=" << rsPeers->getPeerName(*it) ;
		RsPeerDetails details ;

		if(!rsPeers->getPeerDetails(*it,details))
		{
			std::cerr << " no details." << std::endl ;
			continue ;
		}

		// determine common friends
		
		std::set<std::string> common_friends ;

		for(std::list<std::string>::const_iterator it2(details.signers.begin());it2!=details.signers.end();++it2)
			if(my_friends_names.find(*it2) != my_friends_names.end()	&& *it2 != details.name)										
				common_friends.insert(*it2) ;

		bool show = false;

		switch(e)
		{
			case 2: // "Peers shown as denied"
				show = details.ownsign && !(details.state & RS_PEER_STATE_FRIEND) ;
				std::cerr << "case 2, ownsign=" << details.ownsign << ", state_friend=" << (details.state & RS_PEER_STATE_FRIEND) << ", show=" << show << std::endl ;
				break ;

			case 1: // "Unsigned peers who already signed my certificate"
				show = rsPeers->isTrustingMe(details.id) && !(details.state & RS_PEER_STATE_FRIEND) ;
				std::cerr << "case 1, ownsign=" << details.ownsign << ", is_trusting_me=" << rsPeers->isTrustingMe(details.id) << ", show=" << show << std::endl ;
				break ;

			case 0: // "All unsigned friends of my friends"
				show= !details.ownsign ;
				std::cerr << "case 0: ownsign=" << details.ownsign << ", show=" << show << std::endl ;
				break ;

			default: break ;
		}

		if(show)
		{
			selectedPeersTW->insertRow(row) ;

			QCheckBox *cb = new QCheckBox ;
			cb->setChecked(true) ;
			_id_boxes[cb] = details.id ;

			selectedPeersTW->setCellWidget(row,0,cb) ;
			selectedPeersTW->setItem(row,1,new QTableWidgetItem(QString::fromStdString(details.name))) ;

			QComboBox *qcb = new QComboBox ;

			if(common_friends.empty())
				qcb->addItem(tr("*** None ***")) ;
			else
				for(std::set<std::string>::const_iterator it2(common_friends.begin());it2!=common_friends.end();++it2)
					qcb->addItem(QString::fromStdString(*it2));

			selectedPeersTW->setCellWidget(row,2,qcb) ;
			selectedPeersTW->setItem(row,3,new QTableWidgetItem(QString::fromStdString(details.id))) ;
			++row ;
		}
	}
	if(row>0)
	{
		selectedPeersTW->resizeColumnsToContents() ;
		makeFriendButton->setEnabled(true) ;
	}
	else
		makeFriendButton->setEnabled(false) ;

	selectedPeersTW->verticalHeader()->hide() ;
	selectedPeersTW->setSortingEnabled(true) ;
}

int FofPage::nextId() const
{
	return -1 ;
}

bool FofPage::isComplete() const
{
	return _friends_signed ;
}

void FofPage::signAllSelectedUsers() 
{
	std::cerr << "makign lots of friends !!" << std::endl ;

	for(std::map<QCheckBox*,std::string>::const_iterator it(_id_boxes.begin());it!=_id_boxes.end();++it)
		if(it->first->isChecked())
		{
			std::cerr << "Making friend with " << it->second << std::endl ;
			rsPeers->AuthCertificate(it->second, "");
			rsPeers->addFriend(it->second);
		}

	_friends_signed = true ;

	userSelectionCB->setEnabled(false) ;
	selectedPeersTW->setEnabled(false) ;
	makeFriendButton->setEnabled(false) ;

	rsicontrol->getNotify().notifyListChange(NOTIFY_LIST_NEIGHBOURS,0) ;
	rsicontrol->getNotify().notifyListChange(NOTIFY_LIST_FRIENDS,0) ;
	emit completeChanged();
}
//
//============================================================================
//============================================================================
//============================================================================

CertificatePage::CertificatePage(QWidget *parent)
    : QWizardPage(parent)
{
    QString titleStr("<span style=\"font-size:14pt; font-weight:500;"
                               "color:#32cd32;\">%1</span>");
    setTitle( titleStr.arg( tr("Certificate files") ) ) ;

    setSubTitle(tr("Use XPGP certificates saved in files."));
    
    userFileLabel = new QLabel(tr("You have to generate a file with your "
                                  "certificate and give it to your friend. "
                                  "Also, you can use a file generated "
                                  "before."));
    userFileLabel->setWordWrap(true);
                                  
    userFileCreateButton = new QPushButton;
    userFileCreateButton->setText(tr("Export my certificate..."));
    connect(userFileCreateButton, SIGNAL( clicked() ),
            this,                 SLOT( generateCertificateCalled()));

    userFileLayout = new QHBoxLayout;
    userFileLayout->addWidget(userFileLabel);
    userFileLayout->addWidget(userFileCreateButton);

    userFileFrame = new QGroupBox;
    userFileFrame->setFlat(true);
    userFileFrame->setTitle("Export my certificate...");
    userFileFrame->setLayout(userFileLayout);

    friendFileLabel = new QLabel(tr("Specify path to your friend's "
                                    "certificate in the box below " ) );
    friendFileNameEdit = new QLineEdit;
    registerField("friendCertificateFile*", friendFileNameEdit);

    friendFileNameOpenButton= new QPushButton;
    friendFileNameOpenButton->setText(tr("Browse"));
    connect(friendFileNameOpenButton, SIGNAL( clicked()),
            this                   , SLOT( loadFriendCert()));
    
    friendFileLayout = new QHBoxLayout;
    friendFileLayout->addWidget(friendFileNameEdit) ;
    friendFileLayout->addWidget(friendFileNameOpenButton);
    
    certPageLayout = new QVBoxLayout;
    certPageLayout->addWidget(userFileFrame);
    certPageLayout->addWidget(friendFileLabel);
    certPageLayout->addLayout(friendFileLayout);

    setLayout(certPageLayout);
}

//============================================================================

void
CertificatePage::loadFriendCert()
{
    QString fileName =
        QFileDialog::getOpenFileName(this, tr("Select Certificate"),
                                     "", tr("Certificates (*.pqi *.pem)"));

    if (!fileName.isNull())
    {
        friendFileNameEdit->setText(fileName);
        emit completeChanged();
    }
}

//============================================================================

void
CertificatePage::generateCertificateCalled()
{
    qDebug() << "  generateCertificateCalled";

    QString qdir = QFileDialog::getSaveFileName(this,
                                                "Please choose a filename",
                                                QDir::homePath(),
                                                "RetroShare Certificate (*.pqi)");

    if ( rsPeers->SaveCertificateToFile(rsPeers->getOwnId(), qdir.toStdString()) )
    {
        QMessageBox::information(this, tr("RetroShare"),
                         tr("Certificate file successfully created"),
                         QMessageBox::Ok, QMessageBox::Ok);
    }
    else
    {
        QMessageBox::information(this, tr("RetroShare"),
                         tr("Sorry, certificate file creation failed"),
                         QMessageBox::Ok, QMessageBox::Ok);
    }
}

//============================================================================

bool
CertificatePage::isComplete() const
{
    return !( (friendFileNameEdit->text()).isEmpty() );              
}

//============================================================================

int
CertificatePage::nextId() const
{
    std::string id;
    
    QString fn = friendFileNameEdit->text();
    if (QFile::exists(fn))
    {
        std::string fnstr = fn.toStdString();
        if ( rsPeers->LoadCertificateFromFile(fnstr, id) ) 
        {
            wizard()->setField("idField", QString::fromStdString(id));
            
            return ConnectFriendWizard::Page_Conclusion;
        }
        else
        {
            wizard()->setField("errorMessage",
                     QString(tr("Certificate Load Failed:something is wrong with %1 ")).arg(fn) );
            return ConnectFriendWizard::Page_ErrorMessage;
        }
    }
    else
    {
        QString mess =
            QString(tr("Certificate Load Failed:file %1 not found"))
                    .arg(fn);

        wizard()->setField("errorMessage", mess);

        return ConnectFriendWizard::Page_ErrorMessage;
    }    
}
//
//============================================================================
//============================================================================
//============================================================================

ErrorMessagePage::ErrorMessagePage(QWidget *parent)
    : QWizardPage(parent)
{
    QString titleStr("<span style=\"font-size:14pt; font-weight:500;"
                               "color:#32cd32;\">%1</span>");
    setTitle( titleStr.arg( tr("Sorry, some error appeared") ) ) ;
    setSubTitle( tr("Here is the error message: ") );

    messageLabel = new QLabel("zooloo");
    registerField("errorMessage", messageLabel, "text");

    errMessLayout = new QVBoxLayout;
    errMessLayout->addWidget(messageLabel);
    setLayout(errMessLayout);
}

//============================================================================
//
int ErrorMessagePage::nextId() const
{
    return -1;
}
//
//============================================================================
//============================================================================
//============================================================================

ConclusionPage::ConclusionPage(QWidget *parent)
    : QWizardPage(parent)
{
    QString titleStr("<span style=\"font-size:14pt; font-weight:500;"
                               "color:#32cd32;\">%1</span>");
    setTitle( titleStr.arg( tr("Make Friend") ) ) ;

    setSubTitle(tr("Fill details about your friend here"));

    peerDetailsFrame = new QGroupBox;
    peerDetailsFrame->setTitle( tr("Peer details") );

    peerDetailsLayout =  new QGridLayout();
    
    trustLabel = new QLabel( tr("Trust:") );
    peerDetailsLayout->addWidget(trustLabel, 0,0,1,1);
    trustEdit = new QLineEdit();
    peerDetailsLayout->addWidget(trustEdit, 0,1,1,1);
    nameLabel = new QLabel( tr("Name:") );
    peerDetailsLayout->addWidget(nameLabel, 1,0,1,1);
    nameEdit = new QLineEdit();
    peerDetailsLayout->addWidget(nameEdit, 1,1,1,1);
    orgLabel = new QLabel( tr("Org:") );
    peerDetailsLayout->addWidget(orgLabel, 2,0,1,1);
    orgEdit = new QLineEdit();
    peerDetailsLayout->addWidget(orgEdit, 2,1,1,1);
    locLabel = new QLabel( tr("Loc:") );
    peerDetailsLayout->addWidget(locLabel, 3,0,1,1);
    locEdit = new QLineEdit();
    peerDetailsLayout->addWidget(locEdit, 3,1,1,1);
    countryLabel = new QLabel( tr("Country:") );
    peerDetailsLayout->addWidget(countryLabel, 4,0,1,1);
    countryEdit = new QLineEdit();
    peerDetailsLayout->addWidget(countryEdit, 4,1,1,1);
    signersLabel = new QLabel( tr("Signers") );
    peerDetailsLayout->addWidget(signersLabel, 5,0,1,1);
    signersEdit = new QTextEdit();
    peerDetailsLayout->addWidget(signersEdit, 5,1,1,1);

    peerDetailsFrame->setLayout(peerDetailsLayout);

    authCodeLabel = new QLabel( tr("AUTH CODE") );
    authCodeEdit = new QLineEdit();
    registerField("authCode", authCodeEdit);
    
    authCodeLayout = new QHBoxLayout();
    authCodeLayout->addWidget(authCodeLabel);
    authCodeLayout->addWidget(authCodeEdit);
    authCodeLayout->addStretch();

    conclusionPageLayout = new QVBoxLayout();
    conclusionPageLayout->addWidget(peerDetailsFrame);
    conclusionPageLayout->addLayout(authCodeLayout);

    setLayout(conclusionPageLayout);

    peerIdEdit = new QLineEdit(this);
    peerIdEdit->setVisible(false);
    registerField("idField",peerIdEdit);
}

//============================================================================
//
int ConclusionPage::nextId() const
{
    return -1;
}
//
//============================================================================
//
void
ConclusionPage::initializePage()
{
    std::string id = field("idField").toString().toStdString();

    RsPeerDetails detail;
    if (!rsPeers->getPeerDetails(id, detail))
    {
            rsiface->unlockData(); /* UnLock Interface */
            return ;//false;
    }

    std::string trustString;

    switch(detail.trustLvl)
    {
            case RS_TRUST_LVL_GOOD:
                    trustString = "Good";
            break;
            case RS_TRUST_LVL_MARGINAL:
                    trustString = "Marginal";
            break;
            case RS_TRUST_LVL_UNKNOWN:
            default:
                    trustString = "No Trust";
            break;
    }

    QString ts;
    std::list<std::string>::iterator it;
    for(it = detail.signers.begin(); it != detail.signers.end(); it++)
    {
            ts.append(QString::fromStdString( rsPeers->getPeerName(*it) ));
            ts.append( "<" ) ;
            ts.append( QString::fromStdString(*it) );
            ts.append( ">" );
            ts.append( "\n" );
    }

    nameEdit->setText( QString::fromStdString( detail.name ) ) ;
    trustEdit->setText(QString::fromStdString( trustString ) ) ;
    orgEdit->setText(QString::fromStdString( detail.org ) );
    locEdit->setText( QString::fromStdString( detail.location ) );
    countryEdit->setText( QString::fromStdString( detail.email ) );
    signersEdit->setPlainText( ts );
    
    authCodeEdit->setText( QString::fromStdString(detail.authcode) );
}
//
//============================================================================
//

