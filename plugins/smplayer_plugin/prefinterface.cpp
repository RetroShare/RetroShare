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


#include "prefinterface.h"
#include "images.h"
#include "preferences.h"
#include "paths.h"
#include "config.h"
#include "languages.h"
#include "recents.h"

#include <QDir>
#include <QStyleFactory>
#include <QFontDialog>

PrefInterface::PrefInterface(QWidget * parent, Qt::WindowFlags f)
	: PrefWidget(parent, f )
{
	setupUi(this);
	/* volume_icon->hide(); */

	// Style combo
#if !STYLE_SWITCHING
    style_label->hide();
    style_combo->hide();
#else
	style_combo->addItem( "<default>" );
	style_combo->addItems( QStyleFactory::keys() );
#endif

	// Icon set combo
	iconset_combo->addItem( "Default" );

	// User
	QDir icon_dir = Paths::configPath() + "/themes";
	qDebug("icon_dir: %s", icon_dir.absolutePath().toUtf8().data());
	QStringList iconsets = icon_dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
	for (int n=0; n < iconsets.count(); n++) {
		iconset_combo->addItem( iconsets[n] );
	}
	// Global
	icon_dir = Paths::themesPath();
	qDebug("icon_dir: %s", icon_dir.absolutePath().toUtf8().data());
	iconsets = icon_dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
	for (int n=0; n < iconsets.count(); n++) {
		if (iconset_combo->findText( iconsets[n] ) == -1) {
			iconset_combo->addItem( iconsets[n] );
		}
	}

	connect(single_instance_check, SIGNAL(toggled(bool)), 
            this, SLOT(changeInstanceImages()));

#ifdef Q_OS_WIN
	floating_bypass_wm_check->hide();
#endif

	retranslateStrings();
}

PrefInterface::~PrefInterface()
{
}

QString PrefInterface::sectionName() {
	return tr("Interface");
}

QPixmap PrefInterface::sectionIcon() {
    return Images::icon("pref_gui");
}

void PrefInterface::createLanguageCombo() {
	QMap <QString,QString> m = Languages::translations();

	// Language combo
	QDir translation_dir = Paths::translationPath();
	QStringList languages = translation_dir.entryList( QStringList() << "*.qm");
	QRegExp rx_lang("smplayer_(.*)\\.qm");
	language_combo->clear();
	language_combo->addItem( tr("<Autodetect>") );
	for (int n=0; n < languages.count(); n++) {
		if (rx_lang.indexIn(languages[n]) > -1) {
			QString l = rx_lang.cap(1);
			QString text = l;
			if (m.contains(l)) text = m[l] + " ("+l+")";
			language_combo->addItem( text, l );
		}
	}
}

void PrefInterface::retranslateStrings() {
	int mainwindow_resize = mainwindow_resize_combo->currentIndex();
	int timeslider_pos = timeslider_behaviour_combo->currentIndex();

	retranslateUi(this);

	mainwindow_resize_combo->setCurrentIndex(mainwindow_resize);
	timeslider_behaviour_combo->setCurrentIndex(timeslider_pos);

	// Icons
	resize_window_icon->setPixmap( Images::icon("resize_window") );
	/* volume_icon->setPixmap( Images::icon("speaker") ); */

	changeInstanceImages();

	// Seek widgets
	seek1->setLabel( tr("&Short jump") );
	seek2->setLabel( tr("&Medium jump") );
	seek3->setLabel( tr("&Long jump") );
	seek4->setLabel( tr("Mouse &wheel jump") );

	if (qApp->isLeftToRight()) {
		seek1->setIcon( Images::icon("forward10s") );
		seek2->setIcon( Images::icon("forward1m") );
		seek3->setIcon( Images::icon("forward10m") );
	} else {
		seek1->setIcon( Images::flippedIcon("forward10s") );
		seek2->setIcon( Images::flippedIcon("forward1m") );
		seek3->setIcon( Images::flippedIcon("forward10m") );
	}
	seek4->setIcon( Images::icon("mouse", seek1->icon()->width()) );

	// Language combo
	int language_item = language_combo->currentIndex();
	createLanguageCombo();
	language_combo->setCurrentIndex( language_item );

	// Iconset combo
	iconset_combo->setItemText( 0, tr("Default") );

#if STYLE_SWITCHING
	style_combo->setItemText( 0, tr("Default") );
#endif

	int gui_index = gui_combo->currentIndex();
	gui_combo->clear();
	gui_combo->addItem( tr("Default GUI"), "DefaultGUI");
	gui_combo->addItem( tr("Mini GUI"), "MiniGUI");
	gui_combo->addItem( tr("Mpc GUI"), "MpcGUI");
	gui_combo->setCurrentIndex(gui_index);

	floating_width_label->setNum(floating_width_slider->value());
	floating_margin_label->setNum(floating_margin_slider->value());

	createHelp();
}

void PrefInterface::setData(Preferences * pref) {
	setLanguage( pref->language );
	setIconSet( pref->iconset );

	setResizeMethod( pref->resize_method );
	setSaveSize( pref->save_window_size_on_exit );
	setUseSingleInstance(pref->use_single_instance);
	setServerPort(pref->connection_port);
	setUseAutoPort(pref->use_autoport);
	setRecentsMaxItems(pref->history_recents->maxItems());

	setSeeking1(pref->seeking1);
	setSeeking2(pref->seeking2);
	setSeeking3(pref->seeking3);
	setSeeking4(pref->seeking4);

	setUpdateWhileDragging(pref->update_while_seeking);

	setDefaultFont(pref->default_font);

#if STYLE_SWITCHING
	setStyle( pref->style );
#endif

	setGUI(pref->gui);

	setFloatingAnimated(pref->floating_control_animated);
	setFloatingWidth(pref->floating_control_width);
	setFloatingMargin(pref->floating_control_margin);
	setDisplayFloatingInCompactMode(pref->floating_display_in_compact_mode);
#ifndef Q_OS_WIN
	setFloatingBypassWindowManager(pref->bypass_window_manager);
#endif
}

void PrefInterface::getData(Preferences * pref) {
	requires_restart = false;
	language_changed = false;
	iconset_changed = false;
	recents_changed = false;
	port_changed = false;
	style_changed = false;

	if (pref->language != language()) {
		pref->language = language();
		language_changed = true;
		qDebug("PrefInterface::getData: chosen language: '%s'", pref->language.toUtf8().data());
	}

	if (pref->iconset != iconSet()) {
		pref->iconset = iconSet();
		iconset_changed = true;
	}

	pref->resize_method = resizeMethod();
	pref->save_window_size_on_exit = saveSize();

	pref->use_single_instance = useSingleInstance();
	if (pref->connection_port != serverPort()) {
		pref->connection_port = serverPort();
		port_changed = true;
	}

	if (pref->use_autoport != useAutoPort()) {
		pref->use_autoport = useAutoPort();
		port_changed = true;
	}

	if (pref->history_recents->maxItems() != recentsMaxItems()) {
		pref->history_recents->setMaxItems( recentsMaxItems() );
		recents_changed = true;
	}

	pref->seeking1 = seeking1();
	pref->seeking2 = seeking2();
	pref->seeking3 = seeking3();
	pref->seeking4 = seeking4();

	pref->update_while_seeking = updateWhileDragging();

	pref->default_font = defaultFont();

#if STYLE_SWITCHING
    if ( pref->style != style() ) {
        pref->style = style();
		style_changed = true;
	}
#endif

	pref->gui = GUI();

	pref->floating_control_animated = floatingAnimated();
	pref->floating_control_width = floatingWidth();
	pref->floating_control_margin = floatingMargin();
	pref->floating_display_in_compact_mode = displayFloatingInCompactMode();
#ifndef Q_OS_WIN
	pref->bypass_window_manager = floatingBypassWindowManager();
#endif
}

void PrefInterface::setLanguage(QString lang) {
	if (lang.isEmpty()) {
		language_combo->setCurrentIndex(0);
	}
	else {
		int pos = language_combo->findData(lang);
		if (pos != -1) 
			language_combo->setCurrentIndex( pos );
		else
			language_combo->setCurrentText(lang);
	}
}

QString PrefInterface::language() {
	if (language_combo->currentIndex()==0) 
		return "";
	else 
		return language_combo->itemData( language_combo->currentIndex() ).toString();
}

void PrefInterface::setIconSet(QString set) {
	if (set.isEmpty())
		iconset_combo->setCurrentIndex(0);
	else
		iconset_combo->setCurrentText(set);
}

QString PrefInterface::iconSet() {
	if (iconset_combo->currentIndex()==0) 
		return "";
	else
		return iconset_combo->currentText();
}

void PrefInterface::setResizeMethod(int v) {
	mainwindow_resize_combo->setCurrentIndex(v);
}

int PrefInterface::resizeMethod() {
	return mainwindow_resize_combo->currentIndex();
}

void PrefInterface::setSaveSize(bool b) {
	save_size_check->setChecked(b);
}

bool PrefInterface::saveSize() {
	return save_size_check->isChecked();
}


void PrefInterface::setStyle(QString style) {
	if (style.isEmpty()) 
		style_combo->setCurrentIndex(0);
	else
		style_combo->setCurrentText(style);
}

QString PrefInterface::style() {
	if (style_combo->currentIndex()==0)
		return "";
	else 
		return style_combo->currentText();
}

void PrefInterface::setGUI(QString gui_name) {
	int i = gui_combo->findData(gui_name);
	if (i < 0) i=0;
	gui_combo->setCurrentIndex(i);
}

QString PrefInterface::GUI() {
	return gui_combo->itemData(gui_combo->currentIndex()).toString();
}

void PrefInterface::setUseSingleInstance(bool b) {
	single_instance_check->setChecked(b);
	//singleInstanceButtonToggled(b);
}

bool PrefInterface::useSingleInstance() {
	return single_instance_check->isChecked();
}

void PrefInterface::setServerPort(int port) {
	server_port_spin->setValue(port);
}

int PrefInterface::serverPort() {
	return server_port_spin->value();
}

void PrefInterface::setUseAutoPort(bool b) {
	automatic_port_button->setChecked(b);
	manual_port_button->setChecked(!b);
}

bool PrefInterface::useAutoPort() {
	return automatic_port_button->isChecked();
}

void PrefInterface::setRecentsMaxItems(int n) {
	recents_max_items_spin->setValue(n);
}

int PrefInterface::recentsMaxItems() {
	return recents_max_items_spin->value();
}

void PrefInterface::setSeeking1(int n) {
	seek1->setTime(n);
}

int PrefInterface::seeking1() {
	return seek1->time();
}

void PrefInterface::setSeeking2(int n) {
	seek2->setTime(n);
}

int PrefInterface::seeking2() {
	return seek2->time();
}

void PrefInterface::setSeeking3(int n) {
	seek3->setTime(n);
}

int PrefInterface::seeking3() {
	return seek3->time();
}

void PrefInterface::setSeeking4(int n) {
	seek4->setTime(n);
}

int PrefInterface::seeking4() {
	return seek4->time();
}

void PrefInterface::setUpdateWhileDragging(bool b) {
	if (b) 
		timeslider_behaviour_combo->setCurrentIndex(0);
	else
		timeslider_behaviour_combo->setCurrentIndex(1);
}

bool PrefInterface::updateWhileDragging() {
	return (timeslider_behaviour_combo->currentIndex() == 0);
}

void PrefInterface::setDefaultFont(QString font_desc) {
	default_font_edit->setText(font_desc);
}

QString PrefInterface::defaultFont() {
	return default_font_edit->text();
}

void PrefInterface::on_changeFontButton_clicked() {
	QFont f = qApp->font();

	if (!default_font_edit->text().isEmpty()) {
		f.fromString(default_font_edit->text());
	}

	bool ok;
	f = QFontDialog::getFont( &ok, f, this);

	if (ok) {
		default_font_edit->setText( f.toString() );
	}
}

void PrefInterface::changeInstanceImages() {
	if (single_instance_check->isChecked())
		instances_icon->setPixmap( Images::icon("instance1") );
	else
		instances_icon->setPixmap( Images::icon("instance2") );
}

// Floating tab
void PrefInterface::setFloatingAnimated(bool b) {
	floating_animated_check->setChecked(b);
}

bool PrefInterface::floatingAnimated() {
	return floating_animated_check->isChecked();
}

void PrefInterface::setFloatingWidth(int percentage) {
	floating_width_slider->setValue(percentage);
}

int PrefInterface::floatingWidth() {
	return floating_width_slider->value();
}

void PrefInterface::setFloatingMargin(int pixels) {
	floating_margin_slider->setValue(pixels);
}

int PrefInterface::floatingMargin() {
	return floating_margin_slider->value();
}

void PrefInterface::setDisplayFloatingInCompactMode(bool b) {
	floating_compact_check->setChecked(b);
}

bool PrefInterface::displayFloatingInCompactMode() {
	return floating_compact_check->isChecked();
}

#ifndef Q_OS_WIN
void PrefInterface::setFloatingBypassWindowManager(bool b) {
	floating_bypass_wm_check->setChecked(b);
}

bool PrefInterface::floatingBypassWindowManager() {
	return floating_bypass_wm_check->isChecked();
}
#endif

void PrefInterface::createHelp() {
	clearHelp();

	addSectionTitle(tr("Interface"));

	setWhatsThis(mainwindow_resize_combo, tr("Autoresize"),
        tr("The main window can be resized automatically. Select the option "
           "you prefer.") );

	setWhatsThis(save_size_check, tr("Remember position and size"),
        tr("If you check this option, the position and size of the main "
           "window will be saved and restored when you run SMPlayer again.") );

	setWhatsThis(recents_max_items_spin, tr("Recent files"),
        tr("Select the maximum number of items that will be shown in the "
           "<b>Open->Recent files</b> submenu. If you set it to 0 that "
           "menu won't be shown at all.") );

	setWhatsThis(language_combo, tr("Language"),
		tr("Here you can change the language of the application.") );

	setWhatsThis(iconset_combo, tr("Icon set"),
        tr("Select the icon set you prefer for the application.") );

	setWhatsThis(style_combo, tr("Style"),
        tr("Select the style you prefer for the application.") );

	setWhatsThis(gui_combo, tr("GUI"),
        tr("Select the GUI you prefer for the application. Currently "
           "there are two available: Default GUI and Mini GUI.<br>"
           "The <b>Default GUI</b> provides the traditional GUI, with the "
           "toolbar and control bar. The <b>Mini GUI</b> provides a "
           "more simple GUI, without toolbar and a control bar with few "
           "buttons.<br>"
           "<b>Note:</b> this option will take effect the next "
           "time you run SMPlayer.") );

	setWhatsThis(changeFontButton, tr("Default font"),
        tr("You can change here the application's font.") );

	addSectionTitle(tr("Seeking"));

	setWhatsThis(seek1, tr("Short jump"),
        tr("Select the time that should be go forward or backward when you "
           "choose the %1 action.").arg(tr("short jump")) );

	setWhatsThis(seek2, tr("Medium jump"),
        tr("Select the time that should be go forward or backward when you "
           "choose the %1 action.").arg(tr("medium jump")) );

	setWhatsThis(seek3, tr("Long jump"),
        tr("Select the time that should be go forward or backward when you "
           "choose the %1 action.").arg(tr("long jump")) );

	setWhatsThis(seek4, tr("Mouse wheel jump"),
        tr("Select the time that should be go forward or backward when you "
           "move the mouse wheel.") );

	setWhatsThis(timeslider_behaviour_combo, tr("Behaviour of time slider"),
        tr("Select what to do when dragging the time slider.") );

	addSectionTitle(tr("Instances"));

	setWhatsThis(single_instance_check, 
        tr("Use only one running instance of SMPlayer"),
        tr("Check this option if you want to use an already running instance "
           "of SMPlayer when opening other files.") );

	setWhatsThis(automatic_port_button, tr("Automatic port"),
        tr("SMPlayer needs to listen to a port to receive commands from other "
           "instances. If you select this option, a port will be "
           "automatically chosen.") );

	setWhatsThis(server_port_spin, tr("Manual port"),
        tr("SMPlayer needs to listen to a port to receive commands from other "
           "instances. You can change the port in case the default one is "
           "used by another application.") );

	manual_port_button->setWhatsThis( server_port_spin->whatsThis() );

	addSectionTitle(tr("Floating control"));

	setWhatsThis(floating_animated_check, tr("Animated"),
		tr("If this option is enabled, the floating control will appear "
           "with an animation.") );

	setWhatsThis(floating_width_slider, tr("Width"),
		tr("Specifies the width of the control (as a percentage).") );

	setWhatsThis(floating_margin_slider, tr("Margin"),
		tr("This option sets the number of pixels that the floating control "
           "will be away from the bottom of the screen. Useful when the "
           "screen is a TV, as the overscan might prevent the control to be "
           "visible.") );

	setWhatsThis(floating_compact_check, tr("Display in compact mode too"),
		tr("If this option is enabled, the floating control will appear "
           "in compact mode too. <b>Warning:</b> the floating control has not been "
           "designed for compact mode and it might not work properly.") );

#ifndef Q_OS_WIN
	setWhatsThis(floating_bypass_wm_check, tr("Bypass window manager"),
		tr("If this option is checked, the control is displayed bypassing the "
           "window manager. Disable this option if the floating control "
           "doesn't work well with your window manager.") );
#endif
}

#include "moc_prefinterface.cpp"
