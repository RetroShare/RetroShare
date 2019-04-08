/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2008 Robert Fernie
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/

#ifndef _CREATE_GXSFORUM_MSG_DIALOG_H
#define _CREATE_GXSFORUM_MSG_DIALOG_H

#include "ui_CreateGxsForumMsg.h"

#include "util/TokenQueue.h"

#include <retroshare/rsgxsforums.h>
#include <retroshare/rsgxscircles.h>

class UIStateHelper;

class CreateGxsForumMsg : public QDialog, public TokenResponse
{
	Q_OBJECT

public:
        CreateGxsForumMsg(const RsGxsGroupId &fId, const RsGxsMessageId &pId, const RsGxsMessageId &moId, const RsGxsId &posterId = RsGxsId());
	~CreateGxsForumMsg();

	void newMsg(); /* cleanup */
	virtual void loadRequest(const TokenQueue *queue, const TokenRequest &req);
	void insertPastedText(const QString& msg) ;
	void setSubject(const QString& msg);

private slots:
	void fileHashingFinished(QList<HashedFile> hashedFiles);
	/* actions to take.... */
	void createMsg();

	void smileyWidgetForums();
	void addSmileys();
	void addFile();
	void addPicture();
	void checkLength();
	void reject();

protected:
	void closeEvent (QCloseEvent * event);
    
private:
	void loadFormInformation();

	void loadForumInfo(const uint32_t &token);
	void loadParentMsg(const uint32_t &token);
	void loadOrigMsg(const uint32_t &token);
    	void loadForumCircleInfo(const uint32_t &token);

	 RsGxsGroupId mForumId;
	 RsGxsCircleId mCircleId ;
	 RsGxsMessageId mParentId;
	 RsGxsMessageId mOrigMsgId;
	 RsGxsId mPosterId;

	bool mParentMsgLoaded;
	bool mOrigMsgLoaded;
	bool mForumMetaLoaded;
	bool mForumCircleLoaded ;

	RsGxsForumMsg mParentMsg;
	RsGxsForumMsg mOrigMsg;
	RsGroupMetaData mForumMeta;
	RsGxsCircleGroup mForumCircleData ;

	TokenQueue *mForumQueue;
	TokenQueue *mCirclesQueue;
    
	UIStateHelper *mStateHelper;

	/** Qt Designer generated object */
	Ui::CreateGxsForumMsg ui;
};

#endif
