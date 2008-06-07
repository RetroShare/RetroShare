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

#define appDir QApplication::applicationDirPath()

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
  
  styleHtm = appDir + "/style/chat/default.htm";
  
  /* Hide ToolBox frame */
  showAvatarFrame(true);
  connect(ui.avatarFrameButton, SIGNAL(toggled(bool)), this, SLOT(showAvatarFrame(bool)));

  connect(ui.chattextEdit, SIGNAL(textChanged ( ) ), this, SLOT(checkChat( ) ));
  
  connect(ui.sendButton, SIGNAL(clicked( ) ), this, SLOT(sendChat( ) ));
   
  connect(ui.textboldButton, SIGNAL(clicked()), this, SLOT(setFont()));  
  connect(ui.textunderlineButton, SIGNAL(clicked()), this, SLOT(setFont()));  
  connect(ui.textitalicButton, SIGNAL(clicked()), this, SLOT(setFont()));
  connect(ui.fontButton, SIGNAL(clicked()), this, SLOT(getFont())); 
  connect(ui.colorButton, SIGNAL(clicked()), this, SLOT(setColor()));
  connect(ui.emoteiconButton, SIGNAL(clicked()), this, SLOT(smileyWidget()));
  connect(ui.styleButton, SIGNAL(clicked()), SLOT(changeStyle()));

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
  ui.styleButton->setIcon(QIcon(QString(":/images/looknfeel.png")));
  
  ui.textboldButton->setCheckable(true);
  ui.textunderlineButton->setCheckable(true);
  ui.textitalicButton->setCheckable(true);
  
  /*Disabled style Button when will switch style RetroShare will crash need to be fix */
  ui.styleButton->setEnabled(false);
   
  /*QMenu * fontmenu = new QMenu();
  fontmenu->addAction(ui.actionBold);
  fontmenu->addAction(ui.actionUnderline);
  fontmenu->addAction(ui.actionItalic);
  fontmenu->addAction(ui.actionStrike);
  ui.fontButton->setMenu(fontmenu);*/
  
  mCurrentColor = Qt::black;
  mCurrentFont = QFont("Comic Sans MS", 10);

  colorChanged(mCurrentColor);
  setFont();


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
	    	QString offlineMsg = "<br>\n<span style=\"color:#1D84C9\"><strong> ----- PEER OFFLINE (Chat will be lost) -----</strong></span> \n<br>";
		ui.textBrowser->setHtml(ui.textBrowser->toHtml() + offlineMsg);
	}
	

        QString timestamp = "[" + QDateTime::currentDateTime().toString("hh:mm:ss") + "]";
        QString name = QString::fromStdString(ci ->name);        
        QString message = QString::fromStdWString(ci -> msg);

        QHashIterator<QString, QString> i(smileys);
	while(i.hasNext())
	{
		i.next();
		message.replace(i.key(), "<img src=\"" + i.value() + "\">");
	}
	history /*<< nickColor << color << font << fontSize*/ << timestamp << name << message;
	
	QString formatMsg = loadEmptyStyle()/*.replace(nickColor)
				    .replace(color)
				    .replace(font)
				    .replace(fontSize)*/
				    .replace("%timestamp%", timestamp)
                    .replace("%name%", name)
				    .replace("%message%", message);

	ui.textBrowser->setHtml(ui.textBrowser->toHtml() + formatMsg + "\n");
	
	QTextCursor cursor = ui.textBrowser->textCursor();
	cursor.movePosition(QTextCursor::End);
	ui.textBrowser->setTextCursor(cursor);
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

std::cout << "PopupChatDialog:sendChat " << styleHtm.toStdString() << std::endl;

	addChatMsg(&ci);

        /* put proper destination */
	ci.rsid = dialogId;
	ci.name = dialogName;

        rsMsgs -> ChatSend(ci);
        chatWidget ->clear();
	setFont();

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
        mCurrentColor = QColor(color);
        colorChanged(mCurrentColor);
    }
    setFont();
}

void PopupChatDialog::colorChanged(const QColor &c)
{
    QPixmap pix(16, 16);
    pix.fill(c);
    ui.colorButton->setIcon(pix);
}

void PopupChatDialog::getFont()
{
    bool ok;
    mCurrentFont = QFontDialog::getFont(&ok, mCurrentFont, this);
    setFont();
}

void PopupChatDialog::setFont()
{

  mCurrentFont.setBold(ui.textboldButton->isChecked());
  mCurrentFont.setUnderline(ui.textunderlineButton->isChecked());
  mCurrentFont.setItalic(ui.textitalicButton->isChecked());

  ui.chattextEdit->setFont(mCurrentFont);
  ui.chattextEdit->setTextColor(mCurrentColor);

  ui.chattextEdit->setFocus();

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
	smWidget->setWindowTitle("Emoticons");
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

QString PopupChatDialog::loadEmptyStyle()
{
        std::cout << "PopupChatDialog:loadEmptyStyle " << styleHtm.toStdString() << std::endl;
	QString ret;
	QFile file(styleHtm);
	//file.open(QIODevice::ReadOnly);
       	if (file.open(QIODevice::ReadOnly)) {
		ret = file.readAll();
		file.close();
		QString styleTmp = styleHtm;
		QString styleCss = styleTmp.remove(styleHtm.lastIndexOf("."), styleHtm.length()-styleHtm.lastIndexOf(".")) + ".css";
		qDebug() << styleCss.toAscii();
		QFile css(styleCss);
		QString tmp;
		if (css.open(QIODevice::ReadOnly)) {
			tmp = css.readAll();
			css.close();
		}
		else {
			std::cerr << "PopupChatDialog:loadEmptyStyle " << "Missing file of default css " << std::endl;
			tmp = "";
		}
		ret.replace("%css-style%", tmp);
		return ret;
       	}
	else {
                std::cerr << "PopupChatDialog:loadEmptyStyle " << "Missing file of default style " << std::endl;
		ret="%timestamp% %name% \n %message% ";
		return ret;
	}
}

void PopupChatDialog::changeStyle()
{
	QString newStyle = QFileDialog::getOpenFileName(this, tr("Open Style"),
                                                 appDir + "/style/chat/",
                                                 tr("Styles (*.htm)"));
	if(!newStyle.isEmpty())
	{
		QString wholeChat;
		styleHtm = newStyle;
		
		
		for(int i = 0; i < history.size(); i+=4)
		{
			QString formatMsg = loadEmptyStyle();
			wholeChat += formatMsg.replace("%timestamp%", history.at(i+1))
                                  .replace("%name%", history.at(i+2))
				                  .replace("%message%", history.at(i+3)) + "\n";
		}
		ui.textBrowser->setHtml(wholeChat);
	}
	QTextCursor cursor = ui.textBrowser->textCursor();
	cursor.movePosition(QTextCursor::End);
	ui.textBrowser->setTextCursor(cursor);
}
