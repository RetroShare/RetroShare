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

#ifndef _CREATECHANNELMSG_H
#define _CREATECHANNELMSG_H

#include "ui_CreateChannelMsg.h"
#include <stdint.h>

class SubFileItem;
class FileInfo;

class CreateChannelMsg : public QDialog, private Ui::CreateChannelMsg
{
  Q_OBJECT

public:
  /** Default Constructor */
  CreateChannelMsg(std::string cId);
  /** Default Destructor */

	void addAttachment(std::string path);
	void addAttachment(std::string hash, std::string fname, uint64_t size, 
						bool local, std::string srcId);

	void newChannelMsg();

protected:
virtual void dragEnterEvent(QDragEnterEvent *event);
virtual void dropEvent(QDropEvent *event);

private slots:
	void addExtraFile();
	void checkAttachmentReady();

	void cancelMsg();
	void sendMsg();

private:

  void parseRsFileListAttachments(std::string attachList);

  void sendMessage(std::wstring subject, std::wstring msg, std::list<FileInfo> &files);
  
  std::string mChannelId;

	std::list<SubFileItem *> mAttachments;

	bool mCheckAttachment;
};



#endif

