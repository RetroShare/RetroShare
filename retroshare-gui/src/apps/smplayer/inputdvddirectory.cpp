/*  smplayer, GUI front-end for mplayer.
    Copyright (C) 2007 Ricardo Villalba <rvm@escomposlinux.org>

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

#include "inputdvddirectory.h"

#include <QLineEdit>
#include "filedialog.h"

InputDVDDirectory::InputDVDDirectory( QWidget* parent, Qt::WindowFlags f )
	: QDialog(parent, f)
{
	setupUi(this);
}

InputDVDDirectory::~InputDVDDirectory() {
}

void InputDVDDirectory::setFolder(QString folder) {
	dvd_directory_edit->setText( folder );
}

QString InputDVDDirectory::folder() {
	return dvd_directory_edit->text();
}

void InputDVDDirectory::on_searchButton_clicked() {
	QString s = MyFileDialog::getExistingDirectory(
                    this, tr("Choose a directory"),
                    dvd_directory_edit->text() );
	/*
	QString s = QFileDialog::getOpenFileName(
                    dvd_directory_edit->text(),
                    "*.*", this,
                    "select_dvd_device_dialog",
                    tr("Choose a directory or iso file") );
	*/

	if (!s.isEmpty()) {
		dvd_directory_edit->setText(s);
	}
}

#include "moc_inputdvddirectory.cpp"
