/*
 * Retroshare Comment Container Dialog
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

#ifndef MRK_COMMENT_CONTAINER_DIALOG_H
#define MRK_COMMENT_CONTAINER_DIALOG_H

#include "retroshare-gui/mainpage.h"
#include "ui_GxsCommentContainer.h"

#include <retroshare/rsgxscommon.h>
#include <retroshare/rstokenservice.h>

#include <map>

class GxsCommentHeader
{
public:

    /*!
     * This should be used for loading comments of a message on a main comment viewing page
     * @param msgId the message id for which comments will be requested
     */
    virtual void commentLoad(const RsGxsGroupId &grpId, const RsGxsMessageId &msgId) = 0;
};

class GxsServiceDialog;


class GxsCommentContainer : public MainPage
{
  Q_OBJECT

public:
	GxsCommentContainer(QWidget *parent = 0);
	void setup();

        void commentLoad(const RsGxsGroupId &grpId, const RsGxsMessageId &msgId);

        virtual GxsServiceDialog *createServiceDialog() = 0;
        virtual QString getServiceName() = 0;
        virtual RsTokenService *getTokenService() = 0;
        virtual RsGxsCommentService *getCommentService() = 0;
        virtual GxsCommentHeader *createHeaderWidget() = 0;

private:

	GxsServiceDialog *mServiceDialog;

	/* UI - from Designer */
	Ui::GxsCommentContainer ui;

};



class GxsServiceDialog
{

public:
	GxsServiceDialog(GxsCommentContainer *container)
	:mContainer(container) { return; }

virtual ~GxsServiceDialog() { return; }

	void commentLoad(const RsGxsGroupId &grpId, const RsGxsMessageId &msgId)
	{
		mContainer->commentLoad(grpId, msgId);
	}
private:
	GxsCommentContainer *mContainer;

};



#endif

