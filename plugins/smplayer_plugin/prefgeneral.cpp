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
#include "paths.h"

#if USE_ALSA_DEVICES || USE_DSOUND_DEVICES
#include "deviceinfo.h"
#endif

PrefGeneral::PrefGeneral(QWidget * parent, Qt::WindowFlags f)
	: PrefWidget(parent, f )
{
	setupUi(this);

	mplayerbin_edit->setDialogType(FileChooser::GetFileName);
	screenshot_edit->setDialogType(FileChooser::GetDirectory);

	// Read driver info from InfoReader:
	InfoReader * i = InfoReader::obj();
	vo_list = i->voList();
	ao_list = i->aoList();

#if USE_DSOUND_DEVICES
	dsound_devices = DeviceInfo::dsoundDevices();
#endif

#if USE_ALSA_DEVICES
	alsa_devices = DeviceInfo::alsaDevices();
#endif
#if USE_XV_ADAPTORS
	xv_adaptors = DeviceInfo::xvAdaptors();
#endif

	// Channels combo
	channels_combo->addItem( "2", MediaSettings::ChStereo );
	channels_combo->addItem( "4", MediaSettings::ChSurround );
	channels_combo->addItem( "6", MediaSettings::ChFull51 );

	connect(vo_combo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(vo_combo_changed(int)));
	connect(ao_combo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(ao_combo_changed(int)));

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

	int deinterlace_item = deinterlace_combo->currentIndex();
	deinterlace_combo->clear();
	deinterlace_combo->addItem( tr("None"), MediaSettings::NoDeinterlace );
	deinterlace_combo->addItem( tr("Lowpass5"), MediaSettings::L5 );
	deinterlace_combo->addItem( tr("Yadif (normal)"), MediaSettings::Yadif );
	deinterlace_combo->addItem( tr("Yadif (double framerate)"), MediaSettings::Yadif_1 );
	deinterlace_combo->addItem( tr("Linear Blend"), MediaSettings::LB );
	deinterlace_combo->addItem( tr("Kerndeint"), MediaSettings::Kerndeint );
	deinterlace_combo->setCurrentIndex(deinterlace_item);

	int filesettings_method_item = filesettings_method_combo->currentIndex();
	filesettings_method_combo->clear();
	filesettings_method_combo->addItem( tr("one ini file"), "normal");
	filesettings_method_combo->addItem( tr("multiple ini files"), "hash");
	filesettings_method_combo->setCurrentIndex(filesettings_method_item);

	updateDriverCombos();

    // Icons
	/*
    resize_window_icon->setPixmap( Images::icon("resize_window") );
    volume_icon->setPixmap( Images::icon("speaker") );
	*/

	mplayerbin_edit->setCaption(tr("Select the mplayer executable"));
#ifdef Q_OS_WIN
	mplayerbin_edit->setFilter(tr("Executables") +" (*.exe)");
#else
	mplayerbin_edit->setFilter(tr("All files") +" (*)");
#endif
	screenshot_edit->setCaption(tr("Select a directory"));

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

	QString vo = pref->vo;
	if (vo.isEmpty()) {
#ifdef Q_OS_WIN
		vo = "directx,";
#else
		vo = "xv,";
#endif
	}
	setVO( vo );

	QString ao = pref->ao;
#ifndef Q_OS_WIN
	if (ao.isEmpty()) ao = "alsa,";
#endif
	setAO( ao );

	setRememberSettings( !pref->dont_remember_media_settings );
	setRememberTimePos( !pref->dont_remember_time_pos );
	setFileSettingsMethod( pref->file_settings_method );
	setAudioLang( pref->audio_lang );
	setSubtitleLang( pref->subtitle_lang );
	setAudioTrack( pref->initial_audio_track );
	setSubtitleTrack( pref->initial_subtitle_track );
	setCloseOnFinish( pref->close_on_finish );
	setPauseWhenHidden( pref->pause_when_hidden );

	setEq2( pref->use_soft_video_eq );
	setUseAudioEqualizer( pref->use_audio_equalizer );
	setSoftVol( pref->use_soft_vol );
	setAc3DTSPassthrough( pref->use_hwac3 );
	setInitialVolNorm( pref->initial_volnorm );
	setAmplification( pref->softvol_max );
	setInitialPostprocessing( pref->initial_postprocessing );
	setInitialDeinterlace( pref->initial_deinterlace );
	setInitialZoom( pref->initial_panscan_factor );
	setDirectRendering( pref->use_direct_rendering );
	setDoubleBuffer( pref->use_double_buffer );
	setUseSlices( pref->use_slices );
	setStartInFullscreen( pref->start_in_fullscreen );
	setDisableScreensaver( pref->disable_screensaver );
	setBlackbordersOnFullscreen( pref->add_blackborders_on_fullscreen );
	setAutoq( pref->autoq );

	setInitialVolume( pref->initial_volume );
	setDontChangeVolume( pref->dont_change_volume );
	setUseVolume( pref->use_volume_option );
	setAudioChannels( pref->initial_audio_channels );
	setScaleTempoFilter( pref->use_scaletempo );
}

void PrefGeneral::getData(Preferences * pref) {
	requires_restart = false;
	filesettings_method_changed = false;

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
	bool dont_remember_time = !rememberTimePos();
    TEST_AND_SET(pref->dont_remember_time_pos, dont_remember_time);
	if (pref->file_settings_method != fileSettingsMethod()) {
		pref->file_settings_method = fileSettingsMethod();
		filesettings_method_changed = true;
	}

	pref->audio_lang = audioLang();
    pref->subtitle_lang = subtitleLang();

	pref->initial_audio_track = audioTrack();
	pref->initial_subtitle_track = subtitleTrack();

	pref->close_on_finish = closeOnFinish();
	pref->pause_when_hidden = pauseWhenHidden();

	TEST_AND_SET(pref->use_soft_video_eq, eq2());
	TEST_AND_SET(pref->use_soft_vol, softVol());
	TEST_AND_SET(pref->use_audio_equalizer, useAudioEqualizer());
	TEST_AND_SET(pref->use_hwac3, Ac3DTSPassthrough());
	pref->initial_volnorm = initialVolNorm();
	TEST_AND_SET(pref->softvol_max, amplification());
	pref->initial_postprocessing = initialPostprocessing();
	pref->initial_deinterlace = initialDeinterlace();
	pref->initial_panscan_factor = initialZoom();
	TEST_AND_SET(pref->use_direct_rendering, directRendering());
	TEST_AND_SET(pref->use_double_buffer, doubleBuffer());
	TEST_AND_SET(pref->use_slices, useSlices());
	pref->start_in_fullscreen = startInFullscreen();
	TEST_AND_SET(pref->disable_screensaver, disableScreensaver());
	if (pref->add_blackborders_on_fullscreen != blackbordersOnFullscreen()) {
		pref->add_blackborders_on_fullscreen = blackbordersOnFullscreen();
		if (pref->fullscreen) requires_restart = true;
	}
	TEST_AND_SET(pref->autoq, autoq());

	pref->initial_volume = initialVolume();
	pref->dont_change_volume = dontChangeVolume();
	pref->use_volume_option = useVolume();
	pref->initial_audio_channels = audioChannels();
	TEST_AND_SET(pref->use_scaletempo, scaleTempoFilter());
}

void PrefGeneral::updateDriverCombos() {
	int vo_current = vo_combo->currentIndex();
	int ao_current = ao_combo->currentIndex();

	vo_combo->clear();
	ao_combo->clear();

	QString vo;
	for ( int n = 0; n < vo_list.count(); n++ ) {
		vo = vo_list[n].name();
#ifdef Q_OS_WIN
		if ( vo == "directx" ) {
			vo_combo->addItem( "directx (" + tr("fast") + ")", "directx" );
			vo_combo->addItem( "directx (" + tr("slow") + ")", "directx:noaccel" );
		}
		else
#else
		/*
		if (vo == "xv") vo_combo->addItem( "xv (" + tr("fastest") + ")", vo);
		else
		*/
#if USE_XV_ADAPTORS
		if ((vo == "xv") && (!xv_adaptors.isEmpty())) {
			vo_combo->addItem(vo, vo);
			for (int n=0; n < xv_adaptors.count(); n++) {
				vo_combo->addItem( "xv (" + xv_adaptors[n].ID().toString() + " - " + xv_adaptors[n].desc() + ")", 
                                   "xv:adaptor=" + xv_adaptors[n].ID().toString() );
			}
		}
		else
#endif // USE_XV_ADAPTORS
#endif
		if (vo == "x11") vo_combo->addItem( "x11 (" + tr("slow") + ")", vo);
		else
		if (vo == "gl") {
			vo_combo->addItem( vo, vo);
			vo_combo->addItem( "gl (" + tr("fast") + ")", "gl:yuv=2:force-pbo");
			vo_combo->addItem( "gl (" + tr("fast - ATI cards") + ")", "gl:yuv=2:force-pbo:ati-hack");
			vo_combo->addItem( "gl (yuv)", "gl:yuv=3");
		}
		else
		if (vo == "gl2") {
			vo_combo->addItem( vo, vo);
			vo_combo->addItem( "gl2 (yuv)", "gl2:yuv=3");
		}
		else
		if (vo == "null" || vo == "png" || vo == "jpeg" || vo == "gif89a" || 
            vo == "tga" || vo == "pnm" || vo == "md5sum" ) 
		{
			; // Nothing to do
		}
		else
		vo_combo->addItem( vo, vo );
	}
	vo_combo->addItem( tr("User defined..."), "user_defined" );

	QString ao;
	for ( int n = 0; n < ao_list.count(); n++) {
		ao = ao_list[n].name();
		ao_combo->addItem( ao, ao );
#if USE_ALSA_DEVICES
		if ((ao == "alsa") && (!alsa_devices.isEmpty())) {
			for (int n=0; n < alsa_devices.count(); n++) {
				ao_combo->addItem( "alsa (" + alsa_devices[n].ID().toString() + " - " + alsa_devices[n].desc() + ")", 
                                   "alsa:device=hw=" + alsa_devices[n].ID().toString() );
			}
		}
#endif
#if USE_DSOUND_DEVICES
		if ((ao == "dsound") && (!dsound_devices.isEmpty())) {
			for (int n=0; n < dsound_devices.count(); n++) {
				ao_combo->addItem( "dsound (" + dsound_devices[n].ID().toString() + " - " + dsound_devices[n].desc() + ")", 
                                   "dsound:device=" + dsound_devices[n].ID().toString() );
			}
		}
#endif
	}
	ao_combo->addItem( tr("User defined..."), "user_defined" );

	vo_combo->setCurrentIndex( vo_current );
	ao_combo->setCurrentIndex( ao_current );
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
	int idx = vo_combo->findData( vo_driver );
	if (idx != -1) {
		vo_combo->setCurrentIndex(idx);
	} else {
		vo_combo->setCurrentIndex(vo_combo->findData("user_defined"));
		vo_user_defined_edit->setText(vo_driver);
	}
	vo_combo_changed(vo_combo->currentIndex());
}

void PrefGeneral::setAO( QString ao_driver ) {
	int idx = ao_combo->findData( ao_driver );
	if (idx != -1) {
		ao_combo->setCurrentIndex(idx);
	} else {
		ao_combo->setCurrentIndex(ao_combo->findData("user_defined"));
		ao_user_defined_edit->setText(ao_driver);
	}
	ao_combo_changed(ao_combo->currentIndex());
}

QString PrefGeneral::VO() {
	QString vo = vo_combo->itemData(vo_combo->currentIndex()).toString();
	if (vo == "user_defined") {
		vo = vo_user_defined_edit->text();
		if (vo.isEmpty()) {
			vo = vo_combo->itemData(0).toString();
			qDebug("PrefGeneral::VO: user defined vo is empty, using %s", vo.toUtf8().constData());
		}
	}
	return vo;
}

QString PrefGeneral::AO() {
	QString ao = ao_combo->itemData(ao_combo->currentIndex()).toString();
	if (ao == "user_defined") {
		ao = ao_user_defined_edit->text();
		if (ao.isEmpty()) {
			ao = ao_combo->itemData(0).toString();
			qDebug("PrefGeneral::AO: user defined ao is empty, using %s", ao.toUtf8().constData());
		}
	}
	return ao;
}

void PrefGeneral::setRememberSettings(bool b) {
	remember_all_check->setChecked(b);
	//rememberAllButtonToggled(b);
}

bool PrefGeneral::rememberSettings() {
	return remember_all_check->isChecked();
}

void PrefGeneral::setRememberTimePos(bool b) {
	remember_time_check->setChecked(b);
}

bool PrefGeneral::rememberTimePos() {
	return remember_time_check->isChecked();
}

void PrefGeneral::setFileSettingsMethod(QString method) {
	int index = filesettings_method_combo->findData(method);
	if (index < 0) index = 0;
	filesettings_method_combo->setCurrentIndex(index);
}

QString PrefGeneral::fileSettingsMethod() {
	return filesettings_method_combo->itemData(filesettings_method_combo->currentIndex()).toString();
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

void PrefGeneral::setUseAudioEqualizer(bool b) {
	audio_equalizer_check->setChecked(b);
}

bool PrefGeneral::useAudioEqualizer() {
	return audio_equalizer_check->isChecked();
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

void PrefGeneral::setInitialDeinterlace(int ID) {
	int pos = deinterlace_combo->findData(ID);
	if (pos != -1) {
		deinterlace_combo->setCurrentIndex(pos);
	} else {
		qWarning("PrefGeneral::setInitialDeinterlace: ID: %d not found in combo", ID);
	}
}

int PrefGeneral::initialDeinterlace() {
	if (deinterlace_combo->currentIndex() != -1) {
		return deinterlace_combo->itemData( deinterlace_combo->currentIndex() ).toInt();
	} else {
		qWarning("PrefGeneral::initialDeinterlace: no item selected");
		return 0;
	}
}

void PrefGeneral::setInitialZoom(double v) {
	zoom_spin->setValue(v);
}

double PrefGeneral::initialZoom() {
	return zoom_spin->value();
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

void PrefGeneral::setUseSlices(bool b) {
	use_slices_check->setChecked(b);
}

bool PrefGeneral::useSlices() {
	return use_slices_check->isChecked();
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

void PrefGeneral::setUseVolume(Preferences::OptionState value) {
	use_volume_combo->setState(value);
}

Preferences::OptionState PrefGeneral::useVolume() {
	return use_volume_combo->state();
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

void PrefGeneral::setBlackbordersOnFullscreen(bool b) {
	blackborders_on_fs_check->setChecked(b);
}

bool PrefGeneral::blackbordersOnFullscreen() {
	return blackborders_on_fs_check->isChecked();
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

void PrefGeneral::vo_combo_changed(int idx) {
	qDebug("PrefGeneral::vo_combo_changed: %d", idx);
	bool visible = (vo_combo->itemData(idx).toString() == "user_defined");
	vo_user_defined_edit->setShown(visible);
	vo_user_defined_edit->setFocus();
}

void PrefGeneral::ao_combo_changed(int idx) {
	qDebug("PrefGeneral::ao_combo_changed: %d", idx);
	bool visible = (ao_combo->itemData(idx).toString() == "user_defined");
	ao_user_defined_edit->setShown(visible);
	ao_user_defined_edit->setFocus();
}

void PrefGeneral::createHelp() {
	clearHelp();

	addSectionTitle(tr("General"));

	setWhatsThis(mplayerbin_edit, tr("MPlayer executable"), 
		tr("Here you must specify the mplayer "
           "executable that SMPlayer will use.<br>"
           "SMPlayer requires at least MPlayer 1.0rc1 (although a recent "
           "revision from SVN is highly recommended).") + "<br><b>" +
        tr("If this setting is wrong, SMPlayer won't be able to play "
           "anything!") + "</b>");

	setWhatsThis(screenshot_edit, tr("Screenshots folder"),
		tr("Here you can specify a folder where the screenshots taken by "
           "SMPlayer will be stored. If this field is empty the "
           "screenshot feature will be disabled.") );


	setWhatsThis(remember_all_check, tr("Remember settings"),
		tr("Usually SMPlayer will remember the settings for each file you "
           "play (audio track selected, volume, filters...). Disable this "
           "option if you don't like this feature.") );

	setWhatsThis(remember_time_check, tr("Remember time position"),
		tr("If you check this option, SMPlayer will remember the last position "
           "of the file when you open it again. This option works only with "
           "regular files (not with DVDs, CDs, URLs...).") );

	setWhatsThis(filesettings_method_combo, tr("Method to store the file settings"),
		tr("This option allows to change the way the file settings would be "
           "stored. The following options are available:") +"<ul><li>" + 
		tr("<b>one ini file</b>: the settings for all played files will be "
           "saved in a single ini file (%1)").arg(QString("<i>"+Paths::iniPath()+"/smplayer.ini</i>")) + "</li><li>" +
		tr("<b>multiple ini files</b>: one ini file will be used for each played file. "
           "Those ini files will be saved in the folder %1").arg(QString("<i>"+Paths::iniPath()+"/file_settings</i>")) + "</li></ul>" +
		tr("The latter method could be faster if there is info for a lot of files.") );

	setWhatsThis(close_on_finish_check, tr("Close when finished"),
		tr("If this option is checked, the main window will be automatically "
		   "closed when the current file/playlist finishes.") );

	setWhatsThis(pause_if_hidden_check, tr("Pause when minimized"),
		tr("If this option is enabled, the file will be paused when the "
           "main window is hidden. When the window is restored, playback "
           "will be resumed.") );

	// Video tab
	addSectionTitle(tr("Video"));

	setWhatsThis(vo_combo, tr("Video output driver"),
		tr("Select the video output driver. %1 provides the best performance.")
#ifdef Q_OS_WIN
		  .arg("<b><i>directx</i></b>")
#else
		  .arg("<b><i>xv</i></b>")
#endif
		);

	setWhatsThis(postprocessing_check, tr("Enable postprocessing by default"),
		tr("Postprocessing will be used by default on new opened files.") );

	setWhatsThis(autoq_spin, tr("Postprocessing quality"),
		tr("Dynamically changes the level of postprocessing depending on the "
           "available spare CPU time. The number you specify will be the "
           "maximum level used. Usually you can use some big number.") );

	setWhatsThis(deinterlace_combo, tr("Deinterlace by default"),
        tr("Select the deinterlace filter that you want to be used for new "
           "videos opened.") );

	setWhatsThis(zoom_spin, tr("Default zoom"),
		tr("This option sets the default zoom which will be used for "
           "new videos.") );

	setWhatsThis(eq2_check, tr("Software video equalizer"),
		tr("You can check this option if video equalizer is not supported by "
           "your graphic card or the selected video output driver.<br>"
           "<b>Note:</b> this option can be incompatible with some video "
           "output drivers.") );

	setWhatsThis(direct_rendering_check, tr("Direct rendering"),
		tr("If checked, turns on direct rendering (not supported by all "
           "codecs and video outputs)<br>"
           "<b>Warning:</b> May cause OSD/SUB corruption!") );

	setWhatsThis(double_buffer_check, tr("Double buffering"),
		tr("Double buffering fixes flicker by storing two frames in memory, "
           "and displaying one while decoding another. If disabled it can "
           "affect OSD negatively, but often removes OSD flickering.") );

	setWhatsThis(use_slices_check, tr("Draw video using slices"),
		tr("Enable/disable drawing video by 16-pixel height slices/bands. "
           "If disabled, the whole frame is drawn in a single run. "
           "May be faster or slower, depending on video card and available "
           "cache. It has effect only with libmpeg2 and libavcodec codecs.") );

	setWhatsThis(start_fullscreen_check, tr("Start videos in fullscreen"),
		tr("If this option is checked, all videos will start to play in "
           "fullscreen mode.") );

	setWhatsThis(blackborders_on_fs_check, tr("Add black borders on fullscreen"),
		tr("If this option is enabled, black borders will be added to the "
           "image in fullscreen mode. This allows subtitles to be displayed "
           "on the black borders.") /* + "<br>" +
 		tr("This option will be ignored if MPlayer uses its own window, as "
           "some video drivers (like gl) are already able to display the "
           "subtitles automatically in the black borders.") */ );

	setWhatsThis(screensaver_check, tr("Disable screensaver"),
		tr("Check this option to disable the screensaver while playing.<br>"
           "The screensaver will enabled again when play finishes.")
           //+ tr("<br><b>Note:</b> This option works only in X11 and Windows.")
		);

	// Audio tab
	addSectionTitle(tr("Audio"));

	setWhatsThis(ao_combo, tr("Audio output driver"),
		tr("Select the audio output driver.") 
#ifndef Q_OS_WIN
        + " " + 
		tr("%1 is the recommended one. Try to avoid %2 and %3, they are slow "
           "and can have an impact on performance.")
           .arg("<b><i>alsa</i></b>")
           .arg("<b><i>esd</i></b>")
           .arg("<b><i>arts</i></b>")
#endif
		);

	setWhatsThis(audio_equalizer_check, tr("Enable the audio equalizer"),
		tr("Check this option if you want to use the audio equalizer.") );

	setWhatsThis(hwac3_check, tr("AC3/DTS pass-through S/PDIF"),
		tr("Uses hardware AC3 passthrough") );

	setWhatsThis(channels_combo, tr("Channels by default"),
		tr("Requests the number of playback channels. MPlayer "
           "asks the decoder to decode the audio into as many channels as "
           "specified. Then it is up to the decoder to fulfill the "
           "requirement. This is usually only important when playing "
           "videos with AC3 audio (like DVDs). In that case liba52 does "
           "the decoding by default and correctly downmixes the audio "
           "into the requested number of channels. "
           "<b>Note</b>: This option is honored by codecs (AC3 only), "
           "filters (surround) and audio output drivers (OSS at least).") );

	setWhatsThis(scaletempo_combo, tr("High speed playback without altering pitch"),
		tr("Allows to change the playback speed without altering pitch. "
           "Requires at least MPlayer dev-SVN-r24924.") );

	setWhatsThis(softvol_check, tr("Software volume control"),
		tr("Check this option to use the software mixer, instead of "
           "using the sound card mixer.") );

	setWhatsThis(softvol_max_spin, tr("Max. Amplification"),
		tr("Sets the maximum amplification level in percent (default: 110). "
           "A value of 200 will allow you to adjust the volume up to a "
           "maximum of double the current level. With values below 100 the "
           "initial volume (which is 100%) will be above the maximum, which "
           "e.g. the OSD cannot display correctly.") );

	setWhatsThis(volnorm_check, tr("Volume normalization by default"),
		tr("Maximizes the volume without distorting the sound.") );

	setWhatsThis(change_volume_check, tr("Change volume"),
		tr("If checked, SMPlayer will remember the volume for every file "
           "and will restore it when played again. For new files the default "
           "volume will be used.") );

	setWhatsThis(use_volume_combo, tr("Change volume just before playing"),
		tr("If this option is checked the initial volume will be set just "
           "before playback starts. This avoids a loud volume on startup. "
           "Requires at least MPlayer SVN r27872."));

	setWhatsThis(initial_volume_slider, tr("Default volume"),
		tr("Sets the initial volume that new files will use.") );


	addSectionTitle(tr("Preferred audio and subtitles"));

	setWhatsThis(audio_lang_edit, tr("Preferred audio language"),
		tr("Here you can type your preferred language for the audio streams. "
           "When a media with multiple audio streams is found, SMPlayer will "
           "try to use your preferred language.<br>"
           "This only will work with media that offer info about the language "
           "of the audio streams, like DVDs or mkv files.<br>"
           "This field accepts regular expressions. Example: <b>es|esp|spa</b> "
           "will select the audio track if it matches with <i>es</i>, "
           "<i>esp</i> or <i>spa</i>.") );

	setWhatsThis(subtitle_lang_edit, tr("Preferred subtitle language"),
		tr("Here you can type your preferred language for the subtitle stream. "
           "When a media with multiple subtitle streams is found, SMPlayer will "
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
