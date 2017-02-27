#include "WebuiPage.h"

#include <iostream>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>

#include "api/ApiServer.h"
#include "api/ApiServerMHD.h"
#include "api/ApiServerLocal.h"
#include "api/RsControlModule.h"
#include "api/GetPluginInterfaces.h"

#include "rsharesettings.h"

resource_api::ApiServer* WebuiPage::apiServer = 0;
resource_api::ApiServerMHD* WebuiPage::apiServerMHD = 0;
// TODO: LIBRESAPI_LOCAL_SERVER Put indipendent option for libresapilocalserver in appropriate place
#ifdef LIBRESAPI_LOCAL_SERVER
resource_api::ApiServerLocal* WebuiPage::apiServerLocal = 0;
#endif
resource_api::RsControlModule* WebuiPage::controlModule = 0;

WebuiPage::WebuiPage(QWidget */*parent*/, Qt::WindowFlags /*flags*/)
{
    ui.setupUi(this);
    connect(ui.enableWebUI_CB, SIGNAL(clicked(bool)), this, SLOT(onEnableCBClicked(bool)));
    connect(ui.applyStartBrowser_PB, SIGNAL(clicked()), this, SLOT(onApplyClicked()));
}

WebuiPage::~WebuiPage()
{

}

bool WebuiPage::updateParams(QString &errmsg)
{
    std::cerr << "WebuiPage::save()" << std::endl;
    bool ok = true;
    bool changed = false;
    if(ui.enableWebUI_CB->isChecked() != Settings->getWebinterfaceEnabled())
        changed = true;
    if(ui.port_SB->value() != Settings->getWebinterfacePort())
        changed = true;
    if(ui.allIp_CB->isChecked() != Settings->getWebinterfaceAllowAllIps())
        changed = true;
    if(changed)
    {
        // store config
        Settings->setWebinterfaceEnabled(ui.enableWebUI_CB->isChecked());
        Settings->setWebinterfacePort(ui.port_SB->value());
        Settings->setWebinterfaceAllowAllIps(ui.allIp_CB->isChecked());

        // apply config
        checkShutdownWebui();
        ok = checkStartWebui();
    }
    if(!ok)
        errmsg = "Could not start webinterface.";
    return ok;
}

void WebuiPage::load()
{
    std::cerr << "WebuiPage::load()" << std::endl;
    ui.enableWebUI_CB->setChecked(Settings->getWebinterfaceEnabled());
    onEnableCBClicked(Settings->getWebinterfaceEnabled());
    ui.port_SB->setValue(Settings->getWebinterfacePort());
    ui.allIp_CB->setChecked(Settings->getWebinterfaceAllowAllIps());
}

QString WebuiPage::helpText() const
{
    return tr("<h1><img width=\"24\" src=\":/icons/help_64.png\">&nbsp;&nbsp;Webinterface</h1>  \
     <p>The webinterface allows you to control Retroshare from the browser. Multiple devices can share control over one Retroshare instance. So you could start a conversation on a tablet computer and later use a desktop computer to continue it.</p>\
     <p>Warning: don't expose the webinterface to the internet, because there is no access control and no encryption. If you want to use the webinterface over the internet, use a SSH tunnel or a proxy to secure the connection.</p>");
}

/*static*/ bool WebuiPage::checkStartWebui()
{
    if(!Settings->getWebinterfaceEnabled())
        return true;
    if(apiServer || apiServerMHD || controlModule)
        return true;

    apiServer = new resource_api::ApiServer();
    controlModule = new resource_api::RsControlModule(0, 0, apiServer->getStateTokenServer(), apiServer, false);
    apiServer->addResourceHandler("control", dynamic_cast<resource_api::ResourceRouter*>(controlModule), &resource_api::RsControlModule::handleRequest);

    RsPlugInInterfaces ifaces;
    resource_api::getPluginInterfaces(ifaces);
    apiServer->loadMainModules(ifaces);

    apiServerMHD = new resource_api::ApiServerMHD(apiServer);
    bool ok = apiServerMHD->configure(resource_api::getDefaultDocroot(),
                                      Settings->getWebinterfacePort(),
                                      "",
                                      Settings->getWebinterfaceAllowAllIps());
    apiServerMHD->start();

// TODO: LIBRESAPI_LOCAL_SERVER Move in appropriate place
#ifdef LIBRESAPI_LOCAL_SERVER
	apiServerLocal = new resource_api::ApiServerLocal(apiServer, resource_api::ApiServerLocal::serverPath());
#endif
    return ok;
}

/*static*/ void WebuiPage::checkShutdownWebui()
{
    if(apiServer || apiServerMHD)
    {
        apiServerMHD->stop();
        delete apiServerMHD;
        apiServerMHD = 0;
// TODO: LIBRESAPI_LOCAL_SERVER Move in appropriate place
#ifdef LIBRESAPI_LOCAL_SERVER
		delete apiServerLocal;
		apiServerLocal = 0;
#endif
        delete apiServer;
        apiServer = 0;
        delete controlModule;
        controlModule = 0;
    }
}

/*static*/ void WebuiPage::showWebui()
{
    if(Settings->getWebinterfaceEnabled())
    {
        QDesktopServices::openUrl(QUrl(QString("http://localhost:")+QString::number(Settings->getWebinterfacePort())));
    }
    else
    {
        QMessageBox::warning(0, tr("Webinterface not enabled"), tr("The webinterface is not enabled. Enable it in Settings -> Webinterface."));
    }
}

void WebuiPage::onEnableCBClicked(bool checked)
{
    if(checked)
    {
        ui.params_GB->setEnabled(true);
        ui.applyStartBrowser_PB->setEnabled(true);
    }
    else
    {
        ui.params_GB->setEnabled(false);
        ui.applyStartBrowser_PB->setEnabled(false);
    }

    QString S;
    updateParams(S);
}

void WebuiPage::onApplyClicked()
{
    QString errmsg;
    bool ok = updateParams(errmsg);
    if(!ok)
    {
        QMessageBox::warning(0, tr("failed to start Webinterface"), "Failed to start the webinterface.");
        return;
    }
    QDesktopServices::openUrl(QUrl(QString("http://localhost:")+QString::number(ui.port_SB->value())));
}
