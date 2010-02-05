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

#include "CreateForumMsg.h"

#include <gui/settings/rsharesettings.h>

#include <QtGui>
#include <QFileDialog>
#include <QString>
#include <QTextStream>
#include <QHashIterator>
#include <QDesktopServices>
#include <QTextCodec>
#include <QTextEdit>
#include <QTextCursor>
#include <QTextList>
#include <QString>
#include <QtDebug>
#include <QIcon>
#include <QPixmap>

#include "rsiface/rsforums.h"
#include "rsiface/rsfiles.h"

#include "gui/feeds/AttachFileItem.h"

#include <sstream>


/** Constructor */
CreateForumMsg::CreateForumMsg(std::string fId, std::string pId)
: QMainWindow(NULL), mForumId(fId), mParentId(pId)
{
  /* Invoke the Qt Designer generated object setup routine */
  ui.setupUi(this);
  
  RshareSettings config;
  config.loadWidgetInformation(this);
  
  // connect up the buttons.
  connect( ui.postmessage_action, SIGNAL( triggered (bool) ), this, SLOT( createMsg( ) ) );
  connect( ui.close_action, SIGNAL( triggered (bool) ), this, SLOT( cancelMsg( ) ) );
  connect( ui.emoticonButton, SIGNAL(clicked()), this, SLOT(smileyWidgetForums()));
  connect( ui.attachFileButton, SIGNAL(clicked() ), this , SLOT(addFile()));


  newMsg();
  
  loadEmoticonsForums();

}


void  CreateForumMsg::newMsg()
{
	/* clear all */
	ForumInfo fi;
	if (rsForums->getForumInfo(mForumId, fi))
	{
		ForumMsgInfo msg;

		QString name = QString::fromStdWString(fi.forumName);
		QString subj;
		if ((mParentId != "") && (rsForums->getForumMessage(
				mForumId, mParentId, msg)))
		{
			name += " In Reply to: ";
			name += QString::fromStdWString(msg.title);
			
			QString text = QString::fromStdWString(msg.title);
			
			if (text.startsWith("Re:", Qt::CaseInsensitive))
			{
			subj = QString::fromStdWString(msg.title);
			}
			else
			{
			subj = "Re: " + QString::fromStdWString(msg.title);
			}
			
		}

		ui.forumName->setText(name);
		ui.forumSubject->setText(subj);
		
		if (!ui.forumSubject->text().isEmpty())
		{
      ui.forumMessage->setFocus();
    }
		else
		{
      ui.forumSubject->setFocus();
		}

		if (fi.forumFlags & RS_DISTRIB_AUTHEN_REQ)
		{
			ui.signBox->setChecked(true);
			//ui.signBox->setEnabled(false);
			// For Testing.
			ui.signBox->setEnabled(true);
		}
		else
		{
			ui.signBox->setEnabled(true);
		}
	}

	ui.forumMessage->setText("");
}

void  CreateForumMsg::createMsg()
{
	QString name = ui.forumSubject->text();
	QString desc = ui.forumMessage->toHtml();


	ForumMsgInfo msgInfo;

	msgInfo.forumId = mForumId;
	msgInfo.threadId = "";
	msgInfo.parentId = mParentId;
	msgInfo.msgId = "";

	msgInfo.title = name.toStdWString();
	msgInfo.msg = desc.toStdWString();
        msgInfo.msgflags = 0;

	if (ui.signBox->isChecked())
	{
		msgInfo.msgflags = RS_DISTRIB_AUTHEN_REQ;
	}


	if ((msgInfo.msg == L"") && (msgInfo.title == L""))
		return; /* do nothing */

	rsForums->ForumMessageSend(msgInfo);

	close();
	return;
}


void CreateForumMsg::cancelMsg()
{
	close();
	return;
	        
	RshareSettings config;
	config.saveWidgetInformation(this);
}

void CreateForumMsg::loadEmoticonsForums()
{
	QString sm_codes;
	#if defined(Q_OS_WIN32)
	QFile sm_file(QApplication::applicationDirPath() + "/emoticons/emotes.acs");
	#else
	QFile sm_file(QString(":/smileys/emotes.acs"));
	#endif
	if(!sm_file.open(QIODevice::ReadOnly))
	{
		std::cerr << "Could not open resouce file :/emoticons/emotes.acs" << endl ;
		return ;
	}
	sm_codes = sm_file.readAll();
	sm_file.close();
	sm_codes.remove("\n");
	sm_codes.remove("\r");
	int i = 0;
	QString smcode;
	QString smfile;
	while(sm_codes[i] != '{')
	{
		i++;

	}
	while (i < sm_codes.length()-2)
	{
		smcode = "";
		smfile = "";
		while(sm_codes[i] != '\"')
		{
			i++;
		}
		i++;
		while (sm_codes[i] != '\"')
		{
			smcode += sm_codes[i];
			i++;

		}
		i++;

		while(sm_codes[i] != '\"')
		{
			i++;
		}
		i++;
		while(sm_codes[i] != '\"' && sm_codes[i+1] != ';')
		{
			smfile += sm_codes[i];
			i++;
		}
		i++;
		if(!smcode.isEmpty() && !smfile.isEmpty())
			#if defined(Q_OS_WIN32)
		    smileys.insert(smcode, smfile);
	        #else
			smileys.insert(smcode, ":/"+smfile);
			#endif
	}
}

void CreateForumMsg::smileyWidgetForums()
{
	qDebug("MainWindow::smileyWidget()");
	QWidget *smWidget = new QWidget(this , Qt::Popup );
	smWidget->setWindowTitle("Emoticons");
	smWidget->setWindowIcon(QIcon(QString(":/images/rstray3.png")));
	//smWidget->setFixedSize(256,256);

	smWidget->setBaseSize( 4*24, (smileys.size()/4)*24  );

    //Warning: this part of code was taken from kadu instant messenger;
    //         It was EmoticonSelector::alignTo(QWidget* w) function there
    //         comments are Polish, I dont' know how does it work...
    // oblicz pozycj? widgetu do kt?rego r?wnamy
    QWidget* w = ui.emoticonButton;
    QPoint w_pos = w->mapToGlobal(QPoint(0,0));
    // oblicz rozmiar selektora
    QSize e_size = smWidget->sizeHint();
    // oblicz rozmiar pulpitu
    QSize s_size = QApplication::desktop()->size();
    // oblicz dystanse od widgetu do lewego brzegu i do prawego
    int l_dist = w_pos.x();
    int r_dist = s_size.width() - (w_pos.x() + w->width());
    // oblicz pozycj? w zale?no?ci od tego czy po lewej stronie
    // jest wi?cej miejsca czy po prawej
    int x;
    if (l_dist >= r_dist)
        x = w_pos.x() - e_size.width();
    else
        x = w_pos.x() + w->width();
    // oblicz pozycj? y - centrujemy w pionie
    int y = w_pos.y() + w->height()/2 - e_size.height()/2;
    // je?li wychodzi poza doln? kraw?d? to r?wnamy do niej
    if (y + e_size.height() > s_size.height())
        y = s_size.height() - e_size.height();
    // je?li wychodzi poza g?rn? kraw?d? to r?wnamy do niej
    if (y < 0)
         y = 0;
    // ustawiamy selektor na wyliczonej pozycji
    smWidget->move(x, y);

	x = 0;
    y = 0;

	QHashIterator<QString, QString> i(smileys);
	while(i.hasNext())
	{
		i.next();
		QPushButton *smButton = new QPushButton("", smWidget);
		smButton->setGeometry(x*24, y*24, 24,24);
		smButton->setIconSize(QSize(24,24));
		smButton->setIcon(QPixmap(i.value()));
		smButton->setToolTip(i.key());
		//smButton->setFixedSize(24,24);
		++x;
		if(x > 4)
		{
			x = 0;
			y++;
		}
		connect(smButton, SIGNAL(clicked()), this, SLOT(addSmileys()));
        connect(smButton, SIGNAL(clicked()), smWidget, SLOT(close()));
	}

	smWidget->show();
}

void CreateForumMsg::addSmileys()
{
	ui.forumMessage->setText(ui.forumMessage->toHtml() + qobject_cast<QPushButton*>(sender())->toolTip().split("|").first());
}

void CreateForumMsg::addFile()
{
	// select a file
	QString qfile = QFileDialog::getOpenFileName(this, tr("Add Extra File"), "", "", 0,
				QFileDialog::DontResolveSymlinks);
	std::string filePath = qfile.toStdString();
	if (filePath != "")
	{
	    CreateForumMsg::addAttachment(filePath);
	}
}

void CreateForumMsg::addAttachment(std::string filePath) {
	    /* add a AttachFileItem to the attachment section */
	    std::cerr << "CreateForumMsg::addFile() hashing file.";
	    std::cerr << std::endl;

	    /* add widget in for new destination */
	    AttachFileItem *file = new AttachFileItem(filePath);
	    //file->
	    	    
	    ui.verticalLayout->addWidget(file, 1, 0);

	    //when the file is local or is finished hashing, call the fileHashingFinished method to send a forum message
	    if (file->getState() == AFI_STATE_LOCAL) {
		fileHashingFinished(file);
	    } else {
		QObject::connect(file,SIGNAL(fileFinished(AttachFileItem *)),this, SLOT(fileHashingFinished(AttachFileItem *))) ;
	    }
}

void CreateForumMsg::fileHashingFinished(AttachFileItem* file) {
	std::cerr << "CreateForumMsg::fileHashingFinished() started.";
	std::cerr << std::endl;

	//check that the file is ok tos end
	if (file->getState() == AFI_STATE_ERROR) {
	#ifdef CHAT_DEBUG
		    std::cerr << "CreateForumMsg::fileHashingFinished error file is not hashed.";
	#endif
	    return;
	}

	//convert fileSize from uint_64 to string for html link
	char fileSizeChar [100];
	sprintf(fileSizeChar, "%lld", file->FileSize());
	std::string fileSize = *(&fileSizeChar);

	std::string mesgString = "<a href='retroshare://file|" + (file->FileName()) + "|" + fileSize + "|" + (file->FileHash()) + "'>" 
	+ "retroshare://file|" + (file->FileName()) + "|" + fileSize +  "|" + (file->FileHash())  + "</a>";
#ifdef CHAT_DEBUG
	    std::cerr << "CreateForumMsg::anchorClicked mesgString : " << mesgString << std::endl;
#endif

	const char * messageString = mesgString.c_str ();

	//convert char massageString to w_char
	wchar_t* message;
	int requiredSize = mbstowcs(NULL, messageString, 0); // C4996
	/* Add one to leave room for the NULL terminator */
	message = (wchar_t *)malloc( (requiredSize + 1) * sizeof( wchar_t ));
	if (! message) {
	    std::cerr << ("Memory allocation failure.\n");
	}
	int size = mbstowcs( message, messageString, requiredSize + 1); // C4996
	if (size == (size_t) (-1)) {
	   printf("Couldn't convert string--invalid multibyte character.\n");
	}
  	
	ui.forumMessage->setHtml(QString::fromStdWString(message));

}


