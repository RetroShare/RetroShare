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

GroupFrameSettingsWidget::GroupFrameSettingsWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::GroupFrameSettingsWidget)
{
	ui->setupUi(this);

    mType = GroupFrameSettings::Nothing ;
	mEnable = true;

    connect(ui->openAllInNewTabCheckBox,     SIGNAL(toggled(bool)),this,SLOT(saveSettings())) ;
    connect(ui->hideTabBarWithOneTabCheckBox,SIGNAL(toggled(bool)),this,SLOT(saveSettings())) ;
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
