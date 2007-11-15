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

#include "preferences.h"
#include "global.h"
#include "helper.h"
#include "mediasettings.h"

#include <QSettings>
#include <QFileInfo>
#include <QRegExp>

Preferences::Preferences() {
	reset();
	load();
}

Preferences::~Preferences() {
	save();
}

void Preferences::reset() {
#ifdef Q_OS_WIN
	mplayer_bin= "mplayer/mplayer.exe";
#else
	mplayer_bin = "mplayer";
#endif

	/*
	QFileInfo fi(mplayer_bin);
	if (fi.exists()) {
		mplayer_bin = fi.absFilePath();
		qDebug("mplayer_bin: '%s'", mplayer_bin.toUtf8().data());
	}
	*/

	use_fontconfig = FALSE;
	use_ass_subtitles = FALSE;
	font_file = "";
	font_name = "";
	subcp = "ISO-8859-1";
	font_autoscale = 1;
	font_textscale = 5;
	autoload_sub = TRUE;
	subfuzziness = 1;
	ass_color = 0xFFFF00;
    ass_border_color = 0x000000;
	//ass_styles = "Bold=1,Outline=2,Shadow=2";
	ass_styles = "";

	osd = None;

	vo = ""; 
	ao = "";

	color_key = 0x020202;

/*
#ifdef Q_OS_WIN
	dvd_device="E:";
	cdrom_device="E:";
#else
	dvd_device="/dev/dvd";
	cdrom_device="/dev/cdrom";
#endif
*/

	dvd_device = "";
	cdrom_device = "";

	// MPlayer 1.0rc1 require restart, new versions don't
	audio_change_requires_restart = FALSE; // Now we need a svn mplayer
	fast_chapter_change = FALSE;

	use_cache = false;
	cache = 2000;

	use_mplayer_window = FALSE;

	monitor_aspect=""; // Autodetect
	
	latest_dir="";
	last_url="";
	last_dvd_directory="";

	mplayer_verbose="";

	resize_method = Always;
	disable_screensaver = true;

	use_direct_rendering = false;
	use_double_buffer = true;

	screenshot_directory="";
	if (QFile::exists(Helper::appHomePath() + "/screenshots")) {
		screenshot_directory = Helper::appHomePath() + "/screenshots";
	}
	subtitles_on_screenshots = TRUE;

	use_soft_video_eq = FALSE;
	use_soft_vol = FALSE;
    softvol_max = 110; // 110 = default value in mplayer

	audio_lang = "";
	subtitle_lang = "";

	use_idx = false;

	dont_change_volume = false;

	use_hwac3 = false;

	mplayer_additional_options="";
    mplayer_additional_video_filters="";
    mplayer_additional_audio_filters="";

	priority = AboveNormal; // Option only for windows
	frame_drop = FALSE;
	hard_frame_drop = FALSE;
	autosync = FALSE;
	autosync_factor = 100;

	dont_remember_media_settings = FALSE;
	dont_remember_time_pos = FALSE;

#if STYLE_SWITCHING
	style="";
#endif

	fullscreen = FALSE;
	start_in_fullscreen = FALSE;

	compact_mode = FALSE;
	stay_on_top = FALSE;
	size_factor = 100; // 100%

	show_frame_counter = FALSE;

	autoq = 6;

	loop = FALSE;

	use_single_instance = FALSE;
	connection_port = 8000;

	mouse_left_click_function = "";
	mouse_double_click_function = "fullscreen";
	wheel_function = Seeking;

	recents_max_items = 10;

	seeking1 = 10;
	seeking2 = 60;
	seeking3 = 10*60;
	seeking4 = 30;

	log_mplayer = TRUE;
	log_smplayer = TRUE;
	log_filter = ".*";

	language = "";
	iconset = "";

	// "Don't repaint video background" in the preferences dialog
#ifdef Q_OS_WIN
	always_clear_video_background = true;
#else
	always_clear_video_background = false;
#endif

	rx_endoffile = "Exiting... \\(End of file\\)";
	rx_novideo = "Video: no video";

	balloon_count = 5;

	dont_use_eq_options = false;

#if USE_SUBFONT
	use_subfont = false;
#endif

#ifdef Q_OS_WIN
	restore_pos_after_fullscreen = true;
#else
	restore_pos_after_fullscreen = false;
#endif

	save_window_size_on_exit = true;

	enable_vcd_on_windows = false;
	enable_audiocd_on_windows = false;

	close_on_finish = false;

	default_font = "";

	pause_when_hidden = false;


	vcd_initial_title = 2; // Most VCD's start at title #2

	initial_volume = 40;
	initial_contrast = 0;
	initial_brightness = 0;
	initial_hue = 0;
	initial_saturation = 0;
	initial_gamma = 0;

	initial_panscan_factor = 1.0;
	initial_sub_pos = 100; // 100%

	initial_postprocessing = false;
	initial_volnorm = false;

	initial_audio_channels = MediaSettings::ChDefault;

	initial_audio_track = 0;
	initial_subtitle_track = 0;
}

void Preferences::save() {
	qDebug("Preferences::save");

	QSettings * set = settings;

	set->beginGroup( "preferences");

	set->setValue("mplayer_bin", mplayer_bin);
	set->setValue("use_fontconfig", use_fontconfig);
	set->setValue("font_file", font_file);
	set->setValue("font_name", font_name);
	set->setValue("font_autoscale", font_autoscale);
	set->setValue("font_textscale", font_textscale);
	set->setValue("subcp", subcp);
	set->setValue("use_ass_subtitles", use_ass_subtitles);
	set->setValue("autoload_sub", autoload_sub);
	set->setValue("subfuzziness", subfuzziness);
	set->setValue("ass_color", (int) ass_color);
	set->setValue("ass_border_color", (int) ass_border_color);
	set->setValue("ass_styles", ass_styles);

	set->setValue("osd", osd);
	set->setValue("vo", vo);
	set->setValue("ao", ao);

	set->setValue("color_key", QString::number(color_key,16));

	set->setValue("audio_change_requires_restart", audio_change_requires_restart);
	set->setValue("fast_chapter_change", fast_chapter_change);

	set->setValue("dvd_device", dvd_device);
	set->setValue("cdrom_device", cdrom_device);

	set->setValue("use_cache", use_cache);
	set->setValue("cache", cache);
	set->setValue("use_mplayer_window", use_mplayer_window);

	set->setValue("monitor_aspect", monitor_aspect);

	set->setValue("latest_dir", latest_dir);
	set->setValue("last_url", last_url);
	set->setValue("last_dvd_directory", last_dvd_directory);

	set->setValue("mplayer_verbose", mplayer_verbose);
	set->setValue("resize_method", resize_method);
	set->setValue("disable_screensaver", disable_screensaver);

	set->setValue("use_direct_rendering", use_direct_rendering);
	set->setValue("use_double_buffer", use_double_buffer);

	set->setValue("screenshot_directory", screenshot_directory);
	set->setValue("subtitles_on_screenshots", subtitles_on_screenshots);

	set->setValue("use_soft_video_eq", use_soft_video_eq);
	set->setValue("use_soft_vol", use_soft_vol);
	set->setValue("softvol_max", softvol_max);


	set->setValue("audio_lang", audio_lang);
	set->setValue("subtitle_lang", subtitle_lang);

	set->setValue("use_idx", use_idx);

	set->setValue("dont_change_volume", dont_change_volume );

	set->setValue("use_hwac3", use_hwac3 );

	set->setValue("vcd_initial_title", vcd_initial_title);


	set->setValue("mplayer_additional_options", mplayer_additional_options);
	set->setValue("mplayer_additional_video_filters", mplayer_additional_video_filters);
	set->setValue("mplayer_additional_audio_filters", mplayer_additional_audio_filters);

	set->setValue("priority", priority);
	set->setValue("frame_drop", frame_drop);
	set->setValue("hard_frame_drop", hard_frame_drop);
	set->setValue("autosync", autosync);
	set->setValue("autosync_factor", autosync_factor);

	set->setValue("dont_remember_media_settings", dont_remember_media_settings);
	set->setValue("dont_remember_time_pos", dont_remember_time_pos);

#if STYLE_SWITCHING
	set->setValue("style", style);
#endif

	set->setValue("fullscreen", fullscreen);
	set->setValue("start_in_fullscreen", start_in_fullscreen);

	set->setValue("compact_mode", compact_mode);
	set->setValue("stay_on_top", stay_on_top);
	set->setValue("size_factor", size_factor);

	set->setValue("show_frame_counter", show_frame_counter);

	set->setValue("autoq", autoq);

	set->setValue("loop", loop);

	set->setValue("use_single_instance", use_single_instance);
	set->setValue("connection_port", connection_port);

	set->setValue("mouse_left_click_function", mouse_left_click_function);
	set->setValue("mouse_double_click_function", mouse_double_click_function);
	set->setValue("wheel_function", wheel_function);

	set->setValue("recents_max_items", recents_max_items);

	set->setValue("seeking1", seeking1);
	set->setValue("seeking2", seeking2);
	set->setValue("seeking3", seeking3);
	set->setValue("seeking4", seeking4);

	set->setValue("log_mplayer", log_mplayer);
	set->setValue("log_smplayer", log_smplayer);
	set->setValue("log_filter", log_filter);

	set->setValue("language", language);
	set->setValue("iconset", iconset);

	set->setValue("always_clear_video_background", always_clear_video_background);

	set->setValue("rx_endoffile", rx_endoffile);
	set->setValue("rx_novideo", rx_novideo);

	set->setValue("balloon_count", balloon_count);

	set->setValue("dont_use_eq_options", dont_use_eq_options);

#if USE_SUBFONT
	set->setValue("use_subfont", use_subfont);
#endif

	set->setValue("restore_pos_after_fullscreen", restore_pos_after_fullscreen);
	set->setValue("save_window_size_on_exit", save_window_size_on_exit);

	set->setValue("enable_vcd_on_windows", enable_vcd_on_windows);
	set->setValue("enable_audiocd_on_windows", enable_audiocd_on_windows);

	set->setValue("close_on_finish", close_on_finish);

	set->setValue("default_font", default_font);

	set->setValue("pause_when_hidden", pause_when_hidden);

	set->endGroup();


	set->beginGroup( "defaults");

	set->setValue("initial_volume", initial_volume);
	set->setValue("initial_contrast", initial_contrast);
	set->setValue("initial_brightness", initial_brightness);
	set->setValue("initial_hue", initial_hue);
	set->setValue("initial_saturation", initial_saturation);
	set->setValue("initial_gamma", initial_gamma);

	set->setValue("initial_panscan_factor", initial_panscan_factor);
	set->setValue("initial_sub_pos", initial_sub_pos);

	set->setValue("initial_volnorm", initial_volnorm);
	set->setValue("initial_postprocessing", initial_postprocessing);

	set->setValue("initial_audio_channels", initial_audio_channels);

	set->setValue("initial_audio_track", initial_audio_track);
	set->setValue("initial_subtitle_track", initial_subtitle_track);

	set->endGroup();


	set->beginGroup("associations");
	set->setValue("extensions", extensions); 
	set->endGroup();
}

void Preferences::load() {
	qDebug("Preferences::load");

	QSettings * set = settings;

	set->beginGroup( "preferences");

	mplayer_bin = set->value("mplayer_bin", mplayer_bin).toString();

	use_fontconfig = set->value("use_fontconfig", use_fontconfig).toBool();
	font_file = set->value("font_file", font_file).toString();
	font_name = set->value("font_name", font_name).toString();
	font_autoscale = set->value("font_autoscale", font_autoscale).toInt();
	font_textscale = set->value("font_textscale", font_textscale).toInt();
	subcp = set->value("subcp", subcp).toString();
	subfuzziness = set->value("subfuzziness", subfuzziness).toInt();
	use_ass_subtitles = set->value("use_ass_subtitles", use_ass_subtitles).toBool();
	autoload_sub = set->value("autoload_sub", autoload_sub).toBool();
	ass_color = set->value("ass_color", ass_color).toInt();
	ass_border_color = set->value("ass_border_color", ass_border_color).toInt();
	ass_styles = set->value("ass_styles", ass_styles).toString();

	osd = set->value("osd", osd).toInt();
	vo = set->value("vo", vo).toString();
	ao = set->value("ao", ao).toString();

	bool ok;
	QString color = set->value("color_key", QString::number(color_key,16)).toString();
	unsigned int temp_color_key = color.toUInt(&ok, 16);
	if (ok)
		color_key = temp_color_key;
	//color_key = set->value("color_key", color_key).toInt();

	audio_change_requires_restart = set->value("audio_change_requires_restart", audio_change_requires_restart).toBool();
	fast_chapter_change = set->value("fast_chapter_change", fast_chapter_change).toBool();

	dvd_device = set->value("dvd_device", dvd_device).toString();
	cdrom_device = set->value("cdrom_device", cdrom_device).toString();

	use_cache = set->value("use_cache", use_cache).toBool();
	cache = set->value("cache", cache).toInt();
	use_mplayer_window = set->value("use_mplayer_window", use_mplayer_window).toBool();

	monitor_aspect = set->value("monitor_aspect", monitor_aspect).toString();
		
	latest_dir = set->value("latest_dir", latest_dir).toString();
	last_url = set->value("last_url", last_url).toString();
	last_dvd_directory = set->value("last_dvd_directory", last_dvd_directory).toString();

	mplayer_verbose = set->value("mplayer_verbose", mplayer_verbose).toString();
	resize_method = set->value("resize_method", resize_method).toInt();
	disable_screensaver = set->value("disable_screensaver", disable_screensaver).toBool();

	use_direct_rendering = set->value("use_direct_rendering", use_direct_rendering).toBool();
	use_double_buffer = set->value("use_double_buffer", use_double_buffer).toBool();

	screenshot_directory = set->value("screenshot_directory", screenshot_directory).toString();
	subtitles_on_screenshots = set->value("subtitles_on_screenshots", subtitles_on_screenshots).toBool();

	use_soft_video_eq = set->value("use_soft_video_eq", use_soft_video_eq).toBool();
	use_soft_vol = set->value("use_soft_vol", use_soft_vol).toBool();
	softvol_max = set->value("softvol_max", softvol_max).toInt();

	audio_lang = set->value("audio_lang", audio_lang).toString();
	subtitle_lang = set->value("subtitle_lang", subtitle_lang).toString();

	use_idx = set->value("use_idx", use_idx).toBool();

	dont_change_volume = set->value("dont_change_volume", dont_change_volume ).toBool();

	use_hwac3 = set->value("use_hwac3", use_hwac3 ).toBool();

	vcd_initial_title = set->value("vcd_initial_title", vcd_initial_title ).toInt();


	mplayer_additional_options = set->value("mplayer_additional_options", mplayer_additional_options).toString();
	mplayer_additional_video_filters = set->value("mplayer_additional_video_filters", mplayer_additional_video_filters).toString();
	mplayer_additional_audio_filters = set->value("mplayer_additional_audio_filters", mplayer_additional_audio_filters).toString();

	priority = set->value("priority", priority).toInt();
	frame_drop = set->value("frame_drop", frame_drop).toBool();
	hard_frame_drop = set->value("hard_frame_drop", hard_frame_drop).toBool();
	autosync = set->value("autosync", autosync).toBool();
	autosync_factor = set->value("autosync_factor", autosync_factor).toInt();

	dont_remember_media_settings = set->value("dont_remember_media_settings", dont_remember_media_settings).toBool();
	dont_remember_time_pos = set->value("dont_remember_time_pos", dont_remember_time_pos).toBool();

#if STYLE_SWITCHING
	style = set->value("style", style).toString();
#endif

	fullscreen = set->value("fullscreen", fullscreen).toBool();
	start_in_fullscreen = set->value("start_in_fullscreen", start_in_fullscreen).toBool();

	compact_mode = set->value("compact_mode", compact_mode).toBool();
	stay_on_top = set->value("stay_on_top", stay_on_top).toBool();
	size_factor = set->value("size_factor", size_factor).toInt();

	show_frame_counter = set->value("show_frame_counter", show_frame_counter).toBool();

	autoq = set->value("autoq", autoq).toInt();

	loop = set->value("loop", loop).toBool();

	use_single_instance = set->value("use_single_instance", use_single_instance).toBool();
	connection_port = set->value("connection_port", connection_port).toInt();

	mouse_left_click_function = set->value("mouse_left_click_function", mouse_left_click_function).toString();
	mouse_double_click_function = set->value("mouse_double_click_function", mouse_double_click_function).toString();
	wheel_function = set->value("wheel_function", wheel_function).toInt();

	recents_max_items = set->value("recents_max_items", recents_max_items).toInt();

	seeking1 = set->value("seeking1", seeking1).toInt();
	seeking2 = set->value("seeking2", seeking2).toInt();
	seeking3 = set->value("seeking3", seeking3).toInt();
	seeking4 = set->value("seeking4", seeking4).toInt();

	language = set->value("language", language).toString();
	iconset= set->value("iconset", iconset).toString();

	log_mplayer = set->value("log_mplayer", log_mplayer).toBool();
	log_smplayer = set->value("log_smplayer", log_smplayer).toBool();
	log_filter = set->value("log_filter", log_filter).toString();

	always_clear_video_background = set->value("always_clear_video_background", always_clear_video_background).toBool();

	rx_endoffile = set->value("rx_endoffile", rx_endoffile).toString();
	rx_novideo = set->value("rx_novideo", rx_novideo).toString();

	balloon_count = set->value("balloon_count", balloon_count).toInt();

	dont_use_eq_options = set->value("dont_use_eq_options", dont_use_eq_options).toBool();

#if USE_SUBFONT
	use_subfont = set->value("use_subfont", use_subfont).toBool();
#endif

	restore_pos_after_fullscreen = set->value("restore_pos_after_fullscreen", restore_pos_after_fullscreen).toBool();
	save_window_size_on_exit = 	set->value("save_window_size_on_exit", save_window_size_on_exit).toBool();

	enable_vcd_on_windows = set->value("enable_vcd_on_windows", enable_vcd_on_windows).toBool();
	enable_audiocd_on_windows = set->value("enable_audiocd_on_windows", enable_audiocd_on_windows).toBool();

	close_on_finish = set->value("close_on_finish", close_on_finish).toBool();

	default_font = set->value("default_font", default_font).toString();

	pause_when_hidden = set->value("pause_when_hidden", pause_when_hidden).toBool();

	set->endGroup();


	set->beginGroup( "defaults");

	initial_volume = set->value("initial_volume", initial_volume).toInt();
	initial_contrast = set->value("initial_contrast", initial_contrast).toInt();
	initial_brightness = set->value("initial_brightness", initial_brightness).toInt();
	initial_hue = set->value("initial_hue", initial_hue).toInt();
	initial_saturation = set->value("initial_saturation", initial_saturation).toInt();
	initial_gamma = set->value("initial_gamma", initial_gamma).toInt();

	initial_panscan_factor = set->value("initial_panscan_factor", initial_panscan_factor).toDouble();
	initial_sub_pos = set->value("initial_sub_pos", initial_sub_pos).toInt();

	initial_volnorm = set->value("initial_volnorm", initial_volnorm).toBool();
	initial_postprocessing = set->value("initial_postprocessing", initial_postprocessing).toBool();

	initial_audio_channels = set->value("initial_audio_channels", initial_audio_channels).toInt();

	initial_audio_track = set->value("initial_audio_track", initial_audio_track).toInt();
	initial_subtitle_track = set->value("initial_subtitle_track", initial_subtitle_track).toInt();

	set->endGroup();


	set->beginGroup("associations");
	extensions = set->value("extensions").toString();
	set->endGroup();

	/*
	QFileInfo fi(mplayer_bin);
	if (fi.exists()) {
		mplayer_bin = fi.absFilePath();
		qDebug("mplayer_bin: '%s'", mplayer_bin.toUtf8().data());
	}
	*/
}

double Preferences::monitor_aspect_double() {
	qDebug("Preferences::monitor_aspect_double");

	QRegExp exp("(\\d+)[:/](\\d+)");
	if (exp.indexIn( monitor_aspect ) != -1) {
		int w = exp.cap(1).toInt();
		int h = exp.cap(2).toInt();
		qDebug(" monitor_aspect parsed successfully: %d:%d", w, h);
		return (double) w/h;
	}

	bool ok;
	double res = monitor_aspect.toDouble(&ok);
	if (ok) {
		qDebug(" monitor_aspect parsed successfully: %f", res);
		return res;
	} else {
		qDebug(" warning: monitor_aspect couldn't be parsed!");
        qDebug(" monitor_aspect set to 0");
		return 0;
	}
}
