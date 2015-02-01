/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (C) 2014 RetroShare Team
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

#pragma once

#include <map>
#include <set>

#include <QApplication>
#include <QDesktopWidget>
#include <QFrame>

#include <stdint.h>
#include <retroshare/rspeers.h>

#define HOR_SPC       2   /** Space between data points */
#define SCALE_WIDTH   75  /** Width of the scale */

#define BACK_COLOR    Qt::white
#define SCALE_COLOR   Qt::black
#define GRID_COLOR    Qt::lightGray
#define RSDHT_COLOR   Qt::magenta
#define ALLDHT_COLOR  Qt::yellow

#define FONT_SIZE     11

// This class provides a widget to represent an edit a matrix of permissions
// for services and friends. The widget allows to:
//   - set permission for each friend/service combination
//   - set permissions for a given service all friends at once
//   - set permissions for a given friend, all services at once
//
// The code can also be used to display bandwidth for each (friend,service) combination.
// Maybe we should in the future make a base class with the matrix structure and use it for
// various display usages.
//
class RSPermissionMatrixWidget: public QFrame
{
    Q_OBJECT

public:
    RSPermissionMatrixWidget(QWidget *parent=NULL);
    virtual ~RSPermissionMatrixWidget() ;

protected slots:
    // Calls the internal source for a new data points; called by the timer. You might want to overload this
    // if the collection system needs it. Otherwise, the default method will call getValues()
    void updateDisplay() ;

    void defaultPermissionSwitched(uint32_t ServiceId,bool b);
    void userPermissionSwitched(uint32_t ServiceId,const RsPeerId& friend_id,bool b);

    virtual void mousePressEvent(QMouseEvent *e) ;
    virtual void mouseMoveEvent(QMouseEvent *e) ;

protected:
    /** Overloaded QWidget::paintEvent() */
    void paintEvent(QPaintEvent *event);

private:
    bool computeServiceAndPeer(int x,int y,uint32_t& service_id,RsPeerId& peer_id) const ;
    bool computeServiceGlobalSwitch(int x,int y,uint32_t& service_id) const ;

    QRect computeNodePosition(int row,int col,bool selected) const ;

    void switchPermission(uint32_t service_id,const RsPeerId& peer_id) ;
    void switchPermission(uint32_t service_id) ;

    std::vector<RsPeerId> peer_ids ;
    std::vector<uint32_t> service_ids ;

    uint32_t _current_service_id ;
    RsPeerId _current_peer_id ;
    int matrix_start_x ;

    /** A QPainter object that handles drawing the various graph elements. */
    QPainter* _painter;
    QTimer *_timer ;

    /** The current dimensions of the graph. */
    QRect _rec;

    static const int ROW_SIZE ;
    static const int COL_SIZE ;
    static const int ICON_SIZE_X ;
    static const int ICON_SIZE_Y ;
    static const int MATRIX_START_X ;
    static const int MATRIX_START_Y ;
};


