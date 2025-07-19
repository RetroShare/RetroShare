/*******************************************************************************
 * gui/settings/AppearancePage.h                                               *
 *                                                                             *
 * Copyright 2009, Retroshare Team <retroshare.project@gmail.com>              *
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

#ifndef _APPERARANCEPAGE_H
#define _APPERARANCEPAGE_H

#include "retroshare-gui/configpage.h"
#include "gui/MainWindow.h"
#include "ui_AppearancePage.h"
#include "gui/common/FilesDefs.h"

class AppearancePage : public ConfigPage
{
	Q_OBJECT

public:
	/** Default Constructor */
	AppearancePage(QWidget * parent = 0, Qt::WindowFlags flags = Qt::WindowFlags());

	/** Loads the settings for this page */
	virtual void load();

    virtual QPixmap iconPixmap() const { return FilesDefs::getPixmapFromQtResourcePath(":/icons/settings/appearance.svg") ; }
	virtual QString pageName() const { return tr("Appearance") ; }
	virtual QString helpText() const { return ""; }

private slots:
	void loadStyleSheet(int index);

    void switch_status_grpStatus(bool b)   ;
    void switch_status_compactMode(bool b) ;
    void switch_status_showToolTip(bool b) ;
    void switch_status_ShowStatus(bool)  ;
    void switch_status_ShowPeer(bool)    ;
    void switch_status_ShowNAT(bool)    ;
    void switch_status_ShowDHT(bool)    ;
    void switch_status_ShowHashing(bool) ;
    void switch_status_ShowDisc(bool)    ;
    void switch_status_ShowRate(bool)    ;
    void switch_status_ShowOpMode(bool)  ;
    void switch_status_ShowSound(bool)   ;
    void switch_status_ShowToaster(bool) ;
    void switch_status_ShowSystray(bool) ;

    void updateLanguageCode()    ;
    void updateInterfaceStyle()  ;
    void updateSheetName()       ;
    void updateRbtPageOnToolBar();
//    void updateActionButtonLoc() ;
    void updateStatusToolTip()   ;

	void updateCmboToolButtonStyle();
	void updateCmboToolButtonSize();
//	void updateCmboListItemSize();

	void updateStyle() ;
	void updateFontSize();

private:
	void switch_status(MainWindow::StatusElement s,const QString& key,bool b);

	/** Qt Designer generated object */
	Ui::AppearancePage ui;
};

#endif
