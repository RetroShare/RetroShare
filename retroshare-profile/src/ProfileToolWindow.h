/*******************************************************************************
 * retroshare-profile/src/ProfileToolWindow.h                                  *
 *                                                                             *
 * Copyright (c) 2026 RetroShare Team <retroshare.project@gmail.com>           *
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

#ifndef RS_PROFILE_TOOL_WINDOW_H
#define RS_PROFILE_TOOL_WINDOW_H

#include <QDialog>

#include "retroshare/rsids.h"
#include "ui_ProfileToolWindow.h"

class ProfileToolWindow : public QDialog
{
	Q_OBJECT

public:
	explicit ProfileToolWindow(QWidget *parent = nullptr);

private slots:
	void backupNode();
	void restoreNode();
	void upgradeNode();
	void updateButtons();
	void openNodeFolder();
	void openDataDirectory();
	void changeDataDirectory();

private:
	void fillNodeList(const RsPeerId& selectId = RsPeerId());
	RsPeerId selectedNode() const;
	void lockTool(const QString& message);

	Ui::ProfileToolWindow ui;
	bool mLocked;	// a node was logged in by an upgrade: the tool must be closed
};

#endif // RS_PROFILE_TOOL_WINDOW_H
