/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006,  crypton
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

#include "PopupChatDialog.h"

#include <QTextCodec>
#include <QTextEdit>
#include <QToolBar>
#include <QTextCursor>
#include <QTextList>
#include <QString>
#include <QtDebug>
#include <QIcon>
#include <QPixmap>
#include <QHashIterator>

#include "rsiface/rspeers.h"
#include "rsiface/rsmsgs.h"


/* Define the format used for displaying the date and time */
#define DATETIME_FMT  "MMM dd hh:mm:ss"

#include <sstream>


/** Default constructor */
PopupChatDialog::PopupChatDialog(std::string id, std::string name, 
				QWidget *parent, Qt::WFlags flags)
  : QMainWindow(parent, flags), dialogId(id), dialogName(name),
    lastChatTime(0), lastChatName("")
    
{
  /* Invoke Qt Designer generated QObject setup routine */
  ui.setupUi(this);
  
  loadEmoticons();
  
  /* Hide ToolBox frame */
  showAvatarFrame(true);
  connect(ui.avatarFrameButton, SIGNAL(toggled(bool)), this, SLOT(showAvatarFrame(bool)));

  //connect(ui.chattextEdit, SIGNAL(returnPressed( ) ), this, SLOT(sendChat( ) ));
  connect(ui.chattextEdit, SIGNAL(textChanged ( ) ), this, SLOT(checkChat( ) ));
  
  connect(ui.sendButton, SIGNAL(clicked( ) ), this, SLOT(sendChat( ) ));
   
  connect(ui.textboldButton, SIGNAL(clicked()), this, SLOT(setFont()));  
  connect(ui.textunderlineButton, SIGNAL(clicked()), this, SLOT(setFont()));  
  connect(ui.textitalicButton, SIGNAL(clicked()), this, SLOT(setFont()));
  connect(ui.fontButton, SIGNAL(clicked()), this, SLOT(getFont())); 
  connect(ui.colorButton, SIGNAL(clicked()), this, SLOT(setColor()));
  connect(ui.emoteiconButton, SIGNAL(clicked()), this, SLOT(smileyWidget()));

  // Create the status bar
  std::ostringstream statusstr;
  statusstr << "Chatting with: " << dialogName << " !!! " << id;
  statusBar()->showMessage(QString::fromStdString(statusstr.str()));
  ui.textBrowser->setOpenExternalLinks ( false );

  QString title = QString::fromStdString(name) + " :" + tr(" RetroShare - Chat")  ;
  setWindowTitle(title);
  
  //set the default avatar
  //ui.avatarlabel->setPixmap(QPixmap(":/images/retrosharelogo1.png"));
  
  setWindowIcon(QIcon(QString(":/images/rstray3.png")));
  
  ui.textboldButton->setIcon(QIcon(QString(":/images/edit-bold.png")));
  ui.textunderlineButton->setIcon(QIcon(QString(":/images/edit-underline.png")));
  ui.textitalicButton->setIcon(QIcon(QString(":/images/edit-italic.png")));
  ui.fontButton->setIcon(QIcon(QString(":/images/fonts.png")));
  ui.emoteiconButton->setIcon(QIcon(QString(":/images/emoticons/kopete/kopete020.png")));
  
  ui.textboldButton->setCheckable(true);
  ui.textunderlineButton->setCheckable(true);
  ui.textitalicButton->setCheckable(true);
   
  /*QMenu * fontmenu = new QMenu();
  fontmenu->addAction(ui.actionBold);
  fontmenu->addAction(ui.actionUnderline);
  fontmenu->addAction(ui.actionItalic);
  fontmenu->addAction(ui.actionStrike);
  ui.fontButton->setMenu(fontmenu);*/
  
  QPixmap pxm(24,24);
  pxm.fill(Qt::black);
  ui.colorButton->setIcon(pxm);
  
  QFont font = QFont("Comic Sans MS", 10);

}

/** Destructor. */
PopupChatDialog::~PopupChatDialog()
{

}

/** 
 Overloads the default show() slot so we can set opacity*/

void PopupChatDialog::show()
{

  if(!this->isVisible()) {
    QMainWindow::show();
  } else {
    QMainWindow::activateWindow();
    setWindowState(windowState() & ~Qt::WindowMinimized | Qt::WindowActive);
    QMainWindow::raise();
  }
  
}

void PopupChatDialog::closeEvent (QCloseEvent * event)
{
    hide();
    event->ignore();
}

void PopupChatDialog::updateChat()
{
	/* get chat off queue */

	/* write it out */

}


void PopupChatDialog::addChatMsg(ChatInfo *ci)
{
    QTextEdit *msgWidget = ui.textBrowser;

	QString currenttxt = msgWidget->toHtml();


	/* add in lines at the bottom */
	QString extraTxt;


	bool offline = true;

	{
	  RsPeerDetails detail;
	  if (!rsPeers->getPeerDetails(dialogId, detail))
	  {
		std::cerr << "WARNING CANNOT GET PEER INFO!!!!" << std::endl;
	  }
	  else if (detail.state & RS_PEER_STATE_CONNECTED)
	  {
	    offline = false;
	  }
	}

	if (offline)
	{
	    	QString line = "<br>\n<span style=\"color:#1D84C9\"><strong> ----- PEER OFFLINE (Chat will be lost) -----</strong></span> \n<br>";

		extraTxt += line;
	}
	

        QString timestamp = "[" + QDateTime::currentDateTime().toString("hh:mm:ss") + "]";
            //QString pre = tr("Peer:" );
	    QString name = QString::fromStdString(ci->name);        
	    QString line = "<span style=\"color:#C00000\"><strong>" + timestamp + "</strong></span>" +			
            		"<span style=\"color:#2D84C9\"><strong>" + " " + name + "</strong></span>";		
        
        extraTxt += line;


	extraTxt += QString::fromStdWString(ci -> msg);

	currenttxt += extraTxt;
	
    QHashIterator<QString, QString> i(smileys);
	while(i.hasNext())
	{
		i.next();
		currenttxt.replace(i.key(), "<img src=\"" + i.value() + "\">");
	}

	msgWidget->setHtml(currenttxt);

	QScrollBar *qsb =  msgWidget->verticalScrollBar();
	qsb -> setValue(qsb->maximum());
}

void PopupChatDialog::checkChat()
{
	/* if <return> at the end of the text -> we can send it! */
        QTextEdit *chatWidget = ui.chattextEdit;
        std::string txt = chatWidget->toPlainText().toStdString();
	if ('\n' == txt[txt.length()-1])
	{
		//std::cerr << "Found <return> found at end of :" << txt << ": should send!";
		//std::cerr << std::endl;
		if (txt.length()-1 == txt.find('\n')) /* only if on first line! */
		{
			/* should remove last char ... */
			sendChat();
		}
	}
	else
	{
		//std::cerr << "No <return> found in :" << txt << ":";
		//std::cerr << std::endl;
	}
}


void PopupChatDialog::sendChat()
{
        QTextEdit *chatWidget = ui.chattextEdit;

        ChatInfo ci;
        

	

	{
          rsiface->lockData(); /* Lock Interface */
          const RsConfig &conf = rsiface->getConfig();

	  ci.rsid = conf.ownId;
	  ci.name = conf.ownName;

          rsiface->unlockData(); /* Unlock Interface */
	}

        ci.msg = chatWidget->toHtml().toStdWString();
        ci.chatflags = RS_CHAT_PRIVATE;

	addChatMsg(&ci);

        /* put proper destination */
	ci.rsid = dialogId;
	ci.name = dialogName;

        rsMsgs -> ChatSend(ci);
        chatWidget ->clear();

        /* redraw send list */
}

/**
 Toggles the ToolBox on and off, changes toggle button text
 */
void PopupChatDialog::showAvatarFrame(bool show)
{
    if (show) {
        ui.avatarframe->setVisible(true);
        ui.avatarFrameButton->setChecked(true);
        ui.avatarFrameButton->setToolTip(tr("Hide Avatar"));
        ui.avatarFrameButton->setIcon(QIcon(tr(":images/hide_toolbox_frame.png")));
    } else {
        ui.avatarframe->setVisible(false);
        ui.avatarFrameButton->setChecked(false);
        ui.avatarFrameButton->setToolTip(tr("Show Avatar"));
        ui.avatarFrameButton->setIcon(QIcon(tr(":images/show_toolbox_frame.png")));
    }
}

void PopupChatDialog::setColor()
{	    
	bool ok;    
    QRgb color = QColorDialog::getRgba(ui.chattextEdit->textColor().rgba(), &ok, this);
    if (ok) {

        currentColor = QColor(color);
        ui.chattextEdit->setTextColor(currentColor);
        colorChanged(currentColor);
    }
    ui.chattextEdit->setFocus();
}

void PopupChatDialog::colorChanged(const QColor &c)
{
    QPixmap pix(16, 16);
    pix.fill(c);
    ui.colorButton->setIcon(pix);
}

void PopupChatDialog::mergeFormatOnWordOrSelection(const QTextCharFormat &format)
{
    //QTextCursor cursor = ui.chattextEdit->textCursor();
    /*if (!cursor.hasSelection())
        cursor.select(QTextCursor::WordUnderCursor);
    cursor.mergeCharFormat(format);*/
    //ui.chattextEdit->mergeCurrentCharFormat(format);
}

void PopupChatDialog::getFont()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, QFont(ui.chattextEdit->toHtml()), this);
    if (ok) {
        ui.chattextEdit->setFont(font);
    }
}

void PopupChatDialog::setFont()
{
  QFont font = QFont("Comic Sans MS", 10);
  font.setBold(ui.textboldButton->isChecked());
  font.setUnderline(ui.textunderlineButton->isChecked());
  font.setItalic(ui.textitalicButton->isChecked());
  ui.chattextEdit->setFont(font);
}

void PopupChatDialog::loadEmoticons()
{
	QDir smdir(QApplication::applicationDirPath() + "/emoticons/kopete");
	//QDir smdir(":/gui/images/emoticons/kopete");
	QFileInfoList sminfo = smdir.entryInfoList(QStringList() << "*.gif" << "*.png", QDir::Files, QDir::Name);
	foreach(QFileInfo info, sminfo)
	{
		QString smcode = info.fileName().replace(".gif", "");
		QString smstring;
		for(int i = 0; i < 9; i+=3)
		{
			smstring += QString((char)smcode.mid(i,3).toInt());
		}
		//qDebug(smstring.toAscii());
		smileys.insert(smstring, info.absoluteFilePath());
	}
}

void PopupChatDialog::smileyWidget()
{ 
	qDebug("MainWindow::smileyWidget()");
	QWidget *smWidget = new QWidget;
	smWidget->setWindowTitle("Emoteicons");
	smWidget->setWindowIcon(QIcon(QString(":/images/rstray3.png")));
	smWidget->setFixedSize(256,256);
	
	
	
	int x = 0, y = 0;
	
	QHashIterator<QString, QString> i(smileys);
	while(i.hasNext())
	{
		i.next();
		QPushButton *smButton = new QPushButton("", smWidget);
		smButton->setGeometry(x*24, y*24, 24,24);
		smButton->setIconSize(QSize(24,24));
		smButton->setIcon(QPixmap(i.value()));
		smButton->setToolTip(i.key());
		++x;
		if(x > 4)
		{
			x = 0;
			y++;
		}
		connect(smButton, SIGNAL(clicked()), this, SLOT(addSmiley()));
	}
	
	smWidget->show();
}

void PopupChatDialog::addSmiley()
{
	ui.chattextEdit->setText(ui.chattextEdit->toHtml() + qobject_cast<QPushButton*>(sender())->toolTip());
}
