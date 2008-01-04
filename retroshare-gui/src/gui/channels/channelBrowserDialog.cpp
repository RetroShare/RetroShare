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

#include "channelBrowserDialog.h"
const QString ChannelBrowserDialog::IMAGE_QUICKSUBSCRIBE    = ":/images/channelsubscribe.png";
const QString ChannelBrowserDialog::IMAGE_QUICKUNSUBSCRIBE  = ":/images/channeldelete.png";
const QString ChannelBrowserDialog::IMAGE_MANAGE_SUB        = ":/images/channels.png";
const QString ChannelBrowserDialog::IMAGE_MANAGE_CHANNEL    = ":/images/channels.png";
const QString ChannelBrowserDialog::IMAGE_QUICKDELETE       = ":/images/channeldelete.png";
 
ChannelBrowserDialog::ChannelBrowserDialog(QWidget * parent) 
        : QDialog (parent), browserContextMenu(0) 
{
    setupUi(this);
    channelTreeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect( channelTreeWidget, SIGNAL(customContextMenuRequested(QPoint)), 
             this,              SLOT(channelTreeWidgetCustumPopupMenu(QPoint)));
    
}

void ChannelBrowserDialog::channelTreeWidgetCustumPopupMenu( QPoint point )
{
    // block the popup if no results available
    if ((channelTreeWidget->selectedItems()).size() == 0) return; 
    
    // create the menu as required
    if (browserContextMenu == 0)
    {
        // setup the browser context menu
        browserContextMenu = new QMenu(this);
        
        // determine the selected channel and adjust the context menu accordingly i.e. no 
        // quick subscribe entry if the channel is already subscribed to etc
        quickSubscribeAct = new QAction(QIcon(IMAGE_QUICKSUBSCRIBE), tr( "Subscribe" ), this );
        connect( quickSubscribeAct, SIGNAL( triggered() ), this, SLOT( quickSubscribe() ) );
        quickUnsubscribeAct = new QAction(QIcon(IMAGE_QUICKUNSUBSCRIBE), tr( "Remove Subscription" ), this );
        connect( quickUnsubscribeAct, SIGNAL( triggered() ), this, SLOT( quickSubscribe() ) );
        subscribeToChannelAct = new QAction(QIcon(IMAGE_MANAGE_SUB), tr( "Manage Subscription" ), this );
        connect( subscribeToChannelAct, SIGNAL( triggered() ), this, SLOT( subscribeToChannel() ) );
        editChannelAct = new QAction(QIcon(IMAGE_MANAGE_CHANNEL), tr( "Manage Channel" ), this );
        connect( editChannelAct, SIGNAL( triggered() ), this, SLOT( manageChannel() ) );
        quickDeleteAct = new QAction(QIcon(IMAGE_QUICKDELETE), tr( "Quick Delete" ), this );
        connect( quickDeleteAct, SIGNAL( triggered() ), this, SLOT( quickDelete() ) );
        


        browserContextMenu->clear();
        browserContextMenu->addAction( new QAction("NOTE: Entries will be dynamic!!", this));
        browserContextMenu->addSeparator();
        browserContextMenu->addAction( quickSubscribeAct);
        browserContextMenu->addAction( quickUnsubscribeAct);
        browserContextMenu->addAction( subscribeToChannelAct);
        browserContextMenu->addSeparator();
        browserContextMenu->addAction( quickDeleteAct);
        browserContextMenu->addAction( editChannelAct);
    }
    QMouseEvent *mevent = new QMouseEvent( QEvent::MouseButtonPress, point, 
                                                Qt::RightButton, Qt::RightButton, Qt::NoModifier );
    browserContextMenu->exec( mevent->globalPos() );
}

void ChannelBrowserDialog::quickSubscribe(){}
void ChannelBrowserDialog::quickUnsubscribe(){}
void ChannelBrowserDialog::subscribeToChannel(){}
void ChannelBrowserDialog::manageChannel(){}
void ChannelBrowserDialog::quickDelete(){}
