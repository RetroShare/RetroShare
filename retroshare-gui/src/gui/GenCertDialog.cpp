/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006,  crypton
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

#include <rshare.h>
#include <util/rsrandom.h>
#include <retroshare/rsinit.h>
#include <rsserver/rsaccounts.h>
#include "GenCertDialog.h"
#include <QAbstractEventDispatcher>
#include <QFileDialog>
#include <QGraphicsOpacityEffect>
#include <QTimer>
#include <QMessageBox>
#include <QMouseEvent>
#include <QTextBrowser>
#include <QProgressBar>
#include <time.h>
#include <math.h>
#include <iostream>

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

    if(ui.entropy_bar->value() < 20)
    {
        ui.genButton2->setEnabled(false) ;
        ui.genButton2->setIcon(QIcon(":/images/delete.png")) ;
    ui.genButton2->setToolTip(tr("Currently disabled. Please move your mouse around until you reach at least 20%")) ;
    }
    else
    {
        ui.genButton2->setEnabled(true) ;
        ui.genButton2->setIcon(QIcon(":/images/resume.png")) ;
    ui.genButton2->setToolTip(tr("Click to create your node and/or profile")) ;
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
	: QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint), mOnlyGenerateIdentity(onlyGenerateIdentity)
{
	/* Invoke Qt Designer generated QObject setup routine */
	ui.setupUi(this);
	
	ui.headerFrame->setHeaderImage(QPixmap(":/images/contact_new128.png"));
	ui.headerFrame->setHeaderText(tr("Create a new profile"));

	connect(ui.new_gpg_key_checkbox, SIGNAL(clicked()), this, SLOT(newGPGKeyGenUiSetup()));
    connect(ui.adv_checkbox, SIGNAL(clicked()), this, SLOT(updateUiSetup()));
    connect(ui.hidden_checkbox, SIGNAL(clicked()), this, SLOT(updateUiSetup()));

    connect(ui.genButton2, SIGNAL(clicked()), this, SLOT(genPerson()));
	connect(ui.importIdentity_PB, SIGNAL(clicked()), this, SLOT(importIdentity()));
	connect(ui.exportIdentity_PB, SIGNAL(clicked()), this, SLOT(exportIdentity()));

	//ui.genName->setFocus(Qt::OtherFocusReason);

//	QObject *obj = QCoreApplication::eventFilter() ;
//	std::cerr << "Event filter : " << obj << std::endl;
//	QCoreApplication::instance()->setEventFilter(MyEventFilter) ;

	entropy_timer = new QTimer ;
	entropy_timer->start(20) ;
	QObject::connect(entropy_timer,SIGNAL(timeout()),this,SLOT(grabMouse())) ;

//	EntropyCollectorWidget *ecw = new EntropyCollectorWidget(ui.entropy_bar,this) ;
//	ecw->resize(size()) ;
//	ecw->move(0,0) ;
//
//	QGraphicsOpacityEffect *effect = new QGraphicsOpacityEffect ;
//	effect->setOpacity(0.2) ;
//	ecw->setGraphicsEffect(effect) ;
	//ecw->setBackgroundColor(QColor::fromRGB(1,1,1)) ;
//	ecw->show() ;

	ui.entropy_bar->setValue(0) ;

	// make sure that QVariant always takes an 'int' otherwise the program will crash!
	ui.keylength_comboBox->addItem("default (2048 bits, recommended)", QVariant(2048));
	ui.keylength_comboBox->addItem("high (3072 bits)", QVariant(3072));
    ui.keylength_comboBox->addItem("insane (4096 bits)", QVariant(4096));

#if QT_VERSION >= 0x040700
	ui.email_input->setPlaceholderText(tr("[Optional] Visible to your friends, and friends of friends.")) ;
	ui.node_input->setPlaceholderText(tr("[Required] Examples: Home, Laptop,...")) ;
	ui.hiddenaddr_input->setPlaceholderText(tr("[Required] Tor/I2P address - Examples: xa76giaf6ifda7ri63i263.onion (obtained by you from Tor)")) ;
	ui.name_input->setPlaceholderText(tr("[Required] Visible to your friends, and friends of friends."));
	ui.password_input->setPlaceholderText(tr("[Required] This password protects your private PGP key."));
	ui.password_input_2->setPlaceholderText(tr("[Required] Type the same password again here."));
#endif

	ui.node_input->setToolTip(tr("Enter a meaningful node description. e.g. : home, laptop, etc. \nThis field will be used to differentiate different installations with\nthe same profile (PGP key pair).")) ;

	ui.email_input->hide() ;
	ui.email_label->hide() ;

	/* get all available pgp private certificates....
	 * mark last one as default.
	 */

	init();
}

GenCertDialog::~GenCertDialog()
{
	entropy_timer->stop() ;
}

void GenCertDialog::init()
{
	std::cerr << "Finding PGPUsers" << std::endl;

	ui.genPGPuser->clear() ;

	std::list<RsPgpId> pgpIds;
	std::list<RsPgpId>::iterator it;
	haveGPGKeys = false;
#ifdef TO_REMOVE
	/* replace with true/false below */
	if (!mOnlyGenerateIdentity) {
#endif
		if (RsAccounts::GetPGPLogins(pgpIds)) {
			for(it = pgpIds.begin(); it != pgpIds.end(); ++it)
			{
				QVariant userData(QString::fromStdString( (*it).toStdString() ));
				std::string name, email;
				RsAccounts::GetPGPLoginDetails(*it, name, email);
				std::cerr << "Adding PGPUser: " << name << " id: " << *it << std::endl;
				QString gid = QString::fromStdString( (*it).toStdString()).right(8) ;
				ui.genPGPuser->addItem(QString::fromUtf8(name.c_str()) + " <" + QString::fromUtf8(email.c_str()) + "> (" + gid + ")", userData);
				haveGPGKeys = true;
			}
		}
#ifdef TO_REMOVE
	}
#endif
	if (haveGPGKeys) {
		ui.no_gpg_key_label->hide();
		ui.header_label->show();
		ui.new_gpg_key_checkbox->setChecked(false);
		setWindowTitle(tr("Create new node"));
        ui.genButton2->setText(tr("Generate new node"));
		ui.headerFrame->setHeaderText(tr("Create a new node"));
		genNewGPGKey = false;
	} else {
		ui.no_gpg_key_label->setVisible(!mOnlyGenerateIdentity);
		ui.header_label->setVisible(mOnlyGenerateIdentity);
		ui.new_gpg_key_checkbox->setChecked(true);
		ui.new_gpg_key_checkbox->setEnabled(true);
		setWindowTitle(tr("Create new profile"));
        ui.genButton2->setText(tr("Generate new profile and node"));
		ui.headerFrame->setHeaderText(tr("Create a new profile and node"));
		genNewGPGKey = true;
	}

#ifdef TO_REMOVE
	QString text; /* = ui.header_label->text() + "\n";*/
	text += tr("You can create a new profile with this form.");

	if (mOnlyGenerateIdentity) {
		ui.new_gpg_key_checkbox->setChecked(true);
		ui.new_gpg_key_checkbox->hide();
#endif
		ui.genprofileinfo_label->hide();
#ifdef TO_REMOVE
	} else {
		text += "\n";
		text += tr("Alternatively you can use an existing profile. Just uncheck \"Create a new profile\"");
	}
	ui.header_label->setText(text);
#endif
	newGPGKeyGenUiSetup();
    //updateUiSetup();
}

void GenCertDialog::mouseMoveEvent(QMouseEvent *e)
{
	std::cerr << "Mouse : " << e->x() << ", " << e->y() << std::endl;

	QDialog::mouseMoveEvent(e) ;
}

void GenCertDialog::newGPGKeyGenUiSetup() {
	bool adv_state = ui.adv_checkbox->isChecked();
	bool hidden_state = ui.hidden_checkbox->isChecked();
	ui.no_node_label->setVisible(false);

	if (ui.new_gpg_key_checkbox->isChecked()) {
		genNewGPGKey = true;
		ui.name_label->show();
		ui.name_input->show();
//		ui.email_label->show();
//		ui.email_input->show();
		ui.password_label->show();
		ui.password_label_2->show();
		ui.password_input->show();
		ui.password_input_2->show();
		ui.genPGPuserlabel->hide();
		ui.genPGPuser->hide();
		ui.importIdentity_PB->hide() ;
		ui.exportIdentity_PB->hide();
		setWindowTitle(tr("Create new profile"));
        ui.genButton2->setText(tr("Generate new profile and node"));
		ui.headerFrame->setHeaderText(tr("Create a new profile and node"));
		ui.no_gpg_key_label->setText(tr("Welcome to Retroshare. Before you can proceed you need to create a profile and associate a node with it. To do so please fill out this form.\nAlternatively you can import a (previously exported) profile. Just uncheck \"Create a new profile\""));
		ui.genButton2->setVisible(true);
		ui.adv_checkbox->setVisible(true);
		ui.node_label->setVisible(true);
		ui.node_input->setVisible(true);
		ui.entropy_label->setVisible(true);
		ui.entropy_bar->setVisible(true);
		ui.genprofileinfo_label->setVisible(false);
		if (!mOnlyGenerateIdentity) {
			ui.header_label->setVisible(haveGPGKeys);
		}
		//ui.keylength_label->show();
		//ui.keylength_comboBox->show();
	} else {
		//haveGPGKeys = (ui.genPGPuser->count() != 0)?true:false;
		if (haveGPGKeys) {
			QVariant data = ui.genPGPuser->itemData(ui.genPGPuser->currentIndex());
			if (!rsAccounts->selectAccountByString(data.toString().toStdString())) {
				ui.no_node_label->setText(tr("No node is associated with the profile named") + " " + ui.genPGPuser->currentText() + ". " +tr("Please create a node for it by providing a node name."));
				ui.no_node_label->setVisible(true);
			} else {
				ui.genprofileinfo_label->show();
			}
		}
		genNewGPGKey = false;
		ui.name_label->hide();
		ui.name_input->hide();
//		ui.email_label->hide();
//		ui.email_input->hide();
		ui.password_label->hide();
		ui.password_label_2->hide();
		ui.password_input->hide();
		ui.password_input_2->hide();
		ui.genPGPuserlabel->show();
		ui.genPGPuser->show();
		ui.importIdentity_PB->setVisible(!mOnlyGenerateIdentity);
		ui.exportIdentity_PB->setVisible(haveGPGKeys);
		ui.exportIdentity_PB->setEnabled(haveGPGKeys);
		setWindowTitle(tr("Create new node"));
        ui.genButton2->setText(tr("Generate new node"));
		ui.headerFrame->setHeaderText(tr("Create a new node"));
		ui.no_gpg_key_label->setText(tr("Welcome to Retroshare. Before you can proceed you need to import a profile and after that associate a node with it."));
		ui.genButton2->setVisible(haveGPGKeys);
		ui.adv_checkbox->setVisible(haveGPGKeys);
		ui.adv_checkbox->setChecked(haveGPGKeys && adv_state);
		ui.genPGPuser->setVisible(haveGPGKeys);
		ui.genPGPuserlabel->setVisible(haveGPGKeys);
		ui.node_label->setVisible(haveGPGKeys);
		ui.node_input->setVisible(haveGPGKeys);
		ui.entropy_label->setVisible(haveGPGKeys);
		ui.entropy_bar->setVisible(haveGPGKeys);
		//ui.genprofileinfo_label->show();
		ui.header_label->setVisible(false);
		ui.keylength_label->hide();
		ui.keylength_comboBox->hide();
	}
	updateUiSetup();
	ui.adv_checkbox->setChecked(adv_state);
	ui.hidden_checkbox->setChecked(hidden_state);
}

void GenCertDialog::updateUiSetup()
{
    if (ui.adv_checkbox->isChecked())
    {
        ui.hidden_checkbox->show();
        
		if (ui.new_gpg_key_checkbox->isChecked())
		{
			// key length is only for pgp key creation
			ui.keylength_label->show();
			ui.keylength_comboBox->show();
		}
		else
		{
			ui.keylength_label->hide();
			ui.keylength_comboBox->hide();
		}
 
        if(ui.hidden_checkbox->isChecked())
        {
            ui.hiddenaddr_input->show();
            ui.hiddenaddr_label->show();
            ui.label_hiddenaddr2->show();
            ui.hiddenport_label->show();
            ui.hiddenport_spinBox->show();
        }
        else
        {
            ui.hiddenaddr_input->hide();
            ui.hiddenaddr_label->hide();
            ui.label_hiddenaddr2->hide();
            ui.hiddenport_label->hide();
            ui.hiddenport_spinBox->hide();
        }
    }
    else
    {
            ui.hiddenaddr_input->hide();
            ui.hiddenaddr_label->hide();
            ui.label_hiddenaddr2->hide();
            ui.hiddenport_label->hide();
            ui.hiddenport_spinBox->hide();

        ui.hidden_checkbox->hide();
        ui.keylength_label->hide();
        ui.keylength_comboBox->hide();

        if(ui.hidden_checkbox->isChecked())
            ui.hidden_checkbox->setChecked(false) ;
    }

}

void GenCertDialog::exportIdentity()
{
	QString fname = QFileDialog::getSaveFileName(this,tr("Export profile"), "",tr("RetroShare profile files (*.asc)")) ;

	if(fname.isNull())
		return ;

	QVariant data = ui.genPGPuser->itemData(ui.genPGPuser->currentIndex());
	RsPgpId gpg_id (data.toString().toStdString()) ;

	if(RsAccounts::ExportIdentity(fname.toStdString(),gpg_id))
		QMessageBox::information(this,tr("Profile saved"),tr("Your profile was successfully saved\nIt is encrypted\n\nYou can now copy it to another computer\nand use the import button to load it")) ;
	else
		QMessageBox::information(this,tr("Profile not saved"),tr("Your profile was not saved. An error occurred.")) ;
}

void GenCertDialog::importIdentity()
{
	QString fname = QFileDialog::getOpenFileName(this,tr("Import profile"), "",tr("RetroShare profile files (*.asc)")) ;

	if(fname.isNull())
		return ;

	RsPgpId gpg_id ;
	std::string err_string ;

	if(!RsAccounts::ImportIdentity(fname.toStdString(),gpg_id,err_string))
	{
		QMessageBox::information(this,tr("Profile not loaded"),tr("Your profile was not loaded properly:")+" \n    "+QString::fromStdString(err_string)) ;
		return ;
	}
	else
	{
		std::string name,email ;

		RsAccounts::GetPGPLoginDetails(gpg_id, name, email);
		std::cerr << "Adding PGPUser: " << name << " id: " << gpg_id << std::endl;

		QMessageBox::information(this,tr("New profile imported"),tr("Your profile was imported successfully:")+" \n"+"\nName :"+QString::fromStdString(name)+"\nemail: " + QString::fromStdString(email)+"\nKey ID: "+QString::fromStdString(gpg_id.toStdString())+"\n\n"+tr("You can use it now to create a new node.")) ;
	}

	init() ;
}

void GenCertDialog::genPerson()
{
	/* Check the data from the GUI. */
	std::string genLoc  = ui.node_input->text().toUtf8().constData();
	RsPgpId PGPId;
	bool isHiddenLoc = false;

    if (ui.hidden_checkbox->isChecked())
	{
		std::string hl = ui.hiddenaddr_input->text().toStdString();
		uint16_t port  = ui.hiddenport_spinBox->value();
		if (!RsInit::SetHiddenLocation(hl, port))	/* parses it */
		{
			/* Message Dialog */
			QMessageBox::warning(this,
				tr("Invalid hidden node"),
			tr("Please enter a valid address of the form: 31769173498.onion:7800 or [52 characters].b32.i2p"),
			QMessageBox::Ok);
			return;
		}
		isHiddenLoc = true;
	}

	if (!genNewGPGKey) {
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

		if(ui.password_input->text() != ui.password_input_2->text())
		{
			QMessageBox::warning(this,
								 tr("PGP key pair generation failure"),
								 tr("Passwords do not match"),
								 QMessageBox::Ok);
			return;
		}
		//generate a new gpg key
		std::string err_string;
		ui.no_gpg_key_label->setText(tr("Generating new PGP key pair, please be patient: this process needs generating large prime numbers, and can take some minutes on slow computers. \n\nFill in your PGP password when asked, to sign your new key."));
		ui.no_gpg_key_label->show();
		ui.new_gpg_key_checkbox->hide();
		ui.name_label->hide();
		ui.name_input->hide();
//		ui.email_label->hide();
//		ui.email_input->hide();
		ui.password_label_2->hide();
		ui.password_input_2->hide();
		ui.password_label->hide();
		ui.password_input->hide();
		ui.genPGPuserlabel->hide();
		ui.genPGPuser->hide();
		ui.node_label->hide();
		ui.node_input->hide();
        ui.genButton2->hide();
		ui.importIdentity_PB->hide();
		ui.genprofileinfo_label->hide();
        ui.hidden_checkbox->hide();
        ui.adv_checkbox->hide();
        ui.keylength_label->hide();
		ui.keylength_comboBox->hide();

		setCursor(Qt::WaitCursor) ;

		QCoreApplication::processEvents();
		while(QAbstractEventDispatcher::instance()->processEvents(QEventLoop::AllEvents)) ;

		std::string email_str = "" ;
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
	bool okGen = RsAccounts::GenerateSSLCertificate(PGPId, "", genLoc, "", isHiddenLoc, sslPasswd, sslId, err);

	if (okGen)
	{
		/* complete the process */
		RsInit::LoadPassword(sslPasswd);
		if (Rshare::loadCertificate(sslId, false)) {
			accept();
		}
	}
	else
	{
		/* Message Dialog */
		QMessageBox::warning(this,
                               tr("Profile generation failure"),
                               tr("Failed to generate your new certificate, maybe PGP password is wrong!"),
                               QMessageBox::Ok);
		reject();
       }
}
