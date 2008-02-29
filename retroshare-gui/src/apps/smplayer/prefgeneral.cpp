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


#include "prefgeneral.h"
#include "preferences.h"
#include "filedialog.h"
#include "images.h"
#include "mediasettings.h"

PrefGeneral::PrefGeneral(QWidget * parent, Qt::WindowFlags f)
	: PrefWidget(parent, f )
{
	setupUi(this);

	// Read driver info from InfoReader:
	InfoReader * i = InfoReader::obj();
	setDrivers( i->voList(), i->aoList() );

	// Channels combo
	channels_combo->addItem( "2", MediaSettings::ChStereo );
	channels_combo->addItem( "4", MediaSettings::ChSurround );
	channels_combo->addItem( "6", MediaSettings::ChFull51 );

	//createHelp();
	retranslateStrings();
}

PrefGeneral::~PrefGeneral()
{
}

QString PrefGeneral::sectionName() {
	return tr("General");
}

QPixmap PrefGeneral::sectionIcon() {
	return Images::icon("pref_general");
}

void PrefGeneral::retranslateStrings() {
	retranslateUi(this);

    initial_volume_label->setNum( initial_volume_slider->value() );

	channels_combo->setItemText(0, tr("2 (Stereo)") );
	channels_combo->setItemText(1, tr("4 (4.0 Surround)") );
	channels_combo->setItemText(2, tr("6 (5.1 Surround)") );

    // Icons
	/*
    resize_window_icon->setPixmap( Images::icon("resize_window") );
    volume_icon->setPixmap( Images::icon("speaker") );
	*/

	preferred_desc->setText(
		tr("Here you can type your preferred language for the audio "
           "and subtitle streams. When a media with multiple audio or "
           "subtitle streams is found, SMPlayer will try to use your "
           "preferred language. This only will work with media that offer "
           "info about the language of audio and subtitle streams, like DVDs "
           "or mkv files.<br>These fields accept regular expressions. "
           "Example: <b>es|esp|spa</b> will select the track if it matches with "
            "<i>es</i>, <i>esp</i> or <i>spa</i>."));

	createHelp();
}

void PrefGeneral::setData(Preferences * pref) {
	setMplayerPath( pref->mplayer_bin );
	setScreenshotDir( pref->screenshot_directory );
	setVO( pref->vo );
	setAO( pref->ao );
	setRememberSettings( !pref->dont_remember_media_settings );
	setDontRememberTimePos( pref->dont_remember_time_pos );
	setAudioLang( pref->audio_lang );
	setSubtitleLang( pref->subtitle_lang );
	setAudioTrack( pref->initial_audio_track );
	setSubtitleTrack( pref->initial_subtitle_track );
	setCloseOnFinish( pref->close_on_finish );
	setPauseWhenHidden( pref->pause_when_hidden );

	setEq2( pref->use_soft_video_eq );
	setSoftVol( pref->use_soft_vol );
	setAc3DTSPassthrough( pref->use_hwac3 );
	setInitialVolNorm( pref->initial_volnorm );
	setAmplification( pref->softvol_max );
	setInitialPostprocessing( pref->initial_postprocessing );
	setDirectRendering( pref->use_direct_rendering );
	setDoubleBuffer( pref->use_double_buffer );
	setStartInFullscreen( pref->start_in_fullscreen );
	setDisableScreensaver( pref->disable_screensaver );
	setAutoq( pref->autoq );

	setInitialVolume( pref->initial_volume );
	setDontChangeVolume( pref->dont_change_volume );
	setUseVolume( pref->use_volume_option );
	setAudioChannels( pref->initial_audio_channels );
	setScaleTempoFilter( pref->use_scaletempo );
}

void PrefGeneral::getData(Preferences * pref) {
	requires_restart = false;

	if (pref->mplayer_bin != mplayerPath()) {
		requires_restart = true;
		pref->mplayer_bin = mplayerPath();

		qDebug("PrefGeneral::getData: mplayer binary has changed, getting version number");
		// Forces to get info from mplayer to update version number
		InfoReader i( pref->mplayer_bin );
		i.getInfo(); 
		// Update the drivers list at the same time
		//setDrivers( i.voList(), i.aoList() );
	}

	TEST_AND_SET(pref->screenshot_directory, screenshotDir());
	TEST_AND_SET(pref->vo, VO());
    TEST_AND_SET(pref->ao, AO());

	bool dont_remember_ms = !rememberSettings();
    TEST_AND_SET(pref->dont_remember_media_settings, dont_remember_ms);
    TEST_AND_SET(pref->dont_remember_time_pos, dontRememberTimePos());

	pref->audio_lang = audioLang();
    pref->subtitle_lang = subtitleLang();

	pref->initial_audio_track = audioTrack();
	pref->initial_subtitle_track = subtitleTrack();

	pref->close_on_finish = closeOnFinish();
	pref->pause_when_hidden = pauseWhenHidden();

	TEST_AND_SET(pref->use_soft_video_eq, eq2());
	TEST_AND_SET(pref->use_soft_vol, softVol());
	TEST_AND_SET(pref->use_hwac3, Ac3DTSPassthrough());
	pref->initial_volnorm = initialVolNorm();
	TEST_AND_SET(pref->softvol_max, amplification());
	pref->initial_postprocessing = initialPostprocessing();
	TEST_AND_SET(pref->use_direct_rendering, directRendering());
	TEST_AND_SET(pref->use_double_buffer, doubleBuffer());
	pref->start_in_fullscreen = startInFullscreen();
	TEST_AND_SET(pref->disable_screensaver, disableScreensaver());
	TEST_AND_SET(pref->autoq, autoq());

	pref->initial_volume = initialVolume();
	pref->dont_change_volume = dontChangeVolume();
	pref->use_volume_option = useVolume();
	pref->initial_audio_channels = audioChannels();
	TEST_AND_SET(pref->use_scaletempo, scaleTempoFilter());
}

void PrefGeneral::setDrivers(InfoList vo_list, InfoList ao_list) {
	for ( int n = 0; n < vo_list.count(); n++ ) {
		vo_combo->addItem( vo_list[n].name() );
		// Add directx:noaccel
		if ( vo_list[n].name() == "directx" ) {
			vo_combo->addItem( "directx:noaccel" );
		}
		// gl/gl2
		if (vo_list[n].name() == "gl") {
			vo_combo->addItem( "gl:yuv=3" );
			vo_combo->addItem( "gl:yuv=3:lscale=1" );
		}
		if (vo_list[n].name() == "gl2") {
			vo_combo->addItem( "gl2:yuv=3" );
		}
	}

	for ( int n = 0; n < ao_list.count(); n++) {
		ao_combo->addItem( ao_list[n].name() );
	}
}

void PrefGeneral::setMplayerPath( QString path ) {
	mplayerbin_edit->setText( path );	
}

QString PrefGeneral::mplayerPath() {
	return mplayerbin_edit->text();
}

void PrefGeneral::setScreenshotDir( QString path ) {
	screenshot_edit->setText( path );
}

QString PrefGeneral::screenshotDir() {
	return screenshot_edit->text();
}

void PrefGeneral::setVO( QString vo_driver ) {
	vo_combo->setCurrentText( vo_driver );
}

void PrefGeneral::setAO( QString ao_driver ) {
	ao_combo->setCurrentText( ao_driver );
}

QString PrefGeneral::VO() {
	return vo_combo->currentText();
}

QString PrefGeneral::AO() {
	return ao_combo->currentText();
}

void PrefGeneral::setRememberSettings(bool b) {
	remember_all_check->setChecked(b);
	//rememberAllButtonToggled(b);
}

bool PrefGeneral::rememberSettings() {
	return remember_all_check->isChecked();
}

void PrefGeneral::setDontRememberTimePos(bool b) {
	dont_remember_time_check->setChecked(b);
}

bool PrefGeneral::dontRememberTimePos() {
	return dont_remember_time_check->isChecked();
}

void PrefGeneral::setAudioLang(QString lang) {
	audio_lang_edit->setText(lang);
}

QString PrefGeneral::audioLang() {
	return audio_lang_edit->text();
}

void PrefGeneral::setSubtitleLang(QString lang) {
	subtitle_lang_edit->setText(lang);
}

QString PrefGeneral::subtitleLang() {
	return subtitle_lang_edit->text();
}

void PrefGeneral::setAudioTrack(int track) {
	audio_track_spin->setValue(track);
}

int PrefGeneral::audioTrack() {
	return audio_track_spin->value();
}

void PrefGeneral::setSubtitleTrack(int track) {
	subtitle_track_spin->setValue(track);
}

int PrefGeneral::subtitleTrack() {
	return subtitle_track_spin->value();
}

void PrefGeneral::setCloseOnFinish(bool b) {
	close_on_finish_check->setChecked(b);
}

bool PrefGeneral::closeOnFinish() {
	return close_on_finish_check->isChecked();
}

void PrefGeneral::setPauseWhenHidden(bool b) {
	pause_if_hidden_check->setChecked(b);
}

bool PrefGeneral::pauseWhenHidden() {
	return pause_if_hidden_check->isChecked();
}


void PrefGeneral::setEq2(bool b) {
	eq2_check->setChecked(b);
}

bool PrefGeneral::eq2() {
	return eq2_check->isChecked();
}

void PrefGeneral::setSoftVol(bool b) {
	softvol_check->setChecked(b);
}

bool PrefGeneral::softVol() {
	return softvol_check->isChecked();
}

void PrefGeneral::setAc3DTSPassthrough(bool b) {
	hwac3_check->setChecked(b);
}

bool PrefGeneral::Ac3DTSPassthrough() {
	return hwac3_check->isChecked();
}

void PrefGeneral::setInitialVolNorm(bool b) {
	volnorm_check->setChecked(b);
}

bool PrefGeneral::initialVolNorm() {
	return volnorm_check->isChecked();
}

void PrefGeneral::setInitialPostprocessing(bool b) {
	postprocessing_check->setChecked(b);
}

bool PrefGeneral::initialPostprocessing() {
	return postprocessing_check->isChecked();
}

void PrefGeneral::setDirectRendering(bool b) {
	direct_rendering_check->setChecked(b);
}

bool PrefGeneral::directRendering() {
	return direct_rendering_check->isChecked();
}

void PrefGeneral::setDoubleBuffer(bool b) {
	double_buffer_check->setChecked(b);
}

bool PrefGeneral::doubleBuffer() {
	return double_buffer_check->isChecked();
}

void PrefGeneral::setAmplification(int n) {
	softvol_max_spin->setValue(n);
}

int PrefGeneral::amplification() {
	return softvol_max_spin->value();
}

void PrefGeneral::setInitialVolume(int v) {
	initial_volume_slider->setValue(v);
}

int PrefGeneral::initialVolume() {
	return initial_volume_slider->value();
}

void PrefGeneral::setAudioChannels(int ID) {
	int pos = channels_combo->findData(ID);
	if (pos != -1) {
		channels_combo->setCurrentIndex(pos);
	} else {
		qWarning("PrefGeneral::setAudioChannels: ID: %d not found in combo", ID);
	}
}

int PrefGeneral::audioChannels() {
	if (channels_combo->currentIndex() != -1) {
		return channels_combo->itemData( channels_combo->currentIndex() ).toInt();
	} else {
		qWarning("PrefGeneral::audioChannels: no item selected");
		return 0;
	}
}

void PrefGeneral::setDontChangeVolume(bool b) {
	change_volume_check->setChecked(!b);
}

bool PrefGeneral::dontChangeVolume() {
	return !change_volume_check->isChecked();
}

void PrefGeneral::setUseVolume(bool b) {
	use_volume_option_check->setChecked(b);
}

bool PrefGeneral::useVolume() {
	return use_volume_option_check->isChecked();
}

void PrefGeneral::setStartInFullscreen(bool b) {
	start_fullscreen_check->setChecked(b);
}

bool PrefGeneral::startInFullscreen() {
	return start_fullscreen_check->isChecked();
}

void PrefGeneral::setDisableScreensaver(bool b) {
	screensaver_check->setChecked(b);
}

bool PrefGeneral::disableScreensaver() {
	return screensaver_check->isChecked();
}

void PrefGeneral::setAutoq(int n) {
	autoq_spin->setValue(n);
}

int PrefGeneral::autoq() {
	return autoq_spin->value();
}

void PrefGeneral::setScaleTempoFilter(Preferences::OptionState value) {
	scaletempo_combo->setState(value);
}

Preferences::OptionState PrefGeneral::scaleTempoFilter() {
	return scaletempo_combo->state();
}

// Search mplayer executable
void PrefGeneral::on_searchButton_clicked() {
	QString	s = MyFileDialog::getOpenFileName(
                        this, tr("Select the mplayer executable"),
	                    mplayerbin_edit->text(),
#ifdef Q_OS_WIN
    	                tr("Executables") +" (*.exe)"
#else
    	                tr("All files") +" (*)"
#endif
            	         );

	if (!s.isEmpty()) {
		mplayerbin_edit->setText(s);
	}
}

void PrefGeneral::on_selectButton_clicked() {
	QString s = MyFileDialog::getExistingDirectory(
					this, tr("Select a directory"), 
                    screenshot_edit->text() );
	if (!s.isEmpty()) {
		screenshot_edit->setText(s);
	}
}

void PrefGeneral::createHelp() {
	clearHelp();

	setWhatsThis(mplayerbin_edit, tr("MPlayer executable"), 
		tr("Here you must specify the mplayer "
           "executable that smplayer will use.<br>"
           "smplayer requires at least mplayer 1.0rc1 (svn recommended).<br>"
           "<b>If this setting is wrong, smplayer won't "
           "be able to play anything!</b>") );

	setWhatsThis(screenshot_edit, tr("Screenshots folder"),
		tr("Here you can specify a folder where the screenshots taken by "
           "smplayer will be stored. If this field is empty the "
           "screenshot feature will be disabled.") );

	setWhatsThis(vo_combo, tr("Video output driver"),
		tr("Select the video output driver. Usually xv (linux) "
           "and directx (windows) provide the best performance.") );

	setWhatsThis(ao_combo, tr("Audio output driver"),
		tr("Select the audio output driver.") );

	setWhatsThis(remember_all_check, tr("Remember settings"),
		tr("Usually smplayer will remember the settings for each file you "
           "play (audio track selected, volume, filters...). Uncheck this "
           "option if you don't like this feature.") );

	setWhatsThis(dont_remember_time_check, tr("Don't remember time position"),
		tr("If you check this option, smplayer will play all files from "
           "the beginning.") );

	setWhatsThis(close_on_finish_check, tr("Close when finished"),
		tr("If this option is checked, the main window will be automatically "
		   "closed when the current file/playlist finishes.") );

	setWhatsThis(pause_if_hidden_check, tr("Pause when minimized"),
		tr("If this option is enabled, the file will be paused when the "
           "main window is hidden. When the window is restored, play will be "
           "resumed.") );

	// Video/audio tab
	setWhatsThis(eq2_check, tr("Software video equalizer"),
		tr("You can check this option if video equalizer is not supported by "
           "your graphic card or the selected video output driver.<br>"
           "<b>Note:</b> this option can be incompatible "
           "with some video output drivers.") );

	setWhatsThis(postprocessing_check, tr("Enable postprocessing by default"),
		tr("Postprocessing will be used by default on new opened files.") );

	setWhatsThis(autoq_spin, tr("Postprocessing quality"),
		tr("Dynamically changes the level of postprocessing depending on the "
           "available spare CPU time. The number you specify will be the "
           "maximum level used. Usually you can use some big number.") );

	setWhatsThis(direct_rendering_check, tr("Direct rendering"),
		tr("If checked, turns on direct rendering (not supported by all "
           "codecs and video outputs)<br>"
           "<b>WARNING:</b> May cause OSD/SUB corruption!") );

	setWhatsThis(double_buffer_check, tr("Double buffering"),
		tr("Double buffering fixes flicker by storing two frames in memory, "
           "and displaying one while decoding another. If disabled it can "
           "affect OSD negatively, but often removes OSD flickering.") );

	setWhatsThis(start_fullscreen_check, tr("Start videos in fullscreen"),
		tr("If this option is checked, all videos will start to play in "
           "fullscreen mode.") );

	setWhatsThis(screensaver_check, tr("Disable screensaver"),
		tr("Check this option to disable the screensaver while playing.<br>"
           "The screensaver will enabled again when play finishes.<br>"
           "<b>Note:</b> This option works only in X11 and Windows.") );

	setWhatsThis(softvol_check, tr("Software volume control"),
		tr("Check this option to use the software mixer, instead of "
           "using the sound card mixer.") );

	setWhatsThis(softvol_max_spin, tr("Max. Amplification"),
		tr("Sets the maximum amplification level in percent (default: 110). "
           "A value of 200 will allow you to adjust the volume up to a "
           "maximum of double the current level. With values below 100 the "
           "initial volume (which is 100%) will be above the maximum, which "
           "e.g. the OSD cannot display correctly.") );

	setWhatsThis(hwac3_check, tr("AC3/DTS pass-through S/PDIF"),
		tr("Uses hardware AC3 passthrough") );

	setWhatsThis(volnorm_check, tr("Volume normalization by default"),
		tr("Maximizes the volume without distorting the sound.") );

	setWhatsThis(scaletempo_combo, tr("High speed playback without altering pitch"),
		tr("Allows to change the playback speed without altering pitch. "
           "Requires at least MPlayer dev-SVN-r24924.") );

	setWhatsThis(change_volume_check, tr("Change volume"),
		tr("If checked, SMPlayer will remember the volume for every file "
           "and will restore it when played again. For new files the default "
           "volume will be used.") );

	setWhatsThis(initial_volume_slider, tr("Default volume"),
		tr("Sets the initial volume that new files will use.") );

	setWhatsThis(use_volume_option_check, tr("Change volume just before playing"),
		tr("If this option is checked the initial volume will be set by "
           "using the <i>-volume</i> option in MPlayer.<br> "
           "<b>WARNING: THE OFFICIAL MPLAYER DOESN'T HAVE THAT "
           "<i>-volume</i> OPTION, "
           "YOU NEED A PATCHED ONE, OTHERWISE MPLAYER WILL FAIL AND WON'T PLAY "
           "ANYTHING.</b>") );

	setWhatsThis(channels_combo, tr("Channels by default"),
		tr("Requests the number of playback channels. MPlayer "
           "asks the decoder to decode the audio into as many channels as "
           "specified. Then it is up to the decoder to fulfill the "
           "requirement. This is usually only important when playing "
           "videos with AC3 audio (like DVDs). In that case liba52 does "
           "the decoding by default and correctly downmixes the audio "
           "into the requested number of channels. "
           "NOTE: This option is honored by codecs (AC3 only), "
           "filters (surround) and audio output drivers (OSS at least).") );

	setWhatsThis(audio_lang_edit, tr("Preferred audio language"),
		tr("Here you can type your preferred language for the audio streams. "
           "When a media with multiple audio streams is found, smplayer will "
           "try to use your preferred language.<br>"
           "This only will work with media that offer info about the language "
           "of the audio streams, like DVDs or mkv files.<br>"
           "This field accepts regular expressions. Example: <b>es|esp|spa</b> "
           "will select the audio track if it matches with <i>es</i>, "
           "<i>esp</i> or <i>spa</i>.") );

	setWhatsThis(subtitle_lang_edit, tr("Preferred subtitle language"),
		tr("Here you can type your preferred language for the subtitle stream. "
           "When a media with multiple subtitle streams is found, smplayer will "
           "try to use your preferred language.<br>"
           "This only will work with media that offer info about the language "
           "of the subtitle streams, like DVDs or mkv files.<br>"
           "This field accepts regular expressions. Example: <b>es|esp|spa</b> "
           "will select the subtitle stream if it matches with <i>es</i>, "
           "<i>esp</i> or <i>spa</i>.") );

	setWhatsThis(audio_track_spin, tr("Audio track"),
		tr("Specifies the default audio track which will be used when playing "
           "new files. If the track doesn't exist, the first one will be used. "
           "<br><b>Note:</b> the <i>\"preferred audio language\"</i> has "
           "preference over this option.") );

	setWhatsThis(subtitle_track_spin, tr("Subtitle track"),
		tr("Specifies the default subtitle track which will be used when "
           "playing new files. If the track doesn't exist, the first one "
           "will be used. <br><b>Note:</b> the <i>\"preferred subtitle "
           "language\"</i> has preference over this option.") );

}

#include "moc_prefgeneral.cpp"
