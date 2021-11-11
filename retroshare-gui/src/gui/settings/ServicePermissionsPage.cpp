/*******************************************************************************
 * gui/settings/ServicePermissionPage.cpp                                      *
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

#include "ServicePermissionsPage.h"

#include "rshare.h"
#include <retroshare/rspeers.h>
#include <retroshare/rsservicecontrol.h>

#include <iostream>
#include <QTimer>

ServicePermissionsPage::ServicePermissionsPage(QWidget * parent, Qt::WindowFlags flags) :
    ConfigPage(parent, flags)
{
	/* Invoke the Qt Designer generated object setup routine */
	ui.setupUi(this);

	ui.cb_hideOffline->setChecked(true);

	connect(ui.cb_hideOffline, SIGNAL(toggled(bool)), ui.gradFrame, SLOT(setHideOffline(bool)));
	//QObject::connect(ui.tableWidget,SIGNAL(itemChanged(QTableWidgetItem *)),  this, SLOT(tableItemChanged(QTableWidgetItem *)));

	ui.gradFrame->setHideOffline(ui.cb_hideOffline->isChecked());

	// Not implemented?
	ui.pushButton->hide();
}

QString ServicePermissionsPage::helpText() const
{
   return tr("<h1><img width=\"24\" src=\":/icons/help_64.png\">&nbsp;&nbsp;Permissions</h1>  \
    <p>Permissions allow you to control which services are available to which friends.</p>\
    <p>Each interruptor shows two lights, indicating whether you or your friend has enabled\
             that service. Both need to be ON (showing <img height=20 src=\":/images/switch11.png\"/>) to\
                   let information transfer for a specific service/friend combination.</p>\
                   <p>For each service, the global switch <img height=20 src=\":/images/global_switch_on.png\"> / <img height=20 src=\":/images/global_switch_off.png\">\
                   allows you to turn a service ON/OFF for all friends at once.</p>\
                   <p>Be very careful: Some services depend on each other. For instance turning turtle OFF will also\
                   stop all anonymous transfer, distant chat and distant messaging.</p>");
}
