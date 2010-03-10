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

#include "common/vmessagebox.h"

#include "AddLinksDialog.h"
#include "RetroShareLink.h"
#include "rsiface/rsrank.h"

/* Images for context menu icons */
#define IMAGE_EXPORTFRIEND      ":/images/exportpeers_16x16.png"
#define IMAGE_GREAT			    ":/images/filerating5.png"
#define IMAGE_GOOD			    ":/images/filerating4.png"
#define IMAGE_OK			    ":/images/filerating3.png"
#define IMAGE_SUX			    ":/images/filerating2.png"
#define IMAGE_BADLINK			":/images/filerating1.png"

/** Constructor */
AddLinksDialog::AddLinksDialog(QString url, QWidget *parent)
: QDialog(parent)
{
  /* Invoke the Qt Designer generated object setup routine */
  ui.setupUi(this);

  /* add button */
  connect(ui.addLinkButton, SIGNAL(clicked()), this, SLOT(addLinkComment()));
  connect(ui.closepushButton, SIGNAL(clicked()), this, SLOT(close()));
  
  connect( ui.anonBox, SIGNAL( stateChanged ( int ) ), this, SLOT( load ( void  ) ) );

  ui.linkLineEdit->setReadOnly(true);
  ui.linkLineEdit->setText(url);

  RetroShareLink link(url);

  if(link.valid())
	  ui.titleLineEdit->setText(link.name());
  else
	  ui.titleLineEdit->setText("New File");

  load();

  /* Hide platform specific features */
#ifdef Q_WS_WIN

#endif
}

int AddLinksDialog::IndexToScore(int index)
{
	if ((index == -1) || (index > 4))
		return 0;
	int score = 2 - index;
	return score;
}

void AddLinksDialog::addLinkComment()
{
	/* get the title / link / comment */
	QString title = ui.titleLineEdit->text();
	QString link = ui.linkLineEdit->text();
	QString comment = ui.linkTextEdit->toPlainText();
	int32_t score = AddLinksDialog::IndexToScore(ui.scoreBox->currentIndex());

	if ((link == "") || (title == ""))
	{
		QMessageBox::StandardButton sb = QMessageBox::warning(NULL,
									"Add Link Failure",
									"Missing Link and/or Title",
									QMessageBox::Ok);
		/* can't do anything */
		return;
	}

	/* add it either way */
	if (ui.anonBox->isChecked())
	{
		rsRanks->anonRankMsg("", link.toStdWString(), title.toStdWString());
	}
	else
	{
		rsRanks->newRankMsg(link.toStdWString(),
				title.toStdWString(),
				comment.toStdWString(), score);
	}

	close();
}

void AddLinksDialog::load()
{
  if (ui.anonBox->isChecked())
	{

		/* disable comment + score */
		ui.scoreBox->setEnabled(false);
		ui.linkTextEdit->setEnabled(false);

		/* done! */
		return;
	}
	else
	{
		/* enable comment + score */
		ui.scoreBox->setEnabled(true);
		ui.linkTextEdit->setEnabled(true);
	}
}
