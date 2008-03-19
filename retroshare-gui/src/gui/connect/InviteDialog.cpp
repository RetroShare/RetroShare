/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006, 2007 crypton
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
 
#include "InviteDialog.h"

#include "rsiface/rsiface.h"
#include <util/WidgetBackgroundImage.h>

/** Default constructor */
InviteDialog::InviteDialog(QWidget *parent, Qt::WFlags flags)
  : QDialog(parent, flags)
{
  /* Invoke Qt Designer generated QObject setup routine */
  ui.setupUi(this);
  
  /* add a Background image for Invite a Friend Label */
  //WidgetBackgroundImage::setBackgroundImage(ui.invitefriendLabel, ":images/new-contact.png", WidgetBackgroundImage::AdjustHeight);

  connect(ui.cancelButton, SIGNAL(clicked()), this, SLOT(cancelbutton()));
  connect(ui.emailButton, SIGNAL(clicked()), this, SLOT(emailbutton()));
  connect(ui.doneButton, SIGNAL(clicked()), this, SLOT(closebutton()));
 
  //setFixedSize(QSize(434, 462));
}

void InviteDialog::closebutton()
{
	close();
}


void InviteDialog::cancelbutton()
{
	close();
}

	/* for Win32 only */
#if defined(Q_OS_WIN)
#include <windows.h>

#endif

void InviteDialog::emailbutton()
{
	/* for Win32 only */
#if defined(Q_OS_WIN)

	std::string mailstr = "mailto:";

	mailstr += "&subject=RetroShare Invite";
	mailstr += "&body=";
	mailstr += ui.emailText->toPlainText().toStdString();

	/* search and replace the end of lines with: "%0D%0A" */

	std::cerr << "MAIL STRING:" << mailstr.c_str() << std::endl;

	size_t loc;
	while((loc = mailstr.find("\n")) != mailstr.npos)
	{
		/* sdfkasdflkjh */
		mailstr.replace(loc, 1, "%0D%0A");
	}

	HINSTANCE hInst = ShellExecuteA(0, 
		"open",
		mailstr.c_str(), 
		NULL, 
		NULL, 
		SW_SHOW);

    if(reinterpret_cast<int>(hInst) <= 32)
    {
	/* error */
	std::cerr << "ShellExecute Error: " << reinterpret_cast<int>(hInst);
	std::cerr << std::endl;
    }
  
#endif
}


void InviteDialog::setInfo(std::string invite)
{
	ui.emailText->setText(QString::fromStdString(invite));
}

		     
