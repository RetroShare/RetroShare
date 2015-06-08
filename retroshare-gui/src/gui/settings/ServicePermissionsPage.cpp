/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006 - 2010 RetroShare Team
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

#include "ServicePermissionsPage.h"

#include "rshare.h"
#include <retroshare/rspeers.h>
#include <retroshare/rsservicecontrol.h>

#include <iostream>
#include <QTimer>

#define PermissionStateUserRole		(Qt::UserRole)
#define ServiceIdUserRole		(Qt::UserRole + 1)
#define PeerIdUserRole			(Qt::UserRole + 2)

ServicePermissionsPage::ServicePermissionsPage(QWidget * parent, Qt::WindowFlags /*flags*/)
{
  /* Invoke the Qt Designer generated object setup routine */
  ui.setupUi(this);

    //QObject::connect(ui.tableWidget,SIGNAL(itemChanged(QTableWidgetItem *)),  this, SLOT(tableItemChanged(QTableWidgetItem *)));
}

QString ServicePermissionsPage::helpText() const
{
   return tr("<h1><img width=\"24\" src=\":/images/64px_help.png\">&nbsp;&nbsp;Permissions</h1>  \
    <p>Permissions allow you to control which services are available to which friends</p>\
    <p>Each interruptor shows two lights, indicating whether you or your friend has enabled\
             that service. Both needs to be ON (showing <img height=20 src=\":/images/switch11.png\"/>) to\
                   let information transfer for a specific service/friend combination.</p>\
                   <p>For each service, the global switch <img height=20 src=\":/images/global_switch_on.png\"> / <img height=20 src=\":/images/global_switch_off.png\">\
                   allow to turn a service ON/OFF for all friends at once.</p>\
                   <p>Be very careful: Some services depend on each other. For instance turning turtle OFF will also\
                   stop all anonymous transfer, distant chat and distant messaging.</p>");
}

bool ServicePermissionsPage::isHideOfflineChecked()
{
    return ui.cb_hideOffline->checkState() == Qt::Checked;
}


