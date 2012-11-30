/*
 * Retroshare Posted Comments
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

#ifndef MRK_POSTED_COMMENTS_H
#define MRK_POSTED_COMMENTS_H

//#include "gui/mainpage.h"
#include "ui_PostedComments.h"

#include <retroshare/rsposted.h>

#include <map>

#include "util/TokenQueue.h"

class PostedComments: public QWidget, public TokenResponse
{
  Q_OBJECT

public:
	PostedComments(QWidget *parent = 0);

public slots:

        void loadComments(const RsPostedPost& );

private:
        void loadRequest(const TokenQueue *queue, const TokenRequest &req);
        void setUpPostFrame();

        RsPostedPost mCurrentPost;

	/* UI - from Designer */
	Ui::PostedComments ui;

};

#endif

