/*******************************************************************************
 * retroshare-gui/src/gui/Posted/PhotoView.h                                   *
 *                                                                             *
 * Copyright (C) 2020 by RetroShare Team      <retroshare.project@gmail.com>   *
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

#ifndef _PHOTO_VIEW_H
#define _PHOTO_VIEW_H

#include "ui_PhotoView.h"

#include <QDialog>

namespace Ui {
	class PhotoView;
}

class PhotoView : public QDialog
{
	Q_OBJECT

public:
	/** Default Constructor */
	PhotoView(QWidget *parent = 0);

	/** Default Destructor */
	~PhotoView();


public slots:
	void setPixmap(const QPixmap& pixmap);
	void setTitle (const QString &text);
	void setName(const RsGxsId& authorID);
	void setTime(const QString& text);
	void setGroupId(const RsGxsGroupId &groupId);
	void setMessageId(const RsGxsMessageId& messageId);
	void setGroupNameString(const QString& name);

private slots:
	void copyMessageLink();

private:
	RsGxsMessageId mMessageId;
	RsGxsGroupId mGroupId;

  /** Qt Designer generated object */
  Ui::PhotoView  *ui;

};

#endif

