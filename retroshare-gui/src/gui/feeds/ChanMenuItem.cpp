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

#include "rsiface/rschannels.h"
#include "ChanMenuItem.h"

#include <iostream>

/****
 * #define DEBUG_ITEM 1
 ****/

/** Constructor */
ChanMenuItem::ChanMenuItem(std::string chanId)
:QWidget(NULL), mChanId(chanId)
{
  /* Invoke the Qt Designer generated object setup routine */
  setupUi(this);

  /* general ones */
  connect( expandButton, SIGNAL( clicked( void ) ), this, SLOT( toggle ( void ) ) );

  small();
  updateItemStatic();
  updateItem();
  expandButton->setIcon(QIcon(QString(":/images/hide_frame.png")));

}


void ChanMenuItem::updateItemStatic()
{
	/* fill in */
#ifdef DEBUG_ITEM
	std::cerr << "ChanMenuItem::updateItemStatic()";
	std::cerr << std::endl;
#endif

	/* extract details from channels */
	ChannelInfo ci;
	if ((rsChannels) && (rsChannels->getChannelInfo(mChanId, ci)))
	{
		titleLabel->setText(QString::fromStdWString(ci.channelName));
		descLabel->setText(QString::fromStdWString(ci.channelDesc));
		pop_lcd->display((int) ci.pop);
		/* TODO */
		fetches_lcd->display(9999);
		avail_lcd->display(9999);
	}
	else
	{
		titleLabel->setText("Unknown Channel");
		descLabel->setText("No Description");
	}
}


void ChanMenuItem::updateItem()
{
	/* fill in */
#ifdef DEBUG_ITEM
	std::cerr << "ChanMenuItem::updateItem()";
	std::cerr << std::endl;
#endif

}


void ChanMenuItem::small()
{
	expandFrame->hide();
}

void ChanMenuItem::toggle()
{
	if (expandFrame->isHidden())
	{
		expandFrame->show();
	        expandButton->setIcon(QIcon(QString(":/images/expand_frame.png")));
		expandButton->setToolTip("Hide");
	}
	else
	{
		expandFrame->hide();
	        expandButton->setIcon(QIcon(QString(":/images/hide_frame.png")));
		expandButton->setToolTip("Expand");

	}
}

/*********** SPECIFIC FUNCTIOSN ***********************/

void ChanMenuItem::mousePressEvent ( QMouseEvent * event )
{
	selectMe( mChanId );
}


