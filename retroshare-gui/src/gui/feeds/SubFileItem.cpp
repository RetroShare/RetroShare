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
SubFileItem::SubFileItem(std::string hash)
:QWidget(NULL), mFileHash(hash)
{
  /* Invoke the Qt Designer generated object setup routine */
  setupUi(this);

  connect( expandButton, SIGNAL( clicked( void ) ), this, SLOT( toggle ( void ) ) );
  connect( cancelButton, SIGNAL( clicked( void ) ), this, SLOT( cancel ( void ) ) );
  connect( playButton, SIGNAL( clicked( void ) ), this, SLOT( play ( void ) ) );

  amountDone = 1;

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
}

bool SubFileItem::done()
{
	return (amountDone >= 100);
}


void SubFileItem::updateItem()
{
	/* fill in */
#ifdef DEBUG_ITEM
	std::cerr << "SubFileItem::updateItem()";
	std::cerr << std::endl;
#endif
	int msec_rate = 1000;

	if (amountDone < 100)
	{
		amountDone *= 1.1;

		progressBar->setValue(amountDone);
	  	QTimer::singleShot( msec_rate, this, SLOT(updateItem( void ) ));
	}
	else
	{
		/* complete! */
		progressBar->setValue(100);
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


