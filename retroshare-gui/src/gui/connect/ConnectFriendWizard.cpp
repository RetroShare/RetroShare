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

#define SSL_ID_FIELD_CONNECT_FRIEND_WIZARD "idField"
#define GPG_ID_FIELD_CONNECT_FRIEND_WIZARD "GPGidField"
#define LOCATION_FIELD_CONNECT_FRIEND_WIZARD "peerLocation"
#define CERT_STRING_FIELD_CONNECT_FRIEND_WIZARD "peerCertString"
#define SIGN_RADIO_BUTTON_FIELD_CONNECT_FRIEND_WIZARD "signRadioButton"
#define ACCEPT_RADIO_BUTTON_FIELD_CONNECT_FRIEND_WIZARD "acceptRadioButton"




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
    if ( hasVisitedPage(Page_Conclusion) ) {
        std::cerr << "ConnectFriendWizard::accept() called with page conclusion visited" << std::endl;

        std::string ssl_Id = field(SSL_ID_FIELD_CONNECT_FRIEND_WIZARD).toString().toStdString();
        std::string gpg_Id = field(GPG_ID_FIELD_CONNECT_FRIEND_WIZARD).toString().toStdString();
        bool sign = field(SIGN_RADIO_BUTTON_FIELD_CONNECT_FRIEND_WIZARD).toBool();
        bool accept_connection = field(ACCEPT_RADIO_BUTTON_FIELD_CONNECT_FRIEND_WIZARD).toBool();

        if (gpg_Id != "") {
            if (sign) {
                std::cerr << "ConclusionPage::validatePage() signing GPG key." << std::endl;
                rsPeers->signGPGCertificate(gpg_Id); //bye default sign set accept_connection to true;
            } else if (accept_connection) {
                std::cerr << "ConclusionPage::validatePage() accepting GPG key for connection." << std::endl;
                rsPeers->setAcceptToConnectGPGCertificate(gpg_Id, true);
            }
        }

        if (ssl_Id != "") {
            rsPeers->addFriend(ssl_Id, gpg_Id);
            //let's check if there is ip adresses in the wizard.
            if (!this->field("ext_friend_ip").isNull() && !this->field("ext_friend_port").isNull()) {
                std::cerr << "ConnectFriendWizard::accept() : setting ip ext address." << std::endl;
                rsPeers->setExtAddress(ssl_Id, this->field("ext_friend_ip").toString().toStdString(), this->field("ext_friend_port").toInt());
            }
            if (!this->field("local_friend_ip").isNull() && !this->field("local_friend_port").isNull()) {
                std::cerr << "ConnectFriendWizard::accept() : setting ip local address." << std::endl;
                rsPeers->setLocalAddress(ssl_Id, this->field("local_friend_ip").toString().toStdString(), this->field("local_friend_port").toInt());
            }
            if (!this->field(LOCATION_FIELD_CONNECT_FRIEND_WIZARD).isNull()) {
                std::cerr << "ConnectFriendWizard::accept() : setting peerLocation." << std::endl;
                rsPeers->setLocation(ssl_Id, this->field(LOCATION_FIELD_CONNECT_FRIEND_WIZARD).toString().toStdString());
            }
        }

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
    certRadioButton = new QRadioButton(tr("&Use *.rsc files with certificates" ));
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
    
    setSubTitle(tr("Use text representation of the PGP certificates."));

    userCertLabel = new QLabel(tr("The text below is your PGP certificate. "
                                  "You have to provide it to your friend "));

    std::cerr << "TextPage() getting Invite" << std::endl;
    userCertEdit = new QTextEdit;
    std::string invite = rsPeers->GetRetroshareInvite();

    userCertEdit->setText(QString::fromStdString(invite));
    userCertEdit->setReadOnly(true);
    userCertEdit->setMinimumHeight(200);
    userCertEdit->setMinimumWidth(450);
    QFont font;
    font.setPointSize(10);
    font.setBold(false);
    font.setStyleHint(QFont::TypeWriter, QFont::PreferDefault);
    //font.setWeight(75);
    userCertEdit->setFont(font);

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

    friendCertLabel = new QLabel(tr("Please, paste your friends PGP "
                                    "certificate into the box below" )) ;
    
    friendCertEdit = new QTextEdit;

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
                             tr("Your Cert is copied to Clipboard, paste and send it to your"
                                "friend via email or some other way"));
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(userCertEdit->toPlainText());                            

}
//
//============================================================================
//

int TextPage::nextId() const {

    std::string certstr;
    certstr = friendCertEdit->toPlainText().toStdString();
    RsPeerDetails pd;
    if ( rsPeers->loadDetailsFromStringCert(certstr, pd) ) {
#ifdef FRIEND_WIZARD_DEBUG
            std::cerr << "ConnectFriendWizard got id : " << pd.id << "; gpg_id : " << pd.gpg_id << std::endl;
#endif
        wizard()->setField(SSL_ID_FIELD_CONNECT_FRIEND_WIZARD, QString::fromStdString(pd.id));
        wizard()->setField(GPG_ID_FIELD_CONNECT_FRIEND_WIZARD, QString::fromStdString(pd.gpg_id));
        wizard()->setField(LOCATION_FIELD_CONNECT_FRIEND_WIZARD, QString::fromStdString(pd.location));
        wizard()->setField(CERT_STRING_FIELD_CONNECT_FRIEND_WIZARD, QString::fromStdString(certstr));

        wizard()->setField("ext_friend_ip", QString::fromStdString(pd.extAddr));
        wizard()->setField("ext_friend_port", QString::number(pd.extPort));
        wizard()->setField("local_friend_ip", QString::fromStdString(pd.localAddr));
        wizard()->setField("local_friend_port", QString::number(pd.localPort));

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
FofPage::FofPage(QWidget *parent) : QWizardPage(parent) {
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
 
void FofPage::updatePeersList(int e) {
	rsiface->unlockData(); /* UnLock Interface */

	selectedPeersTW->clearContents() ;
        selectedPeersTW->setRowCount(0) ;

	std::list<std::string> ids ;
        rsPeers->getGPGAllList(ids) ;

	int row = 0 ;

	_id_boxes.clear() ;
        std::cerr << "FofPage::updatePeersList() updating peers list with e=" << e << std::endl ;

	// We have to use this trick because signers are given by their names instead of their ids. That's a cause
	// for some confusion when two peers have the same name. 
	//
        std::list<std::string> gpg_ids;
        rsPeers->getGPGAllList(gpg_ids);
        for(std::list<std::string>::const_iterator it(gpg_ids.begin());it!=gpg_ids.end();++it)
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

                for(std::list<std::string>::const_iterator it2(details.gpgSigners.begin());it2!=details.gpgSigners.end();++it2) {
                    if(rsPeers->isGPGAccepted(*it2))										 {
                        common_friends.insert(*it2);
                     }
                }
		bool show = false;

		switch(e)
		{
			case 2: // "Peers shown as denied"
				show = details.ownsign && !(details.state & RS_PEER_STATE_FRIEND) ;
				std::cerr << "case 2, ownsign=" << details.ownsign << ", state_friend=" << (details.state & RS_PEER_STATE_FRIEND) << ", show=" << show << std::endl ;
				break ;

			case 1: // "Unsigned peers who already signed my certificate"
                                show = details.hasSignedMe && !(details.state & RS_PEER_STATE_FRIEND) ;
                                std::cerr << "case 1, ownsign=" << details.ownsign << ", is_authed_me=" << details.hasSignedMe << ", show=" << show << std::endl ;
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
                        _gpg_id_boxes[cb] = details.gpg_id ;

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
        std::cerr << "FofPage::updatePeersList() finished iterating over peers" << std::endl ;

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

int FofPage::nextId() const {
	return -1 ;
}

bool FofPage::isComplete() const {
	return _friends_signed ;
}

void FofPage::signAllSelectedUsers() {
	std::cerr << "makign lots of friends !!" << std::endl ;

	for(std::map<QCheckBox*,std::string>::const_iterator it(_id_boxes.begin());it!=_id_boxes.end();++it)
		if(it->first->isChecked())
		{
			std::cerr << "Making friend with " << it->second << std::endl ;
                        //rsPeers->AuthCertificate(it->second, "");
                        rsPeers->addFriend(it->second, _gpg_id_boxes[it->first]);
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

CertificatePage::CertificatePage(QWidget *parent) : QWizardPage(parent) {
    QString titleStr("<span style=\"font-size:14pt; font-weight:500;"
                               "color:#32cd32;\">%1</span>");
    setTitle( titleStr.arg( tr("Certificate files") ) ) ;

    setSubTitle(tr("Use PGP certificates saved in files."));
    
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
    userFileFrame->setTitle(tr("Export my certificate..."));
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

void CertificatePage::loadFriendCert() {
    QString fileName =
        QFileDialog::getOpenFileName(this, tr("Select Certificate"),
                                     "", tr("RetroShare Certificates (*.rsc)"));

    if (!fileName.isNull())
    {
        friendFileNameEdit->setText(fileName);
        emit completeChanged();
    }
}

//============================================================================

void CertificatePage::generateCertificateCalled() {
    qDebug() << "  generateCertificateCalled";

    QString qdir = QFileDialog::getSaveFileName(this,
                                                tr("Please choose a filename"),
                                                QDir::homePath(),
                                                "RetroShare Certificate (*.rsc)");

    if ( rsPeers->saveCertificateToFile(rsPeers->getOwnId(), qdir.toStdString()) )
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

bool CertificatePage::isComplete() const {
    return !( (friendFileNameEdit->text()).isEmpty() );              
}

//============================================================================

int CertificatePage::nextId() const {
    std::string id;
    
    QString fn = friendFileNameEdit->text();
    if (QFile::exists(fn))
    {
        std::string fnstr = fn.toStdString();
//        if ( rsPeers->LoadCertificateFromFile(fnstr, id) )
        if ( false )
        {
            wizard()->setField(SSL_ID_FIELD_CONNECT_FRIEND_WIZARD, QString::fromStdString(id));
            
            return ConnectFriendWizard::Page_Conclusion;
        }
        else
        {
            wizard()->setField("errorMessage",
//                     QString(tr("Certificate Load Failed:something is wrong with %1 ")).arg(fn) );
                     QString(tr("Not implemented ")));
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

ConclusionPage::ConclusionPage(QWidget *parent) : QWizardPage(parent) {
    QString titleStr("<span style=\"font-size:14pt; font-weight:500;"
                               "color:#32cd32;\">%1</span>");
    setTitle( titleStr.arg( tr("Make Friend") ) ) ;

    setSubTitle(tr("Details about your friend : "));

    peerDetailsFrame = new QGroupBox;
    peerDetailsFrame->setTitle( tr("Peer details") );

    peerDetailsLayout =  new QGridLayout();

    trustLabel = new QLabel( tr("Key validity:") );
    peerDetailsLayout->addWidget(trustLabel, 0,0,1,1);
    trustEdit = new QLabel();
    peerDetailsLayout->addWidget(trustEdit, 0,1,1,1);
    nameLabel = new QLabel( tr("Name:") );
    peerDetailsLayout->addWidget(nameLabel, 1,0,1,1);
    nameEdit = new QLabel();
    peerDetailsLayout->addWidget(nameEdit, 1,1,1,1);
    emailLabel = new QLabel( tr("Email:") );
    peerDetailsLayout->addWidget(emailLabel, 2,0,1,1);
    emailEdit = new QLabel();
    peerDetailsLayout->addWidget(emailEdit, 2,1,1,1);
    locLabel = new QLabel( tr("Loc:") );
    peerDetailsLayout->addWidget(locLabel, 3,0,1,1);
    locEdit = new QLabel();
    peerDetailsLayout->addWidget(locEdit, 3,1,1,1);
    signersLabel = new QLabel( tr("Signers") );
    peerDetailsLayout->addWidget(signersLabel, 4,0,1,1);
    signersEdit = new QTextEdit();
    peerDetailsLayout->addWidget(signersEdit, 4,1,1,1);

    peerDetailsFrame->setLayout(peerDetailsLayout);

    signGPGRadioButton = new QRadioButton();
    signGPGRadioButton->setText(tr("Add as friend and Sign GPG Key"));
    registerField(SIGN_RADIO_BUTTON_FIELD_CONNECT_FRIEND_WIZARD,signGPGRadioButton);
    acceptNoSignGPGRadioButton = new QRadioButton();
    acceptNoSignGPGRadioButton->setText(tr("Add as friend but don't sign GPG Key"));
    registerField(ACCEPT_RADIO_BUTTON_FIELD_CONNECT_FRIEND_WIZARD,acceptNoSignGPGRadioButton);
    peerDetailsLayout->addWidget(signGPGRadioButton, 5,0,1,-1); // QWidget * widget, int fromRow, int fromColumn, int rowSpan, int columnSpan, Qt::Alignment alignment = 0 )
    peerDetailsLayout->addWidget(acceptNoSignGPGRadioButton, 6,0,1,-1); // QWidget * widget, int fromRow, int fromColumn, int rowSpan, int columnSpan, Qt::Alignment alignment = 0 )

    conclusionPageLayout = new QVBoxLayout();
    conclusionPageLayout->addWidget(peerDetailsFrame);

    setLayout(conclusionPageLayout);

    //registering fields for cross pages access. There maybe a cleaner solution
    peerIdEdit = new QLineEdit(this);
    peerIdEdit->setVisible(false);
    registerField(SSL_ID_FIELD_CONNECT_FRIEND_WIZARD,peerIdEdit);

    peerGPGIdEdit = new QLineEdit(this);
    peerGPGIdEdit->setVisible(false);
    registerField(GPG_ID_FIELD_CONNECT_FRIEND_WIZARD,peerGPGIdEdit);

    peerLocation = new QLineEdit(this);
    peerLocation->setVisible(false);
    registerField(LOCATION_FIELD_CONNECT_FRIEND_WIZARD,peerLocation);

    peerCertStringEdit = new QLineEdit(this);
    peerCertStringEdit->setVisible(false);
    registerField(CERT_STRING_FIELD_CONNECT_FRIEND_WIZARD,peerCertStringEdit);

    ext_friend_ip = new QLineEdit(this);
    ext_friend_ip->setVisible(false);
    registerField("ext_friend_ip",ext_friend_ip);

    ext_friend_port = new QLineEdit(this);
    ext_friend_port->setVisible(false);
    registerField("ext_friend_port",ext_friend_port);

    local_friend_ip = new QLineEdit(this);
    local_friend_ip->setVisible(false);
    registerField("local_friend_ip",local_friend_ip);

    local_friend_port = new QLineEdit(this);
    local_friend_port->setVisible(false);
    registerField("local_friend_port",local_friend_port);
}

//============================================================================
//
int ConclusionPage::nextId() const {
    return -1;
}

//
//============================================================================
//
void ConclusionPage::initializePage() {
    std::string id = field(SSL_ID_FIELD_CONNECT_FRIEND_WIZARD).toString().toStdString();
    std::string gpg_id = field(GPG_ID_FIELD_CONNECT_FRIEND_WIZARD).toString().toStdString();
    std::string location = field(LOCATION_FIELD_CONNECT_FRIEND_WIZARD).toString().toStdString();
    std::string certString = field(CERT_STRING_FIELD_CONNECT_FRIEND_WIZARD).toString().toStdString();
    std::cerr << "Conclusion page id : " << id << "; gpg_id : " << gpg_id << std::endl;

    RsPeerDetails detail;
    if (!rsPeers->loadDetailsFromStringCert(certString, detail)) {
        if (!rsPeers->getPeerDetails(id, detail)) {
            if (!rsPeers->getPeerDetails(gpg_id, detail)) {
                rsiface->unlockData(); /* UnLock Interface */
                return ;//false;
            }
        }
    }

    //set the radio button to sign the GPG key
    if (detail.accept_connection && !detail.ownsign) {
        //gpg key connection is already accepted, don't propose to accept it again
        signGPGRadioButton->setText(tr("Peer is already a retroshare friend. Sign his GPG key."));
        signGPGRadioButton->setChecked(true);
        acceptNoSignGPGRadioButton->hide();
        acceptNoSignGPGRadioButton->setChecked(false);
    }
    if (!detail.accept_connection && detail.ownsign) {
        //gpg key is already signed, don't propose to sign it again
        acceptNoSignGPGRadioButton->setText(tr("GPG key is already signed, make it a retroshare friend."));
        acceptNoSignGPGRadioButton->setChecked(true);
        signGPGRadioButton->hide();
        signGPGRadioButton->setChecked(false);
    }
    if (!detail.accept_connection && !detail.ownsign) {
        signGPGRadioButton->setText(tr("Add as friend and Sign GPG Key"));
        signGPGRadioButton->show();
        acceptNoSignGPGRadioButton->setText(tr("Add as friend but don't sign GPG Key"));
        acceptNoSignGPGRadioButton->show();
    }
    if (detail.accept_connection && detail.ownsign && !detail.isOnlyGPGdetail) {
        acceptNoSignGPGRadioButton->setChecked(false);
        acceptNoSignGPGRadioButton->hide();
        signGPGRadioButton->setChecked(false);
        signGPGRadioButton->hide();
        radioButtonsLabel = new QLabel(tr("It seems your friend is already registered. Adding it might just set it's ip address."));
        peerDetailsLayout->addWidget(radioButtonsLabel, 7,0,1,-1); // QWidget * widget, int fromRow, int fromColumn, int rowSpan, int columnSpan, Qt::Alignment alignment = 0 )
    }

    std::string trustString;
    switch(detail.validLvl)
    {
            case RS_TRUST_LVL_ULTIMATE:
                    trustString = "Ultimate";
            break;
            case RS_TRUST_LVL_FULL:
                    trustString = "Full";
            break;
            case RS_TRUST_LVL_MARGINAL:
                    trustString = "Marginal";
            break;
            case RS_TRUST_LVL_NONE:
                    trustString = "None";
            break;
            default:
                    trustString = "No Trust";
            break;
    }

    QString ts;
    std::list<std::string>::iterator it;
    for(it = detail.gpgSigners.begin(); it != detail.gpgSigners.end(); it++) {
            ts.append(QString::fromStdString( rsPeers->getPeerName(*it) ));
            ts.append( "<" ) ;
            ts.append( QString::fromStdString(*it) );
            ts.append( ">" );
            ts.append( "\n" );
    }

    nameEdit->setText( QString::fromStdString( detail.name ) ) ;
    trustEdit->setText(QString::fromStdString( trustString ) ) ;
    emailEdit->setText(QString::fromStdString( detail.email ) );
    locEdit->setText( QString::fromStdString( detail.location ) );
    signersEdit->setPlainText( ts );
    
}

//============================================================================
//
