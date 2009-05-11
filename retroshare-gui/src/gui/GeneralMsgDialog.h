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

#ifndef _GENERAL_MSG_DIALOG_H
#define _GENERAL_MSG_DIALOG_H

#include "ui_GeneralMsgDialog.h"
#include <stdint.h>

class SubDestItem;
class SubFileItem;
class FileInfo;

const uint32_t GMD_TYPE_MESSAGE_IDX = 0;
const uint32_t GMD_TYPE_FORUM_IDX = 1;
const uint32_t GMD_TYPE_CHANNEL_IDX = 2;
const uint32_t GMD_TYPE_BLOG_IDX = 3;

class GeneralMsgDialog : public QDialog, private Ui::GeneralMsgDialog
{
  Q_OBJECT

public:
  /** Default Constructor */
  GeneralMsgDialog(QWidget *parent = 0, uint32_t type = 0);
  /** Default Destructor */

	void addAttachment(std::string path);
	void addAttachment(std::string hash, std::string fname, uint64_t size, 
						bool local, std::string srcId);

	void addDestination(uint32_t type, std::string grpId, std::string inReplyTo);
	void setMsgType(uint32_t type);

protected:
virtual void dragEnterEvent(QDragEnterEvent *event);
virtual void dropEvent(QDropEvent *event);

private slots:
	void addExtraFile();
	void checkAttachmentReady();
	void updateGroupId();
	void newDestination();
	void cancelMsg();
	void sendMsg();

private:

void parseRsFileListAttachments(std::string attachList);

void sendMessage(uint32_t type, std::string grpId, std::string inReplyTo, 
         std::wstring subject, std::wstring msg, std::list<FileInfo> &files);


	/* maps of files and destinations */
	std::list<SubDestItem *> mDestinations;
	std::list<SubFileItem *> mAttachments;

	bool mCheckAttachment;
};



#endif

