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

  connect(ui.lineEdit, SIGNAL(returnPressed( ) ), this, SLOT(sendChat( ) ));
  
  connect(ui.sendButton, SIGNAL(clicked( ) ), this, SLOT(sendChat( ) ));

  connect(ui.colorButton, SIGNAL(clicked()), this, SLOT(setColor()));
  //connect(ui.fontButton, SIGNAL(clicked()), this, SLOT(setFont())); 
   
  connect(ui.textboldButton, SIGNAL(clicked()), this, SLOT(insertBold()));  
  connect(ui.textunderlineButton, SIGNAL(clicked()), this, SLOT(insertUnderline()));  
  connect(ui.textitalicButton, SIGNAL(clicked()), this, SLOT(insertItalic()));
  
  connect(ui.actionBold, SIGNAL(triggered()), this, SLOT(insertBold()));
  connect(ui.actionItalic, SIGNAL(triggered()), this, SLOT(insertItalic()));
  connect(ui.actionUnderline, SIGNAL(triggered()), this, SLOT(insertUnderline()));
  connect(ui.actionStrike, SIGNAL(triggered()), this, SLOT(insertStrike()));

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
  
  ui.actionBold->setIcon(QIcon(":/images/edit-bold.png"));
  ui.actionUnderline->setIcon(QIcon(":/images/edit-underline.png"));
  ui.actionItalic->setIcon(QIcon(":/images/edit-italic.png"));
  //ui.actionStrike->setIcon(QIcon(":/exit.png"));
  
  QMenu * fontmenu = new QMenu();
  fontmenu->addAction(ui.actionBold);
  fontmenu->addAction(ui.actionUnderline);
  fontmenu->addAction(ui.actionItalic);
  fontmenu->addAction(ui.actionStrike);
  ui.fontButton->setMenu(fontmenu);
  


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

void PopupChatDialog::setColor()
{
    QColor col = QColorDialog::getColor(Qt::green, this);
    if (col.isValid()) {

        ui.colorButton->setPalette(QPalette(col));
        ui.lineEdit->setText(QString(tr("<a style=\"color:")) + (col.name()));
        this->insertAutour(tr("\">"), tr("</style>"));
        this->ui.lineEdit->setFocus();
        QTextCharFormat fmt;
        fmt.setForeground(col);
        colorChanged(col);
    }
}

void PopupChatDialog::setFont()
{
    bool ok;
    QFont font = QFontDialog::getFont(&ok, QFont(ui.lineEdit->text()), this);
    if (ok) {
        //ui.lineEdit->setText(font.key());
        ui.lineEdit->setFont(font);
    }
}


void PopupChatDialog::colorChanged(const QColor &c)
{
    QPixmap pix(16, 16);
    pix.fill(c);
    ui.colorButton->setIcon(pix);
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
	    QString line = "<span style=\"color:#1D84C9\"><strong>" + timestamp + 
					 " " + name + "</strong></span> \n<br>";

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
        QLineEdit *lineWidget = ui.lineEdit;

        ChatInfo ci;

	{
          rsiface->lockData(); /* Lock Interface */
          const RsConfig &conf = rsiface->getConfig();

	  ci.rsid = conf.ownId;
	  ci.name = conf.ownName;

          rsiface->unlockData(); /* Unlock Interface */
	}

        ci.msg = lineWidget->text().toStdWString();
        ci.chatflags = RS_CHAT_PRIVATE;

	addChatMsg(&ci);

        /* put proper destination */
	ci.rsid = dialogId;
	ci.name = dialogName;

        rsMsgs -> ChatSend(ci);
        lineWidget -> setText(QString(""));

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

void PopupChatDialog::insertBold()
{
  
  this->insertAutour(tr("<b>"), tr("</b>"));
  this->ui.lineEdit->setFocus();

}

void PopupChatDialog::insertItalic()
{
  
  this->insertAutour(tr("<i>"), tr("</i>"));
  this->ui.lineEdit->setFocus();

}

void PopupChatDialog::insertUnderline()
{
  
  this->insertAutour(tr("<u>"), tr("</u>"));
  this->ui.lineEdit->setFocus();

}

void PopupChatDialog::insertStrike()
{
  
  this->insertAutour(tr("<s>"), tr("</s>"));
  this->ui.lineEdit->setFocus();

}

void PopupChatDialog::insertAutour(QString leftTruc,QString rightTruc)
{
    int p0 = ui.lineEdit->cursorPosition();
    QString stringToInsert = leftTruc ;
    stringToInsert.append(rightTruc);
    ui.lineEdit->insert(stringToInsert);
    ui.lineEdit->setCursorPosition(p0 + leftTruc.size());
    
}



