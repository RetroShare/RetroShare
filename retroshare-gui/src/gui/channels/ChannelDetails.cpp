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

#include <retroshare/rsiface.h>
#include <retroshare/rspeers.h>
#include <retroshare/rsdisc.h>
#include <retroshare/rschannels.h>

#include <QTime>
#include <QDateTime>

#include <sstream>
#include <list>
#include <iostream>
#include <string>


/* Define the format used for displaying the date and time */
#define DATETIME_FMT  "MMM dd hh:mm:ss"

/** Default constructor */
ChannelDetails::ChannelDetails(QWidget *parent, Qt::WFlags flags)
  : QDialog(parent, flags)
{
  /* Invoke Qt Designer generated QObject setup routine */
  ui.setupUi(this);

  connect(ui.applyButton, SIGNAL(clicked()), this, SLOT(applyDialog()));
  connect(ui.cancelButton, SIGNAL(clicked()), this, SLOT(closeinfodlg()));

  ui.applyButton->setToolTip(tr("Close"));
  
  ui.nameline ->setReadOnly(true);
  ui.popline ->setReadOnly(true);
  ui.postline ->setReadOnly(true);
  ui.IDline ->setReadOnly(true);
  ui.DescriptiontextEdit ->setReadOnly(true);
  

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

void ChannelDetails::closeEvent (QCloseEvent * event)
{
 QWidget::closeEvent(event);
}

void ChannelDetails::closeinfodlg()
{
	close();
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

	std::list<ChannelInfo> channelList;
	std::list<ChannelInfo>::iterator it;
	
	ChannelInfo ci;
	rsChannels->getChannelInfo(cId, ci);

	rsChannels->getChannelList(channelList);
	
	
	for(it = channelList.begin(); it != channelList.end(); it++)
	{
	
    // Set Channel Name
    ui.nameline->setText(QString::fromStdWString(ci.channelName));

    // Set Channel Popularity
    {
      std::ostringstream out;
      out << it->pop;
      ui.popline -> setText(QString::fromStdString(out.str()));
    }
	
    // Set Last Channel Post Date 
    {
      QDateTime qtime;
      qtime.setTime_t(it->lastPost);
      QString timestamp = qtime.toString("yyyy-MM-dd hh:mm:ss");
      ui.postline -> setText(timestamp);
    }

    // Set Channel ID
    ui.IDline->setText(QString::fromStdString(ci.channelId));
	
    // Set Channel Description
    ui.DescriptiontextEdit->setText(QString::fromStdWString(ci.channelDesc));
        

	
	}

}

void ChannelDetails::applyDialog()
{

	/* reload now */
	loadChannel();

	/* close the Dialog after the Changes applied */
	closeinfodlg();

}



