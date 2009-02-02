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

#include "videoequalizer.h"
#include "eqslider.h"
#include "images.h"
#include "preferences.h"
#include "global.h"
#include <QLayout>
#include <QPushButton>
#include <QMessageBox>

using namespace Global;

VideoEqualizer::VideoEqualizer( QWidget* parent, Qt::WindowFlags f)
	: QWidget(parent, f)
{
	contrast = new EqSlider(this);
	brightness = new EqSlider(this);
	hue = new EqSlider(this);
	saturation = new EqSlider(this);
	gamma = new EqSlider(this);

	QBoxLayout *bl = new QHBoxLayout; //(0, 4, 2);
	bl->addWidget(contrast);
	bl->addWidget(brightness);
	bl->addWidget(hue);
	bl->addWidget(saturation);
	bl->addWidget(gamma);

	reset_button = new QPushButton( "&Reset", this);
	connect( reset_button, SIGNAL(clicked()), this, SLOT(reset()) );
	set_default_button = new QPushButton( "&Set as default values", this );
	connect( set_default_button, SIGNAL(clicked()), this, SLOT(setDefaults()) );

	QBoxLayout *button_layout = new QVBoxLayout; //(0, 4, 2);
	button_layout->addWidget(set_default_button);
	button_layout->addWidget(reset_button);

	QBoxLayout *layout = new QVBoxLayout(this); //, 4, 2);
	layout->addLayout(bl);
	layout->addLayout(button_layout);

	retranslateStrings();

	adjustSize();
	//setFixedSize( sizeHint() );
}

VideoEqualizer::~VideoEqualizer() {
}

void VideoEqualizer::retranslateStrings() {
	setWindowTitle( tr("Video Equalizer") );
	setWindowIcon( Images::icon("logo") );

	contrast->setLabel( tr("Contrast") );
	contrast->setToolTip( tr("Contrast") );
	contrast->setIcon( Images::icon("contrast") );

	brightness->setLabel( tr("Brightness") );
	brightness->setToolTip( tr("Brightness") );
	brightness->setIcon( Images::icon("brightness") );

	hue->setLabel( tr("Hue") );
	hue->setToolTip( tr("Hue") );
	hue->setIcon( Images::icon("hue") );

	saturation->setLabel( tr("Saturation") );
	saturation->setToolTip( tr("Saturation") );
	saturation->setIcon( Images::icon("saturation") );

	gamma->setLabel( tr("Gamma") );
	gamma->setToolTip( tr("Gamma") );
	gamma->setIcon( Images::icon("gamma") );

	reset_button->setText( tr("&Reset") );
	set_default_button->setText( tr("&Set as default values") );

	// What's this help:
	set_default_button->setWhatsThis(
			tr("Use the current values as default values for new videos.") );

	reset_button->setWhatsThis( tr("Set all controls to zero.") );

}

void VideoEqualizer::reset() {
	contrast->setValue(0);
	brightness->setValue(0);
	hue->setValue(0);
	saturation->setValue(0);
	gamma->setValue(0);
}

void VideoEqualizer::setDefaults() {
	pref->initial_contrast = contrast->value();
	pref->initial_brightness = brightness->value();
	pref->initial_hue = hue->value();
	pref->initial_saturation = saturation->value();
	pref->initial_gamma = gamma->value();

	QMessageBox::information(this, tr("Information"), 
                             tr("The current values have been stored to be "
                                "used as default.") );
}

void VideoEqualizer::hideEvent( QHideEvent * ) {
	emit visibilityChanged();
}

void VideoEqualizer::showEvent( QShowEvent * ) {
	emit visibilityChanged();
}

// Language change stuff
void VideoEqualizer::changeEvent(QEvent *e) {
	if (e->type() == QEvent::LanguageChange) {
		retranslateStrings();
	} else {
		QWidget::changeEvent(e);
	}
}

#include "moc_videoequalizer.cpp"
