/*******************************************************************************
 * retroshare-gui/src/gui/gxsforums/GxsForumMessageDialog.h                    *
 *                                                                             *
 * Copyright (C) 2020 by RetroShare Team   <retroshare.project@gmail.com>      *
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

#ifndef _FORUMMESSAGEDIALOG_H
#define _FORUMMESSAGEDIALOG_H

#include "ui_GxsForumMessageDialog.h"

#include <QDialog>

namespace Ui {
	class GxsForumMessageDialog;
}

class GxsForumMessageDialog : public QDialog
{
	Q_OBJECT

public:
	/** Default Constructor */
	GxsForumMessageDialog(QWidget *parent = 0);

	/** Default Destructor */
	~GxsForumMessageDialog();

public slots:
	void setTitle (const QString &text);
	void setText(const QString& text);
	void setName(const RsGxsId& authorID);
	void setTime(const QString& text);
	void setGroupId(const RsGxsGroupId &groupId);
	void setMessageId(const RsGxsMessageId& messageId);
	void replyForumMessage();

private slots:
	void copyMessageLink();

private:
	RsGxsMessageId mMessageId;
	RsGxsGroupId mGroupId;
	RsGxsId mAuthorID;

  /** Qt Designer generated object */
  Ui::GxsForumMessageDialog  *ui;

};

#endif

