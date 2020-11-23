/*******************************************************************************
 * gui/NetworkView.h                                                           *
 *                                                                             *
 * Copyright (c) 2008 Robert Fernie    <retroshare.project@gmail.com>          *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#ifndef _NETWORK_VIEW_H
#define _NETWORK_VIEW_H

#include <QGraphicsScene>

#include <retroshare/rstypes.h>

#include <retroshare-gui/RsAutoUpdatePage.h>
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
		void setNameSearch(QString) ;

		void changedFoFCheckBox( );
		void redraw();

		void setFreezeState(bool);

	private:

		void  clear();

		QGraphicsScene *mScene;

		/** Qt Designer generated object */
		Ui::NetworkView ui;
		uint _max_friend_level ;
        std::map<RsPgpId,GraphWidget::NodeId> _node_ids ;

		bool _should_update ;
};

#endif

