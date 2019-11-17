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
#include "retroshare/rsjsonapi.h"

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
    connect(ui.apply_PB, SIGNAL(clicked()), this, SLOT(onApplyClicked()));
    connect(ui.password_LE, SIGNAL(textChanged(QString)), this, SLOT(onPasswordValueChanged(QString)));
    connect(ui.startWebBrowser_PB, SIGNAL(clicked()), this, SLOT(onStartWebBrowserClicked()));
    connect(ui.webInterfaceFilesDirectory_PB, SIGNAL(clicked()), this, SLOT(selectWebInterfaceDirectory()));
}

WebuiPage::~WebuiPage()
{

}

void WebuiPage::selectWebInterfaceDirectory()
{
    QString dirname = QFileDialog::getExistingDirectory(NULL,tr("Please select the directory were to find retroshare webinterface files"),ui.webInterfaceFiles_LE->text());

    if(dirname.isNull())
        return;

	whileBlocking(ui.webInterfaceFiles_LE)->setText(dirname);
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
    if(ui.webInterfaceFiles_LE->text() != Settings->getWebinterfaceFilesDirectory())
        changed = true;

    if(changed)
    {
        // store config
        Settings->setWebinterfaceEnabled(ui.enableWebUI_CB->isChecked());
        Settings->setWebinterfacePort(ui.port_SB->value());
        Settings->setWebinterfaceAllowAllIps(ui.allIp_CB->isChecked());
        Settings->setWebinterfaceFilesDirectory(ui.webInterfaceFiles_LE->text());
    }
    return ok;
}

void WebuiPage::onPasswordValueChanged(QString password)
{
    QColor color;

    bool valid = password.length() >= 1;

	if(!valid)
		color = QApplication::palette().color(QPalette::Disabled, QPalette::Base);
	else
		color = QApplication::palette().color(QPalette::Active, QPalette::Base);

	/* unpolish widget to clear the stylesheet's palette cache */
	//ui.searchLineFrame->style()->unpolish(ui.searchLineFrame);

	QPalette palette = ui.password_LE->palette();
	palette.setColor(ui.password_LE->backgroundRole(), color);
	ui.password_LE->setPalette(palette);
}

bool WebuiPage::restart()
{
	// apply config
	checkShutdownWebui();

	return checkStartWebui();
}
void WebuiPage::load()
{
	std::cerr << "WebuiPage::load()" << std::endl;
	whileBlocking(ui.enableWebUI_CB)->setChecked(Settings->getWebinterfaceEnabled());
	whileBlocking(ui.port_SB)->setValue(Settings->getWebinterfacePort());
	whileBlocking(ui.webInterfaceFiles_LE)->setText(Settings->getWebinterfaceFilesDirectory());
	whileBlocking(ui.allIp_CB)->setChecked(Settings->getWebinterfaceAllowAllIps());

#ifdef RS_JSONAPI
    auto smap = rsJsonAPI->getAuthorizedTokens();
    auto it = smap.find("webui");

    if(it != smap.end())
		whileBlocking(ui.password_LE)->setText(QString::fromStdString(it->second));
#endif
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
        return false;

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
	ui.apply_PB->setEnabled(checked);
	ui.startWebBrowser_PB->setEnabled(checked);
	QString S;

    Settings->setWebinterfaceEnabled(checked);

    if(checked)
        checkStartWebui();
    else
        checkShutdownWebui();
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
    rsWebUI->setUserPassword(ui.password_LE->text().toStdString());

    if(!restart())
    {
        QMessageBox::warning(0, tr("failed to start Webinterface"), "Failed to start the webinterface.");
        return;
    }

    emit passwordChanged();
}

void WebuiPage::onStartWebBrowserClicked()
{
    QDesktopServices::openUrl(QUrl(QString("http://localhost:")+QString::number(ui.port_SB->value())));
}
