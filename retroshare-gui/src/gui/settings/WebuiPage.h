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
