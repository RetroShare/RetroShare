#include "GroupFrameSettingsWidget.h"
#include "ui_GroupFrameSettingsWidget.h"

GroupFrameSettingsWidget::GroupFrameSettingsWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::GroupFrameSettingsWidget)
{
	ui->setupUi(this);

	mEnable = true;
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
	GroupFrameSettings groupFrameSettings;
	if (Settings->getGroupFrameSettings(type, groupFrameSettings)) {
		ui->openAllInNewTabCheckBox->setChecked(groupFrameSettings.mOpenAllInNewTab);
		ui->hideTabBarWithOneTabCheckBox->setChecked(groupFrameSettings.mHideTabBarWithOneTab);
	} else {
		hide();
		mEnable = false;
	}
}

void GroupFrameSettingsWidget::saveSettings(GroupFrameSettings::Type type)
{
	if (mEnable) {
		GroupFrameSettings groupFrameSettings;
		groupFrameSettings.mOpenAllInNewTab = ui->openAllInNewTabCheckBox->isChecked();
		groupFrameSettings.mHideTabBarWithOneTab = ui->hideTabBarWithOneTabCheckBox->isChecked();

		Settings->setGroupFrameSettings(type, groupFrameSettings);
	}
}
