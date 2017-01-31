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
#include "gui/ShareManager.h"
#include <QMessageBox>

#include "rshare.h"
#include <retroshare/rsfiles.h>

#include <algorithm>

DirectoriesPage::DirectoriesPage(QWidget * parent, Qt::WindowFlags flags)
    : ConfigPage(parent, flags)
{
    ui.setupUi(this);

    connect(ui.incomingButton, SIGNAL(clicked( bool ) ), this , SLOT( setIncomingDirectory() ) );
    connect(ui.partialButton, SIGNAL(clicked( bool ) ), this , SLOT( setPartialsDirectory() ) );
    connect(ui.editShareButton, SIGNAL(clicked()), this, SLOT(editDirectories()));
    connect(ui.autoCheckDirectories_CB, SIGNAL(clicked(bool)), this, SLOT(toggleAutoCheckDirectories(bool)));

    connect(ui.autoCheckDirectories_CB,     SIGNAL(toggled(bool)),    this,SLOT(updateAutoCheckDirectories())) ;
    connect(ui.autoCheckDirectoriesDelay_SB,SIGNAL(valueChanged(int)),this,SLOT(updateAutoScanDirectoriesPeriod())) ;
    connect(ui.shareDownloadDirectoryCB,    SIGNAL(toggled(bool)),    this,SLOT(updateShareDownloadDirectory())) ;
    connect(ui.followSymLinks_CB,           SIGNAL(toggled(bool)),    this,SLOT(updateFollowSymLinks())) ;
}

void DirectoriesPage::toggleAutoCheckDirectories(bool b)
{
	ui.autoCheckDirectoriesDelay_SB->setEnabled(b);
}

void DirectoriesPage::editDirectories()
{
	ShareManager::showYourself() ;
}

void DirectoriesPage::updateAutoCheckDirectories()       {    rsFiles->setWatchEnabled(ui.autoCheckDirectories_CB->isChecked()) ; }
void DirectoriesPage::updateAutoScanDirectoriesPeriod()  {    rsFiles->setWatchPeriod(ui.autoCheckDirectoriesDelay_SB->value()); }
void DirectoriesPage::updateShareDownloadDirectory()     {    rsFiles->shareDownloadDirectory(ui.shareDownloadDirectoryCB->isChecked());}
void DirectoriesPage::updateFollowSymLinks()             {    rsFiles->setFollowSymLinks(ui.followSymLinks_CB->isChecked()); }

/** Loads the settings for this page */
void DirectoriesPage::load()
{
	ui.shareDownloadDirectoryCB->setChecked(rsFiles->getShareDownloadDirectory());

	int u = rsFiles->watchPeriod() ;
    ui.autoCheckDirectoriesDelay_SB->setValue(u) ;
    ui.autoCheckDirectories_CB->setChecked(rsFiles->watchEnabled()) ; ;

	ui.incomingDir->setText(QString::fromUtf8(rsFiles->getDownloadDirectory().c_str()));
	ui.partialsDir->setText(QString::fromUtf8(rsFiles->getPartialsDirectory().c_str()));
	ui.followSymLinks_CB->setChecked(rsFiles->followSymLinks());
}

void DirectoriesPage::setIncomingDirectory()
{
	QString qdir = QFileDialog::getExistingDirectory(this, tr("Set Incoming Directory"), "", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
	if (qdir.isEmpty()) {
		return;
	}

	ui.incomingDir->setText(qdir);
	std::string dir = ui.incomingDir->text().toUtf8().constData();

    if(!dir.empty())
		rsFiles->setDownloadDirectory(dir);
}

void DirectoriesPage::setPartialsDirectory()
{
	QString qdir = QFileDialog::getExistingDirectory(this, tr("Set Partials Directory"), "", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
	if (qdir.isEmpty()) {
		return;
	}

	ui.partialsDir->setText(qdir);
    std::string	dir = ui.partialsDir->text().toUtf8().constData();
	if (!dir.empty())
		rsFiles->setPartialsDirectory(dir);
}
