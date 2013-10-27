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

#include "RsAutoUpdatePage.h"
#include "ui_NetworkView.h"


class NetworkView : public RsAutoUpdatePage 
{
	Q_OBJECT

	public:
		NetworkView(QWidget *parent = 0);
		virtual ~NetworkView();

		virtual void updateDisplay() ; // derived from RsAutoUpdatePage

	public slots:
		void update() ;

	private slots:

		void setMaxFriendLevel(int) ;
		void setEdgeLength(int) ;

		void changedFoFCheckBox( );
		void redraw();

		void setFreezeState(bool);

	private:

		void  clear();

		QGraphicsScene *mScene;

		/** Qt Designer generated object */
		Ui::NetworkView ui;
		uint _max_friend_level ;
		std::map<std::string,GraphWidget::NodeId> _node_ids ;

		bool _should_update ;
};

#endif

