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

#include "mediasettings.h"
#include "preferences.h"
#include "global.h"
#include <QSettings>

MediaSettings::MediaSettings() {
	reset();
}

MediaSettings::~MediaSettings() {
}

void MediaSettings::reset() {
	current_sec = 0;
	current_sub_id = NoneSelected;
	current_audio_id = NoneSelected;
	current_title_id = NoneSelected;
	current_chapter_id = NoneSelected;
	current_angle_id = NoneSelected;
	letterbox = NoLetterbox;
	aspect_ratio_id = AspectAuto;
	//fullscreen = FALSE;
	volume = pref->initial_volume;
	mute = false;
	external_subtitles = "";
	external_audio = "";
	sub_delay=0;
	audio_delay=0;
	sub_pos = pref->initial_sub_pos; // 100% by default

	brightness = pref->initial_brightness;
	contrast = pref->initial_contrast;
	gamma = pref->initial_gamma;
	hue = pref->initial_hue;
	saturation = pref->initial_saturation;

	speed = 1.0;

	phase_filter = FALSE;
    current_denoiser = NoDenoise;
    deblock_filter = FALSE;
	dering_filter = FALSE;
	noise_filter = FALSE;
	postprocessing_filter = pref->initial_postprocessing;

	current_deinterlacer = NoDeinterlace;
	panscan_filter = "";
	crop_43to169_filter = "";
    karaoke_filter = false;
	extrastereo_filter = false;
	volnorm_filter = pref->initial_volnorm;

	audio_use_channels = pref->initial_audio_channels; //ChDefault; // (0)
	stereo_mode = Stereo; // (0)

	panscan_factor = pref->initial_panscan_factor; // 1.0;

	starting_time = -1; // Not set yet.

	flip = false;

	forced_demuxer="";
    forced_video_codec="";
    forced_audio_codec="";

	original_demuxer="";
    original_video_codec="";
    original_audio_codec="";

	mplayer_additional_options="";
	mplayer_additional_video_filters="";
	mplayer_additional_audio_filters="";

	win_width=400;
	win_height=300;
}

double MediaSettings::win_aspect() {
	return (double) win_width / win_height;
}

void MediaSettings::list() {
	qDebug("MediaSettings::list");

	qDebug("  current_sec: %f", current_sec);
	qDebug("  current_sub_id: %d", current_sub_id);
	qDebug("  current_audio_id: %d", current_audio_id);
	qDebug("  current_title_id: %d", current_title_id);
	qDebug("  current_chapter_id: %d", current_chapter_id);
	qDebug("  current_angle_id: %d", current_angle_id);
	qDebug("  letterbox: %d", letterbox);
	qDebug("  aspect_ratio_id: %d", aspect_ratio_id);
	//qDebug("  fullscreen: %d", fullscreen);
	qDebug("  volume: %d", volume);
	qDebug("  mute: %d", mute);
	qDebug("  external_subtitles: '%s'", external_subtitles.toUtf8().data());
	qDebug("  external_audio: '%s'", external_audio.toUtf8().data());
	qDebug("  sub_delay: %d", sub_delay);
	qDebug("  audio_delay: %d", sub_delay);
	qDebug("  sub_pos: %d", sub_pos);

	qDebug("  brightness: %d", brightness);
	qDebug("  contrast: %d", contrast);
	qDebug("  gamma: %d", gamma);
	qDebug("  hue: %d", hue);
	qDebug("  saturation: %d", saturation);

	qDebug("  speed: %f", speed);

	qDebug("  phase_filter: %d", phase_filter);
	qDebug("  current_denoiser: %d", current_denoiser);
	qDebug("  deblock_filter: %d", deblock_filter);
	qDebug("  dering_filter: %d", dering_filter);
	qDebug("  noise_filter: %d", noise_filter);
	qDebug("  postprocessing_filter: %d", postprocessing_filter);

	qDebug("  current_deinterlacer: %d", current_deinterlacer);
	qDebug("  panscan_filter: '%s'", panscan_filter.toUtf8().data());
	qDebug("  crop_43to169_filter: '%s'", crop_43to169_filter.toUtf8().data());
	qDebug("  karaoke_filter: %d", karaoke_filter);
	qDebug("  extrastereo_filter: %d", extrastereo_filter);
	qDebug("  volnorm_filter: %d", volnorm_filter);

	qDebug("  audio_use_channels: %d", audio_use_channels);
	qDebug("  stereo_mode: %d", stereo_mode);

	qDebug("  panscan_factor: %f", panscan_factor);

	qDebug("  flip: %d", flip);

	qDebug("  forced_demuxer: '%s'", forced_demuxer.toUtf8().data());
	qDebug("  forced_video_codec: '%s'", forced_video_codec.toUtf8().data());
	qDebug("  forced_audio_codec: '%s'", forced_video_codec.toUtf8().data());

	qDebug("  original_demuxer: '%s'", original_demuxer.toUtf8().data());
	qDebug("  original_video_codec: '%s'", original_video_codec.toUtf8().data());
	qDebug("  original_audio_codec: '%s'", original_video_codec.toUtf8().data());

	qDebug("  mplayer_additional_options: '%s'", mplayer_additional_options.toUtf8().data());
	qDebug("  mplayer_additional_video_filters: '%s'", mplayer_additional_video_filters.toUtf8().data());
	qDebug("  mplayer_additional_audio_filters: '%s'", mplayer_additional_audio_filters.toUtf8().data());

	qDebug("  win_width: %d", win_width);
	qDebug("  win_height: %d", win_height); 
	qDebug("  win_aspect(): %f", win_aspect()); 

	qDebug("  starting_time: %f", starting_time);
}

void MediaSettings::save() {
	qDebug("MediaSettings::save");

	QSettings * set = settings;

	/*set->beginGroup( "mediasettings" );*/

	set->setValue( "current_sec", current_sec );
	set->setValue( "current_sub_id", current_sub_id );
	set->setValue( "current_audio_id", current_audio_id );
	set->setValue( "current_title_id", current_title_id );
	set->setValue( "current_chapter_id", current_chapter_id );
	set->setValue( "current_angle_id", current_angle_id );
	set->setValue( "letterbox", letterbox );
	set->setValue( "aspect_ratio_id", aspect_ratio_id );
	//set->setValue( "fullscreen", fullscreen );
	set->setValue( "volume", volume );
	set->setValue( "mute", mute );
	set->setValue( "external_subtitles", external_subtitles );
	set->setValue( "external_audio", external_audio );
	set->setValue( "sub_delay", sub_delay);
	set->setValue( "audio_delay", audio_delay);
	set->setValue( "sub_pos", sub_pos);

	set->setValue( "brightness", brightness);
	set->setValue( "contrast", contrast);
	set->setValue( "gamma", gamma);
	set->setValue( "hue", hue);
	set->setValue( "saturation", saturation);

	set->setValue( "speed", speed);

	set->setValue( "phase_filter", phase_filter);
	set->setValue( "current_denoiser", current_denoiser);
	set->setValue( "deblock_filter", deblock_filter);
	set->setValue( "dering_filter", dering_filter);
	set->setValue( "noise_filter", noise_filter);
	set->setValue( "postprocessing_filter", postprocessing_filter);

	set->setValue( "current_deinterlacer", current_deinterlacer);
	set->setValue( "panscan_filter", panscan_filter);
	set->setValue( "crop_43to169_filter", crop_43to169_filter);
	set->setValue( "karaoke_filter", karaoke_filter);
	set->setValue( "extrastereo_filter", extrastereo_filter);
	set->setValue( "volnorm_filter", volnorm_filter);

	set->setValue( "audio_use_channels", audio_use_channels);
	set->setValue( "stereo_mode", stereo_mode);

	set->setValue( "panscan_factor", panscan_factor);

	set->setValue( "flip", flip);

	set->setValue( "forced_demuxer", forced_demuxer);
	set->setValue( "forced_video_codec", forced_video_codec);
	set->setValue( "forced_audio_codec", forced_audio_codec);

	set->setValue( "original_demuxer", original_demuxer);
	set->setValue( "original_video_codec", original_video_codec);
	set->setValue( "original_audio_codec", original_audio_codec);

	set->setValue( "mplayer_additional_options", mplayer_additional_options);
	set->setValue( "mplayer_additional_video_filters", mplayer_additional_video_filters);
	set->setValue( "mplayer_additional_audio_filters", mplayer_additional_audio_filters);

	set->setValue( "win_width", win_width );
	set->setValue( "win_height", win_height );

	set->setValue( "starting_time", starting_time );

	/*set->endGroup();*/
}

void MediaSettings::load() {
	qDebug("MediaSettings::load");

	QSettings * set = settings;

	/*set->beginGroup( "mediasettings" );*/

	current_sec = set->value( "current_sec", current_sec).toDouble();
	current_sub_id = set->value( "current_sub_id", current_sub_id ).toInt();
	current_audio_id = set->value( "current_audio_id", current_audio_id ).toInt();
	current_title_id = set->value( "current_title_id", current_title_id ).toInt();
	current_chapter_id = set->value( "current_chapter_id", current_chapter_id ).toInt();
	current_angle_id = set->value( "current_angle_id", current_angle_id ).toInt();
	letterbox = (LetterboxType) set->value( "letterbox", letterbox ).toInt();
	aspect_ratio_id = set->value( "aspect_ratio_id", aspect_ratio_id ).toInt();
	//fullscreen = set->value( "fullscreen", fullscreen ).toBool();
	volume = set->value( "volume", volume ).toInt();
	mute = set->value( "mute", mute ).toBool();
	external_subtitles = set->value( "external_subtitles", external_subtitles ).toString();
	external_audio = set->value( "external_audio", external_audio ).toString();
	sub_delay = set->value( "sub_delay", sub_delay).toInt();
	audio_delay = set->value( "audio_delay", audio_delay).toInt();
	sub_pos = set->value( "sub_pos", sub_pos).toInt();

	brightness = set->value( "brightness", brightness).toInt();
	contrast = set->value( "contrast", contrast).toInt();
	gamma = set->value( "gamma", gamma).toInt();
	hue = set->value( "hue", hue).toInt();
	saturation = set->value( "saturation", saturation).toInt();

	speed = set->value( "speed", speed ).toDouble();

	phase_filter = set->value( "phase_filter", phase_filter ).toBool();
	current_denoiser = set->value( "current_denoiser", current_denoiser).toInt();
	deblock_filter = set->value( "deblock_filter", deblock_filter).toBool();
	dering_filter = set->value( "dering_filter", dering_filter).toBool();
	noise_filter = set->value( "noise_filter", noise_filter).toBool();
	postprocessing_filter = set->value( "postprocessing_filter", postprocessing_filter).toBool();

	current_deinterlacer = set->value( "current_deinterlacer", current_deinterlacer ).toInt();
	panscan_filter = set->value( "panscan_filter", panscan_filter).toString();
	crop_43to169_filter = set->value( "crop_43to169_filter", crop_43to169_filter).toString();
	karaoke_filter = set->value( "karaoke_filter", karaoke_filter).toBool();
	extrastereo_filter = set->value( "extrastereo_filter", extrastereo_filter).toBool();
	volnorm_filter = set->value( "volnorm_filter", volnorm_filter).toBool();

	audio_use_channels = set->value( "audio_use_channels", audio_use_channels).toInt();
	stereo_mode = set->value( "stereo_mode", stereo_mode).toInt();

	panscan_factor = set->value( "panscan_factor", panscan_factor).toDouble();

	flip = set->value( "flip", flip).toBool();

	forced_demuxer = set->value( "forced_demuxer", forced_demuxer).toString();
	forced_video_codec = set->value( "forced_video_codec", forced_video_codec).toString();
	forced_audio_codec = set->value( "forced_audio_codec", forced_audio_codec).toString();

	original_demuxer = set->value( "original_demuxer", original_demuxer).toString();
	original_video_codec = set->value( "original_video_codec", original_video_codec).toString();
	original_audio_codec = set->value( "original_audio_codec", original_audio_codec).toString();

	mplayer_additional_options = set->value( "mplayer_additional_options", mplayer_additional_options).toString();
	mplayer_additional_video_filters = set->value( "mplayer_additional_video_filters", mplayer_additional_video_filters).toString();
	mplayer_additional_audio_filters = set->value( "mplayer_additional_audio_filters", mplayer_additional_audio_filters).toString();

	win_width = set->value( "win_width", win_width ).toInt();
	win_height = set->value( "win_height", win_height ).toInt();

	starting_time = set->value( "starting_time", starting_time ).toDouble();

	/*set->endGroup();*/

	// ChDefault not used anymore
	if (audio_use_channels == ChDefault) audio_use_channels = ChStereo;
}

