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

#include "inputurl.h"
#include "images.h"

InputURL::InputURL( QWidget* parent, Qt::WindowFlags f ) 
	: QDialog(parent, f)
{
	setupUi(this);
	url_icon->setPixmap( Images::icon("url_big") );
	url_edit->setFocus();

	playlist_check->setWhatsThis( 
		tr("If this option is checked, the URL will be treated as a playlist: "
        "it will be opened as text and will play the URLs in it.") );
}

InputURL::~InputURL() {
}

void InputURL::setURL(QString url) {
	url_edit->setText(url);
	url_edit->selectAll();
}

QString InputURL::url() {
	return url_edit->text();
}

void InputURL::setPlaylist(bool b) {
	playlist_check->setChecked(b);
}

bool InputURL::isPlaylist() {
	return playlist_check->isChecked();
}

#include "moc_inputurl.cpp"
