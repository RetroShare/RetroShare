import QtQuick 2.7
import QtQuick.Dialogs 1.2

import "../URI.js" as UriJs

Item
{
	id: compRoot

	property var resultFile

	FileDialog
	{
		id: fileDialog
		title: "Please choose a file"
		folder: shortcuts.pictures
		nameFilters: [ "Image files (*.png *.jpg)"]
		visible: false
		selectMultiple: false
		onAccepted: {
			console.log("You chose: " + fileDialog.fileUrl)
			resultFile = fileDialog.fileUrl
		}
		onRejected: {
			console.log("Canceled")
		}
	}


	function open()
	{
		if (Qt.platform.os === "android")
		{
			console.log("ImagePicker Android platform detected")
			mainWindow.addUriHandler("file", androidResult)
			androidImagePicker.openPicker()
		}
		else
		{
			fileDialog.visible = true
		}
	}

	function androidResult (uri)
	{
		console.log("QML Android image uri found" , uri)
		resultFile = normalizeUriToFilePath (uri)
		mainWindow.delUriHandler("media", androidResult)
	}

	function normalizeUriToFilePath (uriStr)
	{
		var uri = new UriJs.URI(uriStr)
		var hPath = uri.path()
		return "file:///"+hPath

	}


}
