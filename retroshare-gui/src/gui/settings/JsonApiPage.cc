/*******************************************************************************
 * gui/settings/JsonApiPage.cpp                                                *
 *                                                                             *
 * Copyright (C) 2018  Gioacchino Mazzurco <gio@eigenlab.org>                  *
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

#include "JsonApiPage.h"

#include "rsharesettings.h"
#include "jsonapi/jsonapi.h"
#include "util/misc.h"

#include <QTimer>
#include <QStringListModel>
#include <QProgressDialog>


JsonApiPage::JsonApiPage(QWidget */*parent*/, Qt::WindowFlags /*flags*/)
{
	ui.setupUi(this);
	connect( ui.addTokenPushButton, &QPushButton::clicked,
	         this, &JsonApiPage::addTokenClicked);
	connect( ui.removeTokenPushButton, &QPushButton::clicked,
	         this, &JsonApiPage::removeTokenClicked );
	connect( ui.tokensListView, &QListView::clicked,
	         this, &JsonApiPage::tokenClicked );
	connect( ui.applyConfigPushButton, &QPushButton::clicked,
	         this, &JsonApiPage::onApplyClicked );
}

bool JsonApiPage::updateParams(QString &errmsg)
{
	bool ok = true;
	bool changed = false;

	bool enabled = ui.enableCheckBox->isChecked();

	if( enabled != Settings->getJsonApiEnabled())
	{
		Settings->setJsonApiEnabled(enabled);
		changed = true;
	}

	uint16_t port = static_cast<uint16_t>(ui.portSpinBox->value());

	if(port != Settings->getJsonApiPort())
	{
		Settings->setJsonApiPort(port);
		changed = true;
	}

	QString listenAddress = ui.listenAddressLineEdit->text();
	if(listenAddress != Settings->getJsonApiListenAddress())
	{
		Settings->setJsonApiListenAddress(listenAddress);
		changed = true;
	}

	if(changed)
	{
		checkShutdownJsonApi();
		ok = checkStartJsonApi();
	}

	if(!ok) errmsg = "Could not start JSON API Server!";
	return ok;
}

void JsonApiPage::load()
{
	whileBlocking(ui.enableCheckBox)->setChecked(Settings->getJsonApiEnabled());
	whileBlocking(ui.portSpinBox)->setValue(Settings->getJsonApiPort());
	whileBlocking(ui.listenAddressLineEdit)->setText(Settings->getJsonApiListenAddress());

    QStringList newTk;

    for(const auto& it : JsonApiServer::instance().getAuthorizedTokens())
	        newTk.push_back(QString::fromStdString(it.first)+":"+QString::fromStdString(it.second)) ;

	whileBlocking(ui.tokensListView)->setModel(new QStringListModel(newTk));
}

QString JsonApiPage::helpText() const { return ""; }

/*static*/ bool JsonApiPage::checkStartJsonApi()
{
	checkShutdownJsonApi();

	if(Settings->getJsonApiEnabled())
		JsonApiServer::instance().start( Settings->getJsonApiPort(), Settings->getJsonApiListenAddress().toStdString() );

	return true;
}

/*static*/ void JsonApiPage::checkShutdownJsonApi()
{
    if(!JsonApiServer::instance().isRunning())
        return;

	JsonApiServer::instance().shutdown();	// this is a blocking call until the thread is terminated.

#ifdef SUSPENDED_CODE
	/* It is important to make a copy of +jsonApiServer+ pointer so the old
		 * object can be deleted later, while the original pointer is
		 * reassigned */

	QProgressDialog* pd = new QProgressDialog("Stopping JSON API Server", QString(), 0, 3000);
	QTimer* prtm = new QTimer;
	prtm->setInterval(16); // 60 FPS
	connect( prtm, &QTimer::timeout,
	         pd, [=](){pd->setValue(pd->value()+16);} );
	pd->show();
	prtm->start();

	/* Must wait for deletion because stopping of the server is async.
		 * It is important to capture a copy so it "survive" after
		 * safeStopJsonApiServer returns */
	QTimer::singleShot(3*1000, [=]()
	{
		prtm->stop();
		pd->close();
		prtm->deleteLater();
		pd->deleteLater();
	});
#endif
}

void JsonApiPage::onApplyClicked(bool)
{
	QString errmsg;
	updateParams(errmsg);
}

void JsonApiPage::addTokenClicked(bool)
{
	QString token(ui.tokenLineEdit->text());
	JsonApiServer::instance().authorizeToken(token.toStdString());

    QStringList newTk;

    for(const auto& it : JsonApiServer::instance().getAuthorizedTokens())
	        newTk.push_back(QString::fromStdString(it.first)+":"+QString::fromStdString(it.second)) ;

	whileBlocking(ui.tokensListView)->setModel(new QStringListModel(newTk));
}

void JsonApiPage::removeTokenClicked(bool)
{
	QString token(ui.tokenLineEdit->text());
	JsonApiServer::instance().revokeAuthToken(token.toStdString());

    QStringList newTk;

    for(const auto& it : JsonApiServer::instance().getAuthorizedTokens())
	        newTk.push_back(QString::fromStdString(it.first)+":"+QString::fromStdString(it.second)) ;

	whileBlocking(ui.tokensListView)->setModel(new QStringListModel(Settings->getJsonApiAuthTokens()) );
}

void JsonApiPage::tokenClicked(const QModelIndex& index)
{
	ui.tokenLineEdit->setText(ui.tokensListView->model()->data(index).toString());
}

