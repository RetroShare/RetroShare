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

#include "prefplaylist.h"
#include "preferences.h"
#include "images.h"

PrefPlaylist::PrefPlaylist(QWidget * parent, Qt::WindowFlags f)
	: PrefWidget(parent, f )
{
	setupUi(this);

	createHelp();
}

PrefPlaylist::~PrefPlaylist()
{
}

QString PrefPlaylist::sectionName() {
	return tr("Playlist");
}

QPixmap PrefPlaylist::sectionIcon() {
    return Images::icon("playlist");
}

void PrefPlaylist::retranslateStrings() {
	retranslateUi(this);
	createHelp();
}

void PrefPlaylist::setData(Preferences * pref) {
	setAutoAddFilesToPlaylist( pref->auto_add_to_playlist );
	setAddConsecutiveFiles( pref->add_to_playlist_consecutive_files );
}

void PrefPlaylist::getData(Preferences * pref) {
	requires_restart = false;

	pref->auto_add_to_playlist = autoAddFilesToPlaylist();
	pref->add_to_playlist_consecutive_files = addConsecutiveFiles();
}

void PrefPlaylist::setAutoAddFilesToPlaylist(bool b) {
	auto_add_to_playlist_check->setChecked(b);
}

bool PrefPlaylist::autoAddFilesToPlaylist() {
	return auto_add_to_playlist_check->isChecked();
}

void PrefPlaylist::setAddConsecutiveFiles(bool b) {
	add_consecutive_files_check->setChecked(b);
}

bool PrefPlaylist::addConsecutiveFiles() {
	return add_consecutive_files_check->isChecked();
}


void PrefPlaylist::createHelp() {
	clearHelp();

	setWhatsThis(auto_add_to_playlist_check, tr("Automatically add files to playlist"),
		tr("If this option is enabled, every time a file is opened, SMPlayer "
           "will first clear the playlist and then add the file to it. In "
           "case of DVDs, CDs and VCDs, all titles in the disc will be added "
           "to the playlist.") );

	setWhatsThis(add_consecutive_files_check, tr("Add consecutive files"),
		tr("If this option is enabled, SMPlayer will look for consecutive "
           "files (e.g. video_1.avi, video_2.avi...) and if found, they'll be "
           "added to the playlist.") );
}

#include "moc_prefplaylist.cpp"
