/*******************************************************************************
 * retroshare-gui/src/gui/Posted/PostedCardView.h                              *
 *                                                                             *
 * Copyright (C) 2019 by Retroshare Team       <retroshare.project@gmail.com>  *
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

#ifndef _POSTED_CARDVIEW_H
#define _POSTED_CARDVIEW_H

#include <QMetaType>

#include <retroshare/rsposted.h>
#include "PostedItem.h"

namespace Ui {
class PostedCardView;
}

class FeedHolder;
class RsPostedPost;

class PostedCardView : public BasePostedItem
{
	Q_OBJECT

public:
	PostedCardView(FeedHolder *parent, uint32_t feedId, const RsGxsGroupId& groupId, const RsGxsMessageId& messageId, bool isHome, bool autoUpdate);
	PostedCardView(FeedHolder *parent, uint32_t feedId, const RsGroupMetaData& group_meta, const RsGxsMessageId& post_id, bool isHome, bool autoUpdate);
	virtual ~PostedCardView();

protected:
	/* GxsGroupFeedItem */

	void setup() override;
	void fill() override;
    void doExpand(bool open) override {}
	void setComment(const RsGxsComment&) override;
	void setReadStatus(bool isNew, bool isUnread) override;
    void toggle() override {}
	void setCommentsSize(int comNb) override;
    void makeUpVote() override;
    void makeDownVote() override;
	void toggleNotes() override;

private:
	/** Qt Designer generated object */
	Ui::PostedCardView *ui;
};

//Q_DECLARE_METATYPE(RsPostedPost)

#endif
