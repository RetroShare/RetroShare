/*******************************************************************************
 * gui/common/RSTextBrowser.cpp                                                *
 *                                                                             *
 * Copyright (C) 2018 RetroShare Team <retroshare.project@gmail.com>           *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify        *
 * it under the terms of the GNU Affero General Public License as              *
 * published by the Free Software Foundation, either version 3 of the          *
 * License, or (at your option) any later version.                             *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                *
 * GNU Affero General Public License for more details.                         *
 *                                                                             *
 * You should have received a copy of the GNU Affero General Public License    *
 * along with this program. If not, see <https://www.gnu.org/licenses/>.       *
 *                                                                             *
 *******************************************************************************/

#include "RSTextBrowser.h"

#include "RSImageBlockWidget.h"
#include "gui/common/FilesDefs.h"
#include "util/imageutil.h"

#include <retroshare/rsinit.h> //To get RsAccounts

#include <QDesktopServices>
#include <QDialog>
#include <QDir>
#include <QGridLayout>
#include <QMenu>
#include <QPainter>
#include <QPlainTextEdit>
#include <QTextDocumentFragment>
#include <QScrollBar>

#include <iostream>

RSTextBrowser::RSTextBrowser(QWidget *parent) :
	QTextBrowser(parent)
{
	setOpenExternalLinks(true);
	setOpenLinks(false);

	mShowImages = true;
	mImageBlockWidget = NULL;
	mLinkClickActive = true;

	highlighter = new RsSyntaxHighlighter(this);

	connect(this, SIGNAL(anchorClicked(QUrl)), this, SLOT(linkClicked(QUrl)));
}

void RSTextBrowser::append(const QString &text)
{
	//In Win RSTextBrowser don't recognize file:///
	QString fileText = text;
	QTextBrowser::append(fileText.replace("file:///","file://"));

}

void RSTextBrowser::linkClicked(const QUrl &url)
{
	if (!mLinkClickActive) {
		return;
	}

	// some links are opened directly in the QTextBrowser with open external links set to true,
	// so we handle links by our own

#ifdef TO_DO
	// If we want extra file links to be anonymous, we need to insert the actual source here.
	if(url.host() == HOST_EXTRAFILE)
	{
		std::cerr << "Extra file link detected. Adding parent id " << _target_sslid << " to sourcelist" << std::endl;

		RetroShareLink link ;
		link.fromUrl(url) ;

		link.createExtraFile( link.name(),link.size(),link.hash(), _target_ssl_id) ;

		QDesktopServices::openUrl(link.toUrl());
	}
	else
#endif
		QDesktopServices::openUrl(url);
}

void RSTextBrowser::setPlaceholderText(const QString &text)
{
	mPlaceholderText = text;
	viewport()->repaint();
}

void RSTextBrowser::paintEvent(QPaintEvent *event)
{
	QTextBrowser::paintEvent(event);

	if (mPlaceholderText.isEmpty() == false && document()->isEmpty()) {
		QWidget *vieportWidget = viewport();
		QPainter painter(vieportWidget);

		QPen pen = painter.pen();
		QColor color = pen.color();
		color.setAlpha(128);
		pen.setColor(color);
		painter.setPen(pen);

		painter.drawText(QRect(QPoint(), vieportWidget->size()), Qt::AlignHCenter | Qt::AlignVCenter | Qt::TextWordWrap, mPlaceholderText);
	}
#ifdef RSTEXTBROWSER_CHECKIMAGE_DEBUG
	QPainter painter(viewport());
	QPen pen = painter.pen();
	pen.setWidth(2);
	pen.setColor(QColor(qRgba(255,0,0,128)));
	painter.setPen(pen);
	painter.drawRect(mCursorRectStart);
	pen.setColor(QColor(qRgba(0,255,0,128)));
	painter.setPen(pen);
	painter.drawRect(mCursorRectLeft);
	pen.setColor(QColor(qRgba(0,0,255,128)));
	painter.setPen(pen);
	painter.drawRect(mCursorRectRight);
	pen.setColor(QColor(qRgba(0,0,0,128)));
	painter.setPen(pen);
	painter.drawRect(mCursorRectEnd);
#endif
}

QVariant RSTextBrowser::loadResource(int type, const QUrl &name)
{
	// case 1: always trust the image if it comes from an internal resource.

	if(name.scheme().compare("qrc",Qt::CaseInsensitive)==0 && type == QTextDocument::ImageResource)
		return QTextBrowser::loadResource(type, name);

	// case 2: always trust the image if it comes from local Config or Data directories.

	if(type == QTextDocument::ImageResource)
	{
		QFileInfo fi = QFileInfo(name.path());
		if(fi.exists() && fi.isFile())
		{
			QString cpath = fi.canonicalFilePath();
			QStringList autPath = { QDir(QString::fromUtf8(RsAccounts::ConfigDirectory().c_str())).canonicalPath()
			                      , QDir(QString::fromUtf8(RsAccounts::systemDataDirectory().c_str())).canonicalPath()
			                      , QDir(QString::fromUtf8(RsAccounts::ConfigDirectory().c_str())+"/stylesheets/").canonicalPath() //May be link
			                      , QDir(QString::fromUtf8(RsAccounts::systemDataDirectory().c_str())+"/stylesheets/").canonicalPath() //May be link
			                      };
			for(auto& it : autPath)
				if (!it.isEmpty() && cpath.startsWith(it, Qt::CaseInsensitive))
					return QPixmap(fi.absoluteFilePath());
		}
	}

	// case 3: only display if the user allows it. Data resources can be bad (svg bombs) but we filter them out globally at the network layer.
	//         It would be good to add here a home-made resource loader that only loads images and not svg crap, just in case.

	if(name.scheme().compare("data",Qt::CaseInsensitive)==0 && mShowImages)
		return QTextBrowser::loadResource(type, name);

	// case 4: otherwise, do not display

    std::cerr << "TEXTBROWSER: refusing load ressource request: type=" << type << " scheme="
              << name.scheme().toStdString() << ", url="
              << name.toString().left(50).toStdString()
              << ((name.toString().length()>50)?"...":"")
              << std::endl;

	if (mImageBlockWidget)
		mImageBlockWidget->show();

	return getBlockedImage();
}

QPixmap RSTextBrowser::getBlockedImage()
{
    return FilesDefs::getPixmapFromQtResourcePath(":/images/imageblocked_24.png");
}

void RSTextBrowser::setImageBlockWidget(RSImageBlockWidget *widget)
{
	if (mImageBlockWidget) {
		// disconnect
		disconnect(mImageBlockWidget, SIGNAL(destroyed()), this, SLOT(destroyImageBlockWidget()));
		disconnect(mImageBlockWidget, SIGNAL(showImages()), this, SLOT(showImages()));
	}

	mImageBlockWidget = widget;

	if (mImageBlockWidget) {
		// connect
		connect(mImageBlockWidget, SIGNAL(destroyed()), this, SLOT(destroyImageBlockWidget()));
		connect(mImageBlockWidget, SIGNAL(showImages()), this, SLOT(showImages()));
	}

	resetImagesStatus(false);
}

void RSTextBrowser::destroyImageBlockWidget()
{
	mImageBlockWidget = NULL;
}

void RSTextBrowser::showImages()
{
	if (mImageBlockWidget && sender() == mImageBlockWidget) {
		mImageBlockWidget->hide();
	}

	if (mShowImages) {
		return;
	}

	mShowImages = true;

	QString html = toHtml();
	clear();
	setHtml(html);
}

void RSTextBrowser::resetImagesStatus(bool load)
{
	if (mImageBlockWidget) {
		mImageBlockWidget->hide();
	}
	mShowImages = load;
}

void RSTextBrowser::activateLinkClick(bool active)
{
	mLinkClickActive = active;
}

/**
 * @brief RSTextBrowser::checkImage
 * @param pos where to check if image is shown in viewport coordinate
 * @param imageStr return html source of cursor
 * @return True if an image is under cursor
 */
bool RSTextBrowser::checkImage(QPoint pos, QString &imageStr)
{
	//Get text cursor under pos. But if pos is under text browser end line this return last cursor.
	QTextCursor cursor = cursorForPosition(pos);
	//First get rect of cursor (could be at left or right of image)
	QRect cursorRectStart = cursorRect(cursor);
	//Second get text
	cursor.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, 1);//To get character just before
	QRect cursorRectLeft = cursorRect(cursor);
	cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, 2);
	QRect cursorRectRight = cursorRect(cursor);
	imageStr = cursor.selection().toHtml();
#ifdef RSTEXTBROWSER_CHECKIMAGE_DEBUG
	mCursorRectStart = cursorRectStart;
	mCursorRectLeft = cursorRectLeft;
	mCursorRectRight = cursorRectRight;

	std::cerr << "cursorRect LTRB :" << cursorRectStart.left() << ";" << cursorRectStart.top() << ";" << cursorRectStart.right() << ";" << cursorRectStart.bottom() << std::endl;
	std::cerr << "cursorRectLeft  :" << cursorRectLeft.left() << ";" << cursorRectLeft.top() << ";" << cursorRectLeft.right() << ";" << cursorRectLeft.bottom() << std::endl;
	std::cerr << "cursorRectRight :" << cursorRectRight.left() << ";" << cursorRectRight.top() << ";" << cursorRectRight.right() << ";" << cursorRectRight.bottom() << std::endl;
	std::cerr << "pos XY          :" << pos.x() << ";" << pos.y() << std::endl;
#endif
	QRect cursorRectEnd = cursorRectStart;
	//Finally set left with right of precedent character.
	if (cursorRectEnd.top() < cursorRectLeft.bottom())
	{
		cursorRectEnd.setLeft(cursorRectLeft.right());
	} else {
		//Image on new line
		cursorRectEnd.setLeft(0);
	}
	//And set Right with left of next character.
	if (cursorRectEnd.bottom() > cursorRectRight.top())
	{
		cursorRectEnd.setRight(cursorRectRight.left());
	} else {
		//New line after Image.
	}
#ifdef RSTEXTBROWSER_CHECKIMAGE_DEBUG
	mCursorRectEnd = cursorRectEnd;

	std::cerr << "final cursorRect:" << cursorRectEnd.left() << ";" << cursorRectEnd.top() << ";" << cursorRectEnd.right() << ";" << cursorRectEnd.bottom() << std::endl;
	viewport()->update();
#endif
	//If pos is on text rect
	if (cursorRectEnd.contains(pos))
	{
		return imageStr.indexOf("base64,") != -1;
	}
	return false;
}

/**
 * @brief RSTextBrowser::anchorForPosition Replace anchorAt that doesn't works as expected.
 * @param pos Where to get anchor from text
 * @return anchor If text at pos is inside anchor, else empty string.
 */
QString RSTextBrowser::anchorForPosition(const QPoint &pos) const
{
	QTextCursor cursor = cursorForPosition(pos);
	cursor.select(QTextCursor::WordUnderCursor);
	QString anchor = "";
	if (!cursor.selectedText().isEmpty()){
		QRegExp rx("<a name=\"(.*)\"",Qt::CaseSensitive, QRegExp::RegExp2);
		rx.setMinimal(true);
		QString sel = cursor.selection().toHtml();
		QStringList anchors;
		int posX=0;
		while ((posX = rx.indexIn(sel,posX)) != -1) {
			anchors << rx.cap(1);
			posX += rx.matchedLength();
		}
		if (!anchors.isEmpty()){
			anchor = anchors.at(0);
		}
	}
	return anchor;
}

void RSTextBrowser::addContextMenuAction(QAction *action)
{
	mContextMenuActions.push_back(action);
}

void RSTextBrowser::contextMenuEvent(QContextMenuEvent *event)
{
	emit calculateContextMenuActions();

	QMenu *contextMenu = createStandardContextMenuFromPoint(event->pos());

	QList<QAction*>::iterator it;
	for (it = mContextMenuActions.begin(); it != mContextMenuActions.end(); ++it) {
		contextMenu->addAction(*it);
	}

	contextMenu->exec(QCursor::pos());

	delete(contextMenu);
}

QMenu *RSTextBrowser::createStandardContextMenuFromPoint(const QPoint &widgetPos)
{
	QMatrix matrix;
	matrix.translate(horizontalScrollBar()->value(), verticalScrollBar()->value());

	QMenu *menu = QTextBrowser::createStandardContextMenu(matrix.map(widgetPos));

	menu->addSeparator();
	QAction *a = menu->addAction(FilesDefs::getIconFromQtResourcePath("://icons/textedit/code.png"), tr("View &Source"), this, SLOT(viewSource()));
	a->setEnabled(!this->document()->isEmpty());

	if (checkImage(widgetPos)) {
		a = menu->addAction(FilesDefs::getIconFromQtResourcePath(":/images/document_save.png"), tr("Save image"), this, SLOT(saveImage()));
		a->setData(widgetPos);
	}

	return menu;
}

void RSTextBrowser::viewSource()
{
	QString text = this->textCursor().selection().toHtml();
	if (text.isEmpty())
		text = this->document()->toHtml();

	QDialog *dialog = new QDialog(this);
	QPlainTextEdit *pte = new QPlainTextEdit(dialog);
	pte->setPlainText(text);
	QGridLayout *gl = new QGridLayout(dialog);
	gl->addWidget(pte,0,0,1,1);
	dialog->setWindowTitle(tr("Document source"));
	dialog->resize(500, 400);

	dialog->exec();

	delete dialog;
}

void RSTextBrowser::saveImage()
{
	QAction *action = dynamic_cast<QAction*>(sender()) ;
	if (!action) {
		return;
	}

	QPoint point = action->data().toPoint();
	QTextCursor cursor = cursorForPosition(point);
	ImageUtil::extractImage(window(), cursor);
}
