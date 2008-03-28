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
  
  /* Hide ToolBox frame */
  showAvatarFrame(true);
  connect(ui.avatarFrameButton, SIGNAL(toggled(bool)), this, SLOT(showAvatarFrame(bool)));

  //connect(ui.chattextEdit, SIGNAL(returnPressed( ) ), this, SLOT(sendChat( ) ));
  
  connect(ui.sendButton, SIGNAL(clicked( ) ), this, SLOT(sendChat( ) ));
   
  connect(ui.textboldButton, SIGNAL(clicked()), this, SLOT(setFont()));  
  connect(ui.textunderlineButton, SIGNAL(clicked()), this, SLOT(setFont()));  
  connect(ui.textitalicButton, SIGNAL(clicked()), this, SLOT(setFont()));
  connect(ui.fontButton, SIGNAL(clicked()), this, SLOT(getFont())); 
  connect(ui.colorButton, SIGNAL(clicked()), this, SLOT(setColor()));

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
    QTextBrowser *msgWidget = ui.textBrowser;

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

	msgWidget->setHtml(currenttxt);

//	std::cerr << " Added Text: " << std::endl;
//	std::cerr << out.str() << std::endl;
	QScrollBar *qsb =  msgWidget->verticalScrollBar();
	qsb -> setValue(qsb->maximum());
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
	    
    QColor col = QColorDialog::getColor(Qt::black, this);
    if (col.isValid()) {

        //ui.colorButton->setPalette(QPalette(col));
        //ui.chattextEdit->setTextCursor();
        ui.chattextEdit->setTextColor(QColor(col));
        //QTextCharFormat fmt;
        //fmt.setForeground(col);
        //mergeFormatOnWordOrSelection(fmt);
        colorChanged(col);
    }
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


