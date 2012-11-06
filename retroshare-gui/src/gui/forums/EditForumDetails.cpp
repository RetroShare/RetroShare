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

#include "EditForumDetails.h"

#include <retroshare/rsforums.h>

#include "util/misc.h"

#include <list>
#include <iostream>
#include <string>


/** Default constructor */
EditForumDetails::EditForumDetails(std::string forumId, QWidget *parent)
  : QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint), m_forumId(forumId)
{
    /* Invoke Qt Designer generated QObject setup routine */
    ui.setupUi(this);

    connect(ui.buttonBox, SIGNAL(accepted()), this, SLOT(applyDialog()));
    connect(ui.buttonBox, SIGNAL(rejected()), this, SLOT(close()));

    loadForum();
}

void EditForumDetails::loadForum()
{
    if (!rsForums) {
        return;
    }

    ForumInfo info;
    rsForums->getForumInfo(m_forumId, info);

    // set name
    ui.nameline->setText(QString::fromStdWString(info.forumName));

    // set description
    ui.DescriptiontextEdit->setText(QString::fromStdWString(info.forumDesc));
}

void EditForumDetails::applyDialog()
{
    if (!rsForums) {
        return;
    }

    // if text boxes have not been edited leave alone
    if (!ui.nameline->isModified() && !ui.DescriptiontextEdit->document()->isModified()) {
        return;
    }

    ForumInfo info;

    info.forumName = misc::removeNewLine(ui.nameline->text()).toStdWString();
    info.forumDesc = ui.DescriptiontextEdit->document()->toPlainText().toStdWString();

    rsForums->setForumInfo(m_forumId, info);

    /* close the Dialog after the Changes applied */
    close();
}
