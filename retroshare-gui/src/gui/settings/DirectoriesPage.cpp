/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006 - 2009 RetroShare Team
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

#include "DirectoriesPage.h"
#include "rshare.h"
#include "rsiface/rsfiles.h"

#include <algorithm>


DirectoriesPage::DirectoriesPage(QWidget * parent, Qt::WFlags flags)
    : QWidget(parent, flags)
{
    ui.setupUi(this);
    setAttribute(Qt::WA_QuitOnClose, false);

    load();
  
  	connect(ui.addButton, SIGNAL(clicked( bool ) ), this , SLOT( addShareDirectory() ) );
  	connect(ui.removeButton, SIGNAL(clicked( bool ) ), this , SLOT( removeShareDirectory() ) );
  	connect(ui.incomingButton, SIGNAL(clicked( bool ) ), this , SLOT( setIncomingDirectory() ) );
  	connect(ui.partialButton, SIGNAL(clicked( bool ) ), this , SLOT( setPartialsDirectory() ) );
  	connect(ui.checkBox, SIGNAL(stateChanged(int)), this, SLOT(shareDownloadDirectory(int)));

    ui.addButton->setToolTip(tr("Add a Share Directory"));
    ui.removeButton->setToolTip(tr("Remove Shared Directory"));
    ui.incomingButton->setToolTip(tr("Browse"));
    ui.partialButton->setToolTip(tr("Browse"));

    if (rsFiles->getShareDownloadDirectory())
    {
    	ui.checkBox->setDown(true);		/* signal not emitted */
    }
    else
    {
    	ui.checkBox->setDown(false);	/* signal not emitted */
    }

  /* Hide platform specific features */
#ifdef Q_WS_WIN

#endif
}

void
DirectoriesPage::closeEvent (QCloseEvent * event)
{
    QWidget::closeEvent(event);
}


/** Saves the changes on this page */
bool
DirectoriesPage::save(QString &errmsg)
{
	/* this is usefull especially when shared incoming files is
	 * default option and when the user don't check/uncheck the
	 * checkBox, so no signal is emitted to update the shared list */
	if (ui.checkBox->isChecked())
	{
		std::list<std::string>::const_iterator it;
		std::list<std::string> dirs;
		rsFiles->getSharedDirectories(dirs);

		if (dirs.end() == std::find(dirs.begin(), dirs.end(), rsFiles->getDownloadDirectory()))
		{
			rsFiles->shareDownloadDirectory();
		}
		rsFiles->setShareDownloadDirectory(true);
	}
	else
	{
		rsFiles->unshareDownloadDirectory();
		rsFiles->setShareDownloadDirectory(false);
	}

	return true;

}
  
/** Loads the settings for this page */
void DirectoriesPage::load()
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

void DirectoriesPage::addShareDirectory()
{

	/* select a dir
	 */

	int ind;
 	QString qdir = QFileDialog::getExistingDirectory(this, tr("Add Shared Directory"), "",
 	                            QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
        ind=qdir.lastIndexOf("/");

	/* add it to the server */
	std::string dir = qdir.toStdString();
	if (dir != "")
	{
		rsFiles->addSharedDirectory(dir);
		load();
	}
}

void DirectoriesPage::removeShareDirectory()
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

void DirectoriesPage::setIncomingDirectory()
{
 	QString qdir = QFileDialog::getExistingDirectory(this, tr("Set Incoming Directory"), "",
				QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

	std::string dir = qdir.toStdString();
	if (dir != "")
	{
		rsFiles->setDownloadDirectory(dir);
		if (ui.checkBox->isChecked())
		{
			std::list<std::string>::const_iterator it;
			std::list<std::string> dirs;
			rsFiles->getSharedDirectories(dirs);

			if (dirs.end() == std::find(dirs.begin(), dirs.end(), rsFiles->getDownloadDirectory()))
			{
				rsFiles->shareDownloadDirectory();
			}
		}
	}
	load();
}

void DirectoriesPage::setPartialsDirectory()
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

void DirectoriesPage::shareDownloadDirectory(int state)
{
	if (state == Qt::Checked)
	{
		std::list<std::string>::const_iterator it;
		std::list<std::string> dirs;
		rsFiles->getSharedDirectories(dirs);

		if (dirs.end() == std::find(dirs.begin(), dirs.end(), rsFiles->getDownloadDirectory()))
		{
			rsFiles->shareDownloadDirectory();
		}
		rsFiles->setShareDownloadDirectory(true);
	}
	else
	{
		rsFiles->unshareDownloadDirectory();
		rsFiles->setShareDownloadDirectory(false);
	}
	load();
}


