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

//============================================================================
//! 
ConnectFriendWizard::ConnectFriendWizard(QWidget *parent)
                    : QWizard(parent)
{
    setPage(Page_Intro, new IntroPage);
    setPage(Page_Text, new TextPage);
    setPage(Page_Cert, new CertificatePage);
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
             
    setSubTitle(tr("This wizard will help you to connect your friend "
                   "to RetroShare network. There are  two possible ways "
                   "to do this:")) ;

    textRadioButton = new QRadioButton(tr("&Enter the certificate manually"));
    certRadioButton = new QRadioButton(tr("&Use *.pqi files with "
                                          "certificates" ));
    textRadioButton->setChecked(true);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(textRadioButton);
    layout->addWidget(certRadioButton);
    setLayout(layout);
}
//
//============================================================================
//
int IntroPage::nextId() const
{
    if (textRadioButton->isChecked()) {
        return ConnectFriendWizard::Page_Text;
    } else {
        return ConnectFriendWizard::Page_Cert;
    }
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

    userCertHelpButton = new QPushButton;
    userCertHelpButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    userCertHelpButton->setFixedSize(20,20);
    userCertHelpButton->setFlat(true);
    userCertHelpButton->setIcon( QIcon(":images/connect/info16.png") );
    connect (userCertHelpButton,  SIGNAL( clicked()),
             this,                SLOT(   showHelpUserCert()) );
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
                                "friend via email, ICQ or some other way"));

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
    userFileCreateButton->setText("Export my certificate...");
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
    friendFileNameOpenButton->setText("...");
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

