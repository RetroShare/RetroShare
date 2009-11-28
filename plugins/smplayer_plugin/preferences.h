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


#ifndef _PREFERENCES_H_
#define _PREFERENCES_H_

/* Global settings */

#include <QString>
#include <QStringList>
#include <QSize>
#include "config.h"
#include "audioequalizerlist.h"
#include "assstyles.h"

class Recents;
class URLHistory;

class Preferences {

public:
	enum OSD { None = 0, Seek = 1, SeekTimer = 2, SeekTimerTotal = 3 };
	enum OnTop { NeverOnTop = 0, AlwaysOnTop = 1, WhilePlayingOnTop = 2 };
	enum Resize { Never = 0, Always = 1, Afterload = 2 };
	enum Priority { Realtime = 0, High = 1, AboveNormal = 2, Normal = 3,
                    BelowNormal = 4, Idle = 5 };
	enum WheelFunction { Seeking = 0, Volume = 1, Zoom = 2, DoNothing = 3,
                         ChangeSpeed = 4 };
	enum OptionState { Detect = -1, Disabled = 0, Enabled = 1 };
	enum H264LoopFilter { LoopDisabled = 0, LoopEnabled = 1, LoopDisabledOnHD = 2 };

	Preferences();
	virtual ~Preferences();

	virtual void reset();

#ifndef NO_USE_INI_FILES
	void save();
	void load();
#endif

	double monitor_aspect_double();



    /* *******
       General
       ******* */

	QString mplayer_bin;
	QString vo; // video output
	QString ao; // audio output

	QString screenshot_directory;

	// SMPlayer will remember all media settings for all videos.
	// This options allow to disable it:
	bool dont_remember_media_settings; 	// Will not remember anything
	bool dont_remember_time_pos;		// Will not remember time pos

	QString audio_lang; 		// Preferred audio language
	QString subtitle_lang;		// Preferred subtitle language

	// Video
	bool use_direct_rendering;
	bool use_double_buffer;
	bool disable_screensaver;
	bool use_soft_video_eq;
	bool use_slices;
	int autoq; 	//!< Postprocessing quality
	bool add_blackborders_on_fullscreen;

	// Audio
	bool use_soft_vol;
	int softvol_max;
	OptionState use_scaletempo;
	bool dont_change_volume; // Don't change volume on startup
	bool use_hwac3; // -afm hwac3
	bool use_audio_equalizer;
	OptionState use_volume_option; //!< Use -volume in command line

	// Misc
	bool loop; 	//!< Loop. If true repeat the file
	int osd;

	QString file_settings_method; //!< Method to be used for saving file settings


    /* ***************
       Drives (CD/DVD)
       *************** */

	QString dvd_device;
	QString cdrom_device;

#ifdef Q_OS_WIN
	bool enable_audiocd_on_windows;
#endif

	int vcd_initial_title;


    /* ***********
       Performance
       *********** */

	int priority;
	bool frame_drop;
	bool hard_frame_drop;
	bool autosync;
	int autosync_factor;

	H264LoopFilter h264_skip_loop_filter;
	int HD_height; //!< An HD is a video which height is equal or greater than this.

	OptionState fast_audio_change; // If activated, not restart mplayer
#if !SMART_DVD_CHAPTERS
	bool fast_chapter_change;
#endif

	int threads; //!< number of threads to use for decoding (-lavdopts threads <1-8>)

	int cache_for_files;
	int cache_for_streams;
	int cache_for_dvds;
	int cache_for_vcds;
	int cache_for_audiocds;


	/* *********
	   Subtitles
	   ********* */

	QString font_file;
	QString font_name;
	bool use_fontconfig;
	QString subcp; // -subcp
	bool use_enca;
	QString enca_lang;
	int font_autoscale; // -subfont-autoscale
	int subfuzziness;
	bool autoload_sub;

	bool use_ass_subtitles;
	int ass_line_spacing;

	bool use_closed_caption_subs;
	bool use_forced_subs_only;

	bool subtitles_on_screenshots;

	//! Use the new sub_vob, sub_demux and sub_file commands
	//! instead of sub_select
	OptionState use_new_sub_commands; 
	OptionState change_sub_scale_should_restart;

	// ASS styles
	AssStyles ass_styles;

	//! If false, options requiring freetype won't be used
	bool freetype_support;


    /* ********
       Advanced
       ******** */

#if USE_ADAPTER
	int adapter; //Screen for overlay. If -1 it won't be used.
#endif

#if USE_COLORKEY
	unsigned int color_key;
#endif

	bool use_mplayer_window;

	QString monitor_aspect;

	bool use_idx; //!< Use -idx

	// Let the user pass options to mplayer
	QString mplayer_additional_options;
	QString mplayer_additional_video_filters;
	QString mplayer_additional_audio_filters;

	// Logs
	bool log_mplayer;
	bool log_smplayer;
	QString log_filter;

    //mplayer log autosaving
    bool autosave_mplayer_log;
    QString mplayer_log_saveto;
    //mplayer log autosaving end

#if REPAINT_BACKGROUND_OPTION
	//! If true, mplayerlayer erases its background
	bool repaint_video_background; 
#endif

	//! If true it will autoload edl files with the same name of the file
    //! to play
	bool use_edl_files;

	//! Preferred connection method: ipv4 or ipv6
	bool prefer_ipv4;

	//! Windows only. If true, smplayer will pass short filenames to mplayer.
	//! To workaround a bug in mplayer.
	bool use_short_pathnames; 

	//! If false, -brightness, -contrast and so on, won't be passed to
	//! mplayer. It seems that some graphic cards don't support those options.
	bool change_video_equalizer_on_startup;

	//! If true, smplayer will use the prefix pausing_keep_force to keep
	//! the pause on slave commands. This experimental prefix was added
	//! in mplayer svn r27665.
	bool use_pausing_keep_force;

	bool use_correct_pts; //!< Pass -correct-pts to mplayer

	QString actions_to_run; //!< List of actions to run every time a video loads.


	/* *********
	   GUI stuff
	   ********* */

	bool fullscreen;
	bool start_in_fullscreen;
	bool compact_mode;
	OnTop stay_on_top;
	int size_factor;

	int resize_method; 	//!< Mainwindow resize method

#if STYLE_SWITCHING
	QString style; 	//!< SMPlayer look
#endif
	bool show_frame_counter;
	bool show_motion_vectors;

	// Function of mouse buttons:
	QString mouse_left_click_function;
	QString mouse_right_click_function;
	QString mouse_double_click_function;
	QString mouse_middle_click_function;
	QString mouse_xbutton1_click_function;
	QString mouse_xbutton2_click_function;
	int wheel_function;

	// Configurable seeking
	int seeking1; // By default 10s
	int seeking2; // By default 1m
	int seeking3; // By default 10m
	int seeking4; // For mouse wheel, by default 30s

	bool update_while_seeking;
#if ENABLE_DELAYED_DRAGGING	
	int time_slider_drag_delay;
#endif

	QString language;
	QString iconset;

	//! Number of times to show the balloon remembering that the program
	//! is still running in the system tray.
	int balloon_count;

	//! If true, the position of the main window will be saved before
	//! entering in fullscreen and will restore when going back to
	//! window mode.
	bool restore_pos_after_fullscreen;

	bool save_window_size_on_exit;

	//! Close the main window when a file or playlist finish
	bool close_on_finish;

	QString default_font;

	//!< Pause the current file when the main window is not visible
	bool pause_when_hidden; 

	//!< Allow frre movement of the video window
	bool allow_video_movement;

	QString gui; //!< The name of the GUI to use

#if USE_MINIMUMSIZE
	int gui_minimum_width;
#endif
	QSize default_size; // Default size of the main window

#if ALLOW_TO_HIDE_VIDEO_WINDOW_ON_AUDIO_FILES
	bool hide_video_window_on_audio_files;
#endif

	bool report_mplayer_crashes;

#if REPORT_OLD_MPLAYER
	bool reported_mplayer_is_old;
#endif

	bool auto_add_to_playlist; //!< Add files to open to playlist
	bool add_to_playlist_consecutive_files;


    /* ***********
       Directories
       *********** */

	QString latest_dir; //!< Directory of the latest file loaded
	QString last_dvd_directory;


    /* **************
       Initial values
       ************** */

	double initial_sub_scale;
	double initial_sub_scale_ass;
	int initial_volume;
	int initial_contrast;
	int initial_brightness;
	int initial_hue;
	int initial_saturation;
	int initial_gamma;

	AudioEqualizerList initial_audio_equalizer;

	//! Default value for panscan (1.0 = no zoom)
	double initial_panscan_factor;

	//! Default value for position of subtitles on screen
	//! 100 = 100% at the bottom
	int initial_sub_pos;

	bool initial_postprocessing; //!< global postprocessing filter
	bool initial_volnorm;

	int initial_deinterlace;

	int initial_audio_channels;
	int initial_stereo_mode;

	int initial_audio_track;
	int initial_subtitle_track;


    /* ************
       MPlayer info
       ************ */

	int mplayer_detected_version; 	//!< Latest version of mplayer parsed

	//! Version of mplayer supplied by the user which will be used if
	//! the version can't be parsed from mplayer output
	int mplayer_user_supplied_version;


    /* *********
       Instances
       ********* */

	bool use_single_instance;
	int connection_port; // Manual port
	bool use_autoport;
	int autoport; // Port automatically chosen by Qt


    /* ****************
       Floating control
       **************** */

	int floating_control_margin;
	int floating_control_width;
	bool floating_control_animated;
	bool floating_display_in_compact_mode;
#ifndef Q_OS_WIN
	bool bypass_window_manager;
#endif


    /* *****
       Proxy
       ***** */

	bool use_proxy;
	int proxy_type;
	QString proxy_host;
	int proxy_port;
	QString proxy_username;
	QString proxy_password;


    /* *******
       History
       ******* */

	Recents * history_recents;
	URLHistory * history_urls;
};

#endif
