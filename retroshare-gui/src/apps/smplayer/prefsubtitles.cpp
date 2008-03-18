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


#include "prefsubtitles.h"
#include "images.h"
#include "preferences.h"
#include "encodings.h"
#include "helper.h"
#include "filedialog.h"

#include <QColorDialog>

PrefSubtitles::PrefSubtitles(QWidget * parent, Qt::WindowFlags f)
	: PrefWidget(parent, f )
{
	setupUi(this);

	encodings = new Encodings(this);
	font_encoding_combo->insertItems( 0, encodings->list() );

	//languageChange();
	createHelp();
}

PrefSubtitles::~PrefSubtitles()
{
}

QString PrefSubtitles::sectionName() {
	return tr("Subtitles");
}

QPixmap PrefSubtitles::sectionIcon() {
    return Images::icon("pref_subtitles");
}


void PrefSubtitles::retranslateStrings() {
	int font_autoscale_item = font_autoscale_combo->currentIndex();
	int font_autoload_item = font_autoload_combo->currentIndex();

	retranslateUi(this);

	font_autoscale_combo->setCurrentIndex(font_autoscale_item);
	font_autoload_combo->setCurrentIndex(font_autoload_item);

	// Encodings combo
	int font_encoding_item = font_encoding_combo->currentIndex();
	font_encoding_combo->clear();
	encodings->retranslate();
	font_encoding_combo->insertItems( 0, encodings->list() );
	font_encoding_combo->setCurrentIndex(font_encoding_item);

	sub_pos_label->setNum( sub_pos_slider->value() );

	createHelp();
}

void PrefSubtitles::setData(Preferences * pref) {
	setFontName( pref->font_name );
	setFontFile( pref->font_file );
	setUseFontconfig( pref->use_fontconfig );
	setFontAutoscale( pref->font_autoscale );
	setFontTextscale( pref->initial_sub_scale );
	setAssFontScale( pref->initial_sub_scale_ass );
	setAutoloadSub( pref->autoload_sub );
	setFontFuzziness( pref->subfuzziness );
	setFontEncoding( pref->subcp );
	setUseFontASS( pref->use_ass_subtitles );
	setAssColor( pref->ass_color );
	setAssBorderColor( pref->ass_border_color );
	setAssStyles( pref->ass_styles );
	setSubPos( pref->initial_sub_pos );
	setSubtitlesOnScreenshots( pref->subtitles_on_screenshots );
}

void PrefSubtitles::getData(Preferences * pref) {
	requires_restart = false;

	TEST_AND_SET(pref->font_name, fontName());
	TEST_AND_SET(pref->font_file, fontFile());
	TEST_AND_SET(pref->use_fontconfig, useFontconfig());
	TEST_AND_SET(pref->font_autoscale, fontAutoscale());
	pref->initial_sub_scale = fontTextscale();
	pref->initial_sub_scale_ass = assFontScale();
	TEST_AND_SET(pref->autoload_sub, autoloadSub());
	TEST_AND_SET(pref->subfuzziness, fontFuzziness());
	TEST_AND_SET(pref->subcp, fontEncoding());
	TEST_AND_SET(pref->use_ass_subtitles, useFontASS());
	TEST_AND_SET(pref->ass_color, assColor());
	TEST_AND_SET(pref->ass_border_color, assBorderColor());
	TEST_AND_SET(pref->ass_styles, assStyles());
	pref->initial_sub_pos = subPos();
	TEST_AND_SET(pref->subtitles_on_screenshots, subtitlesOnScreenshots());
}

void PrefSubtitles::setFontName(QString font_name) {
	fontCombo->setCurrentText(font_name);
}

QString PrefSubtitles::fontName() {
	return fontCombo->currentText();
}

void PrefSubtitles::setFontFile(QString font_file) {
	ttf_font_edit->setText( font_file );
}

QString PrefSubtitles::fontFile() {
	return ttf_font_edit->text();
}


void PrefSubtitles::setUseFontconfig(bool b) {
	system_font_button->setChecked(b);
	ttf_font_button->setChecked(!b);
}

bool PrefSubtitles::useFontconfig() {
	return system_font_button->isChecked();
}

void PrefSubtitles::setFontAutoscale(int n) {
	font_autoscale_combo->setCurrentIndex(n);
}

int PrefSubtitles::fontAutoscale() {
	return font_autoscale_combo->currentIndex();
}

void PrefSubtitles::setFontTextscale(double n) {
	font_text_scale->setValue(n);
}

double PrefSubtitles::fontTextscale() {
	return font_text_scale->value();
}

void PrefSubtitles::setAssFontScale(double n) {
	ass_font_scale->setValue(n);
}

double PrefSubtitles::assFontScale() {
	return ass_font_scale->value();
}

void PrefSubtitles::setAutoloadSub(bool v) {
	font_autoload_check->setChecked(v);
}

bool PrefSubtitles::autoloadSub() {
	return font_autoload_check->isChecked();
}

void PrefSubtitles::setFontEncoding(QString s) {
	int n = encodings->findEncoding( s );
	if (n != -1) 
		font_encoding_combo->setCurrentIndex(n);
	else
		font_encoding_combo->setCurrentText(s);
}

QString PrefSubtitles::fontEncoding() {
	qDebug("PrefSubtitles::fontEncoding");
	QString res = encodings->parseEncoding( font_encoding_combo->currentText() );
	qDebug(" * res: '%s'", res.toUtf8().data() );
	return res;
}

void PrefSubtitles::setSubPos(int pos) {
	sub_pos_slider->setValue(pos);
}

int PrefSubtitles::subPos() {
	return sub_pos_slider->value();
}

void PrefSubtitles::setUseFontASS(bool v) {
	font_ass_check->setChecked(v);
	//assButtonToggled(v);
}

bool PrefSubtitles::useFontASS() {
	return font_ass_check->isChecked();
}

void PrefSubtitles::setAssColor( unsigned int color ) {
	ass_color = color;
#ifdef Q_OS_WIN
	colorButton->setStyleSheet( "border-width: 1px; border-style: solid; border-color: #000000; background: #" + Helper::colorToRRGGBB(ass_color) + ";");
#else
	//colorButton->setAutoFillBackground(true);
	Helper::setBackgroundColor( colorButton, color );
#endif
}

unsigned int PrefSubtitles::assColor() {
	return ass_color;
}

void PrefSubtitles::setAssBorderColor( unsigned int color ) {
	ass_border_color = color;

#ifdef Q_OS_WIN
	borderButton->setStyleSheet( "border-width: 1px; border-style: solid; border-color: #000000; background: #" + Helper::colorToRRGGBB(ass_border_color) + ";");
#else
	//borderButton->setAutoFillBackground(true);
	Helper::setBackgroundColor( borderButton, color );
#endif
}

unsigned int PrefSubtitles::assBorderColor() {
	return ass_border_color;
}

void PrefSubtitles::setAssStyles(QString styles) {
	ass_styles_edit->setText(styles);
}

QString PrefSubtitles::assStyles() {
	return ass_styles_edit->text();
}

void PrefSubtitles::setFontFuzziness(int n) {
	font_autoload_combo->setCurrentIndex(n);
}

int PrefSubtitles::fontFuzziness() {
	return font_autoload_combo->currentIndex();
}

void PrefSubtitles::setSubtitlesOnScreenshots(bool b) {
	subtitles_on_screeshots_check->setChecked(b);
}

bool PrefSubtitles::subtitlesOnScreenshots() {
	return subtitles_on_screeshots_check->isChecked();
}


void PrefSubtitles::on_searchButton_clicked() {
	QString s = "";

	QString f;

	s = MyFileDialog::getOpenFileName(
                        this, tr("Choose a ttf file"), 
	                    ttf_font_edit->text(),
    	                tr("Truetype Fonts") + " (*.ttf)"
#ifdef Q_OS_WIN
                        , &f, QFileDialog::DontUseNativeDialog
#endif
                        );

	if (!s.isEmpty()) {
		ttf_font_edit->setText(s);
	}
}

void PrefSubtitles::on_colorButton_clicked() {
	QColor c = QColorDialog::getColor ( ass_color, this );
	if (c.isValid()) {
		setAssColor( c.rgb() );
	}
}

void PrefSubtitles::on_borderButton_clicked() {
	QColor c = QColorDialog::getColor ( ass_border_color, this );
	if (c.isValid()) {
		setAssBorderColor( c.rgb() );
	}
}

void PrefSubtitles::createHelp() {
	clearHelp();

	addSectionTitle(tr("Subtitles"));

	setWhatsThis(font_autoload_combo, tr("Autoload"), 
        tr("Select the subtitle autoload method.") );

	setWhatsThis(font_autoload_check, tr("Select first available subtitle"), 
        tr("If there are one or more subtitle tracks available, one of them "
           "will be automatically selected, usually the first one, although if "
           "one of them matches the user's preferred language that one will "
           "be used instead.") );

	setWhatsThis(font_encoding_combo, tr("Default subtitle encoding"), 
        tr("Select the encoding which will be used for subtitle files.") );

	setWhatsThis(sub_pos_slider, tr("Subtitle position"),
		tr("This option specifies the position of the subtitles over the "
           "video window. <i>100</i> means the bottom, while <i>0</i> means "
           "the top." ) );

	setWhatsThis(subtitles_on_screeshots_check, 
        tr("Include subtitles on screenshots"), 
        tr("If this option is checked, the subtitles will appear in the "
           "screenshots. Note: it may cause some troubles sometimes." ) );

	addSectionTitle(tr("Font"));

	setWhatsThis(ttf_font_edit, tr("TTF font"), 
        tr("Here you can select a ttf font to be used for the subtitles. "
           "Usually you'll find a lot of ttf fonts in <i>%1</i>.")
#ifdef Q_OS_WIN
        .arg("C:\\Windows\\Fonts\\")
#else
        .arg("/usr/X11R6/lib/X11/fonts/truetype/")
#endif
        );

	setWhatsThis(fontCombo, tr("System font"), 
        tr("Here you can select a system font to be used for the subtitles "
           "and OSD. <b>Note:</b> requires a MPlayer with fontconfig support.") );

	setWhatsThis(font_autoscale_combo, tr("Autoscale"), 
        tr("Select the subtitle autoscaling method.") );

	setWhatsThis(font_text_scale, tr("Default scale for normal subtitles"),
		tr("This option specifies the default font scale for normal (white) "
           "subtitles which will be used for new videos."));

	setWhatsThis(ass_font_scale, tr("Default scale for SSA/ASS subtitles"),
		tr("This option specifies the default font scale for SSA/ASS "
           "subtitles which will be used for new videos."));

	addSectionTitle(tr("SSA/ASS library"));

	setWhatsThis(font_ass_check, tr("Use SSA/ASS library for subtitle rendering"), 
        tr("Check this option to activate the SSA/ASS library. It provides "
           "nicer subtitles, so it's a good idea to do it.") );

	setWhatsThis(colorButton, tr("Text color"), 
        tr("Select the color for the text of the subtitles.") );

	setWhatsThis(borderButton, tr("Border color"), 
        tr("Select the color for the border of the subtitles.") );

	setWhatsThis(ass_styles_edit, tr("SSA/ASS styles"), 
		tr("Here you can override styles for SSA/ASS subtitles. "
           "It can be also used for fine-tuning the rendering of SRT and SUB "
           "subtitles by the SSA/ASS library. "
           "Example: <b>Bold=1,Outline=2,Shadow=4</b>"));
}

#include "moc_prefsubtitles.cpp"
