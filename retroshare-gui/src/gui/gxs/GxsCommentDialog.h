/*******************************************************************************
 * retroshare-gui/src/gui/gxs/GxsCommentDialog.h                               *
 *                                                                             *
 * Copyright 2012-2012 by Robert Fernie   <retroshare.project@gmail.com>       *
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

#ifndef MRK_GXS_COMMENT_DIALOG_H
#define MRK_GXS_COMMENT_DIALOG_H

#include "gui/gxs/GxsCommentContainer.h"

namespace Ui {
class GxsCommentDialog;
}

class GxsCommentDialog: public QWidget 
{
	Q_OBJECT

public:
	GxsCommentDialog(QWidget *parent);
	GxsCommentDialog(QWidget *parent, RsTokenService *token_service, RsGxsCommentService *comment_service);
	virtual ~GxsCommentDialog();

    void setTokenService(RsTokenService *token_service, RsGxsCommentService *comment_service);
	void setCommentHeader(QWidget *header);
	void commentLoad(const RsGxsGroupId &grpId, const std::set<RsGxsMessageId> &msg_versions, const RsGxsMessageId &most_recent_msgId);

	RsGxsGroupId groupId() { return mGrpId; }
	RsGxsMessageId messageId() { return mMostRecentMsgId; }

private slots:
	void refresh();
    void idChooserReady();
	void voterSelectionChanged( int index );
	void sortComments(int);
    void notifyCommentsLoaded(int n);

signals:
    void commentsLoaded(int);

private:
    void init();

	RsGxsGroupId   mGrpId;
	RsGxsMessageId mMostRecentMsgId;
	std::set<RsGxsMessageId> mMsgVersions;

	/* UI - from Designer */
	Ui::GxsCommentDialog *ui;
};

#endif

