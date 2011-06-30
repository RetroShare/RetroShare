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

#include "audioequalizer.h"
#include "eqslider.h"
#include "images.h"
#include "preferences.h"
#include "global.h"
#include <QLayout>
#include <QPushButton>
#include <QMessageBox>

using namespace Global;

AudioEqualizer::AudioEqualizer( QWidget* parent, Qt::WindowFlags f)
	: QWidget(parent, f)
{
	QBoxLayout *bl = new QHBoxLayout; //(0, 4, 2);

	for (int n = 0; n < 10; n++) {
		eq[n] = new EqSlider(this);
		eq[n]->setIcon( QPixmap() );
		eq[n]->sliderWidget()->setRange(-120, 120);
		bl->addWidget(eq[n]);
	}

	reset_button = new QPushButton( "&Reset", this);
	connect( reset_button, SIGNAL(clicked()), this, SLOT(reset()) );

	set_default_button = new QPushButton( "&Set as default values", this );
	connect( set_default_button, SIGNAL(clicked()), this, SLOT(setDefaults()) );

	apply_button = new QPushButton( "&Apply", this );
	connect( apply_button, SIGNAL(clicked()), this, SLOT(applyButtonClicked()) );

	QBoxLayout *button_layout = new QHBoxLayout; //(0, 4, 2);
	button_layout->addStretch();
	button_layout->addWidget(apply_button);
	button_layout->addWidget(reset_button);
	button_layout->addWidget(set_default_button);

	QBoxLayout *layout = new QVBoxLayout(this); //, 4, 2);
	layout->addLayout(bl);
	layout->addLayout(button_layout);

	retranslateStrings();

	adjustSize();
	//setFixedSize( sizeHint() );
}

AudioEqualizer::~AudioEqualizer() {
}

void AudioEqualizer::retranslateStrings() {
	setWindowTitle( tr("Audio Equalizer") );
	setWindowIcon( Images::icon("logo") );

	eq[0]->setLabel( tr("31.25 Hz") );
	eq[1]->setLabel( tr("62.50 Hz") );
	eq[2]->setLabel( tr("125.0 Hz") );
	eq[3]->setLabel( tr("250.0 Hz") );
	eq[4]->setLabel( tr("500.0 Hz") );
	eq[5]->setLabel( tr("1.000 kHz") );
	eq[6]->setLabel( tr("2.000 kHz") );
	eq[7]->setLabel( tr("4.000 kHz") );
	eq[8]->setLabel( tr("8.000 kHz") );
	eq[9]->setLabel( tr("16.00 kHz") );

	apply_button->setText( tr("&Apply") );
	reset_button->setText( tr("&Reset") );
	set_default_button->setText( tr("&Set as default values") );

	// What's this help:
	set_default_button->setWhatsThis(
			tr("Use the current values as default values for new videos.") );

	reset_button->setWhatsThis( tr("Set all controls to zero.") );

}

void AudioEqualizer::reset() {
	for (int n = 0; n < 10; n++) {
		eq[n]->setValue(0);
	}
}

void AudioEqualizer::setDefaults() {
	AudioEqualizerList l;
	for (int n = 0; n < 10; n++) {
		l << eq[n]->value();
	}
	pref->initial_audio_equalizer = l;

	QMessageBox::information(this, tr("Information"), 
                             tr("The current values have been stored to be "
                                "used as default.") );
}

void AudioEqualizer::applyButtonClicked() {
	AudioEqualizerList l;
	for (int n = 0; n < 10; n++) {
		l << eq[n]->value();
	}
	emit applyClicked( l );
}

void AudioEqualizer::hideEvent( QHideEvent * ) {
	emit visibilityChanged();
}

void AudioEqualizer::showEvent( QShowEvent * ) {
	emit visibilityChanged();
}

// Language change stuff
void AudioEqualizer::changeEvent(QEvent *e) {
	if (e->type() == QEvent::LanguageChange) {
		retranslateStrings();
	} else {
		QWidget::changeEvent(e);
	}
}

#include "moc_audioequalizer.cpp"
