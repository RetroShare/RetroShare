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

#include <retroshare/rsiface.h>
#include <retroshare/rsfiles.h>
#include <retroshare/rspeers.h>

TransferPage::TransferPage(QWidget * parent, Qt::WindowFlags flags)
    : ConfigPage(parent, flags)
{
  /* Invoke the Qt Designer generated object setup routine */
  ui.setupUi(this);

	ui._queueSize_SB->setValue(rsFiles->getQueueSize()) ;
	ui._minPrioritized_SB->setValue(rsFiles->getMinPrioritizedTransfers()) ;

	switch(rsFiles->defaultChunkStrategy())
	{
		case FileChunksInfo::CHUNK_STRATEGY_STREAMING: ui._defaultStrategy_CB->setCurrentIndex(0) ; break ;
		case FileChunksInfo::CHUNK_STRATEGY_PROGRESSIVE: ui._defaultStrategy_CB->setCurrentIndex(1) ; break ;
		case FileChunksInfo::CHUNK_STRATEGY_RANDOM: ui._defaultStrategy_CB->setCurrentIndex(2) ; break ;
	}

	ui._diskSpaceLimit_SB->setValue(rsFiles->freeDiskSpaceLimit()) ;

	QObject::connect(ui._queueSize_SB,SIGNAL(valueChanged(int)),this,SLOT(updateQueueSize(int))) ;
	QObject::connect(ui._minPrioritized_SB,SIGNAL(valueChanged(int)),this,SLOT(updateMinPrioritized(int))) ;
	QObject::connect(ui._defaultStrategy_CB,SIGNAL(activated(int)),this,SLOT(updateDefaultStrategy(int))) ;
	QObject::connect(ui._diskSpaceLimit_SB,SIGNAL(valueChanged(int)),this,SLOT(updateDiskSizeLimit(int))) ;

  /* Hide platform specific features */
#ifdef Q_WS_WIN

#endif
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

void TransferPage::updateMinPrioritized(int s)
{
	rsFiles->setMinPrioritizedTransfers(s) ;
}
void TransferPage::updateQueueSize(int s)
{
	if(ui._minPrioritized_SB->value() > s)
	{
		ui._minPrioritized_SB->setValue(s) ;
	}
	rsFiles->setQueueSize(s) ;
}
