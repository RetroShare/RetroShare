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
#include "util/qtthreadsutils.h"

#include <QTimer>
#include <QMessageBox>
#include <QStringListModel>
#include <QProgressDialog>
#include <QRegularExpression>
#include <QRegularExpressionValidator>

#define IMAGE_LEDOFF  ":/images/ledoff1.png"
#define IMAGE_LEDON   ":/images/ledon1.png"

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
    QRegularExpression anRegex ("^" + anRange + ":" + anRange + "$");
    QRegularExpressionValidator *anValidator = new QRegularExpressionValidator(anRegex, this);

    ui.tokenLineEdit->setValidator(anValidator);

    // This limits the possible tokens to alphanumeric

    QString ipRange = "(?:[0-1]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])";
    // You may want to use QRegularExpression for new code with Qt 5 (not mandatory).
    QRegularExpression ipRegex ("^" + ipRange + "\\." + ipRange + "\\." + ipRange + "\\." + ipRange + "$");
    QRegularExpressionValidator *ipValidator = new QRegularExpressionValidator(ipRegex, this);

    ui.listenAddressLineEdit->setValidator(ipValidator);
    ui.providersListView->setSelectionMode(QAbstractItemView::NoSelection);	// prevents edition.

    ui.tokensListView->setSelectionMode(QAbstractItemView::SingleSelection);
    ui.tokensListView->setEditTriggers(QAbstractItemView::NoEditTriggers);	// prevents in-place edition of tokens

    mEventHandlerId = 0;

    rsEvents->registerEventsHandler( [this](std::shared_ptr<const RsEvent> e)
    {
        if(e->mType != RsEventType::JSON_API)
            return;

        auto je = dynamic_cast<const RsJsonApiEvent*>(e.get());

        if(!je)
            return;
#ifdef DEBUG
        std::cerr << "Caught JSONAPI event! code=" << static_cast<int>(je->mJsonApiEventCode) << std::endl;
#endif

        RsQThreadUtils::postToObject([=]() { load(); }, this );
    },
    mEventHandlerId, RsEventType::JSON_API );
}

JsonApiPage::~JsonApiPage()
{
    rsEvents->unregisterEventsHandler(mEventHandlerId);
}
QString JsonApiPage::helpText() const
{
    return tr("<h1><img width=\"24\" src=\":/icons/help_64.png\">&nbsp;&nbsp;Webinterface</h1>  \
     <p>Retroshare provides a JSON API allowing other softwares to communicate with its core using token-controlled HTTP requests to http://localhost:[port]. \
        Please refer to the Retroshare documentation for how to use this feature. </p>\
        <p>Unless you know what you're doing, you shouldn't need to change anything in this page. \
       The web interface for instance will automatically register its own token to the JSON API which will be visible \
        in the list of authenticated tokens after you enable it.</p>");
}
void JsonApiPage::setServerWidgetsEnabled(bool enabled)
{
	ui.applyConfigPushButton->setEnabled(enabled);
	ui.portSpinBox->setEnabled(enabled);
	ui.listenAddressLineEdit->setEnabled(enabled);
}

void JsonApiPage::enableJsonApi(bool checked)
{
	setServerWidgetsEnabled(checked);

    Settings->setJsonApiEnabled(checked);

    if(checked)
        checkStartJsonApi();
    else
        checkShutdownJsonApi();
}

bool JsonApiPage::updateParams()
{
	bool ok = true;

	uint16_t port = static_cast<uint16_t>(ui.portSpinBox->value());
	QString listenAddress = ui.listenAddressLineEdit->text();

	Settings->setJsonApiEnabled(ui.enableCheckBox->isChecked());
	Settings->setJsonApiPort(port);
	Settings->setJsonApiListenAddress(listenAddress);

	return ok;
}

void JsonApiPage::updateTokenList()
{
	QStringList newTk;

	for(const auto& it : rsJsonApi->getAuthorizedTokens())
		newTk.push_back(
		            QString::fromStdString(it.first) + ":" +
		            QString::fromStdString(it.second) );

	QAbstractItemModel *oldModel = ui.tokensListView->model();
	whileBlocking(ui.tokensListView)->setModel(new QStringListModel(newTk,ui.tokensListView));
	delete oldModel;
}

void JsonApiPage::load()
{
    whileBlocking(ui.portSpinBox)->setValue(rsJsonApi->listeningPort());
    whileBlocking(ui.listenAddressLineEdit)->setText(QString::fromStdString(rsJsonApi->getBindingAddress()));
    whileBlocking(ui.enableCheckBox)->setChecked(rsJsonApi->isRunning());

    // the check box is set whileBlocking, so the widgets it commands need to be
    // synced explicitly, otherwise the page opens in an inconsistent state.

    setServerWidgetsEnabled(rsJsonApi->isRunning());

    updateTokenList();

    QStringList newTk2;

    for(const auto& it : rsJsonApi->getResourceProviders())
        newTk2.push_back( QString::fromStdString(it.get().getName())) ;

    QAbstractItemModel *oldProvidersModel = ui.providersListView->model();
    whileBlocking(ui.providersListView)->setModel(new QStringListModel(newTk2,ui.providersListView));
    delete oldProvidersModel;

    if(rsJsonApi->isRunning())
        ui.statusLabelLED->setPixmap(FilesDefs::getPixmapFromQtResourcePath(IMAGE_LEDON)) ;
    else
        ui.statusLabelLED->setPixmap(FilesDefs::getPixmapFromQtResourcePath(IMAGE_LEDOFF)) ;
}

bool JsonApiPage::checkStartJsonApi()
{
	if(!Settings->getJsonApiEnabled()) return false;

	rsJsonApi->setListeningPort(Settings->getJsonApiPort());
	rsJsonApi->setBindingAddress(Settings->getJsonApiListenAddress().toStdString());

	const auto rErr = rsJsonApi->restart();
	if(rErr == RsJsonApiErrorNum::NOT_A_MACHINE_GUN)
	{
		RsDbg() << __PRETTY_FUNCTION__ << " apparently the user is attempting "
				<< "to restart JSON API service in a burst. Re-scheduling "
				<< "restart in a while..." << std::endl;
		RsThread::async([]()
		{
			std::this_thread::sleep_for(std::chrono::seconds(10));
			rsJsonApi->restart();
		});
	}
	else if(rErr)
	{
		RsErr() << __PRETTY_FUNCTION__ << rErr << std::endl;
		return false;
	}

	return true;
}

/*static*/ void JsonApiPage::checkShutdownJsonApi()
{
	rsJsonApi->fullstop(); // this is a blocks until the thread is terminated.
}

void JsonApiPage::onApplyClicked()
{
	// restart
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

	if(!RsJsonApi::parseToken(token.toStdString(),user,passwd))
	{
		QMessageBox::warning( this, tr("Invalid token"),
		                      tr("Tokens should spell as \"user:password\", where both "
		                         "user and password are alphanumeric strings.") );
		return;
	}

	auto err = rsJsonApi->authorizeUser(user,passwd);

	if(err)
		QMessageBox::warning( this, tr("Cannot add token"),
		                      QString::fromStdString(err.message()) );

	updateTokenList();
}

QString JsonApiPage::selectedTokenUser() const
{
	// The list selection is what the user actually points at. The token line
	// edit is only used as a fallback, for tokens typed by hand.

	QString token;
	const QModelIndex index = ui.tokensListView->currentIndex();

	if(index.isValid() && ui.tokensListView->selectionModel()->isSelected(index))
		token = index.data().toString();
	else
		token = ui.tokenLineEdit->text();

	return token.section(':',0,0);	// the core indexes tokens by user, not by "user:password"
}

void JsonApiPage::removeTokenClicked()
{
	const QString user = selectedTokenUser();

	if(user.isEmpty())
	{
		QMessageBox::information( this, tr("No token selected"),
		                          tr("Please select in the list below the token to remove.") );
		return;
	}

	if(!rsJsonApi->revokeAuthToken(user.toStdString()))
		QMessageBox::warning( this, tr("Cannot remove token"),
		                      tr("No authorization was found for user \"%1\".").arg(user) );

	ui.tokenLineEdit->clear();

	updateTokenList();
}

void JsonApiPage::tokenClicked(const QModelIndex& index)
{
	if(index.isValid())	// a click below the last item gives an invalid index
		ui.tokenLineEdit->setText(index.data().toString());
}
