/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006, crypton
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

#ifndef MESSAGEPAGE_H
#define MESSAGEPAGE_H

#include <stdint.h>

#include <retroshare-gui/configpage.h>
#include "ui_MessagePage.h"

#include "gui/msgs/MessageInterface.h"

class MessagePage : public ConfigPage
{
    Q_OBJECT

public:
    MessagePage(QWidget * parent = 0, Qt::WindowFlags flags = 0);
    ~MessagePage();

    /** Loads the settings for this page */
    virtual void load();

	 virtual QPixmap iconPixmap() const { return QPixmap(":/icons/settings/messages.svg") ; }
	 virtual QString pageName() const { return tr("Mail") ; }
	 virtual QString helpText() const { return ""; }


private slots:
    void addTag();
    void editTag();
    void deleteTag();
    void defaultTag();

    void currentRowChangedTag(int row);
    void distantMsgsComboBoxChanged(int);
	 
	void updateMsgToReadOnActivate() ;
	void updateLoadEmbededImages()       ;
	void updateMsgOpen()                 ;
	void updateDistantMsgs()             ;
	void updateMsgTags()    ;

private:
    void fillTags();

    /* Pointer for not include of rsmsgs.h */
    MsgTagType *m_pTags;
    std::list<uint32_t> m_changedTagIds;

    Ui::MessagePage ui;
};

#endif // !MESSAGEPAGE_H

