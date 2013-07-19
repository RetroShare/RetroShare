/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2013 Robert Fernie
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

#ifndef _CREATE_GXSCHANNEL_MSG_H
#define _CREATE_GXSCHANNEL_MSG_H

#include "ui_CreateGxsChannelMsg.h"
#include "util/TokenQueue.h"
#include <retroshare/rsgxschannels.h>

#ifdef CHANNELS_FRAME_CATCHER
#include "util/framecatcher.h"
#endif

class SubFileItem;

class CreateGxsChannelMsg : public QDialog, public TokenResponse, private Ui::CreateGxsChannelMsg
{
	Q_OBJECT

public:
	/** Default Constructor */
	CreateGxsChannelMsg(std::string cId);

	/** Default Destructor */
	~CreateGxsChannelMsg();

	void addAttachment(const std::string &path);
	void addAttachment(const std::string &hash, const std::string &fname, uint64_t size, bool local, const std::string &srcId);

	void newChannelMsg();

	QPixmap picture;

	// overload from TokenResponse
	virtual void loadRequest(const TokenQueue*, const TokenRequest&);

protected:
	virtual void dragEnterEvent(QDragEnterEvent *event);
	virtual void dropEvent(QDropEvent *event);

private slots:
	void addExtraFile();
	void checkAttachmentReady();

	void cancelMsg();
	void sendMsg();
	void pasteLink() ;
	void contextMenu(QPoint) ;

	void addThumbnail();
	void allowAutoMediaThumbNail(bool);

private:
	void loadChannelInfo(const uint32_t &token);
	void saveChannelInfo(const RsGroupMetaData &group);

	void parseRsFileListAttachments(const std::string &attachList);
	void sendMessage(const std::string &subject, const std::string &msg, const std::list<RsGxsFile> &files);
	bool setThumbNail(const std::string& path, int frame);

	std::string mChannelId;
	RsGroupMetaData mChannelMeta;
	bool mChannelMetaLoaded;

	std::list<SubFileItem *> mAttachments;

	bool mCheckAttachment;
	bool mAutoMediaThumbNail;

	TokenQueue *mChannelQueue;

#ifdef CHANNELS_FRAME_CATCHER
	framecatcher* fCatcher;
#endif
};

#endif
