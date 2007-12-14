/****************************************************************
*  RetroShare is distributed under the following license:
*
*  Copyright (C) 2006, 2007 The RetroShare Team
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

#include "channelsDialog.h"

ChannelsDialog::ChannelsDialog(QWidget * parent) : MainPage (parent) 
{
    setupUi(this);
    QGridLayout* grid = new QGridLayout(this);

    // setup the tabs
    channelPages = new QTabWidget(this);
    
    // add the tabs to the channels dialog 
    channelBrowserDialog = new ChannelBrowserDialog();
    channelBrowserDialog->setObjectName(QString::fromUtf8("browserTab"));
    channelPages->addTab(channelBrowserDialog,
 	   		 QIcon(QString::fromUtf8(":/images/channels.png")),
     			 tr("Browse Channels"));
     
    mySubscriptionsDialog = new MySubscriptionsDialog();
    mySubscriptionsDialog->setObjectName(QString::fromUtf8("mySubsTab"));
    channelPages->addTab(mySubscriptionsDialog,
 	   		 QIcon(QString::fromUtf8(":/images/folder-inbox.png")),
     			 tr("My Subscriptions"));

     myChannelsDialog = new MyChannelsDialog();
     myChannelsDialog->setObjectName(QString::fromUtf8("myChannelsTab"));
     channelPages->addTab(myChannelsDialog,
                        QIcon(QString::fromUtf8(":/images/folder-outbox.png")),
     			tr("My Channels"));
    
    grid->addWidget(channelPages, 0, 0, 1, 1);

    channelPages->setCurrentIndex(0);

}







void ChannelsDialog::messageslistWidgetCostumPopupMenu( QPoint point )
{
}


void ChannelsDialog::msgfilelistWidgetCostumPopupMenu( QPoint point )
{
}

void ChannelsDialog::newmessage()
{
}

void ChannelsDialog::newchannel()
{
}

void ChannelsDialog::subscribechannel()
{
	/* more work */

}

void ChannelsDialog::unsubscribechannel()
{
	/* more work */

}


void ChannelsDialog::deletechannel()
{
	/* more work */

}


void ChannelsDialog::getcurrentrecommended()
{
   

}


void ChannelsDialog::getallrecommended()
{
   

}

void ChannelsDialog::insertChannels()
{
}

void ChannelsDialog::updateChannels( QTreeWidgetItem * item, int column )
{

}


void ChannelsDialog::insertMsgTxtAndFiles()
{

}
