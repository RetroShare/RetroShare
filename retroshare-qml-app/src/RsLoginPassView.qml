/*
 * RetroShare Android QML App
 * Copyright (C) 2016-2017  Gioacchino Mazzurco <gio@eigenlab.org>
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

import QtQuick 2.7
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.0
import "components/."

Item
{
	id: loginView
	property string buttonText: qsTr("Unlock")
	property string cancelText: qsTr("Cancel")
	property string iconUrl: "qrc:/icons/emblem-locked.svg"
	property string login
	property bool loginPreset: false
	property bool advancedMode: false
	property string hardcodedPassword: "hardcoded default password"
	property string password: advancedMode ? "" : hardcodedPassword
	property string suggestionText
	signal submit(string login, string password)
	signal cancel()

	Component.onCompleted: loginPreset = login.length > 0

	ColumnLayout
	{
		id: inputView
		width: parent.width
		anchors.centerIn: parent

		Text
		{
			text: loginView.suggestionText
			visible: loginView.suggestionText.length > 0
			font.bold: true
			Layout.alignment: Qt.AlignHCenter
		}

		Image
		{
			source: loginView.iconUrl
			Layout.alignment: Qt.AlignHCenter
			height: 128
			sourceSize.height: height
		}

		Text
		{
			text: qsTr("Name")
			visible: !loginView.loginPreset
			Layout.alignment: Qt.AlignHCenter
			anchors.bottom: nameField.top
			anchors.bottomMargin: 5
		}
		TextField
		{
			id: nameField
			text: loginView.login
			visible: !loginView.loginPreset
			Layout.alignment: Qt.AlignHCenter

			ToolTip
			{
				text: qsTr("Choose a descriptive name, one<br/>" +
						   "that your friends can recognize.",
						   "The linebreak is to make the text fit better in " +
						   "tooltip")
				visible: nameField.activeFocus
				timeout: 5000
			}
		}

		Text
		{
			id: passLabel
			visible: loginView.advancedMode || loginView.loginPreset
			text: nameField.visible ?
					  qsTr("Password") :
					  qsTr("Enter password for %1").arg(loginView.login)
			Layout.alignment: Qt.AlignHCenter
			anchors.bottom: passwordField.top
			anchors.bottomMargin: 5
		}
		TextField
		{
			id: passwordField
			visible: loginView.advancedMode || loginView.loginPreset
			text: loginView.password
			echoMode: TextInput.Password
			Layout.alignment: Qt.AlignHCenter

			ToolTip
			{
				visible: passwordField.activeFocus && !loginView.loginPreset
				timeout: 5000
				text: qsTr("Choose a strong password and don't forget it,<br/>"+
						   "there is no way to recover lost password.",
						   "The linebreak is to make the text fit better in " +
						   "tooltip")
			}
		}

		Row
		{
			Layout.alignment: Qt.AlignHCenter | Qt.AlignBottom
			spacing: 3

			ButtonIcon
			{
				visible: !loginView.loginPreset
				onClicked: loginView.advancedMode = !loginView.advancedMode
				imgUrl: "/icons/options.svg"
				height: bottomButton.height - 5
				width: height
				anchors.verticalCenter: bottomButton.verticalCenter
			}

			ButtonText
			{
				id: bottomButton
				text: loginView.buttonText
				onClicked: loginView.submit(nameField.text, passwordField.text)
				iconUrl: "/icons/network.svg"
				buttonTextPixelSize: 15
			}
			ButtonIcon
			{
				id: cancelButton
				onClicked: loginView.cancel()
				height: bottomButton.height - 5
				width: height
				imgUrl: "/icons/back.png"
				anchors.verticalCenter: bottomButton.verticalCenter
			}
		}
	}
}
