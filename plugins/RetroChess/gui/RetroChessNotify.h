/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2015
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

// This class is a Qt object to get notification from the plugin's service threads,
// and responsible to pass the info the the GUI part.
//
// Because the GUI part is async-ed with the service, it is crucial to use the
// QObject connect system to communicate between the p3Service and the gui part (handled by Qt)
//
#ifndef NETEXAMPLENOTIFY_H
#define NETEXAMPLENOTIFY_H

#include <retroshare/rstypes.h>

#include <QObject>

class RetroChessNotify : public QObject
{
	Q_OBJECT
public:
	explicit RetroChessNotify(QObject *parent = 0);
	void notifyReceivedPaint(const RsPeerId &peer_id, int x, int y) ;
	void notifyReceivedMsg(const RsPeerId &peer_id, QString str) ;
	void notifyChessStart(const RsPeerId &peer_id) ;
	void notifyChessInvite(const RsPeerId &peer_id) ;

signals:
	void NeMsgArrived(const RsPeerId &peer_id, QString str) ; // emitted when the peer gets a msg

	void chessStart(const RsPeerId &peer_id) ;
	void chessInvited(const RsPeerId &peer_id) ;

public slots:
};

#endif // NETEXAMPLENOTIFY_H
