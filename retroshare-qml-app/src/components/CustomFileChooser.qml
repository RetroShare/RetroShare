import QtQuick 2.7
import QtQuick.Dialogs 1.2

Item
{
	id: compRoot

	FileDialog
	{
		id: fileDialog
		title: "Please choose a file"
		folder: shortcuts.pictures
		nameFilters: [ "Image files (*.png *.jpg)"]
		visible: false
		onAccepted: {
			console.log("You chose: " + fileDialog.fileUrls)
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
			mainWindow.addUriHandler("/media", androidResult)
			androidImagePicker.openPicker()
		}
		else
		{
			fileDialog.visible = true
		}
	}

	function androidResult (uri)
	{
		console.log("Android image uri found" , uri)

	}


}
