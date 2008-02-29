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

#include "filedialog.h"
#include <QWidget>

QString MyFileDialog::getOpenFileName( QWidget * parent, 
			const QString & caption, 
			const QString & dir, const QString & filter, 
			QString * selectedFilter, QFileDialog::Options options ) 
{
	return QFileDialog::getOpenFileName( parent, caption, dir, filter,
										 selectedFilter, options );
}

QString MyFileDialog::getExistingDirectory ( QWidget * parent, 
			const QString & caption, 
			const QString & dir, 
			QFileDialog::Options options )
{
	return QFileDialog::getExistingDirectory( parent, caption, dir, options );
}

QString MyFileDialog::getSaveFileName ( QWidget * parent, 
			const QString & caption, 
			const QString & dir, 
			const QString & filter, 
			QString * selectedFilter, 
			QFileDialog::Options options )
{
	return QFileDialog::getSaveFileName( parent, caption, dir, filter,
                                         selectedFilter, options );
}

QStringList MyFileDialog::getOpenFileNames ( QWidget * parent, 
			const QString & caption, 
			const QString & dir, 
			const QString & filter, 
			QString * selectedFilter, 
			QFileDialog::Options options )
{
	return QFileDialog::getOpenFileNames( parent, caption, dir, filter,
                                          selectedFilter, options );
}

