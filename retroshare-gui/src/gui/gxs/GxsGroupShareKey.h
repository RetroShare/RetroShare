/*******************************************************************************
 * retroshare-gui/src/gui/gxs/GxsGroupShareKey.h                               *
 *                                                                             *
 * Copyright 2010 by Christopher Evi-Parker <retroshare.project@gmail.com>     *
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

#ifndef SHAREKEY_H
#define SHAREKEY_H

#include <QDialog>

#include "ui_GxsGroupShareKey.h"

#define CHANNEL_KEY_SHARE 0x00000001
#define FORUM_KEY_SHARE	  0x00000002
#define POSTED_KEY_SHARE  0x00000003

class GroupShareKey : public QDialog
{
	Q_OBJECT

public:
	/*
	 *@param chanId The channel id to send request for
	 */
    GroupShareKey(QWidget *parent = 0, const RsGxsGroupId& grpId = RsGxsGroupId(), int grpType = 0);
    ~GroupShareKey();

protected:
	void changeEvent(QEvent *e);

private slots:
	void shareKey();
  void setTyp();
private:
    RsGxsGroupId mGrpId;
	int mGrpType;

	Ui::ShareKey *ui;
};

#endif // SHAREKEY_H
