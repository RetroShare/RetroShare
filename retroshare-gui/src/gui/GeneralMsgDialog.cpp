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
#include <QtGui>

#include "GeneralMsgDialog.h"
#include "gui/feeds/FeedHolder.h"

#include "gui/feeds/SubFileItem.h"
#include "gui/feeds/SubDestItem.h"

#include "rsiface/rstypes.h"
#include "rsiface/rspeers.h"
#include "rsiface/rsforums.h"
#include "rsiface/rschannels.h"
#include "rsiface/rsmsgs.h"

#include <iostream>

/** Constructor */
GeneralMsgDialog::GeneralMsgDialog(QWidget *parent, uint32_t type)
: QDialog (parent)
{
	/* Invoke the Qt Designer generated object setup routine */
	setupUi(this);

        connect(addButton, SIGNAL(clicked()), this, SLOT(newDestination()));
        connect(typeComboBox, SIGNAL(currentIndexChanged( int )), this, SLOT(updateGroupId()));

        connect(buttonBox, SIGNAL(accepted()), this, SLOT(sendMsg()));
        connect(buttonBox, SIGNAL(rejected()), this, SLOT(cancelMsg()));

	/* set the type to ...
	 * const uint32_t GMD_TYPE_MESSAGE_IDX = 0;
	 * const uint32_t GMD_TYPE_FORUM_IDX = 1;
	 * const uint32_t GMD_TYPE_CHANNEL_IDX = 2;
	 * const uint32_t GMD_TYPE_BLOG_IDX = 3;
	 * */

	typeComboBox->clear();
	typeComboBox->addItem("Message");
	typeComboBox->addItem("Forum");
	typeComboBox->addItem("Channel");
	typeComboBox->addItem("Blog");

	typeComboBox->setCurrentIndex(type);

	setAcceptDrops(true);
}

/* Dropping */

void GeneralMsgDialog::dragEnterEvent(QDragEnterEvent *event)
{
	/* print out mimeType */
	std::cerr << "GeneralMsgDialog::dragEnterEvent() Formats";
	std::cerr << std::endl;
	QStringList formats = event->mimeData()->formats();
	QStringList::iterator it;
	for(it = formats.begin(); it != formats.end(); it++)
	{
		std::cerr << "Format: " << (*it).toStdString();
		std::cerr << std::endl;
	}

	if (event->mimeData()->hasFormat("text/plain"))
	{
		std::cerr << "GeneralMsgDialog::dragEnterEvent() Accepting PlainText";
		std::cerr << std::endl;
		event->acceptProposedAction();
	}
	else if (event->mimeData()->hasUrls())
	{
		std::cerr << "GeneralMsgDialog::dragEnterEvent() Accepting Urls";
		std::cerr << std::endl;
		event->acceptProposedAction();
	}
	else
	{
		std::cerr << "GeneralMsgDialog::dragEnterEvent() No PlainText/Urls";
		std::cerr << std::endl;
	}
}

void GeneralMsgDialog::dropEvent(QDropEvent *event)
{
	std::cerr << "GeneralMsgDialog::dropEvent() Formats";
	std::cerr << std::endl;
	QStringList formats = event->mimeData()->formats();
	QStringList::iterator it;
	for(it = formats.begin(); it != formats.end(); it++)
	{
		std::cerr << "Format: " << (*it).toStdString();
		std::cerr << std::endl;
	}

	if (event->mimeData()->hasText())
	{
		std::cerr << "GeneralMsgDialog::dropEvent() Plain Text:";
		std::cerr << std::endl;
		std::cerr << event->mimeData()->text().toStdString();
		std::cerr << std::endl;
	}

	if (event->mimeData()->hasUrls())
	{
		std::cerr << "GeneralMsgDialog::dropEvent() Urls:";
		std::cerr << std::endl;

		QList<QUrl> urls = event->mimeData()->urls();
		QList<QUrl>::iterator uit;
		for(uit = urls.begin(); uit != urls.end(); uit++)
		{
			std::string localpath = uit->toLocalFile().toStdString();
			std::cerr << "Whole URL: " << uit->toString().toStdString();
			std::cerr << std::endl;
			std::cerr << "or As Local File: " << localpath;
			std::cerr << std::endl;

			if (localpath.size() > 0)
			{
				addAttachment(localpath);
			}
		}
	}
									       

	event->acceptProposedAction();
}

void GeneralMsgDialog::addAttachment(std::string path)
{
	/* add a SubFileItem to the attachment section */
	std::cerr << "GeneralMsgDialog::addAttachment()";
	std::cerr << std::endl;

	/* add widget in for new destination */
	SubFileItem *file = new SubFileItem("unknownHash", path, 12000);

	mAttachments.push_back(file);
	QLayout *layout = fileFrame->layout();
	layout->addWidget(file);

	return;

}


void GeneralMsgDialog::cancelMsg()
{
	std::cerr << "GeneralMsgDialog::cancelMsg()";
	std::cerr << std::endl;
	return;
}

void GeneralMsgDialog::addDestination(uint32_t type, std::string grpId, std::string inReplyTo)
{
	std::cerr << "GeneralMsgDialog::addDestination()";
	std::cerr << std::endl;

	/* add widget in for new destination */
	SubDestItem *dest = new SubDestItem(type, grpId, inReplyTo);

	mDestinations.push_back(dest);
	QLayout *layout = destFrame->layout();
	layout->addWidget(dest);

	return;
}

void GeneralMsgDialog::updateGroupId()
{
	std::cerr << "GeneralMsgDialog::updateGroupId()";
	std::cerr << std::endl;

	int idx = typeComboBox->currentIndex();

	destIdComboBox->clear();
	switch(idx)
	{
		case GMD_TYPE_MESSAGE_IDX:
		{
			/* add a list of friends */
			if (rsPeers)
			{
				std::list<std::string> friends;
				std::list<std::string>::iterator it;

				rsPeers->getFriendList(friends);
				for(it = friends.begin(); it != friends.end(); it++)
				{
					QString id = QString::fromStdString(*it);
					QString name = QString::fromStdString(rsPeers->getPeerName(*it));

					destIdComboBox->addItem(name, id);
				}
			}
		}
			break;
		case GMD_TYPE_FORUM_IDX:
		{
			/* add a list of publishable forums */
			if (rsForums)
			{
				std::list<ForumInfo> forumList;
				std::list<ForumInfo>::iterator it;

				rsForums->getForumList(forumList);
				for(it = forumList.begin(); it != forumList.end(); it++)
				{
					if (it->forumFlags & RS_DISTRIB_PUBLISH)
					{
						QString id = QString::fromStdString(it->forumId);
						QString name = QString::fromStdWString(it->forumName);

						destIdComboBox->addItem(name, id);
					}
				}
			}
		}
			break;
		case GMD_TYPE_CHANNEL_IDX:
		{
			/* add a list of publishable channels */
			if (rsChannels)
			{
				std::list<ChannelInfo> channelList;
				std::list<ChannelInfo>::iterator it;

				rsChannels->getChannelList(channelList);
				for(it = channelList.begin(); it != channelList.end(); it++)
				{
					if (it->channelFlags & RS_DISTRIB_PUBLISH)
					{
						QString id = QString::fromStdString(it->channelId);
						QString name = QString::fromStdWString(it->channelName);

						destIdComboBox->addItem(name, id);
					}
				}
			}
		}
			break;
		default:
		case GMD_TYPE_BLOG_IDX:
		{
			/* empty list  */
		}
			break;
	}

}

void GeneralMsgDialog::newDestination()
{
	/* get details from uint32_t type, std::string grpId, std::string inReplyTo)
	 */

	std::cerr << "GeneralMsgDialog::newDestination()";
	std::cerr << std::endl;

	int idx = typeComboBox->currentIndex();
	std::string grpId;

	if (destIdComboBox->currentIndex() >= 0)
	{
		QVariant qv = destIdComboBox->itemData(destIdComboBox->currentIndex());
		grpId = qv.toString().toStdString();
	}

	switch(idx)
	{
		case GMD_TYPE_MESSAGE_IDX:
			addDestination(FEEDHOLDER_MSG_MESSAGE, grpId, "");
			break;
		case GMD_TYPE_FORUM_IDX:
			addDestination(FEEDHOLDER_MSG_FORUM, grpId, "");
			break;
		case GMD_TYPE_CHANNEL_IDX:
			addDestination(FEEDHOLDER_MSG_CHANNEL, grpId, "");
			break;
		case GMD_TYPE_BLOG_IDX:
			addDestination(FEEDHOLDER_MSG_BLOG, "", "");
			break;
		default:
			break;
	}
}


void GeneralMsgDialog::sendMsg()
{
	std::cerr << "GeneralMsgDialog::sendMsg()";
	std::cerr << std::endl;

	/* construct message bits */
	std::wstring subject = subjectEdit->text().toStdWString();
	std::wstring msg     = msgEdit->toPlainText().toStdWString();

	std::list<FileInfo> files;

	std::list<SubDestItem *>::iterator it;
	std::list<SubFileItem *>::iterator fit;

	for(fit = mAttachments.begin(); fit != mAttachments.end(); fit++)
	{
		if (!(*fit)->isHidden())
		{
			FileInfo fi;
			fi.hash = (*fit)->FileHash();
			fi.fname = (*fit)->FileName();
			fi.size = (*fit)->FileSize();

			files.push_back(fi);
		}
	}

	/* iterate through each mDestinations that is visible, and send */
	for(it = mDestinations.begin(); it != mDestinations.end(); it++)
	{
		if (!(*it)->isHidden())
		{
			sendMessage((*it)->DestType(), (*it)->DestGroupId(), 
				(*it)->DestInReplyTo(), subject, msg, files);
		}
	}
}

void GeneralMsgDialog::sendMessage(uint32_t type, std::string grpId, std::string inReplyTo, 
				std::wstring subject, std::wstring msg, std::list<FileInfo> &files)
{
	std::cerr << "GeneralMsgDialog::sendMessage() Type: " << type << " GroupId: " << grpId;
	std::cerr << std::endl;

	/* construct specific messages */
	switch(type)
	{
		case FEEDHOLDER_MSG_MESSAGE:
		{
			if (rsMsgs)
			{
				/* construct a message */
				MessageInfo mi;
	
				mi.title = subject;
				mi.msg =   msg;
				mi.files = files;
				mi.msgto.push_back(grpId);
	
				rsMsgs->MessageSend(mi);
			}
		}
			break;
		case FEEDHOLDER_MSG_FORUM:
		{
			/* rsForum */
			if (rsForums)
			{
				ForumMsgInfo msgInfo;
				
				msgInfo.forumId = grpId;
				msgInfo.threadId = "";
				msgInfo.parentId = inReplyTo;
				msgInfo.msgId = "";
				
				msgInfo.title = subject;
				msgInfo.msg = msg;
				
				rsForums->ForumMessageSend(msgInfo);
			}
		}
			break;
		case FEEDHOLDER_MSG_CHANNEL:
		{
			/* rsChannels */
			if (rsChannels)
			{
				ChannelMsgInfo msgInfo;
				
				msgInfo.channelId = grpId;
				msgInfo.msgId = "";
				
				msgInfo.subject = subject;
				msgInfo.msg = msg;
				msgInfo.files = files;
				
				rsChannels->ChannelMessageSend(msgInfo);
			}
		}

			break;
		case FEEDHOLDER_MSG_BLOG:

			break;
		default:

			break;
	}
}



			




