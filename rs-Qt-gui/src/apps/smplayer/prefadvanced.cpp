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


#include "prefadvanced.h"
#include "images.h"
#include "preferences.h"

#include <QColorDialog>

PrefAdvanced::PrefAdvanced(QWidget * parent, Qt::WindowFlags f)
	: PrefWidget(parent, f )
{
	setupUi(this);

	// Monitor aspect
	monitoraspect_combo->addItem("Auto");
	monitoraspect_combo->addItem("4:3");
	monitoraspect_combo->addItem("16:9");
	monitoraspect_combo->addItem("5:4");
	monitoraspect_combo->addItem("16:10");

	// MPlayer language combos.
    endoffile_combo->addItem( "Exiting... \\(End of file\\)" );
    endoffile_combo->addItem( "Saliendo... \\(Fin de archivo\\.\\)" );
    endoffile_combo->addItem( "Beenden... \\(Dateiende erreicht\\)" );
    endoffile_combo->addItem( "Sortie... \\(Fin du fichier\\)" );
    endoffile_combo->addItem( "In uscita... \\(Fine del file\\)" );

    novideo_combo->addItem( "Video: no video" );
	novideo_combo->addItem( QString::fromLatin1("Vídeo: no hay video") );
    novideo_combo->addItem( "Video: kein Video" );
    novideo_combo->addItem( QString::fromLatin1("Vidéo : pas de vidéo") );
    novideo_combo->addItem( "Video: nessun video" );

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
	setColorKey( pref->color_key );

	setLogMplayer( pref->log_mplayer );
	setLogSmplayer( pref->log_smplayer );
	setLogFilter( pref->log_filter );

	setEndOfFileText( pref->rx_endoffile );
	setNoVideoText( pref->rx_novideo );
}

void PrefAdvanced::getData(Preferences * pref) {
	requires_restart = false;
	clearing_background_changed = false;
	colorkey_changed = false;
	monitor_aspect_changed = false;

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
	if (pref->color_key != colorKey()) {
		pref->color_key = colorKey();
		colorkey_changed = true;
		requires_restart = true;
	}

	pref->log_mplayer = logMplayer();
	pref->log_smplayer = logSmplayer();
	pref->log_filter = logFilter();

	TEST_AND_SET(pref->rx_endoffile, endOfFileText());
	TEST_AND_SET(pref->rx_novideo, noVideoText());
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

	// Advanced tab
	setWhatsThis(not_clear_background_check, 
        tr("Don't repaint the background of the video window"),
		tr("Checking this option may reduce flickering, but it also might "
           "produce that the video won't be displayed properly.") );

	// Log tab
	setWhatsThis(log_mplayer_check, tr("Log MPlayer output"),
		tr("If checked, smplayer will store the output of mplayer "
           "(you can see it in <b>Options->View logs->mplayer</b>). "
           "In case of problems this log can contain important information, "
           "so it's recommended to keep this option checked.") );

	setWhatsThis(log_smplayer_check, tr("Log SMPlayer output"),
		tr("If this option is checked, smplayer will store the debugging "
           "messages that smplayer outputs "
           "(you can see the log in <b>Options->View logs->smplayer</b>). "
           "This information can be very useful for the developer in case "
           "you find a bug." ) );

	setWhatsThis(log_filter_edit, tr("Filter for SMPlayer logs"),
		tr("This option allows to filter the smplayer messages that will "
           "be stored in the log. Here you can write any regular expression.<br>"
           "For instance: <i>^Core::.*</i> will display only the lines "
           "starting with <i>Core::</i>") );
}

#include "moc_prefadvanced.cpp"
