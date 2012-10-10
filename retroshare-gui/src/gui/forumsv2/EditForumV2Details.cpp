/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2010 RetroShare Team
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

#include "EditForumV2Details.h"

#include <retroshare/rsforumsv2.h>

#include "util/misc.h"

#include <list>
#include <iostream>
#include <string>


/** Default constructor */
EditForumV2Details::EditForumV2Details(std::string forumId, QWidget *parent, Qt::WFlags flags)
  : QDialog(parent, flags), m_forumId(forumId)
{
    /* Invoke Qt Designer generated QObject setup routine */
    ui.setupUi(this);

    connect(ui.buttonBox, SIGNAL(accepted()), this, SLOT(applyDialog()));
    connect(ui.buttonBox, SIGNAL(rejected()), this, SLOT(close()));

    loadForum();
}

void EditForumV2Details::loadForum()
{
    if (!rsForumsV2) {
        return;
    }

#warning "EditForumV2Details incomplete"
#if 0
    ForumInfo info;
    rsForumsV2->getForumInfo(m_forumId, info);

    // set name
    ui.nameline->setText(QString::fromStdWString(info.forumName));

    // set description
    ui.DescriptiontextEdit->setText(QString::fromStdWString(info.forumDesc));
#endif
	
}

void EditForumV2Details::applyDialog()
{
    if (!rsForumsV2) {
        return;
    }

    // if text boxes have not been edited leave alone
    if (!ui.nameline->isModified() && !ui.DescriptiontextEdit->document()->isModified()) {
        return;
    }

#warning "EditForumV2Details incomplete"
#if 0
	
    ForumInfo info;

    info.forumName = misc::removeNewLine(ui.nameline->text()).toStdWString();
    info.forumDesc = ui.DescriptiontextEdit->document()->toPlainText().toStdWString();

    rsForumsV2->setForumInfo(m_forumId, info);
#endif
	
    /* close the Dialog after the Changes applied */
    close();
}
