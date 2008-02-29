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


#include "prefinput.h"
#include "images.h"
#include "preferences.h"

#include "config.h"

PrefInput::PrefInput(QWidget * parent, Qt::WindowFlags f)
	: PrefWidget(parent, f )
{
	setupUi(this);

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

void PrefInput::createMouseCombos() {
	left_click_combo->clear();
	double_click_combo->clear();
	middle_click_combo->clear();

	left_click_combo->addItem( tr("None"), "" );
	left_click_combo->addItem( tr("Play"), "play" );
	left_click_combo->addItem( tr("Play / Pause"), "play_or_pause" );
	left_click_combo->addItem( tr("Pause"), "pause" );
	left_click_combo->addItem( tr("Pause / Frame step"), "pause_and_frame_step" );
	left_click_combo->addItem( tr("Stop"), "stop" );
	left_click_combo->addItem( tr("Fullscreen"), "fullscreen" );
	left_click_combo->addItem( tr("Compact"), "compact" );
	left_click_combo->addItem( tr("Screenshot"), "screenshot" );
	left_click_combo->addItem( tr("On top"), "on_top" );
	left_click_combo->addItem( tr("Mute"), "mute" );
	left_click_combo->addItem( tr("Playlist"), "show_playlist" );
	left_click_combo->addItem( tr("Reset zoom"), "reset_zoom" );
	left_click_combo->addItem( tr("Exit fullscreen"), "exit_fullscreen" );
	left_click_combo->addItem( tr("Normal speed"), "normal_speed" );
	left_click_combo->addItem( tr("Frame counter"), "frame_counter" );
	left_click_combo->addItem( tr("Preferences"), "show_preferences" );
	left_click_combo->addItem( tr("Double size"), "toggle_double_size" );
	left_click_combo->addItem( tr("Show equalizer"), "equalizer" );

	// Copy to other combos
	for (int n=0; n < left_click_combo->count(); n++) {
		double_click_combo->addItem( left_click_combo->itemText(n),
                                     left_click_combo->itemData(n) );
		middle_click_combo->addItem( left_click_combo->itemText(n),
                                     left_click_combo->itemData(n) );
	}
}

void PrefInput::retranslateStrings() {
	int wheel_function = wheel_function_combo->currentIndex();

	retranslateUi(this);

	keyboard_icon->setPixmap( Images::icon("keyboard") );
	mouse_icon->setPixmap( Images::icon("mouse") );

    // Mouse function combos
	int mouse_left = left_click_combo->currentIndex();
	int mouse_double = double_click_combo->currentIndex();
	int mouse_middle = middle_click_combo->currentIndex();
	createMouseCombos();
	left_click_combo->setCurrentIndex(mouse_left);
	double_click_combo->setCurrentIndex(mouse_double);
	middle_click_combo->setCurrentIndex(mouse_middle);

	wheel_function_combo->clear();
	wheel_function_combo->addItem( tr("No function"), Preferences::DoNothing );
	wheel_function_combo->addItem( tr("Media seeking"), Preferences::Seeking );
	wheel_function_combo->addItem( tr("Volume control"), Preferences::Volume );
	wheel_function_combo->addItem( tr("Zoom video"), Preferences::Zoom );
	wheel_function_combo->addItem( tr("Change speed"), Preferences::ChangeSpeed );
	wheel_function_combo->setCurrentIndex(wheel_function);

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
	setMiddleClickFunction( pref->mouse_middle_click_function );
	setWheelFunction( pref->wheel_function );
}

void PrefInput::getData(Preferences * pref) {
	requires_restart = false;

	pref->mouse_left_click_function = leftClickFunction();
	pref->mouse_double_click_function = doubleClickFunction();
	pref->mouse_middle_click_function = middleClickFunction();
	pref->wheel_function = wheelFunction();
}

/*
void PrefInput::setActionsList(QStringList l) {
	left_click_combo->insertStringList( l );
	double_click_combo->insertStringList( l );
}
*/

void PrefInput::setLeftClickFunction(QString f) {
	int pos = left_click_combo->findData(f);
	if (pos == -1) pos = 0; //None
	left_click_combo->setCurrentIndex(pos);
}

QString PrefInput::leftClickFunction() {
	return left_click_combo->itemData( left_click_combo->currentIndex() ).toString();
}

void PrefInput::setDoubleClickFunction(QString f) {
	int pos = double_click_combo->findData(f);
	if (pos == -1) pos = 0; //None
	double_click_combo->setCurrentIndex(pos);
}

QString PrefInput::doubleClickFunction() {
	return double_click_combo->itemData( double_click_combo->currentIndex() ).toString();
}

void PrefInput::setMiddleClickFunction(QString f) {
	int pos = middle_click_combo->findData(f);
	if (pos == -1) pos = 0; //None
	middle_click_combo->setCurrentIndex(pos);
}

QString PrefInput::middleClickFunction() {
	return middle_click_combo->itemData( middle_click_combo->currentIndex() ).toString();
}

void PrefInput::setWheelFunction(int function) {
	int d = wheel_function_combo->findData(function);
	if (d < 0) d = 0;
	wheel_function_combo->setCurrentIndex( d );
}

int PrefInput::wheelFunction() {
	return wheel_function_combo->itemData(wheel_function_combo->currentIndex()).toInt();
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
