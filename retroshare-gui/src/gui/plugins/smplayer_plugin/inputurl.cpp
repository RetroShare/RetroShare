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

	connect(url_edit, SIGNAL(currentIndexChanged(int)), this, SLOT(indexChanged()));
	connect(playlist_check, SIGNAL(stateChanged(int)), this, SLOT(playlistChanged(int)));
}

InputURL::~InputURL() {
}

void InputURL::setURL(QString url, bool is_playlist) {
	url_edit->addItem(url, is_playlist);
}

QString InputURL::url() {
	return url_edit->currentText();
}

void InputURL::setPlaylist(bool b) {
	playlist_check->setChecked(b);
	/*
	int pos = url_edit->currentIndex();
	url_edit->setItemData(pos, b);
	*/
}

bool InputURL::isPlaylist() {
	return playlist_check->isChecked();
}

void InputURL::indexChanged(void) {
	int pos = url_edit->currentIndex();
	if (url_edit->itemText(pos) == url_edit->currentText()) {
		playlist_check->setChecked( url_edit->itemData(pos).toBool() );
	}
}

void InputURL::playlistChanged(int state) {
	/*
	int pos = url_edit->currentIndex();
	if (url_edit->itemText(pos) == url_edit->currentText()) {
		bool is_playlist = (state == Qt::Checked);
		url_edit->setItemIcon( pos, is_playlist ? Images::icon("playlist") : QIcon() );
	}
	*/
}

#include "moc_inputurl.cpp"
