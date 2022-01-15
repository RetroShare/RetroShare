/*******************************************************************************
 * gui/QuikStartWizard.cpp                                                     *
 *                                                                             *
 * Copyright (c) 2006 Retroshare Team  <retroshare.project@gmail.com>          *
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

#include "QuickStartWizard.h"

#include "gui/common/FilesDefs.h"
#ifdef USE_COMBOBOX
#include "gui/common/RSComboBox.h"
#endif
#include "settings/rsharesettings.h"
#include "util/QtVersion.h"

#include "retroshare/rsfiles.h"
#include "retroshare/rsconfig.h"
#include "retroshare/rspeers.h"
#include "retroshare/rstypes.h"

#include <QCheckBox>
#include <QFileDialog>
#include <QHeaderView>
#include <QMessageBox>
#include <QSettings>

QuickStartWizard::QuickStartWizard(QWidget *parent) :
    QDialog(parent)
{
    ui.setupUi(this);

    ui.headerFrame->setHeaderImage(FilesDefs::getPixmapFromQtResourcePath(":/images/tools_wizard.png"));
    ui.headerFrame->setHeaderText("RetroShare");

	  ui.pagesWizard->setCurrentIndex(0);
	  
          loadNetwork();
	  loadShare();
	  loadGeneral();

	  
//	   ui.checkBoxF2FRouting->setChecked(true) ;
//	   ui.checkBoxF2FRouting->setEnabled(false) ;
	  
	  connect( ui.netModeComboBox, SIGNAL( activated(int) ), this, SLOT( toggleUPnP() ) );
//	  connect( ui.checkBoxTunnelConnection, SIGNAL( toggled( bool ) ), this, SLOT( toggleTunnelConnection(bool) ) );
	  
//	  bool b = rsPeers->getAllowTunnelConnection() ;
//    ui.checkBoxTunnelConnection->setChecked(b) ;
    
    QHeaderView_setSectionResizeModeColumn(ui.shareddirList->horizontalHeader(), 0, QHeaderView::Stretch);
    QHeaderView_setSectionResizeModeColumn(ui.shareddirList->horizontalHeader(), 2, QHeaderView::Interactive);
 
    ui.shareddirList->horizontalHeader()->resizeSection( 0, 360 );
    ui.shareddirList->horizontalHeader()->setStretchLastSection(false);
	  
  /* Hide platform specific features */
#ifndef Q_OS_WIN
  ui.checkBoxRunRetroshareAtSystemStartup->setVisible(false);
  ui.chkRunRetroshareAtSystemStartupMinimized->setVisible(false);
#endif
}

QuickStartWizard::~QuickStartWizard()
{
}

void QuickStartWizard::changeEvent(QEvent *e)
{
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui.retranslateUi(this);
        break;
    default:
        break;
    }
}

void QuickStartWizard::on_pushButtonWelcomeNext_clicked()
{
        ui.pagesWizard->setCurrentIndex(1);
}

void QuickStartWizard::on_pushButtonWelcomeExit_clicked()
{
        close();
}

void QuickStartWizard::on_pushButtonConnectionBack_clicked()
{
        ui.pagesWizard->setCurrentIndex(0);
}

void QuickStartWizard::on_pushButtonConnectionNext_clicked()
{
        /* Check if netMode has changed */
        int netMode = 0;
        switch(ui.netModeComboBox->currentIndex())
        {
                case 2:
                        netMode = RS_NETMODE_EXT;
                        break;
                case 1:
                        netMode = RS_NETMODE_UDP;
                        break;
                default:
                case 0:
                        netMode = RS_NETMODE_UPNP;
                        break;
        }
        std::cerr << "ui.netModeComboBox->currentIndex()" << ui.netModeComboBox->currentIndex() << std::endl;
        rsPeers->setNetworkMode(rsPeers->getOwnId(), netMode);

        /* Check if vis has changed */
		uint16_t vs_disc = 0;
		uint16_t vs_dht = 0;
		switch(ui.discoveryComboBox->currentIndex())
		{
			case 0:
				vs_disc = RS_VS_DISC_FULL;
				vs_dht = RS_VS_DHT_FULL;
				break;
			case 1:
				vs_disc = RS_VS_DISC_FULL;
				vs_dht = RS_VS_DHT_OFF;
				break;
			case 2:
				vs_disc = RS_VS_DISC_OFF;
				vs_dht = RS_VS_DHT_FULL;
				break;
			case 3:
			default:
				vs_disc = RS_VS_DISC_OFF;
				vs_dht = RS_VS_DHT_OFF;
				break;
		}
	

        RsPeerDetails detail;
        if (!rsPeers->getPeerDetails(rsPeers->getOwnId(), detail))
        {
                return;
        }
		if ((vs_disc != detail.vs_disc) || (vs_dht != detail.vs_dht))
		{
			rsPeers->setVisState(rsPeers->getOwnId(), vs_disc, vs_dht);
		}

	rsConfig->setMaxDataRates( ui.doubleSpinBoxDownloadSpeed->value(), ui.doubleSpinBoxUploadSpeed->value(), ui.doubleSpinBoxDownloadSpeed->value(), ui.doubleSpinBoxUploadSpeed->value() );

	ui.pagesWizard->setCurrentIndex(2);
}

void QuickStartWizard::on_pushButtonConnectionExit_clicked()
{
        //on_pushButtonConnectionNext_clicked();
        close();
}

void QuickStartWizard::on_pushButtonSharesBack_clicked()
{
        ui.pagesWizard->setCurrentIndex(1);
}

void QuickStartWizard::on_pushButtonSharesNext_clicked()
{
        ui.pagesWizard->setCurrentIndex(3);
}

void QuickStartWizard::on_pushButtonSharesExit_clicked()
{
        close();
}

void QuickStartWizard::on_pushButtonStyleBack_clicked()
{
        ui.pagesWizard->setCurrentIndex(2);
}

void QuickStartWizard::on_pushButtonStyleNext_clicked()
{
        ui.pagesWizard->setCurrentIndex(4);
}

void QuickStartWizard::on_pushButtonStyleExit_clicked()
{
        close();
}

void QuickStartWizard::on_pushButtonSystemBack_clicked()
{
        ui.pagesWizard->setCurrentIndex(3);
}

void QuickStartWizard::on_pushButtonSystemFinish_clicked()
{
  Settings->setStartMinimized(ui.checkBoxStartMinimized->isChecked());
  Settings->setValue("doQuit", ui.checkBoxQuit->isChecked());
#ifdef Q_OS_WIN
  Settings->setRunRetroshareOnBoot(ui.checkBoxRunRetroshareAtSystemStartup->isChecked(), ui.chkRunRetroshareAtSystemStartupMinimized->isChecked());
#endif

  saveChanges();
  
  close();
}

void QuickStartWizard::on_pushButtonSystemExit_clicked()
{
	close();
}

void QuickStartWizard::on_pushButtonSharesAdd_clicked()
{
/* select a dir
	 */

	QString qdir = QFileDialog::getExistingDirectory(this, tr("Select A Folder To Share"), "",
			QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

	/* add it to the server */
	std::string dir = qdir.toStdString();
	if (dir != "")
	{
		SharedDirInfo sdi ;
		sdi.filename = dir ;
        sdi.shareflags = DIR_FLAGS_BROWSABLE | DIR_FLAGS_ANONYMOUS_DOWNLOAD ;

		rsFiles->addSharedDirectory(sdi);

		messageBoxOk(tr("Shared Directory Added!"));
		loadShare();
	}
}

void QuickStartWizard::on_pushButtonSharesRemove_clicked()
{
	/* id current dir */
	/* ask for removal */
	QTableWidget *listWidget = ui.shareddirList;
	int row = listWidget -> currentRow();
	QTableWidgetItem *qdir = listWidget->item(row,0) ;

	QString queryWrn;
	queryWrn.clear();
	queryWrn.append(tr("Do you really want to stop sharing this directory ?"));

	if (qdir)
	{
		if ((QMessageBox::question(this, tr("Warning!"),queryWrn,QMessageBox::Ok|QMessageBox::No, QMessageBox::Ok))== QMessageBox::Ok)
		{
			rsFiles->removeSharedDirectory( qdir->text().toStdString());
			loadShare();
		}
		else
		return;
	}
}

void QuickStartWizard::on_shareIncomingDirectory_clicked()
{
	rsFiles->shareDownloadDirectory(ui.shareIncomingDirectory->isChecked());
	loadShare();
}

void QuickStartWizard::loadShare()
{
	std::cerr << "ShareManager:: In load !!!!!" << std::endl ;

	std::list<SharedDirInfo>::const_iterator it;
	std::list<SharedDirInfo> dirs;
	rsFiles->getSharedDirectories(dirs);

	ui.shareIncomingDirectory->setChecked(rsFiles->getShareDownloadDirectory());

	/* get a link to the table */
	QTableWidget *listWidget = ui.shareddirList;

	/* remove old items ??? */
	listWidget->clearContents() ;
	listWidget->setRowCount(0) ;

	connect(this,SIGNAL(itemClicked(QTableWidgetItem*)),this,SLOT(updateFlags(QTableWidgetItem*))) ;

	int row=0 ;
	for(it = dirs.begin(); it != dirs.end(); ++it,++row)
	{
		listWidget->insertRow(row) ;
		listWidget->setItem(row,0,new QTableWidgetItem(QString::fromStdString((*it).filename)));
#ifdef USE_COMBOBOX
		RSComboBox *cb = new RSComboBox ;
		cb->addItem(tr("Network Wide")) ;
		cb->addItem(tr("Browsable")) ;
		cb->addItem(tr("Universal")) ;

		cb->setToolTip(tr("Please decide whether this directory is\n* Network Wide: \tanonymously shared over the network (including your friends)\n* Browsable: \tbrowsable by your friends\n* Universal: \t\tboth")) ;

		// TODO
		//  - set combobox current value depending on what rsFiles reports.
		//  - use a signal mapper to get the correct row that contains the combo box sending the signal:
		//  		mapper = new SignalMapper(this) ;
		//
		//  		for(all cb)
		//  		{
		//  			signalMapper->setMapping(cb,...)
		//  		}
		//
		int index = 0 ;
		index += ((*it).shareflags & RS_FILE_HINTS_NETWORK_WIDE) > 0 ;
		index += (((*it).shareflags & RS_FILE_HINTS_BROWSABLE) > 0) * 2 ;
		listWidget->setCellWidget(row,1,cb);

		if(index < 1 || index > 3)
			std::cerr << "******* ERROR IN FILE SHARING FLAGS. Flags = " << (*it).shareflags << " ***********" << std::endl ;
		else
			index-- ;

		cb->setCurrentIndex(index) ;
#else
		QCheckBox *cb1 = new QCheckBox ;
		QCheckBox *cb2 = new QCheckBox ;

		cb1->setChecked( (*it).shareflags & DIR_FLAGS_ANONYMOUS_DOWNLOAD ) ;
        cb2->setChecked( (*it).shareflags & DIR_FLAGS_BROWSABLE ) ;

		cb1->setToolTip(tr("If checked, the share is anonymously shared to anybody.")) ;
		cb2->setToolTip(tr("If checked, the share is browsable by your friends.")) ;

		listWidget->setCellWidget(row,1,cb1);
		listWidget->setCellWidget(row,2,cb2);

		QObject::connect(cb1,SIGNAL(toggled(bool)),this,SLOT(updateFlags(bool))) ;
		QObject::connect(cb2,SIGNAL(toggled(bool)),this,SLOT(updateFlags(bool))) ;
#endif
	}

	//ui.incomingDir->setText(QString::fromStdString(rsFiles->getDownloadDirectory()));

	listWidget->update(); /* update display */
	update();
}

void QuickStartWizard::updateFlags(bool b)
{
	std::cerr << "Updating flags (b=" << b << ") !!!" << std::endl ;

	std::list<SharedDirInfo>::iterator it;
	std::list<SharedDirInfo> dirs;
	rsFiles->getSharedDirectories(dirs);

	int row=0 ;
	for(it = dirs.begin(); it != dirs.end(); ++it,++row)
	{
		std::cerr << "Looking for row=" << row << ", file=" << (*it).filename << ", flags=" << (*it).shareflags << std::endl ;
		FileStorageFlags current_flags(0u) ;
		current_flags |= (dynamic_cast<QCheckBox*>(ui.shareddirList->cellWidget(row,1)))->isChecked()? DIR_FLAGS_ANONYMOUS_DOWNLOAD:(FileStorageFlags)0u ;
        current_flags |= (dynamic_cast<QCheckBox*>(ui.shareddirList->cellWidget(row,2)))->isChecked()? DIR_FLAGS_BROWSABLE   :(FileStorageFlags)0u ;

		if( ((*it).shareflags ^ current_flags).toUInt32() )
		{
			(*it).shareflags = current_flags ;
			rsFiles->updateShareFlags(*it) ;	// modifies the flags

			std::cout << "Updating share flags for directory " << (*it).filename << std::endl ;
		}
	}
}

bool QuickStartWizard::messageBoxOk(QString msg)
 {
    QMessageBox mb("Share Manager InfoBox!",msg,QMessageBox::Information,QMessageBox::Ok,0,0,this);
    mb.exec();
    return true;
 }

/*void QuickStartWizard::showEvent(QShowEvent *event)
{
	if (!event->spontaneous())
	{
		loadsharelist();
	}
}*/

/** Loads the settings for this page */
void
QuickStartWizard::loadGeneral()
{
#ifdef Q_OS_WIN
  bool minimized;
  ui.checkBoxRunRetroshareAtSystemStartup->setChecked(Settings->runRetroshareOnBoot(minimized));
  ui.chkRunRetroshareAtSystemStartupMinimized->setChecked(minimized);
#endif

  ui.checkBoxStartMinimized->setChecked(Settings->getStartMinimized());
  ui.checkBoxQuit->setChecked(Settings->value("doQuit", false).toBool());
  
  ui.rbtPageOnToolBar->setChecked(Settings->getPageButtonLoc());
	ui.rbtPageOnListItem->setChecked(!Settings->getPageButtonLoc());

  //ui.checkBoxQuickWizard->setChecked(settings.value(QString::fromUtf8("FirstRun"), false).toBool());
}

//bool QuickStartWizard::firstRunWizard() const {
//  if(ui.checkBoxQuickWizard->isChecked()) return true;
//  return ui.checkBoxQuickWizard->isChecked();
//}

/** Loads the settings for this page */
void QuickStartWizard::loadNetwork()
{

	/* load up configuration from rsPeers */
	RsPeerDetails detail;
	if (!rsPeers->getPeerDetails(rsPeers->getOwnId(), detail))
	{
		return;
	}

	/* set net mode */
	int netIndex = 0;
	switch(detail.netMode)
	{
		case RS_NETMODE_EXT:
			netIndex = 2;
			break;
		case RS_NETMODE_UDP:
			netIndex = 1;
			break;
        case RS_NETMODE_HIDDEN:
            ui.netModeLabel->hide();
            ui.netModeComboBox->hide();
            ui.discoveryLabel->hide();
            ui.discoveryComboBox->hide();
            ui.netModeComboBox->insertItem(3,"Hidden Node");
            netIndex = 3;
            break;
		default:
		case RS_NETMODE_UPNP:
			netIndex = 0;
			break;
	}
	ui.netModeComboBox->setCurrentIndex(netIndex);

	/* DHT + Discovery: (public)
	 * Discovery only:  (private)
	 * DHT only: (inverted)
	 * None: (dark net)
	 */

	netIndex = 3; // NONE.
	if (detail.vs_dht != RS_VS_DISC_OFF)
	{
		if (detail.vs_disc != RS_VS_DISC_OFF)
		{
			netIndex = 0; // PUBLIC
		}
		else
		{
			netIndex = 2; // INVERTED
		}
	}
	else
	{
		if (detail.vs_disc != RS_VS_DISC_OFF)
		{
			netIndex = 1; // PRIVATE
		}
		else
		{
			netIndex = 3; // NONE
		}
	}
	
	ui.discoveryComboBox->setCurrentIndex(netIndex);

	int dlrate = 0;
	int ulrate = 0;
	int dlratewi = 0;
	int ulratewi = 0;
	rsConfig->getMaxDataRates(dlrate, ulrate, dlratewi, ulratewi);
	ui.doubleSpinBoxDownloadSpeed->setValue(dlrate);
	ui.doubleSpinBoxUploadSpeed->setValue(ulrate);

}

void QuickStartWizard::saveChanges()
{

	//bool saveAddr = false;

	Settings->setPageButtonLoc(ui.rbtPageOnToolBar->isChecked());


	RsPeerDetails detail;
	RsPeerId ownId = rsPeers->getOwnId();

	if (!rsPeers->getPeerDetails(ownId, detail))
	{
		return;
	}


	/* Check if netMode has changed */
	int netMode = 0;
        int netIndex = ui.netModeComboBox->currentIndex();
        std::cerr << "ui.netModeComboBox->currentIndex()" << ui.netModeComboBox->currentIndex() << std::endl;
        switch(netIndex)
	{
		case 3:
			netMode = RS_NETMODE_HIDDEN;
			break;
		case 2:
			netMode = RS_NETMODE_EXT;
			break;
		case 1:
			netMode = RS_NETMODE_UDP;
			break;
		default:
		case 0:
			netMode = RS_NETMODE_UPNP;
			break;
	}
    
	rsPeers->setNetworkMode(ownId, netMode);

	/* Check if vis has changed */
	uint16_t vs_disc = 0;
	uint16_t vs_dht = 0;
	switch(ui.discoveryComboBox->currentIndex())
	{
		case 0:
			vs_disc = RS_VS_DISC_FULL;
			vs_dht = RS_VS_DHT_FULL;
			break;
		case 1:
			vs_disc = RS_VS_DISC_FULL;
			vs_dht = RS_VS_DHT_OFF;
			break;
		case 2:
			vs_disc = RS_VS_DISC_OFF;
			vs_dht = RS_VS_DHT_FULL;
			break;
		case 3:
		default:
			vs_disc = RS_VS_DISC_OFF;
			vs_dht = RS_VS_DHT_OFF;
			break;
	}
		
	if ((vs_disc != detail.vs_disc) || (vs_dht != detail.vs_dht))
	{
		rsPeers->setVisState(ownId, vs_disc, vs_dht);
	}

	/*if (0 != netIndex)
	{
		saveAddr = true;
	}*/

	/*if (saveAddr)
	{
	  rsPeers->setLocalAddress(rsPeers->getOwnId(), ui.localAddress->text().toStdString(), ui.localPort->value());
	  rsPeers->setExtAddress(rsPeers->getOwnId(), ui.extAddress->text().toStdString(), ui.extPort->value());
	}*/

	rsConfig->setMaxDataRates( ui.doubleSpinBoxDownloadSpeed->value(), ui.doubleSpinBoxUploadSpeed->value(), ui.doubleSpinBoxDownloadSpeed->value(), ui.doubleSpinBoxUploadSpeed->value() );
	loadNetwork();
}

//void QuickStartWizard::toggleTunnelConnection(bool b)
//{
//        std::cerr << "QuickStartWizard::toggleTunnelConnection() set tunnel to : " << b << std::endl;
//        rsPeers->allowTunnelConnection(b) ;
//}
