/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2014 RetroShare Team
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

#ifndef GXSCHANNELFILESSTATUSWIDGET_H
#define GXSCHANNELFILESSTATUSWIDGET_H

#include <QWidget>

#include "retroshare/rsgxscommon.h"

namespace Ui {
class GxsChannelFilesStatusWidget;
}

class GxsChannelFilesStatusWidget : public QWidget
{
	Q_OBJECT

public:
	explicit GxsChannelFilesStatusWidget(const RsGxsGroupId &groupId, const RsGxsMessageId &messageId, const RsGxsFile &file, QWidget *parent = 0);
	~GxsChannelFilesStatusWidget();

private slots:
	void check();
	void download();
	void cancel();
	void pause();
	void resume();
	void openFolder();

private:
	void setSize(uint64_t size);

private:
	enum State
	{
		STATE_LOCAL,
		STATE_REMOTE,
		STATE_DOWNLOAD,
		STATE_PAUSED,
		STATE_WAITING,
		STATE_CHECKING,
		STATE_ERROR
	} mState;

private:
	RsGxsGroupId mGroupId;
	RsGxsMessageId mMessageId;
	RsGxsFile mFile;

	uint64_t mSize;
	uint64_t mDivisor;

	Ui::GxsChannelFilesStatusWidget *ui;
};

#endif // GXSCHANNELFILESSTATUSWIDGET_H
