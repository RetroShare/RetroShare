#ifndef IMAGEUTIL_H
#define IMAGEUTIL_H

#include <QTextCursor>
#include <QWidget>

class ImageUtil
{
public:
	ImageUtil();

	static void extractImage(QWidget *window, QTextCursor cursor);
};

#endif // IMAGEUTIL_H
