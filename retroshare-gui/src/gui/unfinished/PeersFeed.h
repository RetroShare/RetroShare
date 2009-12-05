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

#ifndef _PEERS_FEED_DIALOG_H
#define _PEERS_FEED_DIALOG_H

#include "gui/mainpage.h"
#include "ui_PeersFeed.h"

#include "gui/feeds/FeedHolder.h"

class PeerItem;

class PeersFeed : public MainPage, public FeedHolder, private Ui::PeersFeed
{
  Q_OBJECT

public:
  /** Default Constructor */
  PeersFeed(QWidget *parent = 0);
  /** Default Destructor */

	/* FeedHolder Functions (for FeedItem functionality) */
virtual void deleteFeedItem(QWidget *item, uint32_t type);
virtual void openChat(std::string peerId);
virtual void openMsg(uint32_t type, std::string grpId, std::string inReplyTo);

private slots:
 
	void updatePeers();
	void updateMode();

private:

  QLayout *mLayout;

  /* lists of feedItems */
  uint32_t				mMode;
  std::map<std::string, PeerItem *> 	mPeers;

};



#endif


