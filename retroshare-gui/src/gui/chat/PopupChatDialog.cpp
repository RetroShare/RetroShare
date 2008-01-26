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

#include "rshare.h"
#include "PopupChatDialog.h"

#include <QTextCodec>
#include <QTextEdit>
#include <QToolBar>
#include <QTextCursor>
#include <QTextList>

#include "rsiface/rspeers.h"


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

  connect(ui.lineEdit, SIGNAL(returnPressed( ) ), this, SLOT(sendChat( ) ));

  connect(ui.colorButton, SIGNAL(clicked()), this, SLOT(setColor()));
  
  connect(ui.textboldButton, SIGNAL(clicked()), this, SLOT(textBold()));
  
  connect(ui.textunderlineButton, SIGNAL(clicked()), this, SLOT(textUnderline()));
  
  connect(ui.textitalicButton, SIGNAL(clicked()), this, SLOT(textItalic()));

  // Create the status bar
  std::ostringstream statusstr;
  statusstr << "Chatting with: " << dialogName << " !!! " << id;
  statusBar()->showMessage(QString::fromStdString(statusstr.str()));
  ui.textBrowser->setOpenExternalLinks ( false );

  QString title = "RS:" + tr("Chatting with") + " " + QString::fromStdString(name);
  setWindowTitle(title);


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
        QTextCharFormat fmt;
        fmt.setForeground(col);
        mergeFormatOnWordOrSelection(fmt);
        colorChanged(col);
    }
}

void PopupChatDialog::textBold()
{
    QTextCharFormat fmt;
    fmt.setFontWeight(ui.textboldButton->isChecked() ? QFont::Bold : QFont::Normal);
    mergeFormatOnWordOrSelection(fmt);
}

void PopupChatDialog::textUnderline()
{
    QTextCharFormat fmt;
    fmt.setFontUnderline(ui.textunderlineButton->isChecked());
    mergeFormatOnWordOrSelection(fmt);
}

void PopupChatDialog::textItalic()
{
    QTextCharFormat fmt;
    fmt.setFontItalic(ui.textitalicButton->isChecked());
    mergeFormatOnWordOrSelection(fmt);
}

void PopupChatDialog::currentCharFormatChanged(const QTextCharFormat &format)
{
    fontChanged(format.font());
    colorChanged(format.foreground().color());
}
   
void PopupChatDialog::mergeFormatOnWordOrSelection(const QTextCharFormat &format)
{
    QTextCursor cursor = ui.textBrowser->textCursor();
    if (!cursor.hasSelection())
        cursor.select(QTextCursor::WordUnderCursor);
    cursor.mergeCharFormat(format);
    ui.textBrowser->mergeCurrentCharFormat(format);
}

void PopupChatDialog::fontChanged(const QFont &f)
{
    //comboFont->setCurrentIndex(comboFont->findText(QFontInfo(f).family()));
    //comboSize->setCurrentIndex(comboSize->findText(QString::number(f.pointSize())));
    ui.textboldButton->setChecked(f.bold());
    ui.textunderlineButton->setChecked(f.italic());
    ui.textitalicButton->setChecked(f.underline());
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

	/* determine how many spaces to add */
	int n = msgWidget->width();
	/* now spaces = (width - txt width) / (pixel / space)
	 */

	//std::cerr << "Width is : " << n << std::endl;
	n -= 256; /* 220 pixels for name */
	if (n > 0)
	{
		n = 2 + n / 10;
	}
	else
	{
		n = 1;
	}

	//std::cerr << "Space count : " << n << std::endl;

	std::string spaces(" ");


	/* add in lines at the bottom */
	QString extraTxt;
	int ts = time(NULL);


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

	
	if ((ci->name == lastChatName) && (ts - lastChatTime < 60))
	{
			/* no name */
	}
	else
	{

#if defined(Q_OS_WIN)
		// Nothing.
#else
		extraTxt += "<br>\n";
#endif
		for(int i = 0; i < n; i++)
		{
			extraTxt += " ";
		}

            QString timestamp = "(" + QDateTime::currentDateTime().toString("hh:mm:ss") + ") ";
            //QString pre = tr("Peer:" );
	    QString name = QString::fromStdString(ci->name);
	    QString line = "<span style=\"color:#1D84C9\"><strong>" + timestamp + 
					 "   " + name + "</strong></span> \n<br>";

		extraTxt += line;

	}
	extraTxt += QString::fromStdWString(ci -> msg);

	/* This might be WIN32 only - or maybe Qt4.2.2 only - but need it for windows at the mom */
#if defined(Q_OS_WIN)
		extraTxt += "\n";
#else
		extraTxt += "\n";
#endif

	lastChatTime = ts;
	lastChatName = ci->name;

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

        rsicontrol -> ChatSend(ci);
        lineWidget -> setText(QString(""));

        /* redraw send list */
}


