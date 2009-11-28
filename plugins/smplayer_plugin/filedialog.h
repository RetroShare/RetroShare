/*  smplayer, GUI front-end for mplayer.
    Copyright (C) 2006-2008 Ricardo Villalba <rvm@escomposlinux.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef _FILEDIALOG_H
#define _FILEDIALOG_H

#include <QString>
#include <QStringList>
#include <QFileDialog>

class QWidget;

class MyFileDialog {

public:
	static QString getOpenFileName( QWidget * parent = 0, 
			const QString & caption = QString(), 
			const QString & dir = QString(), 
			const QString & filter = QString(), 
			QString * selectedFilter = 0, 
			QFileDialog::Options options = QFileDialog::DontResolveSymlinks ) ;

	static QString getExistingDirectory ( QWidget * parent = 0, 
			const QString & caption = QString(), 
			const QString & dir = QString(), 
			QFileDialog::Options options = QFileDialog::ShowDirsOnly );

	static QString getSaveFileName ( QWidget * parent = 0, 
			const QString & caption = QString(), 
			const QString & dir = QString(), 
			const QString & filter = QString(), 
			QString * selectedFilter = 0, 
			QFileDialog::Options options = QFileDialog::DontResolveSymlinks | 
                                           QFileDialog::DontConfirmOverwrite );

	static QStringList getOpenFileNames ( QWidget * parent = 0, 
			const QString & caption = QString(), 
			const QString & dir = QString(), 
			const QString & filter = QString(), 
			QString * selectedFilter = 0, 
			QFileDialog::Options options = QFileDialog::DontResolveSymlinks );

};

#endif

