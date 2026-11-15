/*******************************************************************************
 * retroshare-profile/src/ProfileUpgradeDialog.h                               *
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

#ifndef RS_PROFILE_UPGRADE_DIALOG_H
#define RS_PROFILE_UPGRADE_DIALOG_H

#include <QDialog>

#include "retroshare/rsids.h"
#include "ui_ProfileUpgradeDialog.h"

/**
 * Asks for the password and runs ProfileUpgrader on the given node.
 * exec() returns Accepted only when the upgrade succeeded: the new node is
 * then logged in and start-up can continue with it (see newNodeId()).
 */
class ProfileUpgradeDialog : public QDialog
{
	Q_OBJECT

public:
	explicit ProfileUpgradeDialog(const RsPeerId& sslId, QWidget *parent = nullptr);

	RsPeerId newNodeId() const { return mNewId; }

	/// text of the button shown after a successful upgrade (default: "Start RetroShare")
	void setFinishButtonText(const QString& text) { mFinishText = text; }

	/// true when the upgrade failed after logging in to the old node:
	/// RetroShare must be restarted before logging in again.
	bool needsRestart() const { return mNeedsRestart; }

public slots:
	void reject() override;

private slots:
	void upgradeClicked();
	void browseBackup();

private:
	bool runBackup();
	void runUpgrade();
	void setRunning(bool running);
	void appendLog(const QString& line);

	Ui::ProfileUpgradeDialog ui;

	RsPeerId mOldId;
	RsPeerId mNewId;
	bool mRunning;
	bool mDone;
	bool mNeedsRestart;
	QString mFinishText;
};

#endif // RS_PROFILE_UPGRADE_DIALOG_H
