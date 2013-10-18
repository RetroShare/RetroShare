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
    connect(ui.cleanHashCachePB, SIGNAL(clicked()), this, SLOT(clearHashCache()));
    connect(ui.rememberHashesCB, SIGNAL(clicked(bool)), this, SLOT(clickedRememberHashes(bool)));
    connect(ui.rememberHashesCB, SIGNAL(clicked(bool)), this, SLOT(toggleRememberHashes()));
    connect(ui.autoCheckDirectories_CB, SIGNAL(clicked(bool)), this, SLOT(toggleAutoCheckDirectories(bool)));

	/* Hide platform specific features */
#ifdef Q_WS_WIN

#endif
}

void DirectoriesPage::clearHashCache()
{
	if(QMessageBox::question(this, tr("Cache cleaning confirmation"), tr("This will forget any former hash of non shared files. Do you confirm ?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes)
		rsFiles->clearHashCache() ;
}

void DirectoriesPage::toggleAutoCheckDirectories(bool b)
{
	ui.autoCheckDirectoriesDelay_SB->setEnabled(b);
}

void DirectoriesPage::editDirectories()
{
	ShareManager::showYourself() ;
}

void DirectoriesPage::clickedRememberHashes(bool b)
{
	if (!b) {
		if (QMessageBox::question(this,tr("Cache cleaning confirmation"), tr("This will forget any former hash of non shared files. Do you confirm ?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::No) {
			ui.rememberHashesCB->setChecked(true);
		}
	}
}

void DirectoriesPage::toggleRememberHashes()
{
	bool b = ui.rememberHashesCB->isChecked();
	ui.rememberHashesSB->setEnabled(b);
	ui.cleanHashCachePB->setEnabled(b);
}

/** Saves the changes on this page */
bool DirectoriesPage::save(QString &/*errmsg*/)
{
	rsFiles->setRememberHashFilesDuration(ui.rememberHashesSB->value());
	rsFiles->setWatchPeriod(ui.autoCheckDirectoriesDelay_SB->value());

	std::string dir = ui.incomingDir->text().toUtf8().constData();
	if (!dir.empty())
	{
		rsFiles->setDownloadDirectory(dir);
	}

	dir = ui.partialsDir->text().toUtf8().constData();
	if (!dir.empty())
	{
		rsFiles->setPartialsDirectory(dir);
	}

	if (ui.rememberHashesCB->isChecked()) {
		rsFiles->setRememberHashFiles(true);
	} else {
		rsFiles->setRememberHashFiles(false);
		rsFiles->clearHashCache() ;
	}

	if (ui.autoCheckDirectories_CB->isChecked()) {
		rsFiles->setWatchPeriod(ui.autoCheckDirectoriesDelay_SB->value());
	} else {
		rsFiles->setWatchPeriod(-ui.autoCheckDirectoriesDelay_SB->value());
	}

	rsFiles->shareDownloadDirectory(ui.shareDownloadDirectoryCB->isChecked());

	return true;
}

/** Loads the settings for this page */
void DirectoriesPage::load()
{
	ui.shareDownloadDirectoryCB->setChecked(rsFiles->getShareDownloadDirectory());

	ui.rememberHashesSB->setValue(rsFiles->rememberHashFilesDuration());
	ui.rememberHashesCB->setChecked(rsFiles->rememberHashFiles());
	toggleRememberHashes();

	int u = rsFiles->watchPeriod() ;
	ui.autoCheckDirectoriesDelay_SB->setValue(abs(u)) ;
	ui.autoCheckDirectories_CB->setChecked(u>0) ;
	ui.autoCheckDirectoriesDelay_SB->setEnabled(u>0) ;

	ui.incomingDir->setText(QString::fromUtf8(rsFiles->getDownloadDirectory().c_str()));
	ui.partialsDir->setText(QString::fromUtf8(rsFiles->getPartialsDirectory().c_str()));
}

void DirectoriesPage::setIncomingDirectory()
{
	QString qdir = QFileDialog::getExistingDirectory(this, tr("Set Incoming Directory"), "", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
	if (qdir.isEmpty()) {
		return;
	}

	ui.incomingDir->setText(qdir);
}

void DirectoriesPage::setPartialsDirectory()
{
	QString qdir = QFileDialog::getExistingDirectory(this, tr("Set Partials Directory"), "", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
	if (qdir.isEmpty()) {
		return;
	}

	ui.partialsDir->setText(qdir);
}
