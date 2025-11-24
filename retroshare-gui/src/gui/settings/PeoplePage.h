/*******************************************************************************
 * gui/settings/PeoplePage.h                                                   *
 *                                                                             *
 * Copyright 2006, Crypton         <retroshare.project@gmail.com>              *
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

#ifndef PEOPLEPAGE_H
#define PEOPLEPAGE_H

#include "retroshare-gui/configpage.h"
#include "ui_PeoplePage.h"
#include "gui/common/FilesDefs.h"

class PeoplePage : public ConfigPage
{
	Q_OBJECT

public:
	PeoplePage(QWidget * parent = 0, Qt::WindowFlags flags = Qt::WindowFlags());
	~PeoplePage();

	/** Loads the settings for this page */
	virtual void load();

    virtual QPixmap iconPixmap() const { return FilesDefs::getPixmapFromQtResourcePath(":/icons/settings/people.svg") ; }
	virtual QString pageName() const { return tr("People") ; }
	virtual QString helpText() const { return ""; }

protected slots:
    void updateAutoPositiveOpinion() ;

    void updateThresholdForRemotelyPositiveReputation();
    void updateThresholdForRemotelyNegativeReputation();

    void updateRememberDeletedNodes();
    void updateDeleteBannedNodesThreshold() ;
	void updateAutoAddFriendIdsAsContact()  ;

private:
	Ui::PeoplePage ui;
};

#endif 

