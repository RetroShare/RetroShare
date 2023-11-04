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
#include "util/qtthreadsutils.h"
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

#define IMAGE_LEDOFF  ":/images/ledoff1.png"
#define IMAGE_LEDON   ":/images/ledon1.png"

WebuiPage::WebuiPage(QWidget */*parent*/, Qt::WindowFlags /*flags*/)
{
    ui.setupUi(this);
    connect(ui.enableWebUI_CB, SIGNAL(clicked(bool)), this, SLOT(onEnableCBClicked(bool)));
    connect(ui.allIp_CB, SIGNAL(clicked(bool)), this, SLOT(onAllIPCBClicked(bool)));
    connect(ui.apply_PB, SIGNAL(clicked()), this, SLOT(onApplyClicked()));
    connect(ui.password_LE, SIGNAL(textChanged(QString)), this, SLOT(onPasswordValueChanged(QString)));
    connect(ui.startWebBrowser_PB, SIGNAL(clicked()), this, SLOT(onStartWebBrowserClicked()));
    connect(ui.webInterfaceFilesDirectory_PB, SIGNAL(clicked()), this, SLOT(selectWebInterfaceDirectory()));

    mEventsHandlerId = 0;

    rsEvents->registerEventsHandler( [this](std::shared_ptr<const RsEvent> /* event */)
    {
        std::cerr << "Caught JSONAPI event in webui!" << std::endl;
        RsQThreadUtils::postToObject([=]() { load(); }, this );
    },
    mEventsHandlerId, RsEventType::JSON_API );
}

WebuiPage::~WebuiPage()
{
    rsEvents->unregisterEventsHandler(mEventsHandlerId);
}

void WebuiPage::selectWebInterfaceDirectory()
{
    QString dirname = QFileDialog::getExistingDirectory(NULL,tr("Please select the directory were to find retroshare webinterface files"),ui.webInterfaceFiles_LE->text());

    if(dirname.isNull())
        return;

	whileBlocking(ui.webInterfaceFiles_LE)->setText(dirname);

    QString S;
    updateParams(S);
}

bool WebuiPage::updateParams(QString &errmsg)
{
    std::cerr << "WebuiPage::save()" << std::endl;

    // store config
    Settings->setWebinterfaceEnabled(ui.enableWebUI_CB->isChecked());
    Settings->setWebinterfaceFilesDirectory(ui.webInterfaceFiles_LE->text());

    return true;
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
    if(ui.password_LE->text().isNull())
    {
        QMessageBox::critical(nullptr,tr("Missing passphrase"),tr("Please set a passphrase to proect the access to the WEB interface."));
        return false;
    }

    rsWebUi->setUserPassword(ui.password_LE->text().toStdString());
    rsWebUi->setHtmlFilesDirectory(ui.webInterfaceFiles_LE->text().toStdString());

    setCursor(Qt::WaitCursor) ;
    rsWebUi->restart();
    setCursor(Qt::ArrowCursor) ;

    return true;
}

void WebuiPage::loadParams()
{
	std::cerr << "WebuiPage::load()" << std::endl;
	whileBlocking(ui.enableWebUI_CB)->setChecked(Settings->getWebinterfaceEnabled());
	whileBlocking(ui.webInterfaceFiles_LE)->setText(Settings->getWebinterfaceFilesDirectory());

#ifdef RS_JSONAPI
	auto smap = rsJsonApi->getAuthorizedTokens();
    auto it = smap.find("webui");

    if(it != smap.end())
		whileBlocking(ui.password_LE)->setText(QString::fromStdString(it->second));
    else
        whileBlocking(ui.enableWebUI_CB)->setChecked(false);

    if(rsWebUi->isRunning())
        ui.statusLabelLED->setPixmap(FilesDefs::getPixmapFromQtResourcePath(IMAGE_LEDON)) ;
    else
        ui.statusLabelLED->setPixmap(FilesDefs::getPixmapFromQtResourcePath(IMAGE_LEDOFF)) ;
#else
    ui.statusLabelLED->setPixmap(FilesDefs::getPixmapFromQtResourcePath(IMAGE_LEDOFF)) ;
#endif
}


QString WebuiPage::helpText() const
{
    return tr("<h1><img width=\"24\" src=\":/icons/help_64.png\">&nbsp;&nbsp;Webinterface</h1>  \
     <p>The webinterface allows you to control Retroshare from the browser. Multiple devices can share control over one Retroshare instance. So you could start a conversation on a tablet computer and later use a desktop computer to continue it.</p>\
     <p>Warning: don't expose the webinterface to the internet, because there is no access control and no encryption. If you want to use the webinterface over the internet, use a SSH tunnel or a proxy to secure the connection.</p>");
}

/*static*/ bool WebuiPage::checkStartWebui() // This is supposed to be called from main(). But normally the parameters below (including the paswd
                                            // for the webUI should be saved in p3webui instead.
{
    rsWebUi->setHtmlFilesDirectory(Settings->getWebinterfaceFilesDirectory().toStdString());
    rsWebUi->restart();

    return true;
}

/*static*/ void WebuiPage::checkShutdownWebui()
{
	rsWebUi->stop();
}

/*static*/ void WebuiPage::showWebui()
{
	if(Settings->getWebinterfaceEnabled())
	{
		QUrl webuiUrl;
		webuiUrl.setScheme("http");
		webuiUrl.setHost(QString::fromStdString(rsJsonApi->getBindingAddress()));
		webuiUrl.setPort(rsJsonApi->listeningPort());
		webuiUrl.setPath("/index.html");
		QDesktopServices::openUrl(webuiUrl);
	}
    else
    {
        QMessageBox::warning(0, tr("Webinterface not enabled"), tr("The webinterface is not enabled. Enable it in Settings -> Webinterface."));
    }
}

void WebuiPage::onEnableCBClicked(bool checked)
{
    QString errmsg;
    updateParams(errmsg);

    ui.params_GB->setEnabled(checked);
    ui.startWebBrowser_PB->setEnabled(checked);
    ui.apply_PB->setEnabled(checked);

    if(checked)
    {
        if(!restart())
        {
            QMessageBox::warning(0, tr("failed to start Webinterface"), "Failed to start the webinterface.");
            return;
        }
    }
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
    QString errmsg;
    updateParams(errmsg);

    restart();

    load();
}

void WebuiPage::onStartWebBrowserClicked() { showWebui(); }
