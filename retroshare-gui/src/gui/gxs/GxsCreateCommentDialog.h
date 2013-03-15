/*
 * Retroshare Gxs Support
 *
 * Copyright 2012-2013 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */

#ifndef _MRK_GXS_CREATE_COMMENT_DIALOG_H
#define _MRK_GXS_CREATE_COMMENT_DIALOG_H

#include <QDialog>
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
