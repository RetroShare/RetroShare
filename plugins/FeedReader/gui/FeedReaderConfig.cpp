/*******************************************************************************
 * plugins/FeedReader/gui/AddReaderConfig.cpp                                  *
 *                                                                             *
 * Copyright (C) 2012 by Thunder                                               *
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

#include "FeedReaderConfig.h"
#include "ui_FeedReaderConfig.h"
#include "gui/settings/rsharesettings.h"
#include "interface/rsFeedReader.h"

/** Constructor */
FeedReaderConfig::FeedReaderConfig(QWidget *parent, Qt::WindowFlags flags)
	: ConfigPage(parent, flags), ui(new Ui::FeedReaderConfig)
{
	/* Invoke the Qt Designer generated object setup routine */
	ui->setupUi(this);

	connect(ui->useProxyCheckBox, SIGNAL(toggled(bool)), this, SLOT(useProxyToggled()));

	ui->proxyAddressLineEdit->setEnabled(false);
	ui->proxyPortSpinBox->setEnabled(false);

	loaded = false;
}

/** Destructor */
FeedReaderConfig::~FeedReaderConfig()
{
	delete(ui);
}

/** Loads the settings for this page */
void FeedReaderConfig::load()
{
	ui->updateIntervalSpinBox->setValue(rsFeedReader->getStandardUpdateInterval() / 60);
	ui->storageTimeSpinBox->setValue(rsFeedReader->getStandardStorageTime() / (60 * 60 *24));
	ui->saveInBackgroundCheckBox->setChecked(rsFeedReader->getSaveInBackground());
	ui->setMsgToReadOnActivate->setChecked(FeedReaderSetting_SetMsgToReadOnActivate());
	ui->openAllInNewTabCheckBox->setChecked(FeedReaderSetting_OpenAllInNewTab());

	std::string proxyAddress;
	uint16_t proxyPort;
	ui->useProxyCheckBox->setChecked(rsFeedReader->getStandardProxy(proxyAddress, proxyPort));
	ui->proxyAddressLineEdit->setText(QString::fromUtf8(proxyAddress.c_str()));
	ui->proxyPortSpinBox->setValue(proxyPort);

	loaded = true;
}

bool FeedReaderConfig::save(QString &/*errmsg*/)
{
	rsFeedReader->setStandardUpdateInterval(ui->updateIntervalSpinBox->value() * 60);
	rsFeedReader->setStandardStorageTime(ui->storageTimeSpinBox->value() * 60 *60 * 24);
	rsFeedReader->setStandardProxy(ui->useProxyCheckBox->isChecked(), ui->proxyAddressLineEdit->text().toUtf8().constData(), ui->proxyPortSpinBox->value());
	rsFeedReader->setSaveInBackground(ui->saveInBackgroundCheckBox->isChecked());
	Settings->setValueToGroup("FeedReaderDialog", "SetMsgToReadOnActivate", ui->setMsgToReadOnActivate->isChecked());
	Settings->setValueToGroup("FeedReaderDialog", "OpenAllInNewTab", ui->openAllInNewTabCheckBox->isChecked());

	return true;
}

void FeedReaderConfig::useProxyToggled()
{
	bool enabled = ui->useProxyCheckBox->isChecked();

	ui->proxyAddressLineEdit->setEnabled(enabled);
	ui->proxyPortSpinBox->setEnabled(enabled);
}
