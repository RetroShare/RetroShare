/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2008 Robert Fernie
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
#include <QtGui>

#include "SubFileItem.h"

#include <iostream>

#define DEBUG_ITEM 1

/** Constructor */
SubFileItem::SubFileItem(std::string hash, std::string name, uint64_t size)
:QWidget(NULL), mFileHash(hash), mFileName(name), mFileSize(size)
{
  /* Invoke the Qt Designer generated object setup routine */
  setupUi(this);

  connect( expandButton, SIGNAL( clicked( void ) ), this, SLOT( toggle ( void ) ) );
  connect( cancelButton, SIGNAL( clicked( void ) ), this, SLOT( cancel ( void ) ) );
  connect( playButton, SIGNAL( clicked( void ) ), this, SLOT( play ( void ) ) );

  amountDone = 1000;

  small();
  updateItemStatic();
  updateItem();
}

void SubFileItem::updateItemStatic()
{
	/* fill in */
#ifdef DEBUG_ITEM
	std::cerr << "SubFileItem::updateItemStatic()";
	std::cerr << std::endl;
#endif

	QString filename = "Biggest_File.txt";
	fileLabel->setText(filename);
	fileLabel->setToolTip(filename);

	playButton->setEnabled(false);

	if (mFileSize > 10000000) /* 10 Mb */
	{
		progressBar->setRange(0, mFileSize / 1000000);
		progressBar->setFormat("%v MB");
	}
	else if (mFileSize > 10000) /* 10 Kb */
	{
		progressBar->setRange(0, mFileSize / 1000);
		progressBar->setFormat("%v kB");
	}
	else 
	{
		progressBar->setRange(0, mFileSize);
		progressBar->setFormat("%v B");
	}
}

bool SubFileItem::done()
{
	return (amountDone >= mFileSize);
}


void SubFileItem::updateItem()
{
	/* fill in */
#ifdef DEBUG_ITEM
	std::cerr << "SubFileItem::updateItem()";
	std::cerr << std::endl;
#endif
	int msec_rate = 1000;

	uint64_t divisor = 1;
	if (mFileSize > 10000000) /* 10 Mb */
	{
		divisor = 1000000;
	}
	else if (mFileSize > 10000) /* 10 Kb */
	{
		divisor = 1000;
	}

	if (amountDone < mFileSize)
	{
		amountDone *= 1.1;

		progressBar->setValue(amountDone / divisor);
	  	QTimer::singleShot( msec_rate, this, SLOT(updateItem( void ) ));
	}
	else
	{
		/* complete! */
		amountDone = mFileSize + 1;
		progressBar->setValue(mFileSize / divisor);
		playButton->setEnabled(true);
	}
		
}


void SubFileItem::small()
{
#ifdef DEBUG_ITEM
	std::cerr << "SubFileItem::cancel()";
	std::cerr << std::endl;
#endif

#if 0
	expandFrame->hide();
#endif
}

void SubFileItem::toggle()
{
#if 0
	if (expandFrame->isHidden())
	{
		expandFrame->show();
	}
	else
	{
		expandFrame->hide();
	}
#endif
}

void SubFileItem::cancel()
{
#ifdef DEBUG_ITEM
	std::cerr << "SubFileItem::cancel()";
	std::cerr << std::endl;
#endif
}


void SubFileItem::play()
{
#ifdef DEBUG_ITEM
	std::cerr << "SubFileItem::play()";
	std::cerr << std::endl;
#endif
}


