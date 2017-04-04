/*
 * RetroShare Android QML App
 * Copyright (C) 2016  Gioacchino Mazzurco <gio@eigenlab.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

import QtQuick 2.0
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.0
//import QtQml 2.2

Item
{
	id: loginView
	property string buttonText: "Unlock"
	property string login
	property string password
	signal submit(string login, string password)

	ColumnLayout
	{
		id: inputView
		width: parent.width
		anchors.centerIn: parent

		Image
		{
			source: "qrc:/qml/icons/emblem-locked.png"
			Layout.alignment: Qt.AlignHCenter
		}

		Text
		{
			text: "Login"
			visible: loginView.login.length === 0
			Layout.alignment: Qt.AlignHCenter
			anchors.bottom: nameField.top
			anchors.bottomMargin: 5
		}
		TextField
		{
			id: nameField;
			text: loginView.login
			visible: loginView.login.length === 0
			Layout.alignment: Qt.AlignHCenter
		}

		Text
		{
			id: passLabel
			text: nameField.visible ?
					  "Passphrase" : "Enter passphrase for " + loginView.login
			Layout.alignment: Qt.AlignHCenter
			anchors.bottom: passwordField.top
			anchors.bottomMargin: 5
		}
		TextField
		{
			id: passwordField
			text: loginView.password
			width: passLabel.width
			echoMode: TextInput.Password
			Layout.alignment: Qt.AlignHCenter
		}

		Button
		{
			id: bottomButton
			text: loginView.buttonText
			onClicked: loginView.submit(nameField.text, passwordField.text)
			Layout.alignment: Qt.AlignHCenter | Qt.AlignBottom
		}
	}
}
