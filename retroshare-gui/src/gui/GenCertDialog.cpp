/*******************************************************************************
 * gui/GenCertDialog.cpp                                                       *
 *                                                                             *
 * Copyright (C) 2006 Crypton         <retroshare.project@gmail.com>           *
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

#include "GenCertDialog.h"

#include <QAbstractEventDispatcher>
#include <QLineEdit>
#include <QFileDialog>
#include <QGraphicsOpacityEffect>
#include <QMessageBox>
#include <QMenu>
#include <QMouseEvent>
#include <QTextBrowser>
#include <QTimer>
#include <QProgressBar>

#include <rshare.h>
#include "gui/settings/rsharesettings.h"
#include "util/misc.h"
#include "gui/common/FilesDefs.h"

#include "retroshare/rstor.h"
#include "retroshare/rsidentity.h"
#include "retroshare/rsinit.h"
#include "retroshare/rsnotify.h"
#include "rsserver/rsaccounts.h"
#include "util/rsrandom.h"

#include <time.h>
#include <math.h>
#include <iostream>

#define IMAGE_GOOD ":/images/accepted16.png"
#define IMAGE_BAD  ":/images/cancel.png"

class EntropyCollectorWidget: public QTextBrowser
{
	public:
		EntropyCollectorWidget(QProgressBar *pr,QWidget *p = NULL)
			: QTextBrowser(p) 
		{
			progress = pr ;
			setMouseTracking(true) ;
			entropy_values_collected = 0 ;
		}
		virtual void mouseMoveEvent(QMouseEvent *e)
		{
			std::cerr << "Mouse moved: " << e->x() << ", " << e->y() << std::endl;
			++entropy_values_collected ;

			progress->setValue(entropy_values_collected*100 / 4096) ;
		}

		int entropy_values_collected ;
		QProgressBar *progress ;
};

class MyFilter: public QObject
{
	public:
		virtual bool eventFilter(QObject *obj, QEvent *event)
		{
			if(event->type() == QEvent::MouseMove)
				std::cerr << "Mouse moved !"<< std::endl;

			return QObject::eventFilter(obj,event) ;
		}
};

void GenCertDialog::grabMouse()
{
	static uint32_t last_x = 0 ;
	static uint32_t last_y = 0 ;
	static uint32_t count = 0 ;

	uint32_t x = QCursor::pos().x() ;
	uint32_t y = QCursor::pos().y() ;

	if(last_x == x && last_y == y)
		return ;

	last_x = x ;
	last_y = y ;

	// Let's do some shuffle with mouse coordinates. Does not need to be cryptographically random,
	// since the random number generator will shuffle this appropriately in openssl.
	//
	uint32_t E = ((count*x*86243 + y*y*15641) & 0xffff) ^ 0xb374;
	uint32_t F = ((x*44497*y*count + x*x) & 0xffff) ^ 0x395b ;

	++count ;

//	std::cerr << "Mouse grabed at " << x << " " << y << ". Adding entropy E=" << std::hex << E << ", F=" << F << ", digit =" << E + (F << 16) << std::dec << std::endl;

	ui.entropy_bar->setValue(count*100/2048) ;

    if(!mEntropyOk && ui.entropy_bar->value() >= 20)
	{
		mEntropyOk = true ;
		updateCheckLabels();
	}

	RsInit::collectEntropy(E+(F << 16)) ;
}
//static bool MyEventFilter(void *message, long *result)
//{
//	std::cerr << "Event called " << message << std::endl;
//	return false ;
//}
/** Default constructor */
GenCertDialog::GenCertDialog(bool onlyGenerateIdentity, QWidget *parent)
  : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint)
  , mOnlyGenerateIdentity(onlyGenerateIdentity)
  ,	mGXSNickname("")
{
	/* Invoke Qt Designer generated QObject setup routine */
	ui.setupUi(this);
	
    //ui.headerFrame->setHeaderImage(FilesDefs::getPixmapFromQtResourcePath(":/icons/svg/profile.svg"));
	//ui.headerFrame->setHeaderText(tr("Create a new profile"));

	connect(ui.reuse_existing_node_CB, SIGNAL(triggered()), this, SLOT(switchReuseExistingNode()));
	connect(ui.adv_checkbox, SIGNAL(toggled(bool)), this, SLOT(setupState()));
	connect(ui.nodeType_CB, SIGNAL(currentIndexChanged(int)), this, SLOT(setupState()));

	connect(ui.genButton, SIGNAL(clicked()), this, SLOT(genPerson()));
	connect(ui.importIdentity_PB, SIGNAL(clicked()), this, SLOT(importIdentity()));
	connect(ui.exportIdentity_PB, SIGNAL(clicked()), this, SLOT(exportIdentity()));

	connect(ui.password_input,   SIGNAL(textChanged(QString)), this, SLOT(updateCheckLabels()));
	connect(ui.password2_input, SIGNAL(textChanged(QString)), this, SLOT(updateCheckLabels()));
	connect(ui.name_input,       SIGNAL(textChanged(QString)), this, SLOT(updateCheckLabels()));
	connect(ui.node_input,       SIGNAL(textChanged(QString)), this, SLOT(updateCheckLabels()));
	connect(ui.reuse_existing_node_CB, SIGNAL(toggled(bool)), this, SLOT(updateCheckLabels()));

	connect(ui.cbUseBob, SIGNAL(clicked(bool)), this, SLOT(useBobChecked(bool)));;

	entropy_timer = new QTimer ;
	entropy_timer->start(20) ;
	QObject::connect(entropy_timer,SIGNAL(timeout()),this,SLOT(grabMouse())) ;

	ui.entropy_bar->setValue(0) ;

	// make sure that QVariant always takes an 'int' otherwise the program will crash!
	ui.keylength_comboBox->addItem("Default (2048 bits, recommended)", QVariant(2048));
	ui.keylength_comboBox->addItem("High (3072 bits)", QVariant(3072));
	ui.keylength_comboBox->addItem("Very high (4096 bits)", QVariant(4096));

	// Default value.

	ui.node_input->setText("My computer") ;

#if QT_VERSION >= 0x040700
	ui.node_input->setPlaceholderText(tr("Node name")) ;
	ui.hiddenaddr_input->setPlaceholderText(tr("Tor/I2P address")) ;
	ui.name_input->setPlaceholderText(tr("Username"));
	ui.nickname_input->setPlaceholderText(tr("Chat name"));
	ui.password_input->setPlaceholderText(tr("Password"));
	ui.password2_input->setPlaceholderText(tr("Password again"));
#endif

	ui.nickname_input->setMaxLength(RSID_MAXIMUM_NICKNAME_SIZE);

	/* get all available pgp private certificates....
	 * mark last one as default.
	 */
	 
	//QMenu *menu = new QMenu(tr("Advanced options"));
	//menu->addAction(ui.adv_checkbox);
	//menu->addAction(ui.reuse_existing_node_CB);
 //   ui.optionsButton->setMenu(menu);

    mAllFieldsOk = false ;
    mEntropyOk = false ;

#ifdef RS_ONLYHIDDENNODE
	ui.adv_checkbox->setChecked(true);
	ui.adv_checkbox->setVisible(false);
	ui.nodeType_CB->setCurrentIndex(1);
	ui.nodeType_CB->setEnabled(false);
#endif
//#ifdef RETROTOR
//	ui.adv_checkbox->setChecked(false);
//	ui.adv_checkbox->setVisible(true);
//#endif

	initKeyList();
    setupState();
	updateCheckLabels();
}

GenCertDialog::~GenCertDialog()
{
	entropy_timer->stop() ;
}

void GenCertDialog::switchReuseExistingNode()
{
    if(ui.reuse_existing_node_CB->isChecked())
    {
        // import an existing identity if needed. If none is available, keep the box unchecked.

        if(!haveGPGKeys && !importIdentity())
			ui.reuse_existing_node_CB->setChecked(false);
    }

	initKeyList();
    setupState();
}

void GenCertDialog::initKeyList()
{
	std::cerr << "Finding PGPUsers" << std::endl;

	ui.genPGPuser->clear() ;

	std::list<RsPgpId> pgpIds;
	std::list<RsPgpId>::iterator it;
	haveGPGKeys = false;

	if (RsAccounts::GetPGPLogins(pgpIds)) {
		for(it = pgpIds.begin(); it != pgpIds.end(); ++it)
		{
			QVariant userData(QString::fromStdString( (*it).toStdString() ));
			std::string name, email;
			RsAccounts::GetPGPLoginDetails(*it, name, email);
			std::cerr << "Adding PGPUser: " << name << " id: " << *it << std::endl;
			QString gid = QString::fromStdString( (*it).toStdString()).right(8) ;
			ui.genPGPuser->addItem(QString::fromUtf8(name.c_str()) + " (" + gid + ")", userData);
			haveGPGKeys = true;
		}
	}
}

void GenCertDialog::mouseMoveEvent(QMouseEvent *e)
{
	std::cerr << "Mouse : " << e->x() << ", " << e->y() << std::endl;

	QDialog::mouseMoveEvent(e) ;
}

void GenCertDialog::setupState()
{
	bool adv_state = ui.adv_checkbox->isChecked();

    if(!adv_state)
    {
        ui.reuse_existing_node_CB->setChecked(false) ;
        ui.keylength_comboBox->setCurrentIndex(0) ;
//        ui.nodeType_CB->setCurrentIndex(0);
    }
	ui.reuse_existing_node_CB->setVisible(adv_state) ;

//    ui.nodeType_CB->setVisible(adv_state) ;
//    ui.nodeType_LB->setVisible(adv_state) ;
//    ui.nodeTypeExplanation_TE->setVisible(adv_state) ;

	bool hidden_state = ui.nodeType_CB->currentIndex()==1 || ui.nodeType_CB->currentIndex()==2;
    bool generate_new = !ui.reuse_existing_node_CB->isChecked();
    bool tor_auto = ui.nodeType_CB->currentIndex()==1;

	genNewGPGKey = generate_new;

    switch(ui.nodeType_CB->currentIndex())
    {
    case 0: ui.nodeTypeExplanation_TE->setText(tr("<b>Your IP is visible to trusted nodes only. You can also connect to hidden nodes if running Tor on your machine. Best choice for sharing with trusted friends.</b>"));
        break;
    case 1: ui.nodeTypeExplanation_TE->setText(tr("<b>Your IP is hidden. All traffic happens over the Tor network. Best choice if you cannot trust friend nodes with your own IP.</b>"));
        break;
    case 2: ui.nodeTypeExplanation_TE->setText(tr("<b>Hidden node for advanced users only. Allows to use other proxy solutions such as I2P.</b>"));
        break;
    }

	//ui.no_node_label->setVisible(false);

	setWindowTitle(generate_new?tr("Create new profile and new Retroshare node"):tr("Create new Retroshare node"));
	//ui.headerFrame->setHeaderText(generate_new?tr("Create a new profile and node"):tr("Create a new node"));

    ui.reuse_existing_node_CB->setEnabled(adv_state) ;
    ui.importIdentity_PB->setVisible(adv_state && !generate_new) ;
    //ui.exportIdentity_PB->setVisible(adv_state && !generate_new) ;
    ui.exportIdentity_PB->setVisible(false); // not useful, and probably confusing

    ui.genPGPuser->setVisible(adv_state && haveGPGKeys && !generate_new) ;

	//ui.genprofileinfo_label->setVisible(false);
	//ui.no_gpg_key_label->setText(tr("Welcome to Retroshare. Before you can proceed you need to create a profile and associate a node with it. To do so please fill out this form.\nAlternatively you can import a (previously exported) profile. Just uncheck \"Create a new profile\""));
	//no_gpg_key_label->setVisible(false);

	ui.name_label->setVisible(true);
	ui.name_input->setVisible(generate_new);

    //ui.header_label->setVisible(false) ;

	ui.nickname_label->setVisible(adv_state && !mOnlyGenerateIdentity);
	ui.nickname_input->setVisible(adv_state && !mOnlyGenerateIdentity);

	ui.node_name_check_LB->setVisible(adv_state);
	ui.node_label->setVisible(adv_state);
	ui.node_input->setVisible(adv_state);

	ui.password_input->setVisible(true);
	ui.password_label->setVisible(true);

    if(generate_new)
        ui.password_input->setToolTip(tr("<html><p>Put a strong password here. This password will be required to start your Retroshare node and protects all your data.</p></html>"));
	else
        ui.password_input->setToolTip(tr("<html><p>Please supply the existing password for the selected profile above.</p></html>"));

	ui.password2_check_LB->setVisible(generate_new);
	ui.password2_input->setVisible(generate_new);
	ui.password2_label->setVisible(generate_new);

	ui.keylength_label->setVisible(adv_state);
	ui.keylength_comboBox->setVisible(adv_state);

	ui.entropy_bar->setVisible(true);
	ui.genButton->setVisible(true);

	ui.hiddenaddr_input->setVisible(hidden_state && !tor_auto);
	ui.hiddenaddr_label->setVisible(hidden_state && !tor_auto);

	ui.hiddenport_label->setVisible(hidden_state && !tor_auto);
	ui.hiddenport_spinBox->setVisible(hidden_state && !tor_auto);

	ui.cbUseBob->setVisible(hidden_state && !tor_auto);
#ifndef RS_USE_I2P_BOB
	ui.cbUseBob->setDisabled(true);
	ui.cbUseBob->setToolTip(tr("BOB support is not available"));
#endif

	if(!mAllFieldsOk)
	{
		ui.genButton->setToolTip(tr("<p>Node creation is disabled until all fields correctly set.</p>")) ;

		ui.genButton->setVisible(false) ;
		ui.generate_label->setVisible(false) ;
		ui.info_label->setText("Please choose a profile name and password...") ;
		ui.info_label->setVisible(true) ;
	}
	else if(!mEntropyOk)
	{
		ui.genButton->setToolTip(tr("<p>Node creation is disabled until enough randomness is collected. Please mouve your mouse around until you reach at least 20%.</p>")) ;

		ui.genButton->setVisible(false) ;
		ui.generate_label->setVisible(false) ;
		ui.info_label->setText("Please move your mouse randomly to generate enough random data to create your profile.") ;
		ui.info_label->setVisible(true) ;
	}
	else
	{
		ui.genButton->setEnabled(true) ;
		//ui.genButton->setIcon(QIcon(IMAGE_GOOD)) ;
		ui.genButton->setToolTip(tr("Click to create your node and/or profile")) ;
		ui.genButton->setVisible(true) ;
		ui.generate_label->setVisible(false) ;
		ui.info_label->setVisible(false) ;
	}
}

void GenCertDialog::exportIdentity()
{
	QString fname = QFileDialog::getSaveFileName(this,tr("Export profile"), "",tr("RetroShare profile files (*.asc)")) ;

	if(fname.isNull()) return ;
	if(fname.right(4).toUpper() != ".ASC") fname += ".asc";

	QVariant data = ui.genPGPuser->itemData(ui.genPGPuser->currentIndex());
	RsPgpId gpg_id (data.toString().toStdString()) ;

	if(RsAccounts::ExportIdentity(fname.toStdString(),gpg_id))
		QMessageBox::information(this,tr("Profile saved"),tr("Your profile was successfully saved\nIt is encrypted\n\nYou can now copy it to another computer\nand use the import button to load it")) ;
	else
		QMessageBox::information(this,tr("Profile not saved"),tr("Your profile was not saved. An error occurred.")) ;
}

void GenCertDialog::updateCheckLabels()
{
    QPixmap good( IMAGE_GOOD ) ;
    QPixmap bad ( IMAGE_BAD  ) ;

    bool generate_new = !ui.reuse_existing_node_CB->isChecked();
    mAllFieldsOk = true ;

    if(ui.node_input->text().length() >= 3)
		ui.node_name_check_LB   ->setPixmap(good) ;
    else
    {
        mAllFieldsOk = false ;
		ui.node_name_check_LB   ->setPixmap(bad) ;
    }

    if(!generate_new || ui.name_input->text().length() >= 3)
		ui.profile_name_check_LB   ->setPixmap(good) ;
    else
    {
        mAllFieldsOk = false ;
		ui.profile_name_check_LB   ->setPixmap(bad) ;
    }

	if(ui.password_input->text().length() >= 3)
		ui.password_check_LB   ->setPixmap(good) ;
    else
    {
        mAllFieldsOk = false ;
		ui.password_check_LB   ->setPixmap(bad) ;
    }
    if(ui.password_input->text().length() >= 3 && ui.password_input->text() == ui.password2_input->text())
		ui.password2_check_LB   ->setPixmap(good) ;
    else
    {
        if(generate_new)
			mAllFieldsOk = false ;

		ui.password2_check_LB   ->setPixmap(bad) ;
    }

    if(mEntropyOk)
        ui.randomness_check_LB->setPixmap(FilesDefs::getPixmapFromQtResourcePath(IMAGE_GOOD)) ;
	else
        ui.randomness_check_LB->setPixmap(FilesDefs::getPixmapFromQtResourcePath(IMAGE_BAD)) ;

	setupState();
}

void GenCertDialog::useBobChecked(bool checked)
{
	if (checked) {
		ui.hiddenaddr_input->setPlaceholderText(tr("I2P instance address with BOB enabled"));
		ui.hiddenaddr_label->setText(tr("I2P instance address"));

		ui.hiddenport_spinBox->setEnabled(false);
	} else {
		ui.hiddenaddr_input->setPlaceholderText(tr("hidden service address"));
		ui.hiddenaddr_label->setText(tr("hidden address"));

		ui.hiddenport_spinBox->setEnabled(true);
	}
}

bool GenCertDialog::importIdentity()
{
	QString fname ;
	if(!misc::getOpenFileName(this,RshareSettings::LASTDIR_CERT,tr("Import profile"), tr("RetroShare profile files (*.asc);;All files (*)"),fname))
		return false;

	if(fname.isNull())
		return false;

	RsPgpId gpg_id ;
	std::string err_string ;

	if(!RsAccounts::ImportIdentity(fname.toStdString(),gpg_id,err_string))
	{
		QMessageBox::information(this,tr("Profile not loaded"),tr("Your profile was not loaded properly:")+" \n    "+QString::fromStdString(err_string)) ;
		return false;
	}
	else
	{
		std::string name,email ;

		RsAccounts::GetPGPLoginDetails(gpg_id, name, email);
		std::cerr << "Adding PGPUser: " << name << " id: " << gpg_id << std::endl;

		QMessageBox::information(this,tr("New profile imported"),tr("Your profile was imported successfully:")+" \n"+"\nName :"+QString::fromStdString(name)+"\nKey ID: "+QString::fromStdString(gpg_id.toStdString())+"\n\n"+tr("You can use it now to create a new node.")) ;

		initKeyList();
		setupState();

        return true ;
	}
}

void GenCertDialog::genPerson()
{
	/* Check the data from the GUI. */
	std::string genLoc  = ui.node_input->text().toUtf8().constData();
	RsPgpId PGPId;

	if(ui.nickname_input->isVisible())
	{
		mGXSNickname = ui.nickname_input->text();
	}
	else
	{
		mGXSNickname = ui.name_input->text();
	}

	if (!mGXSNickname.isEmpty())
	{
		if (mGXSNickname.size() < RSID_MINIMUM_NICKNAME_SIZE)
		{
			std::cerr << "GenCertDialog::genPerson() GXS Nickname too short (min " << RSID_MINIMUM_NICKNAME_SIZE<< " chars)";
			std::cerr << std::endl;

			QMessageBox::warning(this, "", tr("The GXS nickname is too short. Please input at least %1 characters.").arg(RSID_MINIMUM_NICKNAME_SIZE), QMessageBox::Ok, QMessageBox::Ok);
			mGXSNickname = "";
			return;
		}
		if (mGXSNickname.size() > RSID_MAXIMUM_NICKNAME_SIZE)
		{
			std::cerr << "GenCertDialog::genPerson() GXS Nickname too long (max " << RSID_MAXIMUM_NICKNAME_SIZE<< " chars)";
			std::cerr << std::endl;

			QMessageBox::warning(this, "", tr("The GXS nickname is too long. Please reduce the length to %1 characters.").arg(RSID_MAXIMUM_NICKNAME_SIZE), QMessageBox::Ok, QMessageBox::Ok);
			mGXSNickname = "";
			return;
		}
	}

	bool isHiddenLoc = (ui.nodeType_CB->currentIndex()>0);
    bool isAutoTor = (ui.nodeType_CB->currentIndex()==1);

    if(isAutoTor && !RsTor::isTorAvailable())
    {
        QMessageBox::critical(this,tr("Tor is not available"),tr("No Tor executable has been found on your system. You need to install Tor before creating a hidden identity.")) ;
        return ;
    }

    if(isHiddenLoc)
	{
		std::string hl = ui.hiddenaddr_input->text().toStdString();
		uint16_t port  = ui.hiddenport_spinBox->value();

		bool useBob    = ui.cbUseBob->isChecked();

		if (useBob && hl.empty())
			hl = "127.0.0.1";

		RsInit::SetHiddenLocation(hl, port, useBob);	/* parses it */
	}


	if (!genNewGPGKey)
    {
		if (genLoc.length() < 3) {
			/* Message Dialog */
			QMessageBox::warning(this,
								 tr("PGP key pair generation failure"),
								 tr("Node field is required with a minimum of 3 characters"),
								 QMessageBox::Ok);
			return;
		}
		int pgpidx = ui.genPGPuser->currentIndex();
		if (pgpidx < 0)
		{
			/* Message Dialog */
			QMessageBox::warning(this,
								 tr("Profile generation failure"),
								 tr("Missing PGP certificate"),
								 QMessageBox::Ok);
			return;
		}
		QVariant data = ui.genPGPuser->itemData(pgpidx);
		PGPId = RsPgpId((data.toString()).toStdString());
	} else {
		if (ui.password_input->text().length() < 3 || ui.name_input->text().length() < 3 || genLoc.length() < 3) {
			/* Message Dialog */
			QMessageBox::warning(this,
								 tr("PGP key pair generation failure"),
								 tr("All fields are required with a minimum of 3 characters"),
								 QMessageBox::Ok);
			return;
		}

		if(ui.password_input->text() != ui.password2_input->text())
		{
			QMessageBox::warning(this,
								 tr("PGP key pair generation failure"),
								 tr("Passwords do not match"),
								 QMessageBox::Ok);
			return;
		}
		//generate a new gpg key
		std::string err_string;
		//_key_label->setText(tr("Generating new node key, please be patient: this process needs generating large prime numbers, and can take some minutes on slow computers. \n\nFill in your password when asked, to sign your new key."));
		//ui.no_gpg_key_label->show();
		//ui.reuse_existing_node_CB->hide();
		ui.name_label->hide();
		ui.name_input->hide();
		ui.nickname_label->hide();
		ui.nickname_input->hide();
		ui.password2_label->hide();
		ui.password2_input->hide();
		ui.password2_check_LB->hide();
		ui.password_check_LB->hide();
		ui.password_label->hide();
		ui.password_input->hide();
		//ui.genPGPuserlabel->hide();
		ui.genPGPuser->hide();
		ui.node_label->hide();
		ui.node_input->hide();
		ui.genButton->hide();
		ui.importIdentity_PB->hide();
		//ui.genprofileinfo_label->hide();
		ui.nodeType_CB->hide();
		//ui.adv_checkbox->hide();
		ui.keylength_label->hide();
		ui.keylength_comboBox->hide();

		setCursor(Qt::WaitCursor) ;

		QCoreApplication::processEvents();
		QAbstractEventDispatcher* ed = QAbstractEventDispatcher::instance();
		std::cout << "Waiting ed->processEvents()" << std::endl;
		time_t waitEnd = time(NULL) + 10;//Wait no more than 10 sec to processEvents
		if (ed->hasPendingEvents())
			while(ed->processEvents(QEventLoop::AllEvents) && (time(NULL) < waitEnd));

		std::string email_str = "" ;
		std::cout << "RsAccounts::GeneratePGPCertificate" << std::endl;
		RsAccounts::GeneratePGPCertificate(
					ui.name_input->text().toUtf8().constData(),
					email_str.c_str(),
					ui.password_input->text().toUtf8().constData(),
					PGPId,
					ui.keylength_comboBox->itemData(ui.keylength_comboBox->currentIndex()).toInt(),
					err_string);

		setCursor(Qt::ArrowCursor) ;
	}
	//generate a random ssl password
	std::string sslPasswd = RSRandom::random_alphaNumericString(RsInit::getSslPwdLen()) ;

	/* GenerateSSLCertificate - selects the PGP Account */
	//RsInit::SelectGPGAccount(PGPId);

	RsPeerId sslId;
	std::cerr << "GenCertDialog::genPerson() Generating SSL cert with gpg id : " << PGPId << std::endl;
	std::string err;
	this->hide();//To show dialog asking password PGP Key.
	std::cout << "RsAccounts::GenerateSSLCertificate" << std::endl;

    // now cache the PGP password so that it's not asked again for immediately signing the key
    rsNotify->cachePgpPassphrase(ui.password_input->text().toUtf8().constData()) ;

    bool okGen = RsAccounts::createNewAccount(PGPId, "", genLoc, "", isHiddenLoc, isAutoTor, sslPasswd, sslId, err);

	if (okGen)
	{
		/* complete the process */
		RsInit::LoadPassword(sslPasswd);
		if (Rshare::loadCertificate(sslId, false)) {

            // Normally we should clear the cached passphrase as soon as possible. However,some other GUI components may still need it at start.
            // (csoler) This is really bad: we have to guess that 30 secs will be enough. I have no better way to do this.

            QTimer::singleShot(30000, []() { rsNotify->clearPgpPassphrase(); } );

            accept();
		}
	}
	else
    {
        // Now clear the cached passphrase
        rsNotify->clearPgpPassphrase();

        /* Message Dialog */
        QMessageBox::warning(this,
                             tr("Profile generation failure"),
                             tr("Failed to generate your new certificate, maybe PGP password is wrong!"),
                             QMessageBox::Ok);

        reject();
    }
}
