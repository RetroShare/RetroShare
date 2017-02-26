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

	void setOpenAllInNewTabText(const QString &text);

	void loadSettings(GroupFrameSettings::Type type);

    void setType(GroupFrameSettings::Type type) { mType = type ; }
protected slots:
	void saveSettings();

private:
	bool mEnable;
	Ui::GroupFrameSettingsWidget *ui;
    GroupFrameSettings::Type mType ;
};

#endif // GROUPFRAMESETTINGSWIDGET_H
