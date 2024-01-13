/*******************************************************************************
 * gui/settings/TransferPage.cpp                                               *
 *                                                                             *
 * Copyright (c) 2010 Retroshare Team <retroshare.project@gmail.com>           *
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

#include "TransferPage.h"

#include "rshare.h"
#include "gui/ShareManager.h"
#include "gui/settings/rsharesettings.h"
#include "util/misc.h"

#include "retroshare/rsiface.h"
#include "retroshare/rsfiles.h"
#include "retroshare/rspeers.h"

#include <QCheckBox>
#include <QMessageBox>
#include <QToolTip>

#include <iostream>

TransferPage::TransferPage(QWidget * parent, Qt::WindowFlags flags)
    : ConfigPage(parent, flags)
{
    /* Invoke the Qt Designer generated object setup routine */
    ui.setupUi(this);

    int max_tr_low,max_tr_high;
    rsTurtle->getMaxTRForwardRateLimits(max_tr_low,max_tr_high);

    ui._max_tr_up_per_sec_SB->setMinimum(max_tr_low);
    ui._max_tr_up_per_sec_SB->setMaximum(max_tr_high);

    whileBlocking(ui._trustFriendNodesWithBannedFiles_CB)->setChecked(rsFiles->trustFriendNodesWithBannedFiles());

    QObject::connect(ui._queueSize_SB,SIGNAL(valueChanged(int)),this,SLOT(updateQueueSize(int))) ;
    QObject::connect(ui._max_up_SB,SIGNAL(valueChanged(int)),this,SLOT(updateMaxUploadSlots(int))) ;
    QObject::connect(ui._defaultStrategy_CB,SIGNAL(activated(int)),this,SLOT(updateDefaultStrategy(int))) ;
    QObject::connect(ui._e2e_encryption_CB,SIGNAL(activated(int)),this,SLOT(updateEncryptionPolicy(int))) ;
    QObject::connect(ui._diskSpaceLimit_SB,SIGNAL(valueChanged(int)),this,SLOT(updateDiskSizeLimit(int))) ;
    QObject::connect(ui._max_tr_up_per_sec_SB, SIGNAL( valueChanged( int ) ), this, SLOT( updateMaxTRUpRate(int) ) );
	QObject::connect(ui._filePermDirectDL_CB,SIGNAL(activated(int)),this,SLOT(updateFilePermDirectDL(int)));
    QObject::connect(ui._trustFriendNodesWithBannedFiles_CB,SIGNAL(toggled(bool)),this,SLOT(toggleTrustFriendNodesWithBannedFiles(bool))) ;

	QObject::connect(ui.incomingButton, SIGNAL(clicked( bool ) ), this , SLOT( setIncomingDirectory() ) );
	QObject::connect(ui.autoDLColl_CB, SIGNAL(toggled(bool)), this, SLOT(updateAutoDLColl()));
	QObject::connect(ui.partialButton, SIGNAL(clicked( bool ) ), this , SLOT( setPartialsDirectory() ) );
	QObject::connect(ui.editShareButton, SIGNAL(clicked()), this, SLOT(editDirectories()));
	QObject::connect(ui.autoCheckDirectories_CB, SIGNAL(clicked(bool)), this, SLOT(toggleAutoCheckDirectories(bool)));
	QObject::connect(ui.minimumFontSize_SB, SIGNAL(valueChanged(int)), this, SLOT(updateFontSize())) ;

	QObject::connect(ui.autoCheckDirectories_CB,     SIGNAL(toggled(bool)),    this,SLOT(updateAutoCheckDirectories())) ;
	QObject::connect(ui.autoCheckDirectoriesDelay_SB,SIGNAL(valueChanged(int)),this,SLOT(updateAutoScanDirectoriesPeriod())) ;
	QObject::connect(ui.shareDownloadDirectoryCB,    SIGNAL(toggled(bool)),    this,SLOT(updateShareDownloadDirectory())) ;
	QObject::connect(ui.followSymLinks_CB,           SIGNAL(toggled(bool)),    this,SLOT(updateFollowSymLinks())) ;

	QObject::connect(ui.prefixesIgnoreList_LE,       SIGNAL(editingFinished()),	this,SLOT(updateIgnoreLists())) ;
	QObject::connect(ui.prefixesIgnoreList_CB,       SIGNAL(toggled(bool)),    		this,SLOT(updateIgnoreLists())) ;
	QObject::connect(ui.suffixesIgnoreList_LE,       SIGNAL(editingFinished()),  this,SLOT(updateIgnoreLists())) ;
	QObject::connect(ui.suffixesIgnoreList_CB,       SIGNAL(toggled(bool)),    		this,SLOT(updateIgnoreLists())) ;
	QObject::connect(ui.ignoreDuplicates_CB,         SIGNAL(toggled(bool)),    		this,SLOT(updateIgnoreDuplicates())) ;
	QObject::connect(ui.maxDepth_SB,                 SIGNAL(valueChanged(int)),		this,SLOT(updateMaxShareDepth(int))) ;
}

void TransferPage::updateIgnoreLists()
{
	uint32_t flags = 0 ;
	if(ui.prefixesIgnoreList_CB->isChecked()) flags |= RS_FILE_SHARE_FLAGS_IGNORE_PREFIXES ;
	if(ui.suffixesIgnoreList_CB->isChecked()) flags |= RS_FILE_SHARE_FLAGS_IGNORE_SUFFIXES ;

	std::list<std::string> lp,ls ;
	{ QStringList L = ui.prefixesIgnoreList_LE->text().split(';') ; for(QStringList::const_iterator it(L.begin());it!=L.end();++it) if(!(*it).isNull()) lp.push_back((*it).toStdString()) ; }
	{ QStringList L = ui.suffixesIgnoreList_LE->text().split(';') ; for(QStringList::const_iterator it(L.begin());it!=L.end();++it) if(!(*it).isNull()) ls.push_back((*it).toStdString()) ; }

	rsFiles->setIgnoreLists(lp,ls,flags) ;

#ifdef DEBUG_TRANSFERS_PAGE
	std::cerr << "Setting ignore lists: " << std::endl;

	std::cerr << "  flags: " << flags << std::endl;
	std::cerr << "  prefixes: " ; for(auto it(lp.begin());it!=lp.end();++it) std::cerr << "\"" << *it << "\" " ; std::cerr << std::endl;
	std::cerr << "  suffixes: " ; for(auto it(ls.begin());it!=ls.end();++it) std::cerr << "\"" << *it << "\" " ; std::cerr << std::endl;
#endif
}
void TransferPage::toggleTrustFriendNodesWithBannedFiles(bool b)
{
    rsFiles->setTrustFriendNodesWithBannedFiles(b);
}
void TransferPage::updateMaxTRUpRate(int b)
{
    rsTurtle->setMaxTRForwardRate(b) ;
}

void TransferPage::updateMaxUploadSlots(int b)
{
    rsFiles->setMaxUploadSlotsPerFriend(b) ;
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

void TransferPage::updateFilePermDirectDL(int i)
{
	switch (i)
	{
		case 0:  rsFiles->setFilePermDirectDL(RS_FILE_PERM_DIRECT_DL_YES);       break;
		case 1:  rsFiles->setFilePermDirectDL(RS_FILE_PERM_DIRECT_DL_NO);        break;
		default: rsFiles->setFilePermDirectDL(RS_FILE_PERM_DIRECT_DL_PER_USER);  break;
	}
}

void TransferPage::load()
{
	ui.ignoreDuplicates_CB->setEnabled(rsFiles->followSymLinks()) ;

    whileBlocking(ui.shareDownloadDirectoryCB)->setChecked(rsFiles->getShareDownloadDirectory());

	int u = rsFiles->watchPeriod() ;
    whileBlocking(ui.autoCheckDirectoriesDelay_SB)->setValue(u) ;
    whileBlocking(ui.autoCheckDirectories_CB)->setChecked(rsFiles->watchEnabled()) ; ;

	whileBlocking(ui.incomingDir)->setText(QString::fromUtf8(rsFiles->getDownloadDirectory().c_str()));
	whileBlocking(ui.autoDLColl_CB)->setChecked(Settings->valueFromGroup("Transfer", "AutoDLColl", false).toBool());
	whileBlocking(ui.partialsDir)->setText(QString::fromUtf8(rsFiles->getPartialsDirectory().c_str()));
	whileBlocking(ui.followSymLinks_CB)->setChecked(rsFiles->followSymLinks());
	whileBlocking(ui.ignoreDuplicates_CB)->setChecked(rsFiles->ignoreDuplicates());
	whileBlocking(ui.maxDepth_SB)->setValue(rsFiles->maxShareDepth());

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
    whileBlocking(ui._max_up_SB)->setValue(rsFiles->getMaxUploadSlotsPerFriend()) ;

		switch (rsFiles->filePermDirectDL())
		{
			case RS_FILE_PERM_DIRECT_DL_YES: whileBlocking(ui._filePermDirectDL_CB)->setCurrentIndex(0) ; break ;
			case RS_FILE_PERM_DIRECT_DL_NO:  whileBlocking(ui._filePermDirectDL_CB)->setCurrentIndex(1) ; break ;
			default:                         whileBlocking(ui._filePermDirectDL_CB)->setCurrentIndex(2) ; break ;
		}

	std::list<std::string> suffixes, prefixes;
	uint32_t ignore_flags ;

	rsFiles->getIgnoreLists(prefixes,suffixes,ignore_flags) ;

	QString ignore_prefixes_string,ignore_suffixes_string ;

	for(auto it(prefixes.begin());it!=prefixes.end();)
	{
		ignore_prefixes_string += QString::fromStdString(*it) ;

		if(++it != prefixes.end())
			ignore_prefixes_string += ";" ;
	}
	for(auto it(suffixes.begin());it!=suffixes.end();)
	{
		ignore_suffixes_string += QString::fromStdString(*it) ;

		if(++it != suffixes.end())
			ignore_suffixes_string += ";" ;
	}
	whileBlocking(ui.prefixesIgnoreList_CB)->setChecked( ignore_flags & RS_FILE_SHARE_FLAGS_IGNORE_PREFIXES ) ;
	whileBlocking(ui.suffixesIgnoreList_CB)->setChecked( ignore_flags & RS_FILE_SHARE_FLAGS_IGNORE_SUFFIXES ) ;
	whileBlocking(ui.prefixesIgnoreList_LE)->setText( ignore_prefixes_string );
	whileBlocking(ui.suffixesIgnoreList_LE)->setText( ignore_suffixes_string );

	Settings->beginGroup(QString("File"));
#if defined(Q_OS_DARWIN)
	whileBlocking(ui.minimumFontSize_SB)->setValue( Settings->value("MinimumFontSize", 13 ).toInt());
#else
	whileBlocking(ui.minimumFontSize_SB)->setValue( Settings->value("MinimumFontSize", 11 ).toInt());
#endif
	Settings->endGroup();
}

void TransferPage::updateDefaultStrategy(int i)
{
	switch(i)
	{
		case 0: rsFiles->setDefaultChunkStrategy(FileChunksInfo::CHUNK_STRATEGY_STREAMING) ;
				  break ;

        case 2:
#ifdef WINDOWS_SYS
                if(QMessageBox::Yes != QMessageBox::warning(nullptr,tr("Warning"),tr("On Windows systems, randomly writing in the middle of large empty files may hang the software for several seconds. Do you want to use this option anyway (otherwise use \"progressive\")?"),QMessageBox::Yes,QMessageBox::No))
                {
                    ui._defaultStrategy_CB->setCurrentIndex(1);
                    return;
                }
#endif
                rsFiles->setDefaultChunkStrategy(FileChunksInfo::CHUNK_STRATEGY_RANDOM) ;
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
void TransferPage::updateIgnoreDuplicates()
{
	rsFiles->setIgnoreDuplicates(ui.ignoreDuplicates_CB->isChecked());
}
void TransferPage::updateMaxShareDepth(int s)
{
	rsFiles->setMaxShareDepth(s) ;
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

	std::string dir = qdir.toUtf8().constData();
	if(!dir.empty())
	{
		if (!rsFiles->setDownloadDirectory(dir))
		{
			ui.incomingDir->setToolTip( tr("Invalid Input. Have you got the right to write on it?") );
			ui.incomingDir->setProperty("WrongValue", true);
		}
		else
		{
			ui.incomingDir->setToolTip( "" );
			ui.incomingDir->setProperty("WrongValue", false);
		}
	}
	ui.incomingDir->style()->unpolish(ui.incomingDir);
	ui.incomingDir->style()->polish(  ui.incomingDir);
	whileBlocking(ui.incomingDir)->setText(QString::fromUtf8(rsFiles->getDownloadDirectory().c_str()));
}

void TransferPage::updateAutoDLColl()
{
	Settings->setValueToGroup("Transfer", "AutoDLColl", ui.autoDLColl_CB->isChecked());
}

void TransferPage::setPartialsDirectory()
{
	QString qdir = QFileDialog::getExistingDirectory(this, tr("Set Partials Directory"), "", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
	if (qdir.isEmpty()) {
		return;
	}

	std::string	dir = qdir.toUtf8().constData();
	if (!dir.empty())
	{
		if (!rsFiles->setPartialsDirectory(dir))
		{
			ui.partialsDir->setToolTip( tr("Invalid Input. It can't be an already shared directory.") );
			ui.partialsDir->setProperty("WrongValue", true);
		}
		else
		{
			ui.partialsDir->setToolTip( "" );
			ui.partialsDir->setProperty("WrongValue", false);
		}
	}
	ui.partialsDir->style()->unpolish(ui.partialsDir);
	ui.partialsDir->style()->polish(  ui.partialsDir);
	whileBlocking(ui.partialsDir)->setText(QString::fromUtf8(rsFiles->getPartialsDirectory().c_str()));
}

void TransferPage::toggleAutoCheckDirectories(bool b)
{
	ui.autoCheckDirectoriesDelay_SB->setEnabled(b);
}

void TransferPage::editDirectories()
{
	ShareManager::showYourself() ;
}

void TransferPage::updateFontSize()
{
	Settings->beginGroup(QString("File"));
	Settings->setValue("MinimumFontSize", ui.minimumFontSize_SB->value());
	Settings->endGroup();
}

void TransferPage::updateAutoCheckDirectories()       {    rsFiles->setWatchEnabled(ui.autoCheckDirectories_CB->isChecked()) ; }
void TransferPage::updateAutoScanDirectoriesPeriod()  {    rsFiles->setWatchPeriod(ui.autoCheckDirectoriesDelay_SB->value()); }
void TransferPage::updateShareDownloadDirectory()     {    rsFiles->shareDownloadDirectory(ui.shareDownloadDirectoryCB->isChecked());}
void TransferPage::updateFollowSymLinks()             {    rsFiles->setFollowSymLinks(ui.followSymLinks_CB->isChecked()); ui.ignoreDuplicates_CB->setEnabled(ui.followSymLinks_CB->isChecked());}
