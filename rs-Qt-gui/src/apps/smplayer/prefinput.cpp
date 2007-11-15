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


#include "prefinput.h"
#include "images.h"
#include "preferences.h"

#include "config.h"

PrefInput::PrefInput(QWidget * parent, Qt::WindowFlags f)
	: PrefWidget(parent, f )
{
	setupUi(this);

	// Mouse function combos
    left_click_combo->addItem( "None" );
	double_click_combo->addItem( "None" );

	retranslateStrings();
}

PrefInput::~PrefInput()
{
}

QString PrefInput::sectionName() {
	return tr("Keyboard and mouse");
}

QPixmap PrefInput::sectionIcon() {
    return Images::icon("input_devices");
}


void PrefInput::retranslateStrings() {
	int wheel_function = wheel_function_combo->currentIndex();

	retranslateUi(this);

	wheel_function_combo->setCurrentIndex(wheel_function);

	keyboard_icon->setPixmap( Images::icon("keyboard") );
	mouse_icon->setPixmap( Images::icon("mouse") );

    // Mouse function combos
	left_click_combo->setItemText( 0, tr("None") );
	double_click_combo->setItemText( 0, tr("None") );

#if !USE_SHORTCUTGETTER
	actioneditor_desc->setText( 
		tr("Here you can change any key shortcut. To do it double click or "
           "start typing over a shortcut cell. Optionally you can also save "
           "the list to share it with other people or load it in another "
           "computer.") );
#endif

	createHelp();
}

void PrefInput::setData(Preferences * pref) {
	setLeftClickFunction( pref->mouse_left_click_function );
	setDoubleClickFunction( pref->mouse_double_click_function );
	setWheelFunction( pref->wheel_function );
}

void PrefInput::getData(Preferences * pref) {
	requires_restart = false;

	pref->mouse_left_click_function = leftClickFunction();
	pref->mouse_double_click_function = doubleClickFunction();
	pref->wheel_function = wheelFunction();
}

void PrefInput::setActionsList(QStringList l) {
	left_click_combo->insertStringList( l );
	double_click_combo->insertStringList( l );
}

void PrefInput::setLeftClickFunction(QString f) {
	if (f.isEmpty()) {
		left_click_combo->setCurrentIndex(0);
	} else {
		left_click_combo->setCurrentText(f);
	}
}

QString PrefInput::leftClickFunction() {
	if (left_click_combo->currentIndex()==0) {
		return "";
	} else {
		return left_click_combo->currentText();
	}
}

void PrefInput::setDoubleClickFunction(QString f) {
	if (f.isEmpty()) {
		double_click_combo->setCurrentIndex(0);
	} else {
		double_click_combo->setCurrentText(f);
	}
}

QString PrefInput::doubleClickFunction() {
	if (double_click_combo->currentIndex()==0) {
		return "";
	} else {
		return double_click_combo->currentText();
	}
}

void PrefInput::setWheelFunction(int function) {
	wheel_function_combo->setCurrentIndex(function);
}

int PrefInput::wheelFunction() {
	return wheel_function_combo->currentIndex();
}

void PrefInput::createHelp() {
	clearHelp();

	setWhatsThis(actions_editor, tr("Shortcut editor"),
        tr("This table allows you to change the key shortcuts of most "
           "available actions. Double click or press enter on a item, or "
           "press the <b>Change shortcut</b> button to enter in the "
           "<i>Modify shortcut</i> dialog. There are two ways to change a "
           "shortcut: if the <b>Capture</b> button is on then just "
           "press the new key or combination of keys that you want to "
           "assign for the action (unfortunately this doesn't work for all "
           "keys). If the <b>Capture</b> button is off "
           "then you could enter the full name of the key.") );

	setWhatsThis(left_click_combo, tr("Left click"),
		tr("Select the action for left click on the mouse.") );

	setWhatsThis(double_click_combo, tr("Double click"),
		tr("Select the action for double click on the mouse.") );

	setWhatsThis(wheel_function_combo, tr("Wheel function"),
		tr("Select the action for the mouse wheel.") );
}

#include "moc_prefinput.cpp"
