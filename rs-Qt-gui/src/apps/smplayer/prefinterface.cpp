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


#include "prefinterface.h"
#include "images.h"
#include "preferences.h"
#include "helper.h"
#include "config.h"

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
	QDir icon_dir = Helper::appHomePath() + "/themes";
	qDebug("icon_dir: %s", icon_dir.absolutePath().toUtf8().data());
	QStringList iconsets = icon_dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
	for (int n=0; n < iconsets.count(); n++) {
		iconset_combo->addItem( iconsets[n] );
	}
	// Global
	icon_dir = Helper::themesPath();
	qDebug("icon_dir: %s", icon_dir.absolutePath().toUtf8().data());
	iconsets = icon_dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
	for (int n=0; n < iconsets.count(); n++) {
		if (iconset_combo->findText( iconsets[n] ) == -1) {
			iconset_combo->addItem( iconsets[n] );
		}
	}

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
	QMap <QString,QString> m;
	m["bg"] = tr("Bulgarian");
	m["ca"] = tr("Catalan");
	m["cs"] = tr("Czech");
	m["de"] = tr("German");
	m["el"] = tr("Greek");
	m["en_US"] = tr("English");
	m["es"] = tr("Spanish");
	m["eu"] = tr("Basque");
	m["fi"] = tr("Finnish");
	m["fr"] = tr("French");
	m["gl"] = tr("Galician");
	m["hu"] = tr("Hungarian");
	m["it"] = tr("Italian");
	m["ja"] = tr("Japanese");
	m["ka"] = tr("Georgian");
	m["nl"] = tr("Dutch");
	m["pl"] = tr("Polish");
	m["pt_BR"] = tr("Portuguese - Brazil");
	m["pt_PT"] = tr("Portuguese - Portugal");
	m["ro_RO"] = tr("Romanian");
	m["ru_RU"] = tr("Russian");
	m["sk"] = tr("Slovak");
	m["sr"] = tr("Serbian");
	m["sv"] = tr("Swedish");
	m["tr"] = tr("Turkish");
	m["uk_UA"] = tr("Ukrainian");
	m["zh_CN"] = tr("Simplified-Chinese");
	m["zh_TW"] = tr("Traditional Chinese");

	// Language combo
	QDir translation_dir = Helper::translationPath();
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

	retranslateUi(this);

	mainwindow_resize_combo->setCurrentIndex(mainwindow_resize);

	// Icons
	/* resize_window_icon->setPixmap( Images::icon("resize_window") ); */
	/* volume_icon->setPixmap( Images::icon("speaker") ); */

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
	seek4->setIcon( Images::icon("mouse_small") );

	// Language combo
	int language_item = language_combo->currentIndex();
	createLanguageCombo();
	language_combo->setCurrentIndex( language_item );

	// Iconset combo
	iconset_combo->setItemText( 0, tr("Default") );

#if STYLE_SWITCHING
	style_combo->setItemText( 0, tr("Default") );
#endif

	createHelp();
}

void PrefInterface::setData(Preferences * pref) {
	setLanguage( pref->language );
	setIconSet( pref->iconset );

	setResizeMethod( pref->resize_method );
	setSaveSize( pref->save_window_size_on_exit );
	setUseSingleInstance(pref->use_single_instance);
	setServerPort(pref->connection_port);
	setRecentsMaxItems(pref->recents_max_items);

	setSeeking1(pref->seeking1);
	setSeeking2(pref->seeking2);
	setSeeking3(pref->seeking3);
	setSeeking4(pref->seeking4);

	setDefaultFont(pref->default_font);

#if STYLE_SWITCHING
	setStyle( pref->style );
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

	if (pref->recents_max_items != recentsMaxItems()) {
		pref->recents_max_items = recentsMaxItems();
		recents_changed = true;
	}

	pref->seeking1 = seeking1();
	pref->seeking2 = seeking2();
	pref->seeking3 = seeking3();
	pref->seeking4 = seeking4();

	pref->default_font = defaultFont();

#if STYLE_SWITCHING
    if ( pref->style != style() ) {
        pref->style = style();
		style_changed = true;
	}
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

void PrefInterface::createHelp() {
	clearHelp();

	setWhatsThis(language_combo, tr("Language"),
		tr("Here you can change the language of the application.") );
}

#include "moc_prefinterface.cpp"
