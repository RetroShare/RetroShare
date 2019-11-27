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
	connect( ui.enableCheckBox,        SIGNAL(toggled(bool)), this, SLOT(enableJsonApi(bool)));
	connect( ui.addTokenPushButton,    SIGNAL(clicked()), this, SLOT(addTokenClicked()));
	connect( ui.removeTokenPushButton, SIGNAL(clicked()), this, SLOT(removeTokenClicked() ));
	connect( ui.tokensListView,        SIGNAL(clicked(const QModelIndex&)), this, SLOT(tokenClicked(const QModelIndex&) ));
	connect( ui.applyConfigPushButton, SIGNAL(clicked()), this, SLOT(onApplyClicked() ));
	connect( ui.portSpinBox, 		   SIGNAL(valueChanged(int)), this, SLOT(updateParams() ));
	connect( ui.listenAddressLineEdit, SIGNAL(textChanged(QString)), this, SLOT(updateParams() ));
	connect( ui.tokenLineEdit, SIGNAL(textChanged(QString)), this, SLOT(checkToken(QString) ));

    // This limits the possible tokens to alphanumeric

    QString anRange = "[a-zA-Z0-9]+";
    QRegExp anRegex ("^" + anRange + ":" + anRange + "$");
    QRegExpValidator *anValidator = new QRegExpValidator(anRegex, this);

    ui.tokenLineEdit->setValidator(anValidator);

    // This limits the possible tokens to alphanumeric

    QString ipRange = "(?:[0-1]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])";
    // You may want to use QRegularExpression for new code with Qt 5 (not mandatory).
    QRegExp ipRegex ("^" + ipRange + "\\." + ipRange + "\\." + ipRange + "\\." + ipRange + "$");
    QRegExpValidator *ipValidator = new QRegExpValidator(ipRegex, this);

    ui.listenAddressLineEdit->setValidator(ipValidator);

}

void JsonApiPage::enableJsonApi(bool checked)
{
	ui.addTokenPushButton->setEnabled(checked);
	ui.applyConfigPushButton->setEnabled(checked);
	ui.removeTokenPushButton->setEnabled(checked);
	ui.tokensListView->setEnabled(checked);
	ui.portSpinBox->setEnabled(checked);
	ui.listenAddressLineEdit->setEnabled(checked);

    Settings->setJsonApiEnabled(checked);

    if(checked)
        checkStartJsonApi();
    else
        checkShutdownJsonApi();
}

bool JsonApiPage::updateParams()
{
	bool ok = true;
	bool changed = false;

	uint16_t port = static_cast<uint16_t>(ui.portSpinBox->value());
	QString listenAddress = ui.listenAddressLineEdit->text();

	Settings->setJsonApiEnabled(ui.enableCheckBox->isChecked());
	Settings->setJsonApiPort(port);
	Settings->setJsonApiListenAddress(listenAddress);

	return ok;
}

void JsonApiPage::load()
{
	whileBlocking(ui.portSpinBox)->setValue(Settings->getJsonApiPort());
	whileBlocking(ui.listenAddressLineEdit)->setText(Settings->getJsonApiListenAddress());
	whileBlocking(ui.enableCheckBox)->setChecked(Settings->getJsonApiEnabled());

    QStringList newTk;

	for(const auto& it : rsJsonApi->getAuthorizedTokens())
		newTk.push_back(
		            QString::fromStdString(it.first) + ":" +
		            QString::fromStdString(it.second) );

	whileBlocking(ui.tokensListView)->setModel(new QStringListModel(newTk));
}

QString JsonApiPage::helpText() const { return ""; }

bool JsonApiPage::checkStartJsonApi()
{
	if(!Settings->getJsonApiEnabled())
        return false;

	rsJsonApi->setListeningPort(Settings->getJsonApiPort());
	rsJsonApi->setBindingAddress(Settings->getJsonApiListenAddress().toStdString());
	rsJsonApi->restart();

	return true;
}

/*static*/ void JsonApiPage::checkShutdownJsonApi()
{
	if(!rsJsonApi->isRunning()) return;
	rsJsonApi->fullstop(); // this is a blocks until the thread is terminated.
}

void JsonApiPage::onApplyClicked()
{
    // restart

    checkShutdownJsonApi();
    checkStartJsonApi();
}

void JsonApiPage::checkToken(QString s)
{
	std::string user,passwd;
	bool valid = RsJsonApi::parseToken(s.toStdString(), user, passwd) &&
	        !user.empty() && !passwd.empty();

    QColor color;

	if(!valid)
		color = QApplication::palette().color(QPalette::Disabled, QPalette::Base);
	else
		color = QApplication::palette().color(QPalette::Active, QPalette::Base);

	/* unpolish widget to clear the stylesheet's palette cache */
	//ui.searchLineFrame->style()->unpolish(ui.searchLineFrame);

	QPalette palette = ui.tokenLineEdit->palette();
	palette.setColor(ui.tokenLineEdit->backgroundRole(), color);
	ui.tokenLineEdit->setPalette(palette);
}
void JsonApiPage::addTokenClicked()
{
	QString token(ui.tokenLineEdit->text());
    std::string user,passwd;

	if(!RsJsonApi::parseToken(token.toStdString(),user,passwd)) return;

	rsJsonApi->authorizeUser(user,passwd);

    QStringList newTk;

	for(const auto& it : rsJsonApi->getAuthorizedTokens())
		newTk.push_back(
		            QString::fromStdString(it.first) + ":" +
		            QString::fromStdString(it.second) );

	whileBlocking(ui.tokensListView)->setModel(new QStringListModel(newTk));
}

void JsonApiPage::removeTokenClicked()
{
	QString token(ui.tokenLineEdit->text());
	rsJsonApi->revokeAuthToken(token.toStdString());

    QStringList newTk;

	for(const auto& it : rsJsonApi->getAuthorizedTokens())
		newTk.push_back(
		            QString::fromStdString(it.first) + ":" +
		            QString::fromStdString(it.second) );

	whileBlocking(ui.tokensListView)->setModel(new QStringListModel(Settings->getJsonApiAuthTokens()) );
}

void JsonApiPage::tokenClicked(const QModelIndex& index)
{
	ui.tokenLineEdit->setText(ui.tokensListView->model()->data(index).toString());
}

