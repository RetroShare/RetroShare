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

#include "ChannelFeed.h"
#include "gui/feeds/ChanGroupItem.h"
#include "gui/feeds/ChanMenuItem.h"
#include "gui/feeds/ChanMsgItem.h"

/** Constructor */
ChannelFeed::ChannelFeed(QWidget *parent)
: MainPage (parent)
{
  /* Invoke the Qt Designer generated object setup routine */
  setupUi(this);

	/* add dynamic widgets in */

	/* add layout */

	/* add form */

	QVBoxLayout *layout = new QVBoxLayout;

	ChanGroupItem *cg1 = new ChanGroupItem("Own Channels");
	ChanGroupItem *cg2 = new ChanGroupItem("Subscribed Channels");
	ChanGroupItem *cg3 = new ChanGroupItem("Popular Channels");
	ChanGroupItem *cg4 = new ChanGroupItem("Other Channels");

	ChanMenuItem *cm1 = new ChanMenuItem("Channel with long name");
	ChanMenuItem *cm2 = new ChanMenuItem("Channel with very very very very long name");
	ChanMenuItem *cm3 = new ChanMenuItem("Channel with long name");
	ChanMenuItem *cm4 = new ChanMenuItem("Retroshare Releases");
	ChanMenuItem *cm5 = new ChanMenuItem("Popular Channel");

	layout->addWidget(cg1);
	layout->addWidget(cm1);

	layout->addWidget(cg2);
	layout->addWidget(cm2);
	layout->addWidget(cm3);
	layout->addWidget(cm4);

	layout->addWidget(cg3);
	layout->addWidget(cm5);

	layout->addWidget(cg4);

	QWidget *middleWidget = new QWidget();
	//middleWidget->setSizePolicy( QSizePolicy::Policy::Maximum, QSizePolicy::Policy::Minimum);
	middleWidget->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Minimum);
	middleWidget->setLayout(layout);


     	QScrollArea *scrollArea = new QScrollArea;
        scrollArea->setBackgroundRole(QPalette::Dark);
	scrollArea->setWidget(middleWidget);
	scrollArea->setWidgetResizable(true);
	scrollArea->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );

	QVBoxLayout *layout2 = new QVBoxLayout;
	layout2->addWidget(scrollArea);
	
     	chanFrame->setLayout(layout2);

	/***** SECOND HALF *****/

	QVBoxLayout *msgLayout = new QVBoxLayout;

	ChanMsgItem *ni1 = new ChanMsgItem(NULL, 0, "JEZ", "MSGID:47654765476", true);
	ChanMsgItem *ni2 = new ChanMsgItem(NULL, 0, "JEZ", "MSGID:47654765476", true);
	ChanMsgItem *ni3 = new ChanMsgItem(NULL, 0, "JEZ", "MSGID:47654765476", true);
	ChanMsgItem *ni4 = new ChanMsgItem(NULL, 0, "JEZ", "MSGID:47654765476", true);
	ChanMsgItem *ni5 = new ChanMsgItem(NULL, 0, "JEZ", "MSGID:47654765476", true);
	ChanMsgItem *ni6 = new ChanMsgItem(NULL, 0, "JEZ", "MSGID:47654765476", true);
	ChanMsgItem *ni7 = new ChanMsgItem(NULL, 0, "JEZ", "MSGID:47654765476", true);
	ChanMsgItem *ni8 = new ChanMsgItem(NULL, 0, "JEZ", "MSGID:47654765476", true);

	msgLayout->addWidget(ni1);
	msgLayout->addWidget(ni2);
	msgLayout->addWidget(ni3);
	msgLayout->addWidget(ni4);
	msgLayout->addWidget(ni5);
	msgLayout->addWidget(ni6);
	msgLayout->addWidget(ni7);
	msgLayout->addWidget(ni8);

	QWidget *middleWidget2 = new QWidget();
	middleWidget2->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Minimum);
	middleWidget2->setLayout(msgLayout);


     	QScrollArea *scrollArea2 = new QScrollArea;
        scrollArea2->setBackgroundRole(QPalette::Dark);
	scrollArea2->setWidget(middleWidget2);
	scrollArea2->setWidgetResizable(true);
	scrollArea2->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );

	QVBoxLayout *layout3 = new QVBoxLayout;
	layout3->addWidget(scrollArea2);

     	msgFrame->setLayout(layout3);
}



void ChannelFeed::deleteFeedItem(QWidget *item, uint32_t type)
{
	return;
}

void ChannelFeed::openChat(std::string peerId)
{
	return;
}

void ChannelFeed::openMsg(uint32_t type, std::string grpId, std::string inReplyTo)
{
	return;
}


