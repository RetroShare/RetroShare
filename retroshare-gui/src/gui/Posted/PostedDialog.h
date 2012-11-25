/*
 * Retroshare Posted Dialog
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

#ifndef MRK_POSTED_DIALOG_H
#define MRK_POSTED_DIALOG_H

#include "retroshare-gui/mainpage.h"
#include "ui_PostedDialog.h"

#include <retroshare/rsposted.h>

#include <map>

class CommentHolder
{
public:

    /*!
     * This should be used for loading comments of a message on a main comment viewing page
     * @param msgId the message id for which comments will be requested
     */
    virtual void commentLoad(const RsPostedPost&) = 0;
};

class PostedListDialog;
class PostedComments;

class PostedDialog : public MainPage, public CommentHolder
{
  Q_OBJECT

public:
	PostedDialog(QWidget *parent = 0);
        void commentLoad(const RsPostedPost &);

private:

	PostedListDialog *mPostedList;
	PostedComments *mPostedComments;

	/* UI - from Designer */
	Ui::PostedDialog ui;

};

#endif

