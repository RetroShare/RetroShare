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

#include "tristatecombo.h"
#include <QEvent>

TristateCombo::TristateCombo( QWidget * parent ) : QComboBox(parent) 
{
	retranslateStrings();
}

TristateCombo::~TristateCombo() {
}

void TristateCombo::retranslateStrings() {
	int i = currentIndex();

	clear();
	addItem( tr("Auto"), Preferences::Detect );
	addItem( tr("Yes"), Preferences::Enabled );
	addItem( tr("No"), Preferences::Disabled );

	setCurrentIndex(i);
}

void TristateCombo::setState( Preferences::OptionState v ) {
	setCurrentIndex( findData(v) );
}

Preferences::OptionState TristateCombo::state() {
	return (Preferences::OptionState) itemData( currentIndex() ).toInt();
}

// Language change stuff
void TristateCombo::changeEvent(QEvent *e) {
	if (e->type() == QEvent::LanguageChange) {
		retranslateStrings();
	} else {
		QWidget::changeEvent(e);
	}
}

#include "moc_tristatecombo.cpp"
