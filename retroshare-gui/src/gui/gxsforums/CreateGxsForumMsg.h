/*******************************************************************************
 * retroshare-gui/src/gui/gxsforums/CreateGxsForumMsg.h                        *
 *                                                                             *
 * Copyright 2013 Robert Fernie        <retroshare.project@gmail.com>          *
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
        CreateGxsForumMsg(const RsGxsGroupId &fId, const RsGxsMessageId &pId, const RsGxsMessageId &moId, const RsGxsId &posterId = RsGxsId(),bool isModerating=false);
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
    bool mIsModerating;			// means that the msg has a orig author Id that is not the Id of the author

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
