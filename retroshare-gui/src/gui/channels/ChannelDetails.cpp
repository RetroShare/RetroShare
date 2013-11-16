/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2009 RetroShare Team
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
#include "ChannelDetails.h"
#include "util/DateTime.h"

#include <retroshare/rsiface.h>
#include <retroshare/rspeers.h>
#include <retroshare/rsdisc.h>
#include <retroshare/rschannels.h>

#include <QTime>
#include <QDateTime>

#include <list>
#include <iostream>
#include <string>


/** Default constructor */
ChannelDetails::ChannelDetails(QWidget *parent)
  : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint)
{
  /* Invoke Qt Designer generated QObject setup routine */
  ui.setupUi(this);

  connect(ui.buttonBox, SIGNAL(rejected()), this, SLOT(close()));

  ui.typeEncrypted->setEnabled(false);
  ui.typePrivate->setEnabled(false);
}


/**
 Overloads the default show() slot so we can set opacity*/

void
ChannelDetails::show()
{
  //loadSettings();
  if(!this->isVisible()) {
    QDialog::show();

  }
}

void ChannelDetails::showDetails(std::string mChannelId)
{
	cId = mChannelId;
	loadChannel();
}

void ChannelDetails::loadChannel()
{
	if (!rsChannels)
	{
		return;
	}
    uint32_t flags = 0;	

	ChannelInfo ci;
	rsChannels->getChannelInfo(cId, ci);
	flags = ci.channelFlags;
	
    // Set Channel Name
    ui.nameline->setText(QString::fromStdWString(ci.channelName));

    // Set Channel Popularity
    {
      ui.popline -> setText(QString::number(ci.pop));
    }
	
    // Set Last Channel Post Date 
    if (ci.lastPost) {
        ui.postline->setText(DateTime::formatLongDateTime(ci.lastPost));
    }

    // Set Channel ID
    ui.IDline->setText(QString::fromStdString(ci.channelId));
	
    // Set Channel Description
    ui.DescriptiontextEdit->setText(QString::fromStdWString(ci.channelDesc));
    
    if (flags |= RS_DISTRIB_PRIVATE)
	{
        ui.typeEncrypted->setChecked(false);
        ui.typePrivate->setChecked(true);
	}
	else if (flags |= RS_DISTRIB_ENCRYPTED)
	{		
		ui.typeEncrypted->setChecked(true);
        ui.typePrivate->setChecked(false);
	}

	// Set destination directory
	ui.destinationDirectoryLineEdit->setText(QString::fromUtf8(ci.destination_directory.c_str()));
}
