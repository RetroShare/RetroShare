/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006 - 2010 RetroShare Team
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

#include <QPushButton>

#include <retroshare/rspeers.h>

#include "util/misc.h"

#include "CreateGroup.h"
#include "gui/common/GroupDefs.h"

/** Default constructor */
CreateGroup::CreateGroup(const std::string groupId, QWidget *parent, Qt::WFlags flags)
  : QDialog(parent, flags)
{
    /* Invoke Qt Designer generated QObject setup routine */
    ui.setupUi(this);

    m_groupId = groupId;

    if (m_groupId.empty() == false) {
        /* edit exisiting group */
        RsGroupInfo groupInfo;
        if (rsPeers->getGroupInfo(m_groupId, groupInfo)) {
            ui.groupname->setText(misc::removeNewLine(groupInfo.name));

            setWindowTitle(tr("Edit Group"));
            ui.headerLabel->setText(tr("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">"
                                       "<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">"
                                       "p, li { white-space: pre-wrap; }"
                                       "</style></head><body style=\" font-family:'MS Shell Dlg 2'; font-size:18pt; font-weight:600; font-style:normal;\">"
                                       "<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-weight:400; color:#ffffff;\">Edit Group</span></p></body></html>"));
        } else {
            /* Group not found, create new */
            m_groupId.clear();
        }
    }

    std::list<RsGroupInfo> groupInfoList;
    rsPeers->getGroupInfoList(groupInfoList);

    std::list<RsGroupInfo>::iterator groupIt;
    for (groupIt = groupInfoList.begin(); groupIt != groupInfoList.end(); groupIt++) {
        if (m_groupId.empty() || groupIt->id != m_groupId) {
            usedGroupNames.append(GroupDefs::name(*groupIt));
        }
    }

    on_groupname_textChanged(ui.groupname->text());
}

/** Destructor. */
CreateGroup::~CreateGroup()
{
}

void CreateGroup::on_groupname_textChanged(QString text)
{
    if (text.isEmpty() || usedGroupNames.contains(misc::removeNewLine(text))) {
        ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    } else {
        ui.buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    }
}

void CreateGroup::on_buttonBox_accepted()
{
    RsGroupInfo groupInfo;

    if (m_groupId.empty()) {
        // add new group
        groupInfo.name = misc::removeNewLine(ui.groupname->text()).toUtf8().constData();
        if (rsPeers->addGroup(groupInfo)) {
            close();
        }
    } else {
        if (rsPeers->getGroupInfo(m_groupId, groupInfo) == true) {
            groupInfo.name = misc::removeNewLine(ui.groupname->text()).toUtf8().constData();
            if (rsPeers->editGroup(m_groupId, groupInfo)) {
                close();
            }
        }
    }
}
