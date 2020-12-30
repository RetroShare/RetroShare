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
#include <util/misc.h>

/** Constructor */
FeedReaderConfig::FeedReaderConfig(QWidget *parent, Qt::WindowFlags flags)
	: ConfigPage(parent, flags), ui(new Ui::FeedReaderConfig)
{
	/* Invoke the Qt Designer generated object setup routine */
	ui->setupUi(this);

	ui->proxyAddressLineEdit->setEnabled(false);
	ui->proxyPortSpinBox->setEnabled(false);

	/* Connect signals */
	connect(ui->updateIntervalSpinBox, (void(QSpinBox::*)(int))&QSpinBox::valueChanged, this, [this]() {
		rsFeedReader->setStandardUpdateInterval(ui->updateIntervalSpinBox->value() * 60);
	});
	connect(ui->storageTimeSpinBox, (void(QSpinBox::*)(int))&QSpinBox::valueChanged, this, [this]() {
		rsFeedReader->setStandardStorageTime(ui->storageTimeSpinBox->value() * 60 *60 * 24);
	});
	connect(ui->saveInBackgroundCheckBox, &QCheckBox::toggled, this, [this]() {
		rsFeedReader->setSaveInBackground(ui->saveInBackgroundCheckBox->isChecked());
	});
	connect(ui->setMsgToReadOnActivate, &QCheckBox::toggled, this, [this]() {
		Settings->setValueToGroup("FeedReaderDialog", "SetMsgToReadOnActivate", ui->setMsgToReadOnActivate->isChecked());
	});
	connect(ui->openAllInNewTabCheckBox, &QCheckBox::toggled, this, [this]() {
		Settings->setValueToGroup("FeedReaderDialog", "OpenAllInNewTab", ui->openAllInNewTabCheckBox->isChecked());
	});
	connect(ui->useProxyCheckBox, &QCheckBox::toggled, this, &FeedReaderConfig::updateProxy);
	connect(ui->proxyAddressLineEdit, &QLineEdit::textChanged, this, &FeedReaderConfig::updateProxy);
	connect(ui->proxyPortSpinBox, (void(QSpinBox::*)(int))&QSpinBox::valueChanged, this, &FeedReaderConfig::updateProxy);

	connect(ui->useProxyCheckBox, SIGNAL(toggled(bool)), this, SLOT(useProxyToggled()));
}

/** Destructor */
FeedReaderConfig::~FeedReaderConfig()
{
	delete(ui);
}

/** Loads the settings for this page */
void FeedReaderConfig::load()
{
	whileBlocking(ui->updateIntervalSpinBox)->setValue(rsFeedReader->getStandardUpdateInterval() / 60);
	whileBlocking(ui->storageTimeSpinBox)->setValue(rsFeedReader->getStandardStorageTime() / (60 * 60 *24));
	whileBlocking(ui->saveInBackgroundCheckBox)->setChecked(rsFeedReader->getSaveInBackground());
	whileBlocking(ui->setMsgToReadOnActivate)->setChecked(FeedReaderSetting_SetMsgToReadOnActivate());
	whileBlocking(ui->openAllInNewTabCheckBox)->setChecked(FeedReaderSetting_OpenAllInNewTab());

	std::string proxyAddress;
	uint16_t proxyPort;
	whileBlocking(ui->useProxyCheckBox)->setChecked(rsFeedReader->getStandardProxy(proxyAddress, proxyPort));
	whileBlocking(ui->proxyAddressLineEdit)->setText(QString::fromUtf8(proxyAddress.c_str()));
	whileBlocking(ui->proxyPortSpinBox)->setValue(proxyPort);

	loaded = true;

	useProxyToggled();
}

void FeedReaderConfig::useProxyToggled()
{
	bool enabled = ui->useProxyCheckBox->isChecked();

	ui->proxyAddressLineEdit->setEnabled(enabled);
	ui->proxyPortSpinBox->setEnabled(enabled);
}

void FeedReaderConfig::updateProxy()
{
	rsFeedReader->setStandardProxy(ui->useProxyCheckBox->isChecked(), ui->proxyAddressLineEdit->text().toUtf8().constData(), ui->proxyPortSpinBox->value());
}
