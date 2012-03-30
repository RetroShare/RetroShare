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
#include "gui/common/PeerDefs.h"
#include "gui/common/GroupDefs.h"

#include "gui/GetStartedDialog.h"

#include <retroshare/rspeers.h> //for rsPeers variable
#include <retroshare/rsiface.h>

#include <QLineEdit>
#include <QTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QCheckBox>
#include <QGroupBox>
#include <QComboBox>
#include <QClipboard>
#include <QTableWidget>
#include <QHeaderView>
#include <QApplication>
#include <QFileDialog>
#include <QTextCodec>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QDesktopServices>
#include <QFile>
#include <QUrl>
#include <QTextStream>
#include <QDragEnterEvent>

#include <QDebug>
#include <set>

#define SSL_ID_FIELD_CONNECT_FRIEND_WIZARD "idField"
#define GPG_ID_FIELD_CONNECT_FRIEND_WIZARD "GPGidField"
#define LOCATION_FIELD_CONNECT_FRIEND_WIZARD "peerLocation"
#define CERT_STRING_FIELD_CONNECT_FRIEND_WIZARD "peerCertString"
#define SIGN_RADIO_BUTTON_FIELD_CONNECT_FRIEND_WIZARD "signCheckBox"
#define ACCEPT_RADIO_BUTTON_FIELD_CONNECT_FRIEND_WIZARD "acceptCheckBox"
#define GROUP_ID_FIELD_CONNECT_FRIEND_WIZARD "groupIdField"




//============================================================================
//! 
ConnectFriendWizard::ConnectFriendWizard(QWidget *parent,const QString& cert)
                    : QWizard(parent)
{
    setPage(Page_Intro, new IntroPage);
    setPage(Page_Text, new TextPage);
    setPage(Page_Cert, new CertificatePage);
    setPage(Page_Foff, new FofPage);
    setPage(Page_Rsid, new RsidPage);
    setPage(Page_Email, new EmailPage);

    setPage(Page_ErrorMessage, new ErrorMessagePage);
    setPage(Page_Conclusion, new ConclusionPage);

	 if(cert.isNull())
		 setStartId(Page_Intro);
	 else
	 {
		 RsPeerDetails pd;
		 std::string error_string ;

		 if ( rsPeers->loadDetailsFromStringCert(cert.toUtf8().constData(), pd,error_string) ) 
		 {
#ifdef FRIEND_WIZARD_DEBUG
			 std::cerr << "ConnectFriendWizard got id : " << pd.id << "; gpg_id : " << pd.gpg_id << std::endl;
#endif
			 setField(SSL_ID_FIELD_CONNECT_FRIEND_WIZARD, QString::fromStdString(pd.id));
			 setField(GPG_ID_FIELD_CONNECT_FRIEND_WIZARD, QString::fromStdString(pd.gpg_id));
			 setField(LOCATION_FIELD_CONNECT_FRIEND_WIZARD, QString::fromUtf8(pd.location.c_str()));
			 setField(CERT_STRING_FIELD_CONNECT_FRIEND_WIZARD, cert);

			 setField("ext_friend_ip", QString::fromStdString(pd.extAddr));
			 setField("ext_friend_port", QString::number(pd.extPort));
			 setField("local_friend_ip", QString::fromStdString(pd.localAddr));
			 setField("local_friend_port", QString::number(pd.localPort));
			 setField("dyndns", QString::fromStdString(pd.dyndns));

			 setStartId(Page_Conclusion) ;
		 }
		 else
		 {
			 // error message 
			 setField("errorMessage",  tr("Certificate Load Failed")+": "+QString::fromStdString(error_string) );
			 setStartId(Page_ErrorMessage) ;
		 }
	 }

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
    setWindowIcon(QIcon(":/images/rstray3.png"));
}


//============================================================================

void ConnectFriendWizard::setGroup(const std::string &groupId)
{
    setField(GROUP_ID_FIELD_CONNECT_FRIEND_WIZARD, QString::fromStdString(groupId));
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
                rsPeers->addFriend("", gpg_Id);
            }

            QString groupId = field(GROUP_ID_FIELD_CONNECT_FRIEND_WIZARD).toString();
            if (groupId.isEmpty() == false) {
                rsPeers->assignPeerToGroup(groupId.toStdString(), gpg_Id, true);
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
            if (!this->field("dyndns").isNull()) {
                std::cerr << "ConnectFriendWizard::accept() : setting DynDNS." << std::endl;
                rsPeers->setDynDNS(ssl_Id, this->field("dyndns").toString().toStdString());
            }
            if (!this->field(LOCATION_FIELD_CONNECT_FRIEND_WIZARD).isNull()) {
                std::cerr << "ConnectFriendWizard::accept() : setting peerLocation." << std::endl;
                rsPeers->setLocation(ssl_Id, this->field(LOCATION_FIELD_CONNECT_FRIEND_WIZARD).toString().toUtf8().constData());
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
    QString titleStr("<span style=\"font-size:16pt; font-weight:500;"
                               "color:white;\">%1</span>");
    setTitle( titleStr.arg( tr("Add a new Friend") ) ) ;
             
    setSubTitle(tr("This wizard will help you to connect to your friend(s) to RetroShare network.\nThese ways are possible to do this:"));

    textRadioButton = new QRadioButton(tr("&Enter the certificate manually"));
    certRadioButton = new QRadioButton(tr("&You get a certificate file from your friend" ));
    foffRadioButton = new QRadioButton(tr("&Make friend with selected friends of my friends" ));
    rsidRadioButton = new QRadioButton(tr("&Enter RetroShare ID manually" ));
    emailRadioButton = new QRadioButton(tr("&Send a Invitation by Email \n (She/He receives a email with instructions howto to download RetroShare) " ));

    textRadioButton->setChecked(true);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(textRadioButton);
    layout->addWidget(certRadioButton);
    //layout->addWidget(foffRadioButton);
    //layout->addWidget(rsidRadioButton);
    layout->addWidget(emailRadioButton);
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
    if (rsidRadioButton->isChecked()) return ConnectFriendWizard::Page_Rsid;
    if (emailRadioButton->isChecked()) return ConnectFriendWizard::Page_Email;

    return ConnectFriendWizard::Page_Foff;
}
void TextPage::toggleSignatureState()
{
	_shouldAddSignatures = !_shouldAddSignatures ;

	if(_shouldAddSignatures)
		userCertIncludeSignaturesButton->setToolTip("Remove signatures") ;
	else
		userCertIncludeSignaturesButton->setToolTip("Include signatures") ;

	updateOwnCert() ;
}

//
//============================================================================
//============================================================================
//============================================================================
//
TextPage::TextPage(QWidget *parent)
    : QWizardPage(parent)
{
    QString titleStr("<span style=\"font-size:16pt; font-weight:500;"
                               "color:white;\">%1</span>");
    setTitle( titleStr.arg( tr("Text certificate") ) ) ;
    
    setSubTitle(tr("Use text representation of the PGP certificates."));

    userCertLabel = new QLabel(tr("The text below is your PGP certificate. "
                                  "You have to provide it to your friend "));

    std::cerr << "TextPage() getting Invite" << std::endl;
    userCertEdit = new QTextEdit;
    userCertEdit->setReadOnly(true);

	 _shouldAddSignatures = false ;
	 updateOwnCert() ;

    userCertHelpButton = new QPushButton;
    userCertHelpButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    userCertHelpButton->setFixedSize(20,20);
    userCertHelpButton->setFlat(true);
    userCertHelpButton->setIcon( QIcon(":images/connect/info16.png") );
    connect (userCertHelpButton,  SIGNAL( clicked()),
             this,                SLOT(   showHelpUserCert()) );
    
    userCertIncludeSignaturesButton = new QPushButton;
    userCertIncludeSignaturesButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    userCertIncludeSignaturesButton->setFixedSize(20,20);
    userCertIncludeSignaturesButton->setFlat(true);
    userCertIncludeSignaturesButton->setIcon( QIcon(":images/gpgp_key_generate.png") );
    userCertIncludeSignaturesButton->setToolTip(tr("Include signatures"));

    connect (userCertIncludeSignaturesButton, SIGNAL(clicked()), this, SLOT(toggleSignatureState()));

    userCertCopyButton = new QPushButton;
    userCertCopyButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    userCertCopyButton->setFixedSize(20,20);

    userCertCopyButton->setFlat(true);
    userCertCopyButton->setIcon( QIcon(":images/copyrslink.png") );
    userCertCopyButton->setToolTip(tr("Copy your Cert to Clipboard"));
    connect (userCertCopyButton,  SIGNAL( clicked()),
             this,                SLOT(   copyCert()) );  
    
    userCertSaveButton = new QPushButton;
    userCertSaveButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    userCertSaveButton->setFixedSize(20,20);
    userCertSaveButton->setFlat(true);
    userCertSaveButton->setIcon( QIcon(":images/document_save.png") );
    userCertSaveButton->setToolTip(tr("Save your Cert into a File"));
    connect (userCertSaveButton,  SIGNAL( clicked()),
             this,                SLOT(   fileSaveAs()) );                 
             
    userCertMailButton = new QPushButton;
    userCertMailButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    userCertMailButton->setFixedSize(20,20);
    userCertMailButton->setFlat(true);
    userCertMailButton->setIcon( QIcon(":images/connect/mail_send.png") );
    userCertMailButton->setToolTip(tr("Run Email program"));
    connect (userCertMailButton,  SIGNAL( clicked()),
             this,                SLOT(   runEmailClient()) );

    userCertButtonsLayout = new QVBoxLayout();
    userCertButtonsLayout->addWidget(userCertHelpButton);

//	 if(rsPeers->hasExportMinimal())
		 userCertButtonsLayout->addWidget(userCertIncludeSignaturesButton);

    userCertButtonsLayout->addWidget(userCertCopyButton);
    userCertButtonsLayout->addWidget(userCertSaveButton);
    userCertButtonsLayout->addWidget(userCertMailButton);

    userCertLayout = new QHBoxLayout();
    userCertLayout->addWidget(userCertEdit);
    userCertLayout->addLayout(userCertButtonsLayout);

    friendCertLabel = new QLabel(tr("Please, paste your friends PGP "
                                    "certificate into the box below" )) ;
    
    friendCertEdit = new QTextEdit;
    friendCertEdit->setAcceptRichText(false);

    //font.setWeight(75);
    QFont font("Courier New",10,50,false);
    font.setStyleHint(QFont::TypeWriter,QFont::PreferMatch);
    font.setStyle(QFont::StyleNormal);
    friendCertEdit->setFont(font);

    friendCertCleanButton = new QPushButton;
    friendCertCleanButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    friendCertCleanButton->setFixedSize(20,20);
    friendCertCleanButton->setFlat(true);
    friendCertCleanButton->setIcon( QIcon(":images/accepted16.png") );
    friendCertCleanButton->setToolTip(tr("Clean certificate"));
    connect (friendCertCleanButton, SIGNAL(clicked()), this, SLOT(cleanFriendCert()));

    friendCertButtonsLayout = new QVBoxLayout();
    friendCertButtonsLayout->addWidget(friendCertCleanButton);

    friendCertLayout = new QHBoxLayout();
    friendCertLayout->addWidget(friendCertEdit);
    friendCertLayout->addLayout(friendCertButtonsLayout);

    //=== add all widgets to one layout
    textPageLayout = new QVBoxLayout();
    textPageLayout->addWidget(userCertLabel);
    textPageLayout->addLayout(userCertLayout);
    textPageLayout->addWidget(friendCertLabel);
    textPageLayout->addLayout(friendCertLayout);
//
    setLayout(textPageLayout);
}

void TextPage::updateOwnCert()
{
    std::string invite = rsPeers->GetRetroshareInvite(_shouldAddSignatures);

    std::cerr << "TextPage() getting Invite: " << invite << std::endl;

    QFont font("Courier New",10,50,false);
    font.setStyleHint(QFont::TypeWriter,QFont::PreferMatch);
    font.setStyle(QFont::StyleNormal);
    userCertEdit->setFont(font);
    userCertEdit->setText(QString::fromUtf8(invite.c_str()));
}

//
//============================================================================
//

static void sendMail (QString sAddress, QString sSubject, QString sBody)
{
#ifdef Q_WS_WIN
    /* search and replace the end of lines with: "%0D%0A" */
    sBody.replace("\n", "%0D%0A");
#endif

    QUrl url = QUrl("mailto:" + sAddress);
    url.addEncodedQueryItem("subject", QUrl::toPercentEncoding(sSubject));
    url.addEncodedQueryItem("body", QUrl::toPercentEncoding(sBody));

    std::cerr << "MAIL STRING:" << (std::string)url.toEncoded().constData() << std::endl;

    /* pass the url directly to QDesktopServices::openUrl */
    QDesktopServices::openUrl (url);
}

void 
TextPage::runEmailClient()
{
    sendMail ("", tr("RetroShare Invite"), userCertEdit->toPlainText());
}

void TextPage::cleanFriendCert()
{
    std::string cert = friendCertEdit->toPlainText().toUtf8().constData() ;
	 cert += "\n";	// add an end of line to avoid a bug
    std::string cleanCert;
	 int error_code ;

    if (rsPeers->cleanCertificate(cert, cleanCert,error_code)) {
        friendCertEdit->setText(QString::fromStdString(cleanCert));

		  if(error_code > 0)
		  {
			  QString msg ;

			  switch(error_code)
			  {
				  case RS_PEER_CERT_CLEANING_CODE_NO_BEGIN_TAG: msg = tr("No or misspelled BEGIN tag found") ;
																				break ;
				  case RS_PEER_CERT_CLEANING_CODE_NO_END_TAG: 	msg = tr("No or misspelled END tag found") ;
																				break ;
				  case RS_PEER_CERT_CLEANING_CODE_NO_CHECKSUM: 	msg = tr("No checksum found (the last 5 chars should be separated by a '=' char), or no newline after tag line (e.g. line beginning with Version:)") ;
																				break ;
				  default:
																				msg = tr("Unknown error. Your cert is probably not even a certificate.") ;
																				break ;
			  }
			  QMessageBox::information(NULL,tr("Certificate cleaning error"),msg) ;
		  }
    }
    QFont font("Courier New",10,50,false);
    font.setStyleHint(QFont::TypeWriter,QFont::PreferMatch);
    font.setStyle(QFont::StyleNormal);
    friendCertEdit->setFont(font);
}

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
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(userCertEdit->toPlainText());
    QMessageBox::information(this,
                             tr("RetroShare"),
                             tr("Your Cert is copied to Clipboard, paste and send it to your "
                                "friend via email or some other way"));

}
//
//============================================================================
//

bool TextPage::fileSave()
{
    if (fileName.isEmpty())
        return fileSaveAs();

    QFile file(fileName);
    if (!file.open(QFile::WriteOnly))
        return false;

    //Todo: move save to file to p3Peers::SaveCertificateToFile

    QTextStream ts(&file);
    ts.setCodec(QTextCodec::codecForName("UTF-8"));
    ts << userCertEdit->document()->toPlainText();
    userCertEdit->document()->setModified(false);
    return true;
}

bool TextPage::fileSaveAs()
{
    QString fn = QFileDialog::getSaveFileName(this, tr("Save as..."),
                                              QString(), tr("RetroShare Certificate (*.rsc );;All Files (*)"));
    if (fn.isEmpty())
        return false;
    setCurrentFileName(fn);
    return fileSave();    
}

void TextPage::setCurrentFileName(const QString &fileName)
{
    this->fileName = fileName;
    userCertEdit->document()->setModified(false);

    setWindowModified(false);
}


//
//============================================================================
//

int TextPage::nextId() const {

    std::string certstr;
    certstr = friendCertEdit->toPlainText().toUtf8().constData();
	 std::string error_string ;
    RsPeerDetails pd;
    if ( rsPeers->loadDetailsFromStringCert(certstr, pd,error_string) ) {
#ifdef FRIEND_WIZARD_DEBUG
            std::cerr << "ConnectFriendWizard got id : " << pd.id << "; gpg_id : " << pd.gpg_id << std::endl;
#endif
        wizard()->setField(SSL_ID_FIELD_CONNECT_FRIEND_WIZARD, QString::fromStdString(pd.id));
        wizard()->setField(GPG_ID_FIELD_CONNECT_FRIEND_WIZARD, QString::fromStdString(pd.gpg_id));
        wizard()->setField(LOCATION_FIELD_CONNECT_FRIEND_WIZARD, QString::fromUtf8(pd.location.c_str()));
        wizard()->setField(CERT_STRING_FIELD_CONNECT_FRIEND_WIZARD, QString::fromUtf8(certstr.c_str()));

        wizard()->setField("ext_friend_ip", QString::fromStdString(pd.extAddr));
        wizard()->setField("ext_friend_port", QString::number(pd.extPort));
        wizard()->setField("local_friend_ip", QString::fromStdString(pd.localAddr));
        wizard()->setField("local_friend_port", QString::number(pd.localPort));
        wizard()->setField("dyndns", QString::fromStdString(pd.dyndns));

        return ConnectFriendWizard::Page_Conclusion ;
    }
    else
    {
        // error message 
        wizard()->setField("errorMessage",  tr("Certificate Load Failed")+": "+QString::fromStdString(error_string) );
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
    QString titleStr("<span style=\"font-size:16pt; font-weight:500;" "color:white;\">%1</span>");
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

	std::string sOwnId = rsPeers->getGPGOwnId();

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
		if (*it == sOwnId) {
			// its me
			continue;
		}

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
			selectedPeersTW->setItem(row,1,new QTableWidgetItem(QString::fromUtf8(details.name.c_str()))) ;

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
    QString titleStr("<span style=\"font-size:16pt; font-weight:500;"
                               "color:white;\">%1</span>");
    setTitle( titleStr.arg( tr("Certificate files") ) ) ;

    setSubTitle(tr("Use PGP certificates saved in files."));
    
    userFileLabel = new QLabel(tr("You have to generate a file with your "
                                  "certificate and give it to your friend. "
                                  "Also, you can use a file generated "
                                  "before."));
    userFileLabel->setWordWrap(true);
    
    setAcceptDrops(true);
                                  
    userFileCreateButton = new QPushButton;
    userFileCreateButton->setText(tr("Export my certificate..."));
    connect(userFileCreateButton, SIGNAL( clicked() ), this, SLOT( generateCertificateCalled()));

    userFileLayout = new QHBoxLayout;
    userFileLayout->addWidget(userFileLabel);
    userFileLayout->addWidget(userFileCreateButton);

    userFileFrame = new QGroupBox;
    userFileFrame->setFlat(true);
    userFileFrame->setTitle(tr("Import friend's certificate..."));
    userFileFrame->setLayout(userFileLayout);

    friendFileLabel = new QLabel(tr("Drag and Drop your friends's certificate in this Window or specify path "
                                    "in the box below " ) );
    friendFileNameEdit = new QLineEdit;
    registerField("friendCertificateFile*", friendFileNameEdit);

    friendFileNameOpenButton= new QPushButton;
    friendFileNameOpenButton->setText(tr("Browse"));
    connect(friendFileNameOpenButton, SIGNAL( clicked()), this, SLOT( loadFriendCert()));
    
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
                                     "", tr("RetroShare Certificate (*.rsc );;All Files (*)"));

    if (!fileName.isNull())
    {
        friendFileNameEdit->setText(fileName);
        emit completeChanged();
    }
}

//============================================================================

void CertificatePage::generateCertificateCalled() {
    qDebug() << "  generateCertificateCalled";

    std::string cert = rsPeers->GetRetroshareInvite(false);
    if (cert.empty()) {
        QMessageBox::information(this, tr("RetroShare"),
                         tr("Sorry, create certificate failed"),
                         QMessageBox::Ok, QMessageBox::Ok);
        return;
    }

    QString qdir = QFileDialog::getSaveFileName(this,
                                                tr("Please choose a filename"),
                                                QDir::homePath(),
                                                tr("RetroShare Certificate (*.rsc );;All Files (*)"));
    //Todo: move save to file to p3Peers::SaveCertificateToFile

    if (qdir.isEmpty() == false) {
        QFile CertFile (qdir);
        if (CertFile.open(QIODevice::WriteOnly/* | QIODevice::Text*/)) {
            if (CertFile.write(QByteArray(cert.c_str())) > 0) {
                QMessageBox::information(this, tr("RetroShare"),
                                 tr("Certificate file successfully created"),
                                 QMessageBox::Ok, QMessageBox::Ok);
            } else {
                QMessageBox::information(this, tr("RetroShare"),
                                 tr("Sorry, certificate file creation failed"),
                                 QMessageBox::Ok, QMessageBox::Ok);
            }
            CertFile.close();
        } else {
            QMessageBox::information(this, tr("RetroShare"),
                             tr("Sorry, certificate file creation failed"),
                             QMessageBox::Ok, QMessageBox::Ok);
        }
    }
}

//============================================================================

void CertificatePage::dragEnterEvent(QDragEnterEvent *event)
{
    // accept just text/uri-list mime format
    if (event->mimeData()->hasFormat("text/uri-list")) 
    {     
        event->acceptProposedAction();
    }
}
 
 
void CertificatePage::dropEvent(QDropEvent *event)
{
	QList<QUrl> urlList;
	QString fName;
	QFileInfo info;

	if (event->mimeData()->hasUrls())
	{
		urlList = event->mimeData()->urls(); // returns list of QUrls
		// if just text was dropped, urlList is empty (size == 0)

		if ( urlList.size() > 0) // if at least one QUrl is present in list
		{
			fName = urlList[0].toLocalFile(); // convert first QUrl to local path
			info.setFile( fName ); // information about file
			if ( info.isFile() ) 
			friendFileNameEdit->setText( fName ); // if is file, setText
		}
	}
	
	event->acceptProposedAction();
}

//============================================================================

bool CertificatePage::isComplete() const {
    return !( (friendFileNameEdit->text()).isEmpty() );              
}

//============================================================================

int CertificatePage::nextId() const
{
    QString fn = friendFileNameEdit->text();
    if (QFile::exists(fn)) {
        //Todo: move read from file to p3Peers::loadCertificateFromFile

        // read from file
        std::string certstr;
        QFile CertFile (fn);
        if (CertFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            certstr = QString (CertFile.readAll()).toStdString();
            CertFile.close();
        }

        if (certstr.empty() == false) {
            RsPeerDetails pd;
				std::string error_string ;
            if ( rsPeers->loadDetailsFromStringCert(certstr, pd,error_string) ) {
#ifdef FRIEND_WIZARD_DEBUG
                std::cerr << "ConnectFriendWizard got id : " << pd.id << "; gpg_id : " << pd.gpg_id << std::endl;
#endif
                wizard()->setField(SSL_ID_FIELD_CONNECT_FRIEND_WIZARD, QString::fromStdString(pd.id));
                wizard()->setField(GPG_ID_FIELD_CONNECT_FRIEND_WIZARD, QString::fromStdString(pd.gpg_id));
                wizard()->setField(LOCATION_FIELD_CONNECT_FRIEND_WIZARD, QString::fromUtf8(pd.location.c_str()));
                wizard()->setField(CERT_STRING_FIELD_CONNECT_FRIEND_WIZARD, QString::fromUtf8(certstr.c_str()));

                wizard()->setField("ext_friend_ip", QString::fromStdString(pd.extAddr));
                wizard()->setField("ext_friend_port", QString::number(pd.extPort));
                wizard()->setField("local_friend_ip", QString::fromStdString(pd.localAddr));
                wizard()->setField("local_friend_port", QString::number(pd.localPort));
                wizard()->setField("dyndns", QString::fromStdString(pd.dyndns));

                return ConnectFriendWizard::Page_Conclusion ;
            } else {
                wizard()->setField("errorMessage", QString(tr("Certificate Load Failed:something is wrong with %1 ")).arg(fn)+": "+QString::fromStdString(error_string) );
                return ConnectFriendWizard::Page_ErrorMessage;
            }
        } else {
            wizard()->setField("errorMessage", QString(tr("Certificate Load Failed:can't read from file %1 ")).arg(fn) );
            return ConnectFriendWizard::Page_ErrorMessage;
        }
    } else {
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
    QString titleStr("<span style=\"font-size:16pt; font-weight:500;"
                               "color:white;\">%1</span>");
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
    QString titleStr("<span style=\"font-size:16pt; font-weight:500;"
                               "color:white;\">%1</span>");
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
    
    optionsFrame = new QGroupBox;
    optionsFrame ->setTitle( tr("Options") );
    optionsLayout =  new QGridLayout();
    
    groupLabel = new QLabel( tr("Add friend to group:") );
    optionsLayout->addWidget(groupLabel, 0,0,1,1);
    
    groupComboBox = new QComboBox(this);
    optionsLayout->addWidget(groupComboBox, 0,1,1,1);
    
    optionsFrame->setLayout(optionsLayout);


    signGPGCheckBox = new QCheckBox();
    signGPGCheckBox->setText(tr("Authenticate friend (Sign GPG Key)"));
    registerField(SIGN_RADIO_BUTTON_FIELD_CONNECT_FRIEND_WIZARD,signGPGCheckBox);
    acceptNoSignGPGCheckBox = new QCheckBox();
    acceptNoSignGPGCheckBox->setText(tr("Add as friend to connect with"));
    registerField(ACCEPT_RADIO_BUTTON_FIELD_CONNECT_FRIEND_WIZARD,acceptNoSignGPGCheckBox);
    optionsLayout->addWidget(acceptNoSignGPGCheckBox, 5,0,1,-1); // QWidget * widget, int fromRow, int fromColumn, int rowSpan, int columnSpan, Qt::Alignment alignment = 0 )
    optionsLayout->addWidget(signGPGCheckBox, 6,0,1,-1); // QWidget * widget, int fromRow, int fromColumn, int rowSpan, int columnSpan, Qt::Alignment alignment = 0 )
    

    conclusionPageLayout = new QVBoxLayout();
    conclusionPageLayout->addWidget(peerDetailsFrame);
    conclusionPageLayout->addWidget(optionsFrame);

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

    dyndns = new QLineEdit(this);
    dyndns->setVisible(false);
    registerField("dyndns",dyndns);

    //groupLabel = new QLabel(this);
    //groupLabel->setVisible(false);
    registerField(GROUP_ID_FIELD_CONNECT_FRIEND_WIZARD, groupLabel);
}

//============================================================================
//
int ConclusionPage::nextId() const {
    return -1;
}

//
//============================================================================
//
void ConclusionPage::groupCurrentIndexChanged(int index)
{
    setField(GROUP_ID_FIELD_CONNECT_FRIEND_WIZARD, groupComboBox->itemData(index, Qt::UserRole));
}

//
//============================================================================
//

void ConclusionPage::initializePage() {
    std::string id = field(SSL_ID_FIELD_CONNECT_FRIEND_WIZARD).toString().toStdString();
    std::string gpg_id = field(GPG_ID_FIELD_CONNECT_FRIEND_WIZARD).toString().toStdString();
//    std::string location = field(LOCATION_FIELD_CONNECT_FRIEND_WIZARD).toString().toStdString();
    std::string certString = field(CERT_STRING_FIELD_CONNECT_FRIEND_WIZARD).toString().toUtf8().constData();
    std::cerr << "Conclusion page id : " << id << "; gpg_id : " << gpg_id << std::endl;

    RsPeerDetails detail;
	 std::string error_string ;
    if (!rsPeers->loadDetailsFromStringCert(certString, detail,error_string)) {
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
        signGPGCheckBox->setChecked(false);
        acceptNoSignGPGCheckBox->hide();
        acceptNoSignGPGCheckBox->setChecked(false);
    }
    if (!detail.accept_connection && detail.ownsign) {
        //gpg key is already signed, don't propose to sign it again
        acceptNoSignGPGCheckBox->setChecked(true);
        signGPGCheckBox->hide();
        signGPGCheckBox->setChecked(false);
    }
    if (!detail.accept_connection && !detail.ownsign) {
        acceptNoSignGPGCheckBox->setChecked(true);
        signGPGCheckBox->show();
        signGPGCheckBox->setChecked(false);
        acceptNoSignGPGCheckBox->show();
    }
    if (detail.accept_connection && detail.ownsign) {
        acceptNoSignGPGCheckBox->setChecked(false);
        acceptNoSignGPGCheckBox->hide();
        signGPGCheckBox->setChecked(false);
        signGPGCheckBox->hide();
        radioButtonsLabel = new QLabel(tr("It seems your friend is already registered. Adding it might just set it's ip address."));
        radioButtonsLabel->setWordWrap(true);
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
            ts.append(QString::fromUtf8( rsPeers->getPeerName(*it).c_str() ));
            ts.append( "<" ) ;
            ts.append( QString::fromStdString(*it) );
            ts.append( ">" );
            ts.append( "\n" );
    }

    nameEdit->setText( QString::fromUtf8( detail.name.c_str() ) ) ;
    trustEdit->setText(QString::fromStdString( trustString ) ) ;
    emailEdit->setText(QString::fromUtf8( detail.email.c_str() ) );
    locEdit->setText( QString::fromUtf8( detail.location.c_str() ) );
    signersEdit->setPlainText( ts );
    
    std::list<RsGroupInfo> groupInfoList;
    rsPeers->getGroupInfoList(groupInfoList);
    GroupDefs::sortByName(groupInfoList);
    groupComboBox->addItem("", ""); // empty value
    for (std::list<RsGroupInfo>::iterator groupIt = groupInfoList.begin(); groupIt != groupInfoList.end(); groupIt++) {
        groupComboBox->addItem(GroupDefs::name(*groupIt), QString::fromStdString(groupIt->id));
    }

    QString groupId = field(GROUP_ID_FIELD_CONNECT_FRIEND_WIZARD).toString();
    if (groupId.isEmpty() == false) {
        groupComboBox->setCurrentIndex(groupComboBox->findData(groupId));
    }
    connect(groupComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(groupCurrentIndexChanged(int)));
}

//============================================================================
//

//
//============================================================================
//============================================================================
//============================================================================

RsidPage::RsidPage(QWidget *parent) : QWizardPage(parent) {
    QString titleStr("<span style=\"font-size:16pt; font-weight:500;"
                               "color:white;\">%1</span>");
    setTitle( titleStr.arg( tr("RetroShare ID") ) ) ;

    setSubTitle(tr("Use RetroShare ID for adding a Friend which is available in your network."));
    

    setAcceptDrops(true);
                                  
    userRsidLayout = new QHBoxLayout;

    userRsidFrame = new QGroupBox;
    userRsidFrame->setFlat(true);
    userRsidFrame->setTitle(tr("Add Friends RetroShare ID..."));
    userRsidFrame->setLayout(userRsidLayout);

    friendRsidLabel = new QLabel(tr("Paste Friends RetroShare ID "
                                    "in the box below " ) );
    friendRsidEdit = new QLineEdit;
    friendRsidEdit->setWhatsThis(tr("Enter the RetroShare ID of your Friend, e.g. Peer@BDE8D16A46D938CF "));
    registerField("friendRSID*", friendRsidEdit);

    
    friendRSIDLayout = new QHBoxLayout;
    friendRSIDLayout->addWidget(friendRsidEdit) ;
    
    RsidLayout = new QVBoxLayout;
    RsidLayout->addWidget(userRsidFrame);
    RsidLayout->addWidget(friendRsidLabel);
    RsidLayout->addLayout(friendRSIDLayout);

    setLayout(RsidLayout);
}


//============================================================================

bool RsidPage::isComplete() const {
    return !( (friendRsidEdit->text()).isEmpty() );              
}

int RsidPage::nextId() const {

    QString rsidstring = friendRsidEdit->text();

    if (rsidstring.isEmpty() == false) {
        // search for peer id in string
        std::string rsidstr = PeerDefs::idFromRsid(rsidstring, false);

        RsPeerDetails pd;
        if (rsidstr.empty() == false && rsPeers->getPeerDetails(rsidstr, pd) ) {
            wizard()->setField(SSL_ID_FIELD_CONNECT_FRIEND_WIZARD, QString::fromStdString(pd.id));
            wizard()->setField(GPG_ID_FIELD_CONNECT_FRIEND_WIZARD, QString::fromStdString(pd.gpg_id));
            wizard()->setField(LOCATION_FIELD_CONNECT_FRIEND_WIZARD, QString::fromUtf8(pd.location.c_str()));

            wizard()->setField("ext_friend_ip", QString::fromStdString(pd.extAddr));
            wizard()->setField("ext_friend_port", QString::number(pd.extPort));
            wizard()->setField("local_friend_ip", QString::fromStdString(pd.localAddr));
            wizard()->setField("local_friend_port", QString::number(pd.localPort));
            wizard()->setField("dyndns", QString::fromStdString(pd.dyndns));

            return ConnectFriendWizard::Page_Conclusion ;
        } else {
            wizard()->setField("errorMessage", QString(tr("This Peer %1 is not available in your Network")).arg(rsidstring) );
            return ConnectFriendWizard::Page_ErrorMessage;
        }
    }

    return ConnectFriendWizard::Page_Rsid;
}

//
//============================================================================
//============================================================================
//============================================================================

EmailPage::EmailPage(QWidget *parent) : QWizardPage(parent) {
    QString titleStr("<span style=\"font-size:16pt; font-weight:500;"
                               "color:white;\">%1</span>");
    setTitle( titleStr.arg( tr("Invite Friends by Email") ) ) ;

    setSubTitle(tr("Enter your friends' email addresses (seperate each on with a semicolon)"));
                                      
    addressLabel = new QLabel(tr("Your friends' email addresses:" ) );
    addressEdit = new QLineEdit;
    addressEdit->setWhatsThis(tr("Enter Friends Email addresses"));
    registerField("addressEdit*", addressEdit);

    subjectLabel = new QLabel(tr("Subject:" ) );
    subjectEdit = new QLineEdit;
    subjectEdit->setText(tr("RetroShare Invitation"));
    registerField("subjectEdit*", subjectEdit);

    emailhbox2Layout = new QHBoxLayout;
    emailhbox2Layout->addWidget(addressLabel);
    emailhbox2Layout->addWidget(addressEdit) ;

    emailhbox3Layout = new QHBoxLayout;

    emailhbox3Layout->addWidget(subjectLabel);
    emailhbox3Layout->addWidget(subjectEdit ) ;

    inviteTextEdit = new QTextEdit;
    inviteTextEdit->setReadOnly(true);

    inviteTextEdit->setPlainText(GetStartedDialog::GetInviteText());

    emailvboxLayout = new QVBoxLayout;
    emailvboxLayout->addLayout(emailhbox2Layout);
    emailvboxLayout->addLayout(emailhbox3Layout);
    emailvboxLayout->addWidget(inviteTextEdit ) ;

    QFont font("Courier New",11,50,false);
    font.setStyleHint(QFont::TypeWriter,QFont::PreferMatch);
    font.setStyle(QFont::StyleNormal);
    inviteTextEdit->setFont(font);

    setLayout(emailvboxLayout);
}

//============================================================================

bool EmailPage::isComplete() const {
    return !( (addressEdit->text()).isEmpty() );
}

int EmailPage::nextId() const {
    return -1;
}

bool EmailPage::validatePage()
{
    QString mailaddresses = addressEdit->text();

    if (mailaddresses.isEmpty() == false)
    {
        QString body = inviteTextEdit->toPlainText();

        body += "\n" + GetStartedDialog::GetCutBelowText();
        body += "\n\n" + QString::fromUtf8(rsPeers->GetRetroshareInvite(false).c_str());

        sendMail (mailaddresses, subjectEdit->text(), body);
        return true;
    }

    return false;
}
