/*******************************************************************************
 * retroshare-gui/src/gui/gxs/GxsCreateCommentDialog.h                         *
 *                                                                             *
 * Copyright 2012-2013 by Robert Fernie   <retroshare.project@gmail.com>       *
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

#ifndef _MRK_GXS_CREATE_COMMENT_DIALOG_H
#define _MRK_GXS_CREATE_COMMENT_DIALOG_H

#include <QDialog>
#include <retroshare/rsidentity.h>

#include "retroshare/rsgxscommon.h"
#include "util/TokenQueue.h"

namespace Ui {
	class GxsCreateCommentDialog;
}

class GxsCreateCommentDialog : public QDialog
{
	Q_OBJECT

public:
	explicit GxsCreateCommentDialog(TokenQueue* tokQ, RsGxsCommentService *service, 
	const RsGxsGrpMsgIdPair& parentId, const RsGxsMessageId& threadId, QWidget *parent = 0);
	~GxsCreateCommentDialog();

	void loadComment(const QString &msgText, const QString &msgAuthor, const RsGxsId &msgAuthorId);	
	
private slots:
	void createComment();
	
private:
	Ui::GxsCreateCommentDialog *ui;
	TokenQueue *mTokenQueue;
	RsGxsCommentService *mCommentService;

	RsGxsGrpMsgIdPair mParentId;
	RsGxsMessageId mThreadId;
};

#endif // _MRK_GXS_CREATE_COMMENT_DIALOG_H
