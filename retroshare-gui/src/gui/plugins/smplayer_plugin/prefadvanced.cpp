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
#include <QColorDialog>
#include <QNetworkProxy>

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

#if !REPAINT_BACKGROUND_OPTION
	repaint_video_background_check->hide();
#endif

	// Monitor aspect
	monitoraspect_combo->addItem("Auto");
	monitoraspect_combo->addItem("4:3");
	monitoraspect_combo->addItem("16:9");
	monitoraspect_combo->addItem("5:4");
	monitoraspect_combo->addItem("16:10");

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

	int proxy_type_item = proxy_type_combo->currentIndex();
	proxy_type_combo->clear();
	proxy_type_combo->addItem( tr("Http"), QNetworkProxy::HttpProxy);
	proxy_type_combo->addItem( tr("Socks5"), QNetworkProxy::Socks5Proxy);
	proxy_type_combo->setCurrentIndex(proxy_type_item);

	createHelp();
}

void PrefAdvanced::setData(Preferences * pref) {
	setMonitorAspect( pref->monitor_aspect );

#if REPAINT_BACKGROUND_OPTION	
	setRepaintVideoBackground( pref->repaint_video_background );
#endif
	setUseMplayerWindow( pref->use_mplayer_window );
	setMplayerAdditionalArguments( pref->mplayer_additional_options );
	setMplayerAdditionalVideoFilters( pref->mplayer_additional_video_filters );
	setMplayerAdditionalAudioFilters( pref->mplayer_additional_audio_filters );
#if USE_COLORKEY
	setColorKey( pref->color_key );
#endif
	setPreferIpv4( pref->prefer_ipv4 );
	setUseIdx( pref->use_idx );
	setUseCorrectPts( pref->use_correct_pts );
	setActionsToRun( pref->actions_to_run );

	setLogMplayer( pref->log_mplayer );
	setLogSmplayer( pref->log_smplayer );
	setLogFilter( pref->log_filter );

    setSaveMplayerLog( pref->autosave_mplayer_log );
    setMplayerLogName( pref->mplayer_log_saveto );

	setUseShortNames( pref->use_short_pathnames );

	// Proxy
	setUseProxy( pref->use_proxy );
	setProxyHostname( pref->proxy_host );
	setProxyPort( pref->proxy_port );
	setProxyUsername( pref->proxy_username );
	setProxyPassword( pref->proxy_password );
	setProxyType( pref->proxy_type );
}

void PrefAdvanced::getData(Preferences * pref) {
	requires_restart = false;

#if REPAINT_BACKGROUND_OPTION
	repaint_video_background_changed = false;
#endif

	monitor_aspect_changed = false;
#if USE_COLORKEY
	colorkey_changed = false;
#endif
	pref->prefer_ipv4 = preferIpv4();
	TEST_AND_SET(pref->use_idx, useIdx());
	TEST_AND_SET(pref->use_correct_pts, useCorrectPts());
	pref->actions_to_run = actionsToRun();

	if (pref->monitor_aspect != monitorAspect()) {
		pref->monitor_aspect = monitorAspect();
		monitor_aspect_changed = true;
		requires_restart = true;
	}

#if REPAINT_BACKGROUND_OPTION
	if (pref->repaint_video_background != repaintVideoBackground()) {
		pref->repaint_video_background = repaintVideoBackground();
		repaint_video_background_changed = true;
    }
#endif

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

	pref->use_short_pathnames = useShortNames();

	// Proxy
	proxy_changed = ( (pref->use_proxy != useProxy()) || 
                      (pref->proxy_host != proxyHostname()) ||
                      (pref->proxy_port != proxyPort()) ||
                      (pref->proxy_username != proxyUsername()) ||
                      (pref->proxy_password != proxyPassword()) ||
                      (pref->proxy_type != proxyType()) );

	pref->use_proxy = useProxy();
	pref->proxy_host = proxyHostname();
	pref->proxy_port = proxyPort();
	pref->proxy_username = proxyUsername();
	pref->proxy_password = proxyPassword();
	pref->proxy_type = proxyType();
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

#if REPAINT_BACKGROUND_OPTION
void PrefAdvanced::setRepaintVideoBackground(bool b) {
	repaint_video_background_check->setChecked(b);
}

bool PrefAdvanced::repaintVideoBackground() {
	return repaint_video_background_check->isChecked();
}
#endif

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

void PrefAdvanced::setPreferIpv4(bool b) {
	if (b) 
		ipv4_button->setChecked(true);
	else 
		ipv6_button->setChecked(true);
}

bool PrefAdvanced::preferIpv4() {
	return ipv4_button->isChecked();
}

void PrefAdvanced::setUseIdx(bool b) {
	idx_check->setChecked(b);
}

bool PrefAdvanced::useIdx() {
	return idx_check->isChecked();
}

void PrefAdvanced::setUseCorrectPts(bool b) {
	correct_pts_check->setChecked(b);
}

bool PrefAdvanced::useCorrectPts() {
	return correct_pts_check->isChecked();
}

void PrefAdvanced::setActionsToRun(QString actions) {
	actions_to_run_edit->setText(actions);
}

QString PrefAdvanced::actionsToRun() {
	return actions_to_run_edit->text();
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

void PrefAdvanced::setUseProxy(bool b) {
	use_proxy_check->setChecked(b);
}

bool PrefAdvanced::useProxy() {
	return 	use_proxy_check->isChecked();
}

void PrefAdvanced::setProxyHostname(QString host) {
	proxy_hostname_edit->setText(host);
}

QString PrefAdvanced::proxyHostname() {
	return proxy_hostname_edit->text();
}

void PrefAdvanced::setProxyPort(int port) {
	proxy_port_spin->setValue(port);
}

int PrefAdvanced::proxyPort() {
	return proxy_port_spin->value();
}

void PrefAdvanced::setProxyUsername(QString username) {
	proxy_username_edit->setText(username);
}

QString PrefAdvanced::proxyUsername() {
	return proxy_username_edit->text();
}

void PrefAdvanced::setProxyPassword(QString password) {
	proxy_password_edit->setText(password);
}

QString PrefAdvanced::proxyPassword() {
	return proxy_password_edit->text();
}

void PrefAdvanced::setProxyType(int type) {
	int index = proxy_type_combo->findData(type);
	if (index == -1) index = 0;
	proxy_type_combo->setCurrentIndex(index);
}

int PrefAdvanced::proxyType() {
	int index = proxy_type_combo->currentIndex();
	return proxy_type_combo->itemData(index).toInt();
}


void PrefAdvanced::createHelp() {
	clearHelp();

	addSectionTitle(tr("Advanced"));

	setWhatsThis(monitoraspect_combo, tr("Monitor aspect"),
        tr("Select the aspect ratio of your monitor.") );

	setWhatsThis(mplayer_use_window_check, tr("Run MPlayer in its own window"),
        tr("If you check this option, the MPlayer video window won't be "
           "embedded in SMPlayer's main window but instead it will use its "
           "own window. Note that mouse and keyboard events will be handled "
           "directly by MPlayer, that means key shortcuts and mouse clicks "
           "probably won't work as expected when the MPlayer window has the "
           "focus.") );

	setWhatsThis(idx_check, tr("Rebuild index if needed"),
		tr("Rebuilds index of files if no index was found, allowing seeking. "
		   "Useful with broken/incomplete downloads, or badly created files. "
           "This option only works if the underlying media supports "
           "seeking (i.e. not with stdin, pipe, etc).<br> "
           "<b>Note:</b> the creation of the index may take some time.") );

	setWhatsThis(correct_pts_check, tr("Correct pts"),
		tr("Switches MPlayer to an experimental mode where timestamps for "
           "video frames are calculated differently and video filters which "
           "add new frames or modify timestamps of existing ones are "
           "supported. The more accurate timestamps can be visible for "
           "example when playing subtitles timed to scene changes with the "
           "SSA/ASS library enabled. Without correct pts the subtitle timing "
           "will typically be off by some frames. This option does not work "
           "correctly with some demuxers and codecs.") );

#ifdef Q_OS_WIN
	setWhatsThis(shortnames_check, tr("Pass short filenames (8+3) to MPlayer"),
		tr("Currently MPlayer can't open filenames which contains characters "
           "outside the local codepage. Checking this option will make "
           "SMPlayer to pass to MPlayer the short version of the filenames, "
           "and thus it will able to open them.") );
#endif

#if REPAINT_BACKGROUND_OPTION
	setWhatsThis(repaint_video_background_check, 
        tr("Repaint the background of the video window"),
		tr("Checking this option may reduce flickering, but it also might "
           "produce that the video won't be displayed properly.") );
#endif

#if USE_COLORKEY
	setWhatsThis(colorkey_view, tr("Colorkey"),
        tr("If you see parts of the video over any other window, you can "
           "change the colorkey to fix it. Try to select a color close to "
           "black.") );
#endif

	setWhatsThis(actions_to_run_edit, tr("Actions list"),
		tr("Here you can specify a list of <i>actions</i> which will be "
           "run every time a file is opened. You'll find all available "
           "actions in the key shortcut editor in the <b>Keyboard and mouse</b> "
           "section. The actions must be separated by spaces. Checkable "
           "actions can be followed by <i>true</i> or <i>false</i> to "
           "enable or disable the action.") +"<br>"+
		tr("Example:") +" <i>auto_zoom compact true</i><br>" +
		tr("Limitation: the actions are run only when a file is opened and "
           "not when the mplayer process is restarted (e.g. you select an "
           "audio or video filter).") );

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

	addSectionTitle(tr("Network"));

	setWhatsThis(ipv4_button, tr("IPv4"),
		tr("Use IPv4 on network connections. Falls back on IPv6 automatically."));

	setWhatsThis(ipv6_button, tr("IPv6"),
		tr("Use IPv6 on network connections. Falls back on IPv4 automatically."));

	setWhatsThis(use_proxy_check, tr("Enable proxy"),
		tr("Enable/disable the use of the proxy.") );

	setWhatsThis(proxy_hostname_edit, tr("Host"),
		tr("The host name of the proxy.") );

	setWhatsThis(proxy_port_spin, tr("Port"),
		tr("The port of the proxy.") );

	setWhatsThis(proxy_username_edit, tr("Username"),
		tr("If the proxy requires authentication, this sets the username.") );

	setWhatsThis(proxy_password_edit, tr("Password"),
		tr("The password for the proxy. <b>Warning:</b> the password will be saved "
           "as plain text in the configuration file.") );

	setWhatsThis(proxy_type_combo, tr("Type"),
		tr("Select the proxy type to be used.") );

	addSectionTitle(tr("Logs"));

	setWhatsThis(log_smplayer_check, tr("Log SMPlayer output"),
		tr("If this option is checked, SMPlayer will store the debugging "
           "messages that SMPlayer outputs "
           "(you can see the log in <b>Options -> View logs -> SMPlayer</b>). "
           "This information can be very useful for the developer in case "
           "you find a bug." ) );

	setWhatsThis(log_mplayer_check, tr("Log MPlayer output"),
		tr("If checked, SMPlayer will store the output of MPlayer "
           "(you can see it in <b>Options -> View logs -> MPlayer</b>). "
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
		tr("This option allows to filter the SMPlayer messages that will "
           "be stored in the log. Here you can write any regular expression.<br>"
           "For instance: <i>^Core::.*</i> will display only the lines "
           "starting with <i>Core::</i>") );

}

#include "moc_prefadvanced.cpp"
