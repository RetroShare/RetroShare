/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2012, RetroShare Team
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
#include <retroshare/rspeers.h>
#include "ProfileManager.h"
#include "gui/GenCertDialog.h"
#include "gui/common/RSTreeWidgetItem.h"

#include <QAbstractEventDispatcher>
#include <QFileDialog>
#include <QMessageBox>
#include <QTreeWidget>
#include <QMenu>

#include <time.h>

#define IMAGE_EXPORT         ":/images/exportpeers_16x16.png"



/** Default constructor */
ProfileManager::ProfileManager(QWidget *parent, Qt::WFlags flags)
  : QDialog(parent, flags)
{
  /* Invoke Qt Designer generated QObject setup routine */
  ui.setupUi(this);

  connect(ui.identityTreeWidget, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( identityTreeWidgetCostumPopupMenu( QPoint ) ) );
  connect(ui.newIdentity_PB, SIGNAL(clicked()), this, SLOT(newIdentity()));
  connect(ui.importIdentity_PB, SIGNAL(clicked()), this, SLOT(importIdentity()));
  connect(ui.exportIdentity_PB, SIGNAL(clicked()), this, SLOT(exportIdentity()));

  ui.identityTreeWidget -> setColumnCount(3);

  init() ;
}

void ProfileManager::identityTreeWidgetCostumPopupMenu( QPoint )
{
  QTreeWidgetItem *wi = getCurrentIdentity();
  if (!wi)
    return;

	QMenu contextMnu( this );

	//QAction* exportidentityAct = new QAction(QIcon(IMAGE_EXPORT), tr( "Export Identity" ), &contextMnu );
	//connect( exportidentityAct , SIGNAL( triggered() ), this, SLOT( exportIdentity() ) );
	//contextMnu.addAction( exportidentityAct);

	contextMnu.exec(QCursor::pos());
}

void ProfileManager::init()
{
    std::cerr << "Finding PGPUsers" << std::endl;
    
    QTreeWidget *identityTreeWidget = ui.identityTreeWidget;

#define COLUMN_NAME			0
#define COLUMN_EMAIL		1
#define COLUMN_GID			2

		QTreeWidgetItem *item;
 
    std::list<std::string> pgpIds;
    std::list<std::string>::iterator it;
    bool foundGPGKeys = false;
    
    if (RsInit::GetPGPLogins(pgpIds)) {
            for(it = pgpIds.begin(); it != pgpIds.end(); it++)
            {
                    QVariant userData(QString::fromStdString(*it));
                    std::string name, email;
                    RsInit::GetPGPLoginDetails(*it, name, email);
                    std::cerr << "Adding PGPUser: " << name << " id: " << *it << std::endl;
                    QString gid = QString::fromStdString(*it).right(8) ;
                    ui.genPGPuser->addItem(QString::fromUtf8(name.c_str()) + " <" + QString::fromUtf8(email.c_str()) + "> (" + gid + ")", userData);
                    
                    item = new RSTreeWidgetItem(NULL, 0);
                    item -> setText(COLUMN_NAME, QString::fromUtf8(name.c_str()));
                    item -> setText(COLUMN_EMAIL, QString::fromUtf8(email.c_str()));
                    item -> setText(COLUMN_GID, gid);
                    identityTreeWidget->addTopLevelItem(item);

                    foundGPGKeys = true;
            }
    }
    
    identityTreeWidget->update(); /* update display */

}

void ProfileManager::exportIdentity()
{
	QString fname = QFileDialog::getSaveFileName(this,tr("Export Identity"), "",tr("RetroShare Identity files (*.asc)")) ;

	if(fname.isNull())
		return ;

	QVariant data = ui.genPGPuser->itemData(ui.genPGPuser->currentIndex());
	
	std::string gpg_id = data.toString().toStdString() ;

	if(RsInit::exportIdentity(fname.toStdString(),gpg_id))
		QMessageBox::information(this,tr("Identity saved"),tr("Your identity was successfully saved\nIt is encrypted\n\nYou can now copy it to another computer\nand use the import button to load it")) ;
	else
		QMessageBox::information(this,tr("Identity not saved"),tr("Your identity was not saved. An error occured.")) ;
}

void ProfileManager::importIdentity()
{
	QString fname = QFileDialog::getOpenFileName(this,tr("Export Identity"), "",tr("RetroShare Identity files (*.asc)")) ;

	if(fname.isNull())
		return ;

	std::string gpg_id ;
	std::string err_string ;

	if(!RsInit::importIdentity(fname.toStdString(),gpg_id,err_string))
	{
		QMessageBox::information(this,tr("Identity not loaded"),tr("Your identity was not loaded properly:")+" \n    "+QString::fromStdString(err_string)) ;
		return ;
	}
	else
	{
		std::string name,email ;

		RsInit::GetPGPLoginDetails(gpg_id, name, email);
		std::cerr << "Adding PGPUser: " << name << " id: " << gpg_id << std::endl;

		QMessageBox::information(this,tr("New identity imported"),tr("Your identity was imported successfuly:")+" \n"+"\nName :"+QString::fromStdString(name)+"\nemail: " + QString::fromStdString(email)+"\nKey ID: "+QString::fromStdString(gpg_id)+"\n\n"+tr("You can use it now to create a new location.")) ;
	}

	init() ;

}

void ProfileManager::selectFriend()
{
#if 0
	/* still need to find home (first) */

	QString fileName = QFileDialog::getOpenFileName(this, tr("Select Trusted Friend"), "",
                                             tr("Certificates (*.pqi *.pem)"));

	std::string fname, userName;
	fname = fileName.toStdString();
	if (RsInit::ValidateTrustedUser(fname, userName))
	{
		ui.genFriend -> setText(QString::fromStdString(userName));
	}
	else
	{
		ui.genFriend -> setText("<Invalid Selected>");
	}
#endif
}

void ProfileManager::checkChanged(int /*i*/)
{
#if 0
	if (i)
	{
		selectFriend();
	}
	else
	{
		/* invalidate selection */
		std::string fname = "";
		std::string userName = "";
		RsInit::ValidateTrustedUser(fname, userName);
		ui.genFriend -> setText("<None Selected>");
	}
#endif
}

void ProfileManager::loadCertificates()
{
    std::string lockFile;
    int retVal = RsInit::LockAndLoadCertificates(false, lockFile);
	switch(retVal)
	{
		case 0: close();
				break;
		case 1:	QMessageBox::warning(	this,
										tr("Multiple instances"),
										tr("Another RetroShare using the same profile is "
											"already running on your system. Please close "
											"that instance first") );
				break;
		case 2:	QMessageBox::warning(	this,
										tr("Multiple instances"),
										tr("An unexpected error occurred when Retroshare"
											"tried to acquire the single instance lock") );
				break;
		case 3:	QMessageBox::warning(	this,
										tr("Generate ID Failure"),
										tr("Failed to Load your new Certificate!") );
				break;
		default: std::cerr << "StartDialog::loadCertificates() unexpected switch value " << retVal << std::endl;
	}
}

void ProfileManager::newIdentity()
{
				GenCertDialog gd;
				gd.exec ();
}

QTreeWidgetItem *ProfileManager::getCurrentIdentity()
{ 
        if (ui.identityTreeWidget->selectedItems().size() != 0)  {
            return ui.identityTreeWidget -> currentItem();
        } 

        return NULL;
} 
