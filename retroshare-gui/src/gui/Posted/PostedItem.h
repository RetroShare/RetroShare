/*
 * Retroshare Posted Plugin.
 *
 * Copyright 2012-2012 by Robert Fernie.
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

#ifndef MRK_POSTED_POSTED_ITEM_H
#define MRK_POSTED_POSTED_ITEM_H

#include "ui_PostedItem.h"

#include <retroshare/rsposted.h>

class RsPostedPost;
class PostedItem;

class PostedHolder
{
	public:

    virtual void showComments(const RsGxsMessageId& threadId) = 0;
};


class PostedItem : public QWidget, private Ui::PostedItem
{
  Q_OBJECT

public:
	PostedItem(PostedHolder *parent, const RsPostedPost &item);

        RsPostedPost getPost() const;

private slots:
        void loadComments();

private:
	uint32_t     mType;
        bool mSelected;

        std::string mThreadId;
        PostedHolder *mPostHolder;
        RsPostedPost mPost;

};


#endif

