/*******************************************************************************
 * gui/common/AvatarWidget.h                                                   *
 *                                                                             *
 * Copyright (C) 2012, Robert Fernie <retroshare.project@gmail.com>            *
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

#ifndef AVATARWIDGET_H
#define AVATARWIDGET_H

#include <QLabel>
#include <stdint.h>
#include <retroshare/rstypes.h>
#include <retroshare/rsmsgs.h>

namespace Ui {
	class AvatarWidget;
}

class AvatarWidget : public QLabel
{
	Q_OBJECT
	Q_PROPERTY(QString frameState READ frameState)

public:
	enum FrameType {
		NO_FRAME,
		NORMAL_FRAME,
		STATUS_FRAME
	};

public:
	AvatarWidget(QWidget *parent = 0);
	~AvatarWidget();

	QString frameState();
	void setFrameType(FrameType type);
    void setId(const ChatId& id) ;
    void setGxsId(const RsGxsId& id) ;
    void setOwnId();
    void setDefaultAvatar(const QString &avatar_file_name);

protected:
    void mouseReleaseEvent(QMouseEvent *event);

private slots:
    void updateStatus(const QString& peerId, int status);
	void updateAvatar(const QString& peerId);
    void updateOwnAvatar();
    void removeAvatar();
    void showContextMenu(const QPoint &pos);

private:
    void refreshAvatarImage() ;
    void refreshStatus();
    void updateStatus(int status);
    void selectAvatar();

	QString defaultAvatar;
	Ui::AvatarWidget *ui;

    ChatId mId;
    RsGxsId mGxsId;

	struct {
		bool isOwnId : 1;
//		bool isGpg : 1;
	} mFlag;
	FrameType mFrameType;
	uint32_t  mPeerState;
};

#endif // AVATARWIDGET_H
