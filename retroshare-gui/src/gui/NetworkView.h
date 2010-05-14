/****************************************************************
 *  RetroShare GUI is distributed under the following license:
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

#ifndef _NETWORK_VIEW_H
#define _NETWORK_VIEW_H

#include <QGraphicsScene>

#include "mainpage.h"
#include "ui_NetworkView.h"


class NetworkView : public MainPage 
{
  Q_OBJECT

public:
	NetworkView(QWidget *parent = 0);


private slots:

void insertPeers();
void insertSignatures();
void insertConnections();

void changedScene();

void changedFoFCheckBox( );
void changedDrawSignatures( );
void changedDrawFriends( );

/** Called when Settings button is toggled */
void shownwSettingsFrame(bool show);

private:

	void  clearPeerItems();
	void  clearOtherItems();
	void  clearLineItems();

	QGraphicsScene *mScene;

        std::map<std::string, QGraphicsItem *> mPeerItems;
        std::list<QGraphicsItem *> mOtherItems;

        std::list<QGraphicsItem *> mLineItems;
	bool mLineChanged;


	/** Qt Designer generated object */
	Ui::NetworkView ui;
};

#endif

