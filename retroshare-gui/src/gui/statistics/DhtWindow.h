/*******************************************************************************
 * gui/statistics/DhtWindow.h                                                  *
 *                                                                             *
 * Copyright (c) 2011 Robert Fernie   <retroshare.project@gmail.com>           *
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

#ifndef RSDHT_WINDOW_H
#define RSDHT_WINDOW_H

#include <retroshare-gui/RsAutoUpdatePage.h>

#include "ui_DhtWindow.h"

class DhtWindow : public RsAutoUpdatePage/*,  public Ui::DhtWindow*/ {
    Q_OBJECT
public:

    DhtWindow(QWidget *parent = 0);
    ~DhtWindow();

	void updateNetStatus();
	void updateNetPeers();
	void updateDhtPeers();
	void updateRelays();
	void getDHTStatus();

public slots:
	virtual void updateDisplay() ;
	
  void filterColumnChanged(int);
  void filterItems(const QString &text);
  
private slots:
	/** Create the context popup menu and it's submenus */
	void DHTCustomPopupMenu( QPoint point );
	void copyIP();
	
protected:
    //void changeEvent(QEvent *e);

private:

    bool filterItem(QTreeWidgetItem *item, const QString &text, int filterColumn);

    /** Qt Designer generated object */
    Ui::DhtWindow ui;

};

#endif // RSDHT_WINDOW_H

