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

#include "infofile.h"
#include <QFileInfo>
#include <QCoreApplication>
#include "helper.h"
#include "constants.h"


InfoFile::InfoFile() {
	row = 0;
}

InfoFile::~InfoFile() {
}

QString InfoFile::getInfo(MediaData md) {
	QString s;

	// General
	QFileInfo fi(md.filename);

	QString icon;
	switch (md.type) {
		case TYPE_FILE	:	if (md.novideo) 
								icon = "type_audio.png";
							else
								icon = "type_video.png"; 
							break;
		case TYPE_DVD	: 	icon = "type_dvd.png"; break;
		case TYPE_VCD	: 	icon = "type_vcd.png"; break;
		case TYPE_AUDIO_CD	: 	icon = "type_vcd.png"; break;
		case TYPE_STREAM : 	icon = "type_url.png"; break;
		default 		: 	icon = "type_unknown.png";
	}
	icon = "<img src=\":/icons-png/" + icon + "\"> ";

	if (md.type == TYPE_DVD) {
		s += title( icon + "dvd://" + QString::number(Helper::dvdSplitTitle(md.filename) ) );
	} else {
		s += title( icon + md.displayName() );
	}

	s += openPar( tr("General") );
	if (fi.exists()) {
		//s += addItem( tr("Path"), fi.dirPath() );
		s += addItem( tr("File"), fi.absoluteFilePath() );
		s += addItem( tr("Size"), tr("%1 KB (%2 MB)").arg(fi.size()/1024)
                                  .arg(fi.size()/1048576) );
	} else {
		QString url = md.filename;
		if (url.endsWith(IS_PLAYLIST_TAG)) {
			url = url.remove( QRegExp(IS_PLAYLIST_TAG_RX) );
		}
		s += addItem( tr("URL"), url );
	}
	s += addItem( tr("Length"), Helper::formatTime((int)md.duration) );
	s += addItem( tr("Demuxer"), md.demuxer );
	s += closePar();

	// Clip info
	QString c;
	if (!md.clip_name.isEmpty()) c+= addItem( tr("Name"), md.clip_name );
	if (!md.clip_artist.isEmpty()) c+= addItem( tr("Artist"), md.clip_artist );
	if (!md.clip_author.isEmpty()) c+= addItem( tr("Author"), md.clip_author );
	if (!md.clip_album.isEmpty()) c+= addItem( tr("Album"), md.clip_album );
	if (!md.clip_genre.isEmpty()) c+= addItem( tr("Genre"), md.clip_genre );
	if (!md.clip_date.isEmpty()) c+= addItem( tr("Date"), md.clip_date );
	if (!md.clip_track.isEmpty()) c+= addItem( tr("Track"), md.clip_track );
	if (!md.clip_copyright.isEmpty()) c+= addItem( tr("Copyright"), md.clip_copyright );
	if (!md.clip_comment.isEmpty()) c+= addItem( tr("Comment"), md.clip_comment );
	if (!md.clip_software.isEmpty()) c+= addItem( tr("Software"), md.clip_software );
	if (!md.stream_title.isEmpty()) c+= addItem( tr("Stream title"), md.stream_title );
	if (!md.stream_url.isEmpty()) c+= addItem( tr("Stream URL"), md.stream_url );

	if (!c.isEmpty()) {
		s += openPar( tr("Clip info") );
		s += c;
		s += closePar();
	}

	// Video info
	if (!md.novideo) {
		s += openPar( tr("Video") );
		s += addItem( tr("Resolution"), QString("%1 x %2").arg(md.video_width).arg(md.video_height) );
		s += addItem( tr("Aspect ratio"), QString::number(md.video_aspect) );
		s += addItem( tr("Format"), md.video_format );
		s += addItem( tr("Bitrate"), tr("%1 kbps").arg(md.video_bitrate / 1000) );
		s += addItem( tr("Frames per second"), md.video_fps );
		s += addItem( tr("Selected codec"), md.video_codec );
		s += closePar();
	}

	// Audio info
	s += openPar( tr("Initial Audio Stream") );
	s += addItem( tr("Format"), md.audio_format );
	s += addItem( tr("Bitrate"), tr("%1 kbps").arg(md.audio_bitrate / 1000) );
	s += addItem( tr("Rate"), tr("%1 Hz").arg(md.audio_rate) );
	s += addItem( tr("Channels"), QString::number(md.audio_nch) );
	s += addItem( tr("Selected codec"), md.audio_codec );
	s += closePar();

	// Audio Tracks
	if (md.audios.numItems() > 0) {
		s += openPar( tr("Audio Streams") );
		row++;
		s += openItem();
		s += "<td>" + tr("#", "Info for translators: this is a abbreviation for number") + "</td><td>" + 
              tr("Language") + "</td><td>" + tr("Name") +"</td><td>" +
              tr("ID", "Info for translators: this is a identification code") + "</td>";
		s += closeItem();
		for (int n = 0; n < md.audios.numItems(); n++) {
			row++;
			s += openItem();
			QString lang = md.audios.itemAt(n).lang();
			if (lang.isEmpty()) lang = "<i>&lt;"+tr("empty")+"&gt;</i>";
			QString name = md.audios.itemAt(n).name();
			if (name.isEmpty()) name = "<i>&lt;"+tr("empty")+"&gt;</i>";
			s += QString("<td>%1</td><td>%2</td><td>%3</td><td>%4</td>")
                 .arg(n).arg(lang).arg(name)
                 .arg(md.audios.itemAt(n).ID());
			s += closeItem();
		}
		s += closePar();
	}

	// Subtitles
	if (md.subs.numItems() > 0) {
		s += openPar( tr("Subtitles") );
		row++;
		s += openItem();
		s += "<td>" + tr("#", "Info for translators: this is a abbreviation for number") + "</td><td>" + 
              tr("Type") + "</td><td>" +
              tr("Language") + "</td><td>" + tr("Name") +"</td><td>" +
              tr("ID", "Info for translators: this is a identification code") + "</td>";
		s += closeItem();
		for (int n = 0; n < md.subs.numItems(); n++) {
			row++;
			s += openItem();
			QString t;
			switch (md.subs.itemAt(n).type()) {
				case SubData::File: t = "FILE_SUB"; break;
				case SubData::Vob:	t = "VOB"; break;
				default:			t = "SUB";
			}
			QString lang = md.subs.itemAt(n).lang();
			if (lang.isEmpty()) lang = "<i>&lt;"+tr("empty")+"&gt;</i>";
			QString name = md.subs.itemAt(n).name();
			if (name.isEmpty()) name = "<i>&lt;"+tr("empty")+"&gt;</i>";
			/*
			s += QString("<td>%1</td><td>%2</td><td>%3</td><td>%4</td><td>%5</td>")
                 .arg(n).arg(t).arg(lang).arg(name)
                 .arg(md.subs.itemAt(n).ID());
			*/
            s += "<td>" + QString::number(n) + "</td><td>" + t + 
                 "</td><td>" + lang + "</td><td>" + name + 
                 "</td><td>" + QString::number(md.subs.itemAt(n).ID()) + "</td>";
			s += closeItem();
		}
		s += closePar();
	}

	return s;
}

QString InfoFile::title(QString text) {
	return "<h1>" + text + "</h1>";
}

QString InfoFile::openPar(QString text) {
	return "<h2>" + text + "</h2>"
           "<table width=\"100%\">";
}

QString InfoFile::closePar() {
	row = 0;
	return "</table>";
}

QString InfoFile::openItem() {
	if (row % 2 == 1)
		return "<tr bgcolor=\"#c4daf4\">";
	else
		return "<tr bgcolor=\"#ffffc6\">";
}

QString InfoFile::closeItem() {
	return "</tr>";
}

QString InfoFile::addItem( QString tag, QString value ) {
	row++;
	return openItem() + 
           "<td><b>" + tag + "</b></td>" +
           "<td>" + value + "</td>" +
           closeItem();
}


inline QString InfoFile::tr( const char * sourceText, const char * comment, int n )  {
	return QCoreApplication::translate("InfoFile", sourceText, comment, QCoreApplication:: CodecForTr, n );
}

