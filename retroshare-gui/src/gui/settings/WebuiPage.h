/*******************************************************************************
 * gui/settings/WebuiPage.h                                                    *
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

#include <retroshare-gui/configpage.h>
#include "ui_WebuiPage.h"

namespace resource_api{
    class ApiServer;
    class ApiServerMHD;
	class ApiServerLocal;
    class RsControlModule;
}

class WebuiPage : public ConfigPage
{
  Q_OBJECT

public:
  /** Default Constructor */
  WebuiPage(QWidget * parent = 0, Qt::WindowFlags flags = 0);
    /** Default Destructor */
  ~WebuiPage();

  /** Loads the settings for this page */
  virtual void load();

  virtual QPixmap iconPixmap() const { return QPixmap(":/icons/settings/webinterface.svg") ; }
  virtual QString pageName() const { return tr("Webinterface") ; }
  virtual QString helpText() const;

  // call this after start of libretroshare/Retroshare
  // checks the settings and starts the webinterface if required
  static bool checkStartWebui();
  // call this before shutdown of libretroshare
  // it stops the webinterface if its running
  static void checkShutdownWebui();

  // show webinterface in default browser (if enabled)
  static void showWebui();

public slots:
  void onEnableCBClicked(bool checked);
  void onPortValueChanged(int value);
  void onAllIPCBClicked(bool checked);
  void onApplyClicked();

private:
  /** Qt Designer generated object */
  Ui::WebuiPage ui;

  bool updateParams(QString &errmsg);

  static resource_api::ApiServer* apiServer;
  static resource_api::ApiServerMHD* apiServerMHD;
 #ifdef LIBRESAPI_LOCAL_SERVER
  static resource_api::ApiServerLocal* apiServerLocal;
 #endif
  static resource_api::RsControlModule* controlModule;
};
