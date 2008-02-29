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

#include "inputmplayerversion.h"
#include "mplayerversion.h"
#include <QLineEdit>
#include <QComboBox>

InputMplayerVersion::InputMplayerVersion( QWidget* parent, Qt::WindowFlags f )
	: QDialog(parent, f)
{
	setupUi(this);
}

InputMplayerVersion::~InputMplayerVersion() {
}

void InputMplayerVersion::setVersionFromOutput(QString text) {
	orig_string->setText(text);
}

void InputMplayerVersion::setVersion(int current_version) {
	int index = 0;

	if (current_version == MPLAYER_1_0_RC2_SVN) index = 1;
	else
	if (current_version > MPLAYER_1_0_RC2_SVN) index = 2;

	version_combo->setCurrentIndex(index);
}

int InputMplayerVersion::version() {
	int r = -1;
	switch (version_combo->currentIndex()) {
		case 0 : r = MPLAYER_1_0_RC1_SVN; break; // rc1 or older
		case 1 : r = MPLAYER_1_0_RC2_SVN; break; // rc2
		case 2 : r = 25844; break; // last svn at the moment of writing this
	}
	return r;
}

#include "moc_inputmplayerversion.cpp"
