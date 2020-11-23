/*******************************************************************************
 * gui/settings/MessagePage.h                                                  *
 *                                                                             *
 * Copyright (C) 2006 Crypton <retroshare.project@gmail.com>                   *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#ifndef MESSAGEPAGE_H
#define MESSAGEPAGE_H

#include <stdint.h>

#include "retroshare-gui/configpage.h"
#include "ui_MessagePage.h"

#include "gui/msgs/MessageInterface.h"
#include "gui/common/FilesDefs.h"

class MessagePage : public ConfigPage
{
    Q_OBJECT

public:
    MessagePage(QWidget * parent = 0, Qt::WindowFlags flags = 0);
    ~MessagePage();

    /** Loads the settings for this page */
    virtual void load();

     virtual QPixmap iconPixmap() const { return FilesDefs::getPixmapFromQtResourcePath(":/icons/settings/messages.svg") ; }
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
	void updateLoadEmoticons();

private:
    void fillTags();

    /* Pointer for not include of rsmsgs.h */
    MsgTagType *m_pTags;
    std::list<uint32_t> m_changedTagIds;

    Ui::MessagePage ui;
};

#endif // !MESSAGEPAGE_H

