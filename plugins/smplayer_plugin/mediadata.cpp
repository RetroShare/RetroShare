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

#include "mediadata.h"
#include <QFileInfo>
#include <cmath>


MediaData::MediaData() {
	reset();
}

MediaData::~MediaData() {
}

void MediaData::reset() {
	filename="";
	dvd_id="";
	type = TYPE_UNKNOWN;
	duration=0;

	novideo = FALSE;

	video_width=0;
    video_height=0;
    video_aspect= (double) 4/3;

	videos.clear();
	audios.clear();
	titles.clear();

	subs.clear();

#if GENERIC_CHAPTER_SUPPORT
	chapters = 0;
#else
	//chapters=0;
	//angles=0;

	mkv_chapters=0;
#endif

	initialized=false;

	// Clip info;
	clip_name = "";
    clip_artist = "";
    clip_author = "";
    clip_album = "";
    clip_genre = "";
    clip_date = "";
    clip_track = "";
    clip_copyright = "";
    clip_comment = "";
    clip_software = "";

	stream_title = "";
	stream_url = "";

	// Other data
	demuxer="";
	video_format="";
	audio_format="";
	video_bitrate=0;
	video_fps="";
	audio_bitrate=0;
	audio_rate=0;
	audio_nch=0;
	video_codec="";
	audio_codec="";
}

QString MediaData::displayName() {
	if (!clip_name.isEmpty()) return clip_name;
	else
	if (!stream_title.isEmpty()) return stream_title;

	QFileInfo fi(filename);
	if (fi.exists()) 
		return fi.fileName(); // filename without path
	else
		return filename;
}


void MediaData::list() {
	qDebug("MediaData::list");

	qDebug("  filename: '%s'", filename.toUtf8().data());
	qDebug("  duration: %f", duration);

	qDebug("  video_width: %d", video_width); 
	qDebug("  video_height: %d", video_height); 
	qDebug("  video_aspect: %f", video_aspect); 

	qDebug("  type: %d", type);
	qDebug("  novideo: %d", novideo);
	qDebug("  dvd_id: '%s'", dvd_id.toUtf8().data());

	qDebug("  initialized: %d", initialized);

#if GENERIC_CHAPTER_SUPPORT
	qDebug("  chapters: %d", chapters);
#else
	qDebug("  mkv_chapters: %d", mkv_chapters);
#endif

	qDebug("  Subs:");
	subs.list();

	qDebug("  Videos:");
	videos.list();

	qDebug("  Audios:");
	audios.list();

	qDebug("  Titles:");
	titles.list();

	//qDebug("  chapters: %d", chapters);
	//qDebug("  angles: %d", angles);

	qDebug("  demuxer: '%s'", demuxer.toUtf8().data() );
	qDebug("  video_format: '%s'", video_format.toUtf8().data() );
	qDebug("  audio_format: '%s'", audio_format.toUtf8().data() );
	qDebug("  video_bitrate: %d", video_bitrate );
	qDebug("  video_fps: '%s'", video_fps.toUtf8().data() );
	qDebug("  audio_bitrate: %d", audio_bitrate );
	qDebug("  audio_rate: %d", audio_rate );
	qDebug("  audio_nch: %d", audio_nch );
	qDebug("  video_codec: '%s'", video_codec.toUtf8().data() );
	qDebug("  audio_codec: '%s'", audio_codec.toUtf8().data() );
}

