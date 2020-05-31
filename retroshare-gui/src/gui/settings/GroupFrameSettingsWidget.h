/*******************************************************************************
 * gui/settings/GroupFrameSettingsWidget.h                                     *
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
	void saveSyncAllValue();
	void saveStoreAllValue();

private:
	bool mEnable;
	Ui::GroupFrameSettingsWidget *ui;
    GroupFrameSettings::Type mType ;
};

#endif // GROUPFRAMESETTINGSWIDGET_H
