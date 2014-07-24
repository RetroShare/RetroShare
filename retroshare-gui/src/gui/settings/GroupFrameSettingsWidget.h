#ifndef GROUPFRAMESETTINGSWIDGET_H
#define GROUPFRAMESETTINGSWIDGET_H

#include <QWidget>

#include "gui/settings/rsharesettings.h"

namespace Ui {
class GroupFrameSettingsWidget;
}

class GroupFrameSettingsWidget : public QWidget
{
	Q_OBJECT

public:
	explicit GroupFrameSettingsWidget(QWidget *parent = 0);
	~GroupFrameSettingsWidget();

	void loadSettings(GroupFrameSettings::Type type);
	void saveSettings(GroupFrameSettings::Type type);

private:
	bool mEnable;
	Ui::GroupFrameSettingsWidget *ui;
};

#endif // GROUPFRAMESETTINGSWIDGET_H
