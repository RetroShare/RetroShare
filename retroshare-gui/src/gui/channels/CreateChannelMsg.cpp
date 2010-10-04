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

#include <QDragEnterEvent>
#include <QUrl>
#include <QFileDialog>
#include <QTimer>
#include <QMessageBox>
#include <QBuffer>

#include "CreateChannelMsg.h"
#include "gui/feeds/SubFileItem.h"

#include <retroshare/rschannels.h>
#include <retroshare/rsfiles.h>

#include <iostream>

/** Constructor */
CreateChannelMsg::CreateChannelMsg(std::string cId)
: QDialog (NULL), mChannelId(cId) ,mCheckAttachment(true)
{
	/* Invoke the Qt Designer generated object setup routine */
	setupUi(this);

	setAttribute ( Qt::WA_DeleteOnClose, true );

	connect(buttonBox, SIGNAL(accepted()), this, SLOT(sendMsg()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(cancelMsg()));

	connect(addFileButton, SIGNAL(clicked() ), this , SLOT(addExtraFile()));
	connect(addfilepushButton, SIGNAL(clicked() ), this , SLOT(addExtraFile()));	
	connect(addThumbnailButton, SIGNAL(clicked() ), this , SLOT(addThumbnail()));
	
    //buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

	setAcceptDrops(true);
	
	newChannelMsg();
}

/* Dropping */

void CreateChannelMsg::dragEnterEvent(QDragEnterEvent *event)
{
	/* print out mimeType */
	std::cerr << "CreateChannelMsg::dragEnterEvent() Formats";
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
		std::cerr << "CreateChannelMsg::dragEnterEvent() Accepting PlainText";
		std::cerr << std::endl;
		event->acceptProposedAction();
	}
	else if (event->mimeData()->hasUrls())
	{
		std::cerr << "CreateChannelMsg::dragEnterEvent() Accepting Urls";
		std::cerr << std::endl;
		event->acceptProposedAction();
	}
	else if (event->mimeData()->hasFormat("application/x-rsfilelist"))
	{
		std::cerr << "CreateChannelMsg::dragEnterEvent() accepting Application/x-qabs...";
		std::cerr << std::endl;
		event->acceptProposedAction();
	}
	else
	{
		std::cerr << "CreateChannelMsg::dragEnterEvent() No PlainText/Urls";
		std::cerr << std::endl;
	}
}

void CreateChannelMsg::dropEvent(QDropEvent *event)
{
	if (!(Qt::CopyAction & event->possibleActions()))
	{
		std::cerr << "CreateChannelMsg::dropEvent() Rejecting uncopyable DropAction";
		std::cerr << std::endl;

		/* can't do it */
		return;
	}

	std::cerr << "CreateChannelMsg::dropEvent() Formats";
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
		std::cerr << "CreateChannelMsg::dropEvent() Plain Text:";
		std::cerr << std::endl;
		std::cerr << event->mimeData()->text().toStdString();
		std::cerr << std::endl;
	}

	if (event->mimeData()->hasUrls())
	{
		std::cerr << "CreateChannelMsg::dropEvent() Urls:";
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
	else if (event->mimeData()->hasFormat("application/x-rsfilelist"))
	{
		std::cerr << "CreateChannelMsg::dropEvent() Application/x-rsfilelist";
		std::cerr << std::endl;


		QByteArray data = event->mimeData()->data("application/x-rsfilelist");
		std::cerr << "Data Len:" << data.length();
		std::cerr << std::endl;
		std::cerr << "Data is:" << data.data();
		std::cerr << std::endl;

		std::string newattachments(data.data());
		parseRsFileListAttachments(newattachments);
	}
									       

	event->setDropAction(Qt::CopyAction);
	event->accept();
}

void CreateChannelMsg::parseRsFileListAttachments(std::string attachList)
{
	/* split into lines */
	QString input = QString::fromStdString(attachList);

	QStringList attachItems = input.split("\n");
	QStringList::iterator it;
	QStringList::iterator it2;

	for(it = attachItems.begin(); it != attachItems.end(); it++)
	{
		std::cerr << "CreateChannelMsg::parseRsFileListAttachments() Entry: ";

		QStringList parts = (*it).split("/");

		bool ok = false;
		quint64     qsize = 0;

		std::string fname;
		std::string hash;
		uint64_t    size = 0;
		std::string source;

		int i = 0;
		for(it2 = parts.begin(); it2 != parts.end(); it2++, i++)
		{
			std::cerr << "\"" << it2->toStdString() << "\" ";
			switch(i)
			{
				case 0:
					fname = it2->toStdString();
					break;
				case 1:
					hash = it2->toStdString();
					break;
				case 2:
					qsize = it2->toULongLong(&ok, 10);
					size = qsize;
					break;
				case 3:
					source = it2->toStdString();
					break;
			}
		}

		std::cerr << std::endl;

		std::cerr << "\tfname: " << fname << std::endl;
		std::cerr << "\thash: " << hash << std::endl;
		std::cerr << "\tsize: " << size << std::endl;
		std::cerr << "\tsource: " << source << std::endl;

		/* basic error checking */
		if ((ok) && (hash.size() == 40))
		{
			std::cerr << "Item Ok" << std::endl;
			if (source == "Local")
			{
				addAttachment(hash, fname, size, true, "");
			}
			else
			{
				// TEMP NOT ALLOWED UNTIL FT WORKING.
				addAttachment(hash, fname, size, false, source);
			}

		}
		else
		{
			std::cerr << "Error Decode: Hash size: " << hash.size() << std::endl;
		}

	}
}


void CreateChannelMsg::addAttachment(std::string hash, std::string fname, uint64_t size, bool local, std::string srcId)
{
	/* add a SubFileItem to the attachment section */
	std::cerr << "CreateChannelMsg::addAttachment()";
	std::cerr << std::endl;

	/* add widget in for new destination */

	uint32_t flags = SFI_TYPE_CHANNEL;
	if (local)
	{
		flags |= SFI_STATE_LOCAL;
	}
	else
	{
		flags |= SFI_STATE_REMOTE;
		// TMP REMOVED REMOTE ADD FOR DEMONSTRATOR
		return;
	}

	SubFileItem *file = new SubFileItem(hash, fname, "", size, flags, srcId); // destroyed when fileFrame (this subfileitem) is destroyed

	mAttachments.push_back(file);
	QLayout *layout = fileFrame->layout();
	layout->addWidget(file);

	if (mCheckAttachment)
	{
		checkAttachmentReady();
	}

	return;
}


void CreateChannelMsg::addExtraFile()
{
	/* add a SubFileItem to the attachment section */
	std::cerr << "CreateChannelMsg::addExtraFile() opening file dialog";
	std::cerr << std::endl;

	// select a file
	QString qfile = QFileDialog::getOpenFileName(this, tr("Add Extra File"), "", "", 0,
				QFileDialog::DontResolveSymlinks);
	std::string filePath = qfile.toStdString();
	if (filePath != "")
	{
		addAttachment(filePath);
	}
}


void CreateChannelMsg::addAttachment(std::string path)
{
	/* add a SubFileItem to the attachment section */
	std::cerr << "CreateChannelMsg::addAttachment()";
	std::cerr << std::endl;

	/* add widget in for new destination */
	uint32_t flags =  SFI_TYPE_CHANNEL | SFI_STATE_EXTRA;

	// channels creates copy of file into channels directory and shares this

	FileInfo fInfo;
	rsChannels->channelExtraFileHash(path, mChannelId, fInfo);

	// file is not innitial
	SubFileItem *file = new SubFileItem(fInfo.hash, fInfo.fname, fInfo.path, fInfo.size,
			flags, mChannelId); // destroyed when fileFrame (this subfileitem) is destroyed

	mAttachments.push_back(file);
	QLayout *layout = fileFrame->layout();
	layout->addWidget(file);

	if (mCheckAttachment)
	{
		checkAttachmentReady();
	}

	return;

}

void CreateChannelMsg::checkAttachmentReady()
{
	std::list<SubFileItem *>::iterator fit;

	mCheckAttachment = false;

	for(fit = mAttachments.begin(); fit != mAttachments.end(); fit++)
	{
		if (!(*fit)->isHidden())
		{
			if (!(*fit)->ready())
			{
				/* ensure file is hashed or file will be hashed, thus
				 * recognized by librs but not correctly by gui (can't
				 * formally remove it)
				 */
				buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
				buttonBox->button(QDialogButtonBox::Cancel)->setEnabled(false);
				break;


				return;
			}
		}
	}

	if (fit == mAttachments.end())
	{
		buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
		buttonBox->button(QDialogButtonBox::Cancel)->setEnabled(true);
	}

	/* repeat... */
	int msec_rate = 1000;
	QTimer::singleShot( msec_rate, this, SLOT(checkAttachmentReady(void)));
}


void CreateChannelMsg::cancelMsg()
{
	std::cerr << "CreateChannelMsg::cancelMsg() :"
			  << "Deleting EXTRA attachments" << std::endl;

	std::cerr << std::endl;

	std::list<SubFileItem* >::const_iterator it;

	for(it = mAttachments.begin(); it != mAttachments.end(); it++)
		rsChannels->channelExtraFileRemove((*it)->FileHash(), mChannelId);

	close();
	return;
}

void CreateChannelMsg::newChannelMsg()
{

	if (!rsChannels)
		return;

	ChannelInfo ci;
	if (!rsChannels->getChannelInfo(mChannelId, ci))
	{

		return;
	}
			
	channelName->setText(QString::fromStdWString(ci.channelName));



}


void CreateChannelMsg::sendMsg()
{
	std::cerr << "CreateChannelMsg::sendMsg()";
	std::cerr << std::endl;

	/* construct message bits */
	std::wstring subject = subjectEdit->text().toStdWString();
	std::wstring msg     = msgEdit->toPlainText().toStdWString();

	std::list<FileInfo> files;

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

			/* commence downloads - if we don't have the file */

			if (!(*fit)->done())
			{
				if ((*fit)->ready())
				{
					(*fit)->download();
				}
			// Skips unhashed files.
			}
		}
	}

	sendMessage(subject, msg, files);

}

void CreateChannelMsg::sendMessage(std::wstring subject, std::wstring msg, std::list<FileInfo> &files)
{
	QString name = subjectEdit->text();

	if(name.isEmpty())
	{	/* error message */
		QMessageBox::warning(this, tr("RetroShare"),
                   tr("Please add a Subject"),
                   QMessageBox::Ok, QMessageBox::Ok);
                   
		return; //Don't add  an empty Subject!!
	}
	else






	/* rsChannels */
	if (rsChannels)
	{
		ChannelMsgInfo msgInfo;
				
		msgInfo.channelId = mChannelId;
		msgInfo.msgId = "";
				
		msgInfo.subject = subject;
		msgInfo.msg = msg;
		msgInfo.files = files;

		QByteArray ba;
		QBuffer buffer(&ba);

		if(!picture.isNull()){
			// send chan image

			buffer.open(QIODevice::WriteOnly);
			picture.save(&buffer, "PNG"); // writes image into ba in PNG format
			msgInfo.thumbnail.image_thumbnail = (unsigned char*) ba.data();
			msgInfo.thumbnail.im_thumbnail_size = ba.size();
		}
				
		rsChannels->ChannelMessageSend(msgInfo);
	}
			
	close();
  return;

}

void CreateChannelMsg::addThumbnail()
{
        QString fileName = QFileDialog::getOpenFileName(this, tr("Load File"), QDir::homePath(), tr("Pictures (*.png *.xpm *.jpg)"));
	if(!fileName.isEmpty())
	{
		picture = QPixmap(fileName).scaled(156,107, Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
		
		// to show the selected 
		thumbnail_label->setPixmap(picture);

		std::cerr << "Sending Thumbnail image down the pipe" << std::endl ;

		// send Thumbnail down the pipe for other peers to get it.
		QByteArray ba;
		QBuffer buffer(&ba);
		buffer.open(QIODevice::WriteOnly);
		picture.save(&buffer, "PNG"); // writes image into ba in PNG format

		std::cerr << "Image size = " << ba.size() << std::endl ;

		//updateThumbnail() ;
	}
}
