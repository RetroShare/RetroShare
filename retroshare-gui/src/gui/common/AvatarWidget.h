/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (c) 2010, RetroShare Team
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/

#ifndef AVATARWIDGET_H
#define AVATARWIDGET_H

#include <QWidget>

namespace Ui {
	class AvatarWidget;
}

class AvatarWidget : public QWidget
{
	Q_OBJECT

public:
	enum FrameType {
		NO_FRAME,
		NORMAL_FRAME,
		STATUS_FRAME
	};

public:
	AvatarWidget(QWidget *parent = 0);
	~AvatarWidget();

	void setFrameType(FrameType type);
	void setId(const std::string& id, bool isGpg);
	void setOwnId();

protected:
	void resizeEvent(QResizeEvent *event);
	void mouseReleaseEvent(QMouseEvent *event);

private slots:
	void updateStatus(const QString peerId, int status);
	void updateAvatar(const QString& peerId);
	void updateOwnAvatar();

private:
	void refreshStatus();

	Ui::AvatarWidget *ui;

	std::string mId;
	struct {
		bool isOwnId : 1;
		bool isGpg : 1;
	} mFlag;
	FrameType mFrameType;
};

#endif // AVATARWIDGET_H
