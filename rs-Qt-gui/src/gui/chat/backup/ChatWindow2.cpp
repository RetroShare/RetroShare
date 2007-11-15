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
#include "ChatWindow.h"
#include "config/gconfig.h"
#include "ChatAvatarFrame.h"

#include <QTextCodec>
#include <QTextEdit>
#include <QToolBar>
#include <QTextCursor>
#include <QTextList>


/* Define the format used for displaying the date and time */
#define DATETIME_FMT  "MMM dd hh:mm:ss"

#include <sstream>


/** Default constructor */
ChatWindow::ChatWindow(std::string id, std::string name, 
				QWidget *parent, Qt::WFlags flags)
  : QMainWindow(parent, flags), dialogId(id), dialogName(name),
    lastChatTime(0), lastChatName("")
{
  /* Invoke Qt Designer generated QObject setup routine */
  ui.setupUi(this);

  GConfig config;
  config.loadWidgetInformation(this);
  
  _isAvatarFrameOpened = true;
  
  	//creates sub widgets
	_avatarFrame = new QtChatAvatarFrame(this);
	////


  connect(ui.lineEdit, SIGNAL(returnPressed( ) ), this, SLOT(sendChat( ) ));

  connect(ui.colorButton, SIGNAL(clicked()), this, SLOT(setColor()));
  
  connect(ui.textboldButton, SIGNAL(clicked()), this, SLOT(textBold()));
  
  connect(ui.textunderlineButton, SIGNAL(clicked()), this, SLOT(textUnderline()));
  
  connect(ui.textitalicButton, SIGNAL(clicked()), this, SLOT(textItalic()));
  
  connect(ui.avatarFrameButton, SIGNAL(clicked()), SLOT(avatarFrameButtonClicked()));


  addAvatarFrame();

  // Create the status bar
  std::ostringstream statusstr;
  statusstr << "Chatting with: " << dialogName << " !!! " << id;
  statusBar()->showMessage(QString::fromStdString(statusstr.str()));
  ui.textBrowser->setOpenExternalLinks ( false );

}



/** 
 Overloads the default show() slot so we can set opacity*/

void
ChatWindow::show()
{
  //loadSettings();
  if(!this->isVisible()) {
    QMainWindow::show();

  }
}

void ChatWindow::closeEvent (QCloseEvent * event)
{


 QMainWindow::closeEvent(event);
}

void ChatWindow::setColor()
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

void ChatWindow::textBold()
{
    QTextCharFormat fmt;
    fmt.setFontWeight(ui.textboldButton->isChecked() ? QFont::Bold : QFont::Normal);
    mergeFormatOnWordOrSelection(fmt);
}

void ChatWindow::textUnderline()
{
    QTextCharFormat fmt;
    fmt.setFontUnderline(ui.textunderlineButton->isChecked());
    mergeFormatOnWordOrSelection(fmt);
}

void ChatWindow::textItalic()
{
    QTextCharFormat fmt;
    fmt.setFontItalic(ui.textitalicButton->isChecked());
    mergeFormatOnWordOrSelection(fmt);
}

void ChatWindow::currentCharFormatChanged(const QTextCharFormat &format)
{
    fontChanged(format.font());
    colorChanged(format.foreground().color());
}
   
void ChatWindow::mergeFormatOnWordOrSelection(const QTextCharFormat &format)
{
    QTextCursor cursor = ui.textBrowser->textCursor();
    if (!cursor.hasSelection())
        cursor.select(QTextCursor::WordUnderCursor);
    cursor.mergeCharFormat(format);
    ui.textBrowser->mergeCurrentCharFormat(format);
}

void ChatWindow::fontChanged(const QFont &f)
{
    //comboFont->setCurrentIndex(comboFont->findText(QFontInfo(f).family()));
    //comboSize->setCurrentIndex(comboSize->findText(QString::number(f.pointSize())));
    ui.textboldButton->setChecked(f.bold());
    ui.textunderlineButton->setChecked(f.italic());
    ui.textitalicButton->setChecked(f.underline());
}



void ChatWindow::colorChanged(const QColor &c)
{
    QPixmap pix(16, 16);
    pix.fill(c);
    ui.colorButton->setIcon(pix);
}


void ChatWindow::updateChat()
{
	/* get chat off queue */

	/* write it out */

}



void ChatWindow::addChatMsg(ChatInfo *ci)
{
        QTextBrowser *msgWidget = ui.textBrowser;

	QString currenttxt = msgWidget->toHtml();

	/* determine how many spaces to add */
	int n = msgWidget->width();
	/* now spaces = (width - txt width) / (pixel / space)
	 */

	std::cerr << "Width is : " << n << std::endl;
	n -= 256; /* 220 pixels for name */
	if (n > 0)
	{
		n = 2 + n / 10;
	}
	else
	{
		n = 1;
	}

	std::cerr << "Space count : " << n << std::endl;

	std::string spaces(" ");


	/* add in lines at the bottom */
	std::ostringstream out;
	int ts = time(NULL);
	if ((ci->name == lastChatName) && (ts - lastChatTime < 60))
	{
			/* no name */
	}
	else
	{
		for(int i = 0; i < n; i++)
		{
			out << spaces; 
		}

		out << "[" << ci->name << " Said @" << ts << "]" << std::endl;
		out << "<br>" << std::endl;

	}
	out << ci -> msg << std::endl;
	lastChatTime = ts;
	lastChatName = ci->name;

	QString extra = QString::fromStdString(out.str());
	currenttxt += extra;

	msgWidget->setHtml(currenttxt);

	std::cerr << " Added Text: " << std::endl;
	std::cerr << out.str() << std::endl;
	QScrollBar *qsb =  msgWidget->verticalScrollBar();
	qsb -> setValue(qsb->maximum());
}


void ChatWindow::sendChat()
{
        QTextEdit *lineWidget = ui.lineEdit;

        ChatInfo ci;

	{
          rsiface->lockData(); /* Lock Interface */
          const RsConfig &conf = rsiface->getConfig();

	  ci.rsid = conf.ownId;
	  ci.name = conf.ownName;

          rsiface->unlockData(); /* Unlock Interface */
	}

        ci.msg = lineWidget->documentTitle().toStdString();
        ci.chatflags = RS_CHAT_PRIVATE;

	addChatMsg(&ci);

        /* put proper destination */
	ci.rsid = dialogId;
	ci.name = dialogName;

        rsicontrol -> ChatSend(ci);
        lineWidget -> setText(QString(""));

        /* redraw send list */
}


void ChatWindow::avatarFrameButtonClicked() {
	if (_isAvatarFrameOpened) {
		removeAvatarFrame();
	} else {
		addAvatarFrame();
	}
}


void ChatWindow::addAvatarFrame() {
	QBoxLayout * glayout = dynamic_cast<QBoxLayout *>(layout());
	_avatarFrame->setVisible(true);
	//_avatarFrame->setMinimumSize(64, 0);
	glayout->insertWidget(1, _avatarFrame);

	_isAvatarFrameOpened = true;
	ui.avatarFrameButton->setIcon(QIcon(":images/show_toolbox_frame.png"));
	update();
}

void ChatWindow::removeAvatarFrame() {
	QBoxLayout * glayout = dynamic_cast<QBoxLayout *>(layout());
	_avatarFrame->setVisible(false);
	_avatarFrame->setMinimumSize(0, 0);
	glayout->removeWidget(_avatarFrame);

	_isAvatarFrameOpened = false;
	ui.avatarFrameButton->setIcon(QIcon(":images/hide_toolbox_frame.png"));
	update();
}


void ChatWindow::updateUserAvatar() {

	QPixmap pixmap;
	//std::string myData = _cChatHandler.getCUserProfile().getUserProfile().getIcon().getData();
	//pixmap.loadFromData((uchar *)myData.c_str(), myData.size());
	_avatarFrame->setUserPixmap(pixmap);
}

//void ChatWindow::updateAvatarFrame() {
//	QMutexLocker locker(&_mutex);

	//QtContactList * qtContactList = _qtrs->getQtContactList();
	//CContactList & cContactList = qtrsContactList->getCContactList();

	//IMContactSet imContactSet = _imChatSession->getIMContactSet();
	//IMContactSet::iterator it;
	//for (it = imContactSet.begin(); it != imContactSet.end(); it++) {

		//std::string contactId = cContactList.findContactThatOwns(*it);
		//ContactProfile profile = cContactList.getContactProfile(contactId);

		//std::string data = profile.getIcon().getData();
		//QPixmap pixmap;
		//pixmap.loadFromData((uchar *)data.c_str(), data.size());
		//_avatarFrame->addRemoteContact(
			//QString::fromStdString(contactId),
			//QString::fromStdString(profile.getDisplayName()),
			//QString::fromStdString((*it).getContactId()),
			//pixmap
		//);
	//}
//}
