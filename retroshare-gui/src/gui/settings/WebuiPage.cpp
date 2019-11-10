/*******************************************************************************
 * gui/settings/WebuiPage.cpp                                                  *
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

#include "WebuiPage.h"

#include <iostream>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QSpinBox>

#include "util/misc.h"
#include "retroshare/rswebui.h"

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
    connect(ui.port_SB, SIGNAL(valueChanged(int)), this, SLOT(onPortValueChanged(int)));
    connect(ui.allIp_CB, SIGNAL(clicked(bool)), this, SLOT(onAllIPCBClicked(bool)));
    connect(ui.applyStartBrowser_PB, SIGNAL(clicked()), this, SLOT(onApplyClicked()));
    connect(ui.webInterfaceFiles_LE, SIGNAL(clicked()), this, SLOT(selectWebInterfaceDirectory()));
}

WebuiPage::~WebuiPage()
{

}

void WebuiPage::selectWebInterfaceDirectory()
{
    QString dirname = QFileDialog::getExistingDirectory(NULL,tr("Please select the directory were to find retroshare webinterface files"),ui.webInterfaceFiles_LE->text());
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
	whileBlocking(ui.enableWebUI_CB)->setChecked(Settings->getWebinterfaceEnabled());
	whileBlocking(ui.port_SB)->setValue(Settings->getWebinterfacePort());
	whileBlocking(ui.webInterfaceFiles_LE)->setText(Settings->getWebinterfaceFilesDirectory());
	whileBlocking(ui.allIp_CB)->setChecked(Settings->getWebinterfaceAllowAllIps());
	onEnableCBClicked(Settings->getWebinterfaceEnabled());
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

    rsWebUI->setListeningPort(Settings->getWebinterfacePort());
    rsWebUI->setHtmlFilesDirectory(Settings->getWebinterfaceFilesDirectory().toStdString());

    rsWebUI->restart();

    return true;
}

/*static*/ void WebuiPage::checkShutdownWebui()
{
    rsWebUI->stop();
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
	ui.params_GB->setEnabled(checked);
	ui.applyStartBrowser_PB->setEnabled(checked);
	QString S;
	updateParams(S);
}

void WebuiPage::onPortValueChanged(int /*value*/)
{
	QString S;
	updateParams(S);
}

void WebuiPage::onAllIPCBClicked(bool /*checked*/)
{
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
