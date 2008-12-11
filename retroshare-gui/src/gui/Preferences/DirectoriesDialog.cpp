/****************************************************************
 *  RShare is distributed under the following license:
 *
 *  Copyright (C) 2006, crypton
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
#include "rsiface/rsfiles.h"
#include "DirectoriesDialog.h"


/** Constructor */
DirectoriesDialog::DirectoriesDialog(QWidget *parent)
: ConfigPage(parent)
{
  /* Invoke the Qt Designer generated object setup routine */
  ui.setupUi(this);

 /* Create RshareSettings object */
  _settings = new RshareSettings();

  connect(ui.addButton, SIGNAL(clicked( bool ) ), this , SLOT( addShareDirectory() ) );
  connect(ui.removeButton, SIGNAL(clicked( bool ) ), this , SLOT( removeShareDirectory() ) );
  connect(ui.incomingButton, SIGNAL(clicked( bool ) ), this , SLOT( setIncomingDirectory() ) );
  connect(ui.partialButton, SIGNAL(clicked( bool ) ), this , SLOT( setPartialsDirectory() ) );


  /* Hide platform specific features */
#ifdef Q_WS_WIN

#endif
}

/** Saves the changes on this page */
bool
DirectoriesDialog::save(QString &errmsg)
{

	return true;
}

/** Loads the settings for this page */
void DirectoriesDialog::load()
{
	std::list<std::string>::const_iterator it;
	std::list<std::string> dirs;
	rsFiles->getSharedDirectories(dirs);
	
	/* get a link to the table */
	QListWidget *listWidget = ui.dirList;
	
	/* remove old items ??? */
	listWidget->clear();
	
	for(it = dirs.begin(); it != dirs.end(); it++)
	{
		/* (0) Dir Name */
		listWidget->addItem(QString::fromStdString(*it));
	}

	ui.incomingDir->setText(QString::fromStdString(rsFiles->getDownloadDirectory()));
	ui.partialsDir->setText(QString::fromStdString(rsFiles->getPartialsDirectory()));
	
	listWidget->update(); /* update display */


}

void DirectoriesDialog::addShareDirectory()
{

	/* select a dir
	 */


 	QString qdir = QFileDialog::getOpenFileName(this, tr("Add Shared Directory"),tr("All Files (*)"));
	
	/* add it to the server */
	std::string dir = qdir.toStdString();
	if (dir != "")
	{
		rsFiles->addSharedDirectory(dir);
		load();
	}
}

void DirectoriesDialog::removeShareDirectory()
{
	/* id current dir */
	/* ask for removal */
	QListWidget *listWidget = ui.dirList;
	QListWidgetItem *qdir = listWidget -> currentItem();
	if (qdir)
	{
		rsFiles->removeSharedDirectory( qdir->text().toStdString());
		load();
	}
}

void DirectoriesDialog::setIncomingDirectory()
{
 	QString qdir = QFileDialog::getExistingDirectory(this, tr("Set Incoming Directory"), "",
				QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
	
	std::string dir = qdir.toStdString();
	if (dir != "")
	{
		rsFiles->setDownloadDirectory(dir);
	}
	load();
}

void DirectoriesDialog::setPartialsDirectory()
{
 	QString qdir = QFileDialog::getExistingDirectory(this, tr("Set Partials Directory"), "",
				QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
	
	std::string dir = qdir.toStdString();
	if (dir != "")
	{
		rsFiles->setPartialsDirectory(dir);
	}
	load();
}

