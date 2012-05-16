/*************************************:***************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2011 - 2011 RetroShare Team
 *
 *  Cyril Soler (csoler@users.sourceforge.net)
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

#include "ui_RsCollectionDialog.h"
#include "RsCollectionFile.h"

class QCheckBox ;

class RsCollectionDialog: public QDialog, public Ui::RsCollectionDialog
{
	Q_OBJECT

public:
	RsCollectionDialog(const QString& filename,const std::vector<RsCollectionFile::DLinfo>& dlinfos) ;

private slots:
	void download() ;
	void selectAll() ;
	void deselectAll() ;
	void cancel() ;
	void updateSizes() ;
	void itemChanged(QTreeWidgetItem *item, int column);

protected:
	bool eventFilter(QObject *obj, QEvent *ev);

private:
	void selectDeselectAll(bool select);
	void connectUpdate(bool doConnect);

	const std::vector<RsCollectionFile::DLinfo>& _dlinfos ;
	QString _filename ;
};
