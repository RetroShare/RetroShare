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

#include "gui/profile/ProfileEdit.h"

#include "rsiface/rspeers.h"
#include "rsiface/rsQblog.h"

#include <sstream>

#include <QContextMenuEvent>
#include <QMenu>
#include <QFile>
#include <QFileDialog>
#include <QCursor>
#include <QPoint>
#include <QMouseEvent>
#include <QMessageBox>
#include <QPixmap>
#include <QPrintDialog>


/** Constructor */
ProfileEdit::ProfileEdit(QWidget *parent)
: QDialog(parent)
{
  /* Invoke the Qt Designer generated object setup routine */
  ui.setupUi(this);
  
  connect( ui.profileTreeWidget, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( profileCustomPopupMenu( QPoint ) ) );
 
  // connect up the buttons. 
  connect(ui.addButton, SIGNAL(clicked()), this, SLOT(profileEntryAdd()));
  connect(ui.moveDownButton, SIGNAL(clicked()), this, SLOT(profileEntryMoveDown()));
  connect(ui.moveUpButton, SIGNAL(clicked()), this, SLOT(profileEntryMoveUp()));
  connect(ui.closeButton, SIGNAL(clicked()), this, SLOT(close()));

  /* Hide platform specific features */
#ifdef Q_WS_WIN

#endif
}

void ProfileEdit::profileCustomPopupMenu( QPoint point )
{

      QMenu contextMnu( this );
      QMouseEvent *mevent = new QMouseEvent( QEvent::MouseButtonPress, point, Qt::RightButton, Qt::RightButton, Qt::NoModifier );

      QAction *removeAct = new QAction( tr( "Remove Profile Entry" ), this );
      QAction *moveupAct = new QAction( tr( "Move Profile Entry Up" ), this );
      QAction *movedownAct = new QAction( tr( "Move Profile Entry Down" ), this );

      connect( removeAct , SIGNAL( triggered() ), this, SLOT( profileEntryRemove() ) );
      connect( moveupAct , SIGNAL( triggered() ), this, SLOT( profileEntryMoveUp() ) );
      connect( movedownAct , SIGNAL( triggered() ), this, SLOT( profileEntryMoveDown() ) );

      contextMnu.clear();
      contextMnu.addAction( removeAct );
      contextMnu.addAction( moveupAct );
      contextMnu.addAction( movedownAct );
      contextMnu.exec( mevent->globalPos() );
}

void ProfileEdit::clear()
{
	return;
}

void ProfileEdit::update()
{
	/* load it up! */
	std::string pId = "OwnId";

	if (!rsQblog)
	{
		clear();
		return;
	}

	std::list< std::pair<std::wstring, std::wstring> > profile;
	std::list< std::pair<std::wstring, std::wstring> >::iterator pit;

	rsQblog -> getPeerProfile(pId, profile);

	QList<QTreeWidgetItem *> itemList;
	for(pit = profile.begin(); pit != profile.end(); pit++)
	{
		QTreeWidgetItem *item = new QTreeWidgetItem();
		item->setText(0, QString::fromStdWString(pit->first));
		item->setText(1, QString::fromStdWString(pit->second));

		itemList.push_back(item);
	}

	ui.profileTreeWidget->clear();
	ui.profileTreeWidget->insertTopLevelItems(0, itemList);

	return;
}

/* For context Menus */
/* for Profile */
void ProfileEdit::profileEntryAdd()
{
	return;
}

void ProfileEdit::profileEntryRemove()
{
	return;
}

void ProfileEdit::profileEntryMoveUp()
{
	return;
}

void ProfileEdit::profileEntryMoveDown()
{
	return;
}


void ProfileEdit::profileEntryCustomChanged() /* check box */
{
	return;
}


