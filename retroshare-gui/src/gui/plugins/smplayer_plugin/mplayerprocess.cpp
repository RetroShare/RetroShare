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

#include "mplayerprocess.h"
#include <QRegExp>
#include <QStringList>
#include <QApplication>

#include "global.h"
#include "preferences.h"
#include "mplayerversion.h"
#include "helper.h"

using namespace Global;

MplayerProcess::MplayerProcess(QObject * parent) : MyProcess(parent) 
{
#if NOTIFY_SUB_CHANGES
	qRegisterMetaType<SubTracks>("SubTracks");
#endif

#if NOTIFY_AUDIO_CHANGES
	qRegisterMetaType<Tracks>("Tracks");
#endif

	connect( this, SIGNAL(lineAvailable(QByteArray)),
			 this, SLOT(parseLine(QByteArray)) );

	connect( this, SIGNAL(finished(int,QProcess::ExitStatus)), 
             this, SLOT(processFinished(int,QProcess::ExitStatus)) );

	connect( this, SIGNAL(error(QProcess::ProcessError)),
             this, SLOT(gotError(QProcess::ProcessError)) );

	notified_mplayer_is_running = false;
	last_sub_id = -1;
	mplayer_svn = -1; // Not found yet
}

MplayerProcess::~MplayerProcess() {
}

bool MplayerProcess::start() {
	md.reset();
	notified_mplayer_is_running = false;
	last_sub_id = -1;
	mplayer_svn = -1; // Not found yet
	received_end_of_file = false;

#if NOTIFY_SUB_CHANGES
	subs.clear();
	subtitle_info_received = false;
	subtitle_info_changed = false;
#endif

#if NOTIFY_AUDIO_CHANGES
	audios.clear();
	audio_info_changed = false;
#endif

#if GENERIC_CHAPTER_SUPPORT
	dvd_current_title = -1;
#endif

	MyProcess::start();
	return waitForStarted();
}

void MplayerProcess::writeToStdin(QString text) {
	if (isRunning()) {
		//qDebug("MplayerProcess::writeToStdin");
		write( text.toLocal8Bit() + "\n");
	} else {
		qWarning("MplayerProcess::writeToStdin: process not running");
	}
}

static QRegExp rx_av("^[AV]: *([0-9,:.-]+)");
static QRegExp rx_frame("^[AV]:.* (\\d+)\\/.\\d+");// [0-9,.]+");
static QRegExp rx("^(.*)=(.*)");
#if !NOTIFY_AUDIO_CHANGES
static QRegExp rx_audio_mat("^ID_AID_(\\d+)_(LANG|NAME)=(.*)");
#endif
static QRegExp rx_video("^ID_VID_(\\d+)_(LANG|NAME)=(.*)");
static QRegExp rx_title("^ID_DVD_TITLE_(\\d+)_(LENGTH|CHAPTERS|ANGLES)=(.*)");
static QRegExp rx_winresolution("^VO: \\[(.*)\\] (\\d+)x(\\d+) => (\\d+)x(\\d+)");
static QRegExp rx_ao("^AO: \\[(.*)\\]");
static QRegExp rx_paused("^ID_PAUSED");
#if !CHECK_VIDEO_CODEC_FOR_NO_VIDEO
static QRegExp rx_novideo("^Video: no video");
#endif
static QRegExp rx_cache("^Cache fill:.*");
static QRegExp rx_create_index("^Generating Index:.*");
static QRegExp rx_play("^Starting playback...");
static QRegExp rx_connecting("^Connecting to .*");
static QRegExp rx_resolving("^Resolving .*");
static QRegExp rx_screenshot("^\\*\\*\\* screenshot '(.*)'");
static QRegExp rx_endoffile("^Exiting... \\(End of file\\)|^ID_EXIT=EOF");
static QRegExp rx_mkvchapters("\\[mkv\\] Chapter (\\d+) from");
static QRegExp rx_aspect2("^Movie-Aspect is ([0-9,.]+):1");
static QRegExp rx_fontcache("^\\[ass\\] Updating font cache|^\\[ass\\] Init");
 
// VCD
static QRegExp rx_vcd("^ID_VCD_TRACK_(\\d+)_MSF=(.*)");

// Audio CD
static QRegExp rx_cdda("^ID_CDDA_TRACK_(\\d+)_MSF=(.*)");


//Subtitles
static QRegExp rx_subtitle("^ID_(SUBTITLE|FILE_SUB|VOBSUB)_ID=(\\d+)");
static QRegExp rx_sid("^ID_(SID|VSID)_(\\d+)_(LANG|NAME)=(.*)");
static QRegExp rx_subtitle_file("^ID_FILE_SUB_FILENAME=(.*)");

// Audio
#if NOTIFY_AUDIO_CHANGES
static QRegExp rx_audio("^ID_AUDIO_ID=(\\d+)");
static QRegExp rx_audio_info("^ID_AID_(\\d+)_(LANG|NAME)=(.*)");
#endif

//Clip info
static QRegExp rx_clip_name("^ (name|title): (.*)", Qt::CaseInsensitive);
static QRegExp rx_clip_artist("^ artist: (.*)", Qt::CaseInsensitive);
static QRegExp rx_clip_author("^ author: (.*)", Qt::CaseInsensitive);
static QRegExp rx_clip_album("^ album: (.*)", Qt::CaseInsensitive);
static QRegExp rx_clip_genre("^ genre: (.*)", Qt::CaseInsensitive);
static QRegExp rx_clip_date("^ (creation date|year): (.*)", Qt::CaseInsensitive);
static QRegExp rx_clip_track("^ track: (.*)", Qt::CaseInsensitive);
static QRegExp rx_clip_copyright("^ copyright: (.*)", Qt::CaseInsensitive);
static QRegExp rx_clip_comment("^ comment: (.*)", Qt::CaseInsensitive);
static QRegExp rx_clip_software("^ software: (.*)", Qt::CaseInsensitive);

static QRegExp rx_stream_title("^.* StreamTitle='(.*)';StreamUrl='(.*)';");


void MplayerProcess::parseLine(QByteArray ba) {
	//qDebug("MplayerProcess::parseLine: '%s'", ba.data() );

	QString tag;
	QString value;

#if COLOR_OUTPUT_SUPPORT
    QString line = Helper::stripColorsTags(QString::fromLocal8Bit(ba));
#else
	QString line = QString::fromLocal8Bit(ba);
#endif

	// Parse A: V: line
	//qDebug("%s", line.toUtf8().data());
    if (rx_av.indexIn(line) > -1) {
		double sec = rx_av.cap(1).toDouble();
		//qDebug("cap(1): '%s'", rx_av.cap(1).toUtf8().data() );
		//qDebug("sec: %f", sec);

#if NOTIFY_SUB_CHANGES
		if (notified_mplayer_is_running) {
			if (subtitle_info_changed) {
				qDebug("MplayerProcess::parseLine: subtitle_info_changed");
				subtitle_info_changed = false;
				subtitle_info_received = false;
				emit subtitleInfoChanged(subs);
			}
			if (subtitle_info_received) {
				qDebug("MplayerProcess::parseLine: subtitle_info_received");
				subtitle_info_received = false;
				emit subtitleInfoReceivedAgain(subs);
			}
		}
#endif

#if NOTIFY_AUDIO_CHANGES
		if (notified_mplayer_is_running) {
			if (audio_info_changed) {
				qDebug("MplayerProcess::parseLine: audio_info_changed");
				audio_info_changed = false;
				emit audioInfoChanged(audios);
			}
		}
#endif

		if (!notified_mplayer_is_running) {
			qDebug("MplayerProcess::parseLine: starting sec: %f", sec);
#if GENERIC_CHAPTER_SUPPORT
			if ( (md.chapters <= 0) && (dvd_current_title > 0) && 
                 (md.titles.find(dvd_current_title) != -1) )
			{
				int idx = md.titles.find(dvd_current_title);
				md.chapters = md.titles.itemAt(idx).chapters();
				qDebug("MplayerProcess::parseLine: setting chapters to %d", md.chapters);
			}
#endif

#if CHECK_VIDEO_CODEC_FOR_NO_VIDEO
			// Another way to find out if there's no video
			if (md.video_codec.isEmpty()) {
				md.novideo = true;
				emit receivedNoVideo();
			}
#endif

			emit receivedStartingTime(sec);
			emit mplayerFullyLoaded();

			emit receivedCurrentFrame(0); // Ugly hack: set the frame counter to 0

			notified_mplayer_is_running = true;
		}
		
	    emit receivedCurrentSec( sec );

		// Check for frame
        if (rx_frame.indexIn(line) > -1) {
			int frame = rx_frame.cap(1).toInt();
			//qDebug(" frame: %d", frame);
			emit receivedCurrentFrame(frame);
		}
	}
	else {
		// Emulates mplayer version in Ubuntu:
		//if (line.startsWith("MPlayer 1.0rc1")) line = "MPlayer 2:1.0~rc1-0ubuntu13.1 (C) 2000-2006 MPlayer Team";

		emit lineAvailable(line);

		// Parse other things
		qDebug("MplayerProcess::parseLine: '%s'", line.toUtf8().data() );

		// Screenshot
		if (rx_screenshot.indexIn(line) > -1) {
			QString shot = rx_screenshot.cap(1);
			qDebug("MplayerProcess::parseLine: screenshot: '%s'", shot.toUtf8().data());
			emit receivedScreenshot( shot );
		}
		else

		// End of file
		if (rx_endoffile.indexIn(line) > -1)  {
			qDebug("MplayerProcess::parseLine: detected end of file");
			if (!received_end_of_file) {
				// In case of playing VCDs or DVDs, maybe the first title
    	        // is not playable, so the GUI doesn't get the info about
        	    // available titles. So if we received the end of file
            	// first let's pretend the file has started so the GUI can have
	            // the data.
				if ( !notified_mplayer_is_running) {
					emit mplayerFullyLoaded();
				}

				//emit receivedEndOfFile();
				// Send signal once the process is finished, not now!
				received_end_of_file = true;
			}
		}
		else

		// Window resolution
		if (rx_winresolution.indexIn(line) > -1) {
			/*
			md.win_width = rx_winresolution.cap(4).toInt();
			md.win_height = rx_winresolution.cap(5).toInt();
		    md.video_aspect = (double) md.win_width / md.win_height;
			*/

			int w = rx_winresolution.cap(4).toInt();
			int h = rx_winresolution.cap(5).toInt();

			emit receivedVO( rx_winresolution.cap(1) );
			emit receivedWindowResolution( w, h );
			//emit mplayerFullyLoaded();
		}
		else

#if !CHECK_VIDEO_CODEC_FOR_NO_VIDEO
		// No video
		if (rx_novideo.indexIn(line) > -1) {
			md.novideo = TRUE;
			emit receivedNoVideo();
			//emit mplayerFullyLoaded();
		}
		else
#endif

		// Pause
		if (rx_paused.indexIn(line) > -1) {
			emit receivedPause();
		}

		// Stream title
		if (rx_stream_title.indexIn(line) > -1) {
			QString s = rx_stream_title.cap(1);
			QString url = rx_stream_title.cap(2);
			qDebug("MplayerProcess::parseLine: stream_title: '%s'", s.toUtf8().data());
			qDebug("MplayerProcess::parseLine: stream_url: '%s'", url.toUtf8().data());
			md.stream_title = s;
			md.stream_url = url;
			emit receivedStreamTitleAndUrl( s, url );
		}

#if NOTIFY_SUB_CHANGES
		// Subtitles
		if ((rx_subtitle.indexIn(line) > -1) || (rx_sid.indexIn(line) > -1) || (rx_subtitle_file.indexIn(line) > -1)) {
			int r = subs.parse(line);
			subtitle_info_received = true;
			subtitle_info_changed = ((r == SubTracks::SubtitleAdded) || (r == SubTracks::SubtitleChanged));
		}
#endif

#if NOTIFY_AUDIO_CHANGES
		// Audio
		if (rx_audio.indexIn(line) > -1) {
			int ID = rx_audio.cap(1).toInt();
			qDebug("MplayerProcess::parseLine: ID_AUDIO_ID: %d", ID);
			if (audios.find(ID) == -1) audio_info_changed = true;
			audios.addID( ID );
		}

		if (rx_audio_info.indexIn(line) > -1) {
			int ID = rx_audio_info.cap(1).toInt();
			QString lang = rx_audio_info.cap(3);
			QString t = rx_audio_info.cap(2);
			qDebug("MplayerProcess::parseLine: Audio: ID: %d, Lang: '%s' Type: '%s'", 
                    ID, lang.toUtf8().data(), t.toUtf8().data());

			int idx = audios.find(ID);
			if (idx == -1) {
				qDebug("MplayerProcess::parseLine: audio %d doesn't exist, adding it", ID);

				audio_info_changed = true;
				if ( t == "NAME" ) 
					audios.addName(ID, lang);
				else
					audios.addLang(ID, lang);
			} else {
				qDebug("MplayerProcess::parseLine: audio %d exists, modifing it", ID);

				if (t == "NAME") {
					//qDebug("MplayerProcess::parseLine: name of audio %d: %s", ID, audios.itemAt(idx).name().toUtf8().constData());
					if (audios.itemAt(idx).name() != lang) {
						audio_info_changed = true;
						audios.addName(ID, lang);
					}
				} else {
					//qDebug("MplayerProcess::parseLine: language of audio %d: %s", ID, audios.itemAt(idx).lang().toUtf8().constData());
					if (audios.itemAt(idx).lang() != lang) {
						audio_info_changed = true;
						audios.addLang(ID, lang);
					}
				}
			}
		}
#endif

		// The following things are not sent when the file has started to play
		// (or if sent, smplayer will ignore anyway...)
		// So not process anymore, if video is playing to save some time
		if (notified_mplayer_is_running) {
			return;
		}

		if ( (mplayer_svn == -1) && (line.startsWith("MPlayer ")) ) {
			mplayer_svn = MplayerVersion::mplayerVersion(line);
			qDebug("MplayerProcess::parseLine: MPlayer SVN: %d", mplayer_svn);
			if (mplayer_svn <= 0) {
				qWarning("MplayerProcess::parseLine: couldn't parse mplayer version!");
				emit failedToParseMplayerVersion(line);
			}
		}

#if !NOTIFY_SUB_CHANGES
		// Subtitles
		if (rx_subtitle.indexIn(line) > -1) {
			md.subs.parse(line);
		}
		else
		if (rx_sid.indexIn(line) > -1) {
			md.subs.parse(line);
		}
		else
		if (rx_subtitle_file.indexIn(line) > -1) {
			md.subs.parse(line);
		}
#endif
		// AO
		if (rx_ao.indexIn(line) > -1) {
			emit receivedAO( rx_ao.cap(1) );
		}
		else

#if !NOTIFY_AUDIO_CHANGES
		// Matroska audio
		if (rx_audio_mat.indexIn(line) > -1) {
			int ID = rx_audio_mat.cap(1).toInt();
			QString lang = rx_audio_mat.cap(3);
			QString t = rx_audio_mat.cap(2);
			qDebug("MplayerProcess::parseLine: Audio: ID: %d, Lang: '%s' Type: '%s'", 
                    ID, lang.toUtf8().data(), t.toUtf8().data());

			if ( t == "NAME" ) 
				md.audios.addName(ID, lang);
			else
				md.audios.addLang(ID, lang);
		}
		else
#endif

		// Video tracks
		if (rx_video.indexIn(line) > -1) {
			int ID = rx_video.cap(1).toInt();
			QString lang = rx_video.cap(3);
			QString t = rx_video.cap(2);
			qDebug("MplayerProcess::parseLine: Video: ID: %d, Lang: '%s' Type: '%s'", 
                    ID, lang.toUtf8().data(), t.toUtf8().data());

			if ( t == "NAME" ) 
				md.videos.addName(ID, lang);
			else
				md.videos.addLang(ID, lang);
		}
		else

		// Matroshka chapters
		if (rx_mkvchapters.indexIn(line)!=-1) {
			int c = rx_mkvchapters.cap(1).toInt();
			qDebug("MplayerProcess::parseLine: mkv chapters: %d", c);
#if GENERIC_CHAPTER_SUPPORT
			if ((c+1) > md.chapters) {
				md.chapters = c+1;
				qDebug("MplayerProcess::parseLine: chapters set to: %d", md.chapters);
			}
#else
			if ((c+1) > md.mkv_chapters) {
				md.mkv_chapters = c+1;
				qDebug("MplayerProcess::parseLine: mkv_chapters set to: %d", md.mkv_chapters);
			}
#endif
		}
		else

		// VCD titles
		if (rx_vcd.indexIn(line) > -1 ) {
			int ID = rx_vcd.cap(1).toInt();
			QString length = rx_vcd.cap(2);
			//md.titles.addID( ID );
			md.titles.addName( ID, length );
		}
		else

		// Audio CD titles
		if (rx_cdda.indexIn(line) > -1 ) {
			int ID = rx_cdda.cap(1).toInt();
			QString length = rx_cdda.cap(2);
			double duration = 0;
			QRegExp r("(\\d+):(\\d+):(\\d+)");
			if ( r.indexIn(length) > -1 ) {
				duration = r.cap(1).toInt() * 60;
				duration += r.cap(2).toInt();
			}
			md.titles.addID( ID );
			/*
			QString name = QString::number(ID) + " (" + length + ")";
			md.titles.addName( ID, name );
			*/
			md.titles.addDuration( ID, duration );
		}
		else

		// DVD titles
		if (rx_title.indexIn(line) > -1) {
			int ID = rx_title.cap(1).toInt();
			QString t = rx_title.cap(2);

			if (t=="LENGTH") {
				double length = rx_title.cap(3).toDouble();
				qDebug("MplayerProcess::parseLine: Title: ID: %d, Length: '%f'", ID, length);
				md.titles.addDuration(ID, length);
			} 
			else
			if (t=="CHAPTERS") {
				int chapters = rx_title.cap(3).toInt();
				qDebug("MplayerProcess::parseLine: Title: ID: %d, Chapters: '%d'", ID, chapters);
				md.titles.addChapters(ID, chapters);
			}
			else
			if (t=="ANGLES") {
				int angles = rx_title.cap(3).toInt();
				qDebug("MplayerProcess::parseLine: Title: ID: %d, Angles: '%d'", ID, angles);
				md.titles.addAngles(ID, angles);
			}
		}
		else

		// Catch cache messages
		if (rx_cache.indexIn(line) > -1) {
			emit receivedCacheMessage(line);
		}
		else

		// Creating index
		if (rx_create_index.indexIn(line) > -1) {
			emit receivedCreatingIndex(line);
		}
		else

		// Catch connecting message
		if (rx_connecting.indexIn(line) > -1) {
			emit receivedConnectingToMessage(line);
		}
		else

		// Catch resolving message
		if (rx_resolving.indexIn(line) > -1) {
			emit receivedResolvingMessage(line);
		}
		else

		// Aspect ratio for old versions of mplayer
		if (rx_aspect2.indexIn(line) > -1) {
			md.video_aspect = rx_aspect2.cap(1).toDouble();
			qDebug("MplayerProcess::parseLine: md.video_aspect set to %f", md.video_aspect);
		}
		else

		// Clip info

		//QString::trimmed() is used for removing leading and trailing whitespaces
		//Some .mp3 files contain tags with starting and ending whitespaces
		//Unfortunately MPlayer gives us leading and trailing whitespaces, Winamp for example doesn't show them

		// Name
		if (rx_clip_name.indexIn(line) > -1) {
			QString s = rx_clip_name.cap(2).trimmed();
			qDebug("MplayerProcess::parseLine: clip_name: '%s'", s.toUtf8().data());
			md.clip_name = s;
		}
		else

		// Artist
		if (rx_clip_artist.indexIn(line) > -1) {
			QString s = rx_clip_artist.cap(1).trimmed();
			qDebug("MplayerProcess::parseLine: clip_artist: '%s'", s.toUtf8().data());
			md.clip_artist = s;
		}
		else

		// Author
		if (rx_clip_author.indexIn(line) > -1) {
			QString s = rx_clip_author.cap(1).trimmed();
			qDebug("MplayerProcess::parseLine: clip_author: '%s'", s.toUtf8().data());
			md.clip_author = s;
		}
		else

		// Album
		if (rx_clip_album.indexIn(line) > -1) {
			QString s = rx_clip_album.cap(1).trimmed();
			qDebug("MplayerProcess::parseLine: clip_album: '%s'", s.toUtf8().data());
			md.clip_album = s;
		}
		else

		// Genre
		if (rx_clip_genre.indexIn(line) > -1) {
			QString s = rx_clip_genre.cap(1).trimmed();
			qDebug("MplayerProcess::parseLine: clip_genre: '%s'", s.toUtf8().data());
			md.clip_genre = s;
		}
		else

		// Date
		if (rx_clip_date.indexIn(line) > -1) {
			QString s = rx_clip_date.cap(2).trimmed();
			qDebug("MplayerProcess::parseLine: clip_date: '%s'", s.toUtf8().data());
			md.clip_date = s;
		}
		else

		// Track
		if (rx_clip_track.indexIn(line) > -1) {
			QString s = rx_clip_track.cap(1).trimmed();
			qDebug("MplayerProcess::parseLine: clip_track: '%s'", s.toUtf8().data());
			md.clip_track = s;
		}
		else

		// Copyright
		if (rx_clip_copyright.indexIn(line) > -1) {
			QString s = rx_clip_copyright.cap(1).trimmed();
			qDebug("MplayerProcess::parseLine: clip_copyright: '%s'", s.toUtf8().data());
			md.clip_copyright = s;
		}
		else

		// Comment
		if (rx_clip_comment.indexIn(line) > -1) {
			QString s = rx_clip_comment.cap(1).trimmed();
			qDebug("MplayerProcess::parseLine: clip_comment: '%s'", s.toUtf8().data());
			md.clip_comment = s;
		}
		else

		// Software
		if (rx_clip_software.indexIn(line) > -1) {
			QString s = rx_clip_software.cap(1).trimmed();
			qDebug("MplayerProcess::parseLine: clip_software: '%s'", s.toUtf8().data());
			md.clip_software = s;
		}
		else

		if (rx_fontcache.indexIn(line) > -1) {
			//qDebug("MplayerProcess::parseLine: updating font cache");
			emit receivedUpdatingFontCache();
		}
		else

		// Catch starting message
		/*
		pos = rx_play.indexIn(line);
		if (pos > -1) {
			emit mplayerFullyLoaded();
		}
		*/

		//Generic things
		if (rx.indexIn(line) > -1) {
			tag = rx.cap(1);
			value = rx.cap(2);
			//qDebug("MplayerProcess::parseLine: tag: %s, value: %s", tag.toUtf8().data(), value.toUtf8().data());

#if !NOTIFY_AUDIO_CHANGES
			// Generic audio
			if (tag == "ID_AUDIO_ID") {
				int ID = value.toInt();
				qDebug("MplayerProcess::parseLine: ID_AUDIO_ID: %d", ID);
				md.audios.addID( ID );
			}
			else
#endif

			// Video
			if (tag == "ID_VIDEO_ID") {
				int ID = value.toInt();
				qDebug("MplayerProcess::parseLine: ID_VIDEO_ID: %d", ID);
				md.videos.addID( ID );
			}
			else
			if (tag == "ID_LENGTH") {
				md.duration = value.toDouble();
				qDebug("MplayerProcess::parseLine: md.duration set to %f", md.duration);
			}
			else
			if (tag == "ID_VIDEO_WIDTH") {
				md.video_width = value.toInt();
				qDebug("MplayerProcess::parseLine: md.video_width set to %d", md.video_width);
			}
			else
			if (tag == "ID_VIDEO_HEIGHT") {
				md.video_height = value.toInt();
				qDebug("MplayerProcess::parseLine: md.video_height set to %d", md.video_height);
			}
			else
			if (tag == "ID_VIDEO_ASPECT") {
				md.video_aspect = value.toDouble();
				if ( md.video_aspect == 0.0 ) {
					// I hope width & height are already set.
					md.video_aspect = (double) md.video_width / md.video_height;
				}
				qDebug("MplayerProcess::parseLine: md.video_aspect set to %f", md.video_aspect);
			}
			else
			if (tag == "ID_DVD_DISC_ID") {
				md.dvd_id = value;
				qDebug("MplayerProcess::parseLine: md.dvd_id set to '%s'", md.dvd_id.toUtf8().data());
			}
			else
			if (tag == "ID_DEMUXER") {
				md.demuxer = value;
			}
			else
			if (tag == "ID_VIDEO_FORMAT") {
				md.video_format = value;
			}
			else
			if (tag == "ID_AUDIO_FORMAT") {
				md.audio_format = value;
			}
			else
			if (tag == "ID_VIDEO_BITRATE") {
				md.video_bitrate = value.toInt();
			}
			else
			if (tag == "ID_VIDEO_FPS") {
				md.video_fps = value;
			}
			else
			if (tag == "ID_AUDIO_BITRATE") {
				md.audio_bitrate = value.toInt();
			}
			else
			if (tag == "ID_AUDIO_RATE") {
				md.audio_rate = value.toInt();
			}
			else
			if (tag == "ID_AUDIO_NCH") {
				md.audio_nch = value.toInt();
			}
			else
			if (tag == "ID_VIDEO_CODEC") {
				md.video_codec = value;
			}
			else
			if (tag == "ID_AUDIO_CODEC") {
				md.audio_codec = value;
			}
#if GENERIC_CHAPTER_SUPPORT
			else
			if (tag == "ID_CHAPTERS") {
				md.chapters = value.toInt();
			}
			else
			if (tag == "ID_DVD_CURRENT_TITLE") {
				dvd_current_title = value.toInt();
			}
#endif
		}
	}
}

// Called when the process is finished
void MplayerProcess::processFinished(int exitCode, QProcess::ExitStatus exitStatus) {
	qDebug("MplayerProcess::processFinished: exitCode: %d, status: %d", exitCode, (int) exitStatus);
	// Send this signal before the endoffile one, otherwise
	// the playlist will start to play next file before all
	// objects are notified that the process has exited.
	emit processExited();
	if (received_end_of_file) emit receivedEndOfFile();
}

void MplayerProcess::gotError(QProcess::ProcessError error) {
	qDebug("MplayerProcess::gotError: %d", (int) error);
}

#include "moc_mplayerprocess.cpp"
