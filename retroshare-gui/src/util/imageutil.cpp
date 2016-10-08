#include "imageutil.h"
#include "util/misc.h"

#include <QApplication>
#include <QByteArray>
#include <QImage>
#include <QMessageBox>
#include <QString>
#include <QTextCursor>
#include <QTextDocumentFragment>

ImageUtil::ImageUtil() {}

void ImageUtil::extractImage(QWidget *window, QTextCursor cursor)
{
	cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, 1);
	cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 2);
	QString imagestr = cursor.selection().toHtml();
	bool success = false;
	int start = imagestr.indexOf("base64,") + 7;
	int stop = imagestr.indexOf("\"", start);
	int length = stop - start;
	if((start >= 0) && (length > 0))
	{
		QByteArray ba = QByteArray::fromBase64(imagestr.mid(start, length).toLatin1());
		QImage image = QImage::fromData(ba);
		if(!image.isNull())
		{
			QString file;
			success = true;
			if(misc::getSaveFileName(window, RshareSettings::LASTDIR_IMAGES, "Save Picture File", "Pictures (*.png *.xpm *.jpg)", file))
			{
				if(!image.save(file, 0, 100))
					if(!image.save(file + ".png", 0, 100))
						QMessageBox::warning(window, QApplication::translate("ImageUtil", "Save image"), QApplication::translate("ImageUtil", "Cannot save the image, invalid filename"));
			}
		}
	}
	if(!success)
	{
		QMessageBox::warning(window, QApplication::translate("ImageUtil", "Save image"), QApplication::translate("ImageUtil", "Not an image"));
	}
}

