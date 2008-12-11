/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006, 2007 crypton
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
#include "ShareManager.h"

#include "rsiface/rsfiles.h"



#include <QMessageBox>

/** Default constructor */
ShareManager::ShareManager(QWidget *parent, Qt::WFlags flags)
  : QDialog(parent, flags)
{
  /* Invoke Qt Designer generated QObject setup routine */
  ui.setupUi(this);


  connect(ui.addButton, SIGNAL(clicked( bool ) ), this , SLOT( addShareDirectory() ) );
  connect(ui.removeButton, SIGNAL(clicked( bool ) ), this , SLOT( removeShareDirectory() ) );
  connect(ui.buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
  connect(ui.buttonBox, SIGNAL(accepted()), this, SLOT(accept()));

	ui.addButton->setToolTip(tr("Add a Share Directory"));
	ui.removeButton->setToolTip(tr("Remove selected Shared Directory"));
	
	load();

}
/** Loads the settings for this page */
void ShareManager::load()
{
	std::list<std::string>::const_iterator it;
	std::list<std::string> dirs;
	rsFiles->getSharedDirectories(dirs);
	
	/* get a link to the table */
	QListWidget *listWidget = ui.shareddirList;
	
	/* remove old items ??? */
	listWidget->clear();
	
	for(it = dirs.begin(); it != dirs.end(); it++)
	{
		/* (0) Dir Name */
		listWidget->addItem(QString::fromStdString(*it));
	}

	//ui.incomingDir->setText(QString::fromStdString(rsFiles->getDownloadDirectory()));
	
	listWidget->update(); /* update display */


}

void ShareManager::addShareDirectory()
{

	/* select a dir
	 */


 	QString qdir = QFileDialog::getExistingDirectory(this, tr("Add Shared Directory"), "",
				QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
	
	/* add it to the server */
	std::string dir = qdir.toStdString();
	if (dir != "")
	{
		rsFiles->addSharedDirectory(dir);
		load();
	}
}

void ShareManager::removeShareDirectory()
{
	/* id current dir */
	/* ask for removal */
	QListWidget *listWidget = ui.shareddirList;
	QListWidgetItem *qdir = listWidget -> currentItem();
	if (qdir)
	{
		rsFiles->removeSharedDirectory( qdir->text().toStdString());
		load();
	}
}







