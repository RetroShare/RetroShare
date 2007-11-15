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

#ifndef _MEDIADATA_H_
#define _MEDIADATA_H_

/* Here we store some volatile info about the file we need to remember */

#include "config.h"

#if SUBTITLES_BY_INDEX
#include "subtracks.h"
#endif

#include <QString>
#include <QSettings>
#include "tracks.h"

// Types of media

#define TYPE_UNKNOWN -1
#define TYPE_FILE 0
#define TYPE_DVD 1
#define TYPE_STREAM 2
#define TYPE_VCD 3
#define TYPE_AUDIO_CD 4

class MediaData {

public:
	MediaData();
	virtual ~MediaData();

	virtual void reset();

	QString filename;
	double duration;

	//Resolution of the video
	int video_width;
	int video_height;
	double video_aspect;

	int type; // file, dvd...
	QString dvd_id;

	bool novideo; // Only audio

	bool initialized;

	void list();

	TrackList audios;
	TrackList titles; // for DVDs

#if SUBTITLES_BY_INDEX
	SubTracks subs;
#else
	TrackList subtitles;
#endif

	//int chapters, angles; // for DVDs

	// Matroshka chapters
	int mkv_chapters;

	// Clip info
	QString clip_name;
	QString clip_artist;
	QString clip_author;
	QString clip_album;
	QString clip_genre;
	QString clip_date;
	QString clip_track;
	QString clip_copyright;
	QString clip_comment;
	QString clip_software;

	QString stream_title;
	QString stream_url;


	// Other data not really useful for us,
	// just to show info to the user.
	QString demuxer;
	QString video_format;
	QString audio_format;
	int video_bitrate;
	QString video_fps;
	int audio_bitrate;
	int audio_rate;
	int audio_nch; // channels?
	QString video_codec;
	QString audio_codec;

	/*QString info();*/
	QString displayName();
};

#endif
