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
#ifndef _ChannelBrowserDialog_h_
#define _ChannelBrowserDialog_h_

#include <QtGui>

#include "ui_ChannelBrowserDialog.h"

class ChannelBrowserDialog : public QDialog,
public Ui::ChannelBrowserDialog 
{
    Q_OBJECT
        
public:
    ChannelBrowserDialog(QWidget * parent = 0 );
    
private slots:
    void channelTreeWidgetCustumPopupMenu( QPoint point );

private:
    /* constants for the image files */
    static const QString IMAGE_QUICKSUBSCRIBE;
    static const QString IMAGE_QUICKUNSUBSCRIBE;
    static const QString IMAGE_MANAGE_SUB;
    static const QString IMAGE_MANAGE_CHANNEL;
    static const QString IMAGE_QUICKDELETE;
 
    QMenu * browserContextMenu;
    
    // actions for the context menu
    /** context menu: allows one-click subscription to a channel */
    QAction* quickSubscribeAct;
    /** context menu: allows one-click removal of channel subscription*/
    QAction* quickUnsubscribeAct;
    /** context menu: switches to the my subscriptions page with this
        channel selected */
    QAction* subscribeToChannelAct;
    /** context menu: switches to my channels page with this channel
        selected */
    QAction* editChannelAct;
    /** conetxt menu: one-click delete (with confirmation pop-up) */
    QAction* quickDeleteAct;

    void quickSubscribe();
    void quickUnsubscribe();
    void subscribeToChannel();
    void manageChannel();
    void quickDelete();
};


#endif

