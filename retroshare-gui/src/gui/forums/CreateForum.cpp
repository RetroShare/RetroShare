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


#include "CreateForum.h"

#include "rsiface/rsforums.h"

/** Constructor */
CreateForum::CreateForum(QWidget *parent)
: QWidget(parent)
{
  /* Invoke the Qt Designer generated object setup routine */
  ui.setupUi(this);
  
  // connect up the buttons.
  connect( ui.cancelButton, SIGNAL( clicked ( bool ) ), this, SLOT( cancelForum( ) ) );
  connect( ui.createButton, SIGNAL( clicked ( bool ) ), this, SLOT( createForum( ) ) );

  newForum();

}


void  CreateForum::newForum()
{
	/* clear all */
	ui.forumTypePublic->setChecked(true);
	ui.forumMsgAnon->setChecked(true);
}

void  CreateForum::createForum()
{
	QString name = ui.forumName->text();
	QString desc = ui.forumDesc->toHtml();
	uint32_t flags = 0;

	if (ui.forumTypePublic->isChecked())
	{
		flags |= RS_FORUM_PUBLIC;
	}
	else if (ui.forumTypePrivate->isChecked())
	{
		flags |= RS_FORUM_PRIVATE;
	}
	else if (ui.forumTypeEncrypted->isChecked())
	{
		flags |= RS_FORUM_ENCRYPTED;
	}

	if (ui.forumMsgAuth->isChecked())
	{
		flags |= RS_FORUM_MSG_AUTH;
	}
	else if (ui.forumMsgAnon->isChecked())
	{
		flags |= RS_FORUM_MSG_ANON;
	}

	rsForums->createForum(name.toStdWString(), desc.toStdWString(), flags);

	close();
	return;
}


void  CreateForum::cancelForum()
{
	close();
	return;
}

