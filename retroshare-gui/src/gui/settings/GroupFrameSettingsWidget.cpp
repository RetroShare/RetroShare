#include <iostream>

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
		ui->openAllInNewTabCheckBox->setChecked(groupFrameSettings.mOpenAllInNewTab);
		ui->hideTabBarWithOneTabCheckBox->setChecked(groupFrameSettings.mHideTabBarWithOneTab);
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
	}
}
