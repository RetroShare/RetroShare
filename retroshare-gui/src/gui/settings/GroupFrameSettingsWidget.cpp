/*******************************************************************************
 * gui/settings/GroupFrameSettingsWidget.cpp                                   *
 *                                                                             *
 * Copyright 2009, Retroshare Team <retroshare.project@gmail.com>              *
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

#include <iostream>

#include "gui/notifyqt.h"
#include "util/misc.h"
#include "GroupFrameSettingsWidget.h"
#include "ui_GroupFrameSettingsWidget.h"
#include "retroshare/rsgxschannels.h"
#include "retroshare/rsgxsforums.h"
#include "retroshare/rsposted.h"

GroupFrameSettingsWidget::GroupFrameSettingsWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::GroupFrameSettingsWidget)
{
	ui->setupUi(this);

    mType = GroupFrameSettings::Nothing ;
	mEnable = true;

    connect(ui->openAllInNewTabCheckBox,     SIGNAL(toggled(bool)),this,SLOT(saveSettings())) ;
    connect(ui->hideTabBarWithOneTabCheckBox,SIGNAL(toggled(bool)),this,SLOT(saveSettings())) ;

    connect(ui->pbSyncApply,SIGNAL(clicked()),this,SLOT(saveSyncAllValue())) ;
    connect(ui->pbStoreApply,SIGNAL(clicked()),this,SLOT(saveStoreAllValue())) ;

    ui->cmbSync->addItem(tr(" 5 days"     ),QVariant(   5));
    ui->cmbSync->addItem(tr(" 2 weeks"    ),QVariant(  15));
    ui->cmbSync->addItem(tr(" 1 month"    ),QVariant(  30));
    ui->cmbSync->addItem(tr(" 3 months"   ),QVariant(  90));
    ui->cmbSync->addItem(tr(" 6 months"   ),QVariant( 180));
    ui->cmbSync->addItem(tr(" 1 year  "    ),QVariant( 365));
    ui->cmbSync->addItem(tr(" Indefinitly"),QVariant(   0));

    ui->cmbStore->addItem(tr(" 5 days"     ),QVariant(   5));
    ui->cmbStore->addItem(tr(" 2 weeks"    ),QVariant(  15));
    ui->cmbStore->addItem(tr(" 1 month"    ),QVariant(  30));
    ui->cmbStore->addItem(tr(" 3 months"   ),QVariant(  90));
    ui->cmbStore->addItem(tr(" 6 months"   ),QVariant( 180));
    ui->cmbStore->addItem(tr(" 1 year  "    ),QVariant( 365));
    ui->cmbStore->addItem(tr(" Indefinitly"),QVariant(   0));

    ui->cmbSync->setCurrentIndex(2);
    ui->cmbStore->setCurrentIndex(5);
}

GroupFrameSettingsWidget::~GroupFrameSettingsWidget()
{
	delete ui;
}

void GroupFrameSettingsWidget::setOpenAllInNewTabText(const QString &text)
{
	ui->openAllInNewTabCheckBox->setText(text);
}

void GroupFrameSettingsWidget::loadSettings(GroupFrameSettings::Type type)
{
    mType = type ;

	GroupFrameSettings groupFrameSettings;
	if (Settings->getGroupFrameSettings(type, groupFrameSettings)) {
		whileBlocking(ui->openAllInNewTabCheckBox)->setChecked(groupFrameSettings.mOpenAllInNewTab);
		whileBlocking(ui->hideTabBarWithOneTabCheckBox)->setChecked(groupFrameSettings.mHideTabBarWithOneTab);
	} else {
		hide();
		mEnable = false;
	}
}

void GroupFrameSettingsWidget::saveSettings()
{
    if(mType == GroupFrameSettings::Nothing)
    {
        std::cerr << "(EE) No type initialized for groupFrameSettings. This is a bug." << std::endl;
        return;
    }

	if (mEnable)
    {
		GroupFrameSettings groupFrameSettings;
		groupFrameSettings.mOpenAllInNewTab = ui->openAllInNewTabCheckBox->isChecked();
		groupFrameSettings.mHideTabBarWithOneTab = ui->hideTabBarWithOneTabCheckBox->isChecked();

		Settings->setGroupFrameSettings(mType, groupFrameSettings);

		NotifyQt::getInstance()->notifySettingsChanged();
	}
}

void GroupFrameSettingsWidget::saveSyncAllValue()
{
	uint32_t t = ui->cmbSync->currentData().toUInt() * 86400;
	switch (mType) {
		case GroupFrameSettings::Nothing: {
			break;
		}
		case GroupFrameSettings::Forum: {
			rsGxsForums->setSyncPeriodAll(t);
			break;
		}
		case GroupFrameSettings::Channel: {
			rsGxsChannels->setSyncPeriodAll(t);
			break;
		}
		case GroupFrameSettings::Posted: {
			rsPosted->setSyncPeriodAll(t);
			break;
		}
	}
}

void GroupFrameSettingsWidget::saveStoreAllValue()
{
	uint32_t t = ui->cmbStore->currentData().toUInt() * 86400;
	switch (mType) {
		case GroupFrameSettings::Nothing: {
			break;
		}
		case GroupFrameSettings::Forum: {
			rsGxsForums->setStoragePeriodAll(t);
			break;
		}
		case GroupFrameSettings::Channel: {
			rsGxsChannels->setStoragePeriodAll(t);
			break;
		}
		case GroupFrameSettings::Posted: {
			rsPosted->setStoragePeriodAll(t);
			break;
		}
	}
}