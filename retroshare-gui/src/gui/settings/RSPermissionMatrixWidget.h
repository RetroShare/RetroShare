/*******************************************************************************
 * gui/settings/RSPermissionMatrixWidget.h                                     *
 *                                                                             *
 * Copyright (c) 2014 Retroshare Team <retroshare.project@gmail.com>           *
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

#pragma once

#include <map>
#include <set>

#include <QApplication>
#include <QFrame>

#include <stdint.h>
#include <retroshare/rspeers.h>

#define HOR_SPC       2   /** Space between data points */
#define SCALE_WIDTH   75  /** Width of the scale */

#define BACK_COLOR       Qt::white
#define FOREGROUND_COLOR Qt::black
#define SCALE_COLOR      Qt::black
#define GRID_COLOR       Qt::lightGray
#define RSDHT_COLOR      Qt::magenta
#define ALLDHT_COLOR     Qt::yellow

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
    QString ServiceDescription(uint16_t serviceid);

public slots:
    void setHideOffline(bool hide);

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
    int _max_width;
    int _max_height;

    /** The current dimensions of the graph. */
    QRect _rec;

    bool mHideOffline;

    static const float fROW_SIZE ;
    static const float fCOL_SIZE ;
    static const float fICON_SIZE_X ;
    static const float fICON_SIZE_Y ;
    static const float fMATRIX_START_X ;
    static const float fMATRIX_START_Y ;
};


