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


#include "prefadvanced.h"
#include "images.h"
#include "preferences.h"
#include "config.h"
#include <QColorDialog>

PrefAdvanced::PrefAdvanced(QWidget * parent, Qt::WindowFlags f)
	: PrefWidget(parent, f )
{
	setupUi(this);

#ifndef Q_OS_WIN
	shortnames_check->hide();
#endif

#if !USE_COLORKEY
	colorkey_label->hide();
	colorkey_view->hide();
	changeButton->hide();
#endif

	// Monitor aspect
	monitoraspect_combo->addItem("Auto");
	monitoraspect_combo->addItem("4:3");
	monitoraspect_combo->addItem("16:9");
	monitoraspect_combo->addItem("5:4");
	monitoraspect_combo->addItem("16:10");

	// MPlayer language combos.
    endoffile_combo->addItem( "Exiting... \\(End of file\\)" ); // English
    endoffile_combo->addItem( "Saliendo... \\(Fin de archivo\\.\\)" ); // Spanish
    endoffile_combo->addItem( "Beenden... \\(Dateiende erreicht\\)" ); // German
    endoffile_combo->addItem( "Sortie... \\(Fin du fichier\\)" ); // French
    endoffile_combo->addItem( "In uscita... \\(Fine del file\\)" ); // Italian
    endoffile_combo->addItem( QString::fromUtf8("Wychodzę... \\(Koniec pliku\\)") ); // Polish
    endoffile_combo->addItem( QString::fromUtf8("Выходим... \\(Конец файла\\)") ); // Russian

    novideo_combo->addItem( "Video: no video" ); // English
    novideo_combo->addItem( QString::fromUtf8("Vídeo: no hay video") ); // Spanish
    novideo_combo->addItem( "Video: kein Video" ); // German
    novideo_combo->addItem( QString::fromUtf8("Vidéo : pas de vidéo") ); // French
    novideo_combo->addItem( "Video: nessun video" ); // Italian
    novideo_combo->addItem( "Wideo: brak obrazu" ); // Polish
    novideo_combo->addItem( QString::fromUtf8("Видео: нет видео") ); // Russian

	retranslateStrings();
}

PrefAdvanced::~PrefAdvanced()
{
}

QString PrefAdvanced::sectionName() {
	return tr("Advanced");
}

QPixmap PrefAdvanced::sectionIcon() {
    return Images::icon("pref_advanced");
}


void PrefAdvanced::retranslateStrings() {
	retranslateUi(this);

	monitor_aspect_icon->setPixmap( Images::icon("monitor") );

	monitoraspect_combo->setItemText(0, tr("Auto") );

	createHelp();
}

void PrefAdvanced::setData(Preferences * pref) {
	setMonitorAspect( pref->monitor_aspect );
	
	setClearBackground( pref->always_clear_video_background );
	setUseMplayerWindow( pref->use_mplayer_window );
	setMplayerAdditionalArguments( pref->mplayer_additional_options );
	setMplayerAdditionalVideoFilters( pref->mplayer_additional_video_filters );
	setMplayerAdditionalAudioFilters( pref->mplayer_additional_audio_filters );
#if USE_COLORKEY
	setColorKey( pref->color_key );
#endif

	setLogMplayer( pref->log_mplayer );
	setLogSmplayer( pref->log_smplayer );
	setLogFilter( pref->log_filter );

    setSaveMplayerLog( pref->autosave_mplayer_log );
    setMplayerLogName( pref->mplayer_log_saveto );

	setEndOfFileText( pref->rx_endoffile );
	setNoVideoText( pref->rx_novideo );

	setUseShortNames( pref->use_short_pathnames );
}

void PrefAdvanced::getData(Preferences * pref) {
	requires_restart = false;
	clearing_background_changed = false;
	monitor_aspect_changed = false;
#if USE_COLORKEY
	colorkey_changed = false;
#endif

	if (pref->monitor_aspect != monitorAspect()) {
		pref->monitor_aspect = monitorAspect();
		monitor_aspect_changed = true;
		requires_restart = true;
	}

	if (pref->always_clear_video_background != clearBackground()) {
		pref->always_clear_video_background = clearBackground();
		clearing_background_changed = true;
    }

	TEST_AND_SET(pref->use_mplayer_window, useMplayerWindow());
	TEST_AND_SET(pref->mplayer_additional_options, mplayerAdditionalArguments());
	TEST_AND_SET(pref->mplayer_additional_video_filters, mplayerAdditionalVideoFilters());
	TEST_AND_SET(pref->mplayer_additional_audio_filters, mplayerAdditionalAudioFilters());
#if USE_COLORKEY
	if (pref->color_key != colorKey()) {
		pref->color_key = colorKey();
		colorkey_changed = true;
		requires_restart = true;
	}
#endif
	pref->log_mplayer = logMplayer();
	pref->log_smplayer = logSmplayer();
	pref->log_filter = logFilter();
    pref->autosave_mplayer_log = saveMplayerLog();
    pref->mplayer_log_saveto = mplayerLogName();

	TEST_AND_SET(pref->rx_endoffile, endOfFileText());
	TEST_AND_SET(pref->rx_novideo, noVideoText());

	pref->use_short_pathnames = useShortNames();
}

void PrefAdvanced::setMonitorAspect(QString asp) {
	if (asp.isEmpty())
		monitoraspect_combo->setCurrentIndex( 0 );
	else
		monitoraspect_combo->setCurrentText(asp);
		//monitoraspect_combo->setEditText(asp);
}

QString PrefAdvanced::monitorAspect() {
	if (monitoraspect_combo->currentIndex() == 0 ) 
		return "";
	else
		return monitoraspect_combo->currentText();
}

void PrefAdvanced::setClearBackground(bool b) {
	not_clear_background_check->setChecked(!b);
}

bool PrefAdvanced::clearBackground() {
	return !not_clear_background_check->isChecked();
}

void PrefAdvanced::setUseMplayerWindow(bool v) {
	mplayer_use_window_check->setChecked(v);
}

bool PrefAdvanced::useMplayerWindow() {
	return mplayer_use_window_check->isChecked();
}

void PrefAdvanced::setUseShortNames(bool b) {
	shortnames_check->setChecked(b);
}

bool PrefAdvanced::useShortNames() {
	return shortnames_check->isChecked();
}

void PrefAdvanced::setMplayerAdditionalArguments(QString args) {
	mplayer_args_edit->setText(args);
}

QString PrefAdvanced::mplayerAdditionalArguments() {
	return mplayer_args_edit->text();
}

void PrefAdvanced::setMplayerAdditionalVideoFilters(QString s) {
	mplayer_vfilters_edit->setText(s);
}

QString PrefAdvanced::mplayerAdditionalVideoFilters() {
	return mplayer_vfilters_edit->text();
}

void PrefAdvanced::setMplayerAdditionalAudioFilters(QString s) {
	mplayer_afilters_edit->setText(s);
}

QString PrefAdvanced::mplayerAdditionalAudioFilters() {
	return mplayer_afilters_edit->text();
}

#if USE_COLORKEY
void PrefAdvanced::setColorKey(unsigned int c) {
	QString color = QString::number(c, 16);
	while (color.length() < 6) color = "0"+color;
	colorkey_view->setText( "#" + color );
}

unsigned int PrefAdvanced::colorKey() {
	QString c = colorkey_view->text();
	if (c.startsWith("#")) c = c.mid(1);

	bool ok;
	unsigned int color = c.toUInt(&ok, 16);

	if (!ok) 
		qWarning("PrefAdvanced::colorKey: cannot convert color to uint");

	qDebug("PrefAdvanced::colorKey: color: %s", QString::number(color,16).toUtf8().data() );

	return color;
}
#endif

void PrefAdvanced::on_changeButton_clicked() {
	//bool ok;
	//int color = colorkey_view->text().toUInt(&ok, 16);
	QColor color( colorkey_view->text() );
	QColor c = QColorDialog::getColor ( color, this );
	if (c.isValid()) {
		//colorkey_view->setText( QString::number( c.rgb(), 16 ) );
		colorkey_view->setText( c.name() );
	}
}

// Log options
void PrefAdvanced::setLogMplayer(bool b) {
	log_mplayer_check->setChecked(b);
}

bool PrefAdvanced::logMplayer() {
	return log_mplayer_check->isChecked();
}

void PrefAdvanced::setLogSmplayer(bool b) {
	log_smplayer_check->setChecked(b);
}

bool PrefAdvanced::logSmplayer() {
	return log_smplayer_check->isChecked();
}

void PrefAdvanced::setLogFilter(QString filter) {
	log_filter_edit->setText(filter);
}

QString PrefAdvanced::logFilter() {
	return log_filter_edit->text();
}


void PrefAdvanced::setSaveMplayerLog(bool b) {
    log_mplayer_save_check->setChecked(b);
}

bool PrefAdvanced::saveMplayerLog() {
    return log_mplayer_save_check->isChecked();
}

void PrefAdvanced::setMplayerLogName(QString filter) {
    log_mplayer_save_name->setText(filter);
}

QString PrefAdvanced::mplayerLogName() {
    return log_mplayer_save_name->text();
}


// MPlayer language page
void PrefAdvanced::setEndOfFileText(QString t) {
	endoffile_combo->setCurrentText(t);
}

QString PrefAdvanced::endOfFileText() {
	return endoffile_combo->currentText();
}

void PrefAdvanced::setNoVideoText(QString t) {
	novideo_combo->setCurrentText(t);
}

QString PrefAdvanced::noVideoText() {
	return novideo_combo->currentText();
}

void PrefAdvanced::createHelp() {
	clearHelp();

	addSectionTitle(tr("Advanced"));

	setWhatsThis(monitoraspect_combo, tr("Monitor aspect"),
        tr("Select the aspect ratio of your monitor.") );

	setWhatsThis(log_smplayer_check, tr("Log SMPlayer output"),
		tr("If this option is checked, smplayer will store the debugging "
           "messages that smplayer outputs "
           "(you can see the log in <b>Options->View logs->smplayer</b>). "
           "This information can be very useful for the developer in case "
           "you find a bug." ) );

	setWhatsThis(log_mplayer_check, tr("Log MPlayer output"),
		tr("If checked, smplayer will store the output of mplayer "
           "(you can see it in <b>Options->View logs->mplayer</b>). "
           "In case of problems this log can contain important information, "
           "so it's recommended to keep this option checked.") );

	setWhatsThis(log_mplayer_save_check, tr("Autosave MPlayer log"),
		tr("If this option is checked, the MPlayer log will be saved to the "
           "specified file every time a new file starts to play. "
           "It's intended for external applications, so they can get "
           "info about the file you're playing.") );

	setWhatsThis(log_mplayer_save_name, tr("Autosave MPlayer log filename"),
 		tr("Enter here the path and filename that will be used to save the "
           "MPlayer log.") );

	setWhatsThis(log_filter_edit, tr("Filter for SMPlayer logs"),
		tr("This option allows to filter the smplayer messages that will "
           "be stored in the log. Here you can write any regular expression.<br>"
           "For instance: <i>^Core::.*</i> will display only the lines "
           "starting with <i>Core::</i>") );

	setWhatsThis(mplayer_use_window_check, tr("Run MPlayer in its own window"),
        tr("If you check this option, the MPlayer video window won't be "
           "embedded in SMPlayer's main window but instead it will use its "
           "own window. Note that mouse and keyboard events will be handled "
           "directly by MPlayer, that means key shortcuts and mouse clicks "
           "probably won't work as expected when the MPlayer window has the "
           "focus.") );

	setWhatsThis(not_clear_background_check, 
        tr("Don't repaint the background of the video window"),
		tr("Checking this option may reduce flickering, but it also might "
           "produce that the video won't be displayed properly.") );

#ifdef Q_OS_WIN
	setWhatsThis(shortnames_check, tr("Pass short filenames (8+3) to MPlayer"),
		tr("Currently MPlayer can't open filenames which contains characters "
           "outside the local codepage. Checking this option will make "
           "SMPlayer to pass to MPlayer the short version of the filenames, "
           "and thus it will able to open them.") );
#endif

#if USE_COLORKEY
	setWhatsThis(colorkey_view, tr("Colorkey"),
        tr("If you see parts of the video over any other window, you can "
           "change the colorkey to fix it. Try to select a color close to "
           "black.") );
#endif

	addSectionTitle(tr("Options for MPlayer"));

	setWhatsThis(mplayer_args_edit, tr("Options"),
        tr("Here you can type options for MPlayer. Write them separated "
           "by spaces.") );

	setWhatsThis(mplayer_vfilters_edit, tr("Video filters"),
        tr("Here you can add video filters for MPlayer. Write them separated "
           "by commas. Don't use spaces!") );

	setWhatsThis(mplayer_afilters_edit, tr("Audio filters"),
        tr("Here you can add audio filters for MPlayer. Write them separated "
           "by commas. Don't use spaces!") );

	addSectionTitle(tr("MPlayer language"));

	setWhatsThis(endoffile_combo, tr("End of file"),
        tr("Select or type a regular expression for 'End of file'") );

	setWhatsThis(novideo_combo, tr("No video"),
        tr("Select or type a regular expression for 'No video'") );
}

#include "moc_prefadvanced.cpp"
