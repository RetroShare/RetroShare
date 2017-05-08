/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006 - 2010 RetroShare Team
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

#include "TransferPage.h"

#include "rshare.h"

#include <iostream>

#include <util/misc.h>
#include <gui/ShareManager.h>
#include <retroshare/rsiface.h>
#include <retroshare/rsfiles.h>
#include <retroshare/rspeers.h>

TransferPage::TransferPage(QWidget * parent, Qt::WindowFlags flags)
    : ConfigPage(parent, flags)
{
    /* Invoke the Qt Designer generated object setup routine */
    ui.setupUi(this);

    QObject::connect(ui._queueSize_SB,SIGNAL(valueChanged(int)),this,SLOT(updateQueueSize(int))) ;
    QObject::connect(ui._defaultStrategy_CB,SIGNAL(activated(int)),this,SLOT(updateDefaultStrategy(int))) ;
    QObject::connect(ui._e2e_encryption_CB,SIGNAL(activated(int)),this,SLOT(updateEncryptionPolicy(int))) ;
    QObject::connect(ui._diskSpaceLimit_SB,SIGNAL(valueChanged(int)),this,SLOT(updateDiskSizeLimit(int))) ;
    QObject::connect(ui._max_tr_up_per_sec_SB, SIGNAL( valueChanged( int ) ), this, SLOT( updateMaxTRUpRate(int) ) );

	QObject::connect(ui.incomingButton, SIGNAL(clicked( bool ) ), this , SLOT( setIncomingDirectory() ) );
	QObject::connect(ui.partialButton, SIGNAL(clicked( bool ) ), this , SLOT( setPartialsDirectory() ) );
	QObject::connect(ui.editShareButton, SIGNAL(clicked()), this, SLOT(editDirectories()));
	QObject::connect(ui.autoCheckDirectories_CB, SIGNAL(clicked(bool)), this, SLOT(toggleAutoCheckDirectories(bool)));

	QObject::connect(ui.autoCheckDirectories_CB,     SIGNAL(toggled(bool)),    this,SLOT(updateAutoCheckDirectories())) ;
	QObject::connect(ui.autoCheckDirectoriesDelay_SB,SIGNAL(valueChanged(int)),this,SLOT(updateAutoScanDirectoriesPeriod())) ;
	QObject::connect(ui.shareDownloadDirectoryCB,    SIGNAL(toggled(bool)),    this,SLOT(updateShareDownloadDirectory())) ;
	QObject::connect(ui.followSymLinks_CB,           SIGNAL(toggled(bool)),    this,SLOT(updateFollowSymLinks())) ;
}

void TransferPage::updateMaxTRUpRate(int b)
{
    rsTurtle->setMaxTRForwardRate(b) ;
}

void TransferPage::updateEncryptionPolicy(int b)
{
    switch(b)
    {
    case 1: rsFiles->setDefaultEncryptionPolicy(RS_FILE_CTRL_ENCRYPTION_POLICY_STRICT) ;
        break ;
    default:
    case 0: rsFiles->setDefaultEncryptionPolicy(RS_FILE_CTRL_ENCRYPTION_POLICY_PERMISSIVE) ;
        break ;
    }
}

void TransferPage::load()
{
    whileBlocking(ui.shareDownloadDirectoryCB)->setChecked(rsFiles->getShareDownloadDirectory());

	int u = rsFiles->watchPeriod() ;
    whileBlocking(ui.autoCheckDirectoriesDelay_SB)->setValue(u) ;
    whileBlocking(ui.autoCheckDirectories_CB)->setChecked(rsFiles->watchEnabled()) ; ;

	whileBlocking(ui.incomingDir)->setText(QString::fromUtf8(rsFiles->getDownloadDirectory().c_str()));
	whileBlocking(ui.partialsDir)->setText(QString::fromUtf8(rsFiles->getPartialsDirectory().c_str()));
	whileBlocking(ui.followSymLinks_CB)->setChecked(rsFiles->followSymLinks());

	whileBlocking(ui._queueSize_SB)->setValue(rsFiles->getQueueSize()) ;

    switch(rsFiles->defaultChunkStrategy())
    {
    case FileChunksInfo::CHUNK_STRATEGY_STREAMING: whileBlocking(ui._defaultStrategy_CB)->setCurrentIndex(0) ; break ;
    case FileChunksInfo::CHUNK_STRATEGY_PROGRESSIVE: whileBlocking(ui._defaultStrategy_CB)->setCurrentIndex(1) ; break ;
    case FileChunksInfo::CHUNK_STRATEGY_RANDOM: whileBlocking(ui._defaultStrategy_CB)->setCurrentIndex(2) ; break ;
    }

    switch(rsFiles->defaultEncryptionPolicy())
    {
    case RS_FILE_CTRL_ENCRYPTION_POLICY_PERMISSIVE: whileBlocking(ui._e2e_encryption_CB)->setCurrentIndex(0) ; break ;
    case RS_FILE_CTRL_ENCRYPTION_POLICY_STRICT    : whileBlocking(ui._e2e_encryption_CB)->setCurrentIndex(1) ; break ;
    }

    whileBlocking(ui._diskSpaceLimit_SB)->setValue(rsFiles->freeDiskSpaceLimit()) ;
    whileBlocking(ui._max_tr_up_per_sec_SB)->setValue(rsTurtle->getMaxTRForwardRate()) ;
}

void TransferPage::updateDefaultStrategy(int i)
{
	switch(i)
	{
		case 0: rsFiles->setDefaultChunkStrategy(FileChunksInfo::CHUNK_STRATEGY_STREAMING) ;
				  break ;

		case 2: rsFiles->setDefaultChunkStrategy(FileChunksInfo::CHUNK_STRATEGY_RANDOM) ;
				  break ;

		case 1: rsFiles->setDefaultChunkStrategy(FileChunksInfo::CHUNK_STRATEGY_PROGRESSIVE) ;
				  break ;
		default: ;
	}
}

void TransferPage::updateDiskSizeLimit(int s)
{
	rsFiles->setFreeDiskSpaceLimit(s) ;
}

void TransferPage::updateQueueSize(int s)
{
	rsFiles->setQueueSize(s) ;
}
void TransferPage::setIncomingDirectory()
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

void TransferPage::setPartialsDirectory()
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
void TransferPage::toggleAutoCheckDirectories(bool b)
{
	ui.autoCheckDirectoriesDelay_SB->setEnabled(b);
}

void TransferPage::editDirectories()
{
	ShareManager::showYourself() ;
}

void TransferPage::updateAutoCheckDirectories()       {    rsFiles->setWatchEnabled(ui.autoCheckDirectories_CB->isChecked()) ; }
void TransferPage::updateAutoScanDirectoriesPeriod()  {    rsFiles->setWatchPeriod(ui.autoCheckDirectoriesDelay_SB->value()); }
void TransferPage::updateShareDownloadDirectory()     {    rsFiles->shareDownloadDirectory(ui.shareDownloadDirectoryCB->isChecked());}
void TransferPage::updateFollowSymLinks()             {    rsFiles->setFollowSymLinks(ui.followSymLinks_CB->isChecked()); }
