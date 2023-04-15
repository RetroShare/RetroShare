/*******************************************************************************
 * gui/common/RSTextBrowser.h                                                  *
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

#ifndef RSTEXTBROWSER_H
#define RSTEXTBROWSER_H

#include <QTextBrowser>
#include "util/RsSyntaxHighlighter.h"

//#define RSTEXTBROWSER_CHECKIMAGE_DEBUG 1

class RSImageBlockWidget;

//cppcheck-suppress noConstructor
class RSTextBrowser : public QTextBrowser
{
	Q_OBJECT

	Q_PROPERTY(QColor textColorQuote READ textColorQuote WRITE setTextColorQuote)
	Q_PROPERTY(QVariant textColorQuotes READ textColorQuotes WRITE setTextColorQuotes)

public:
	explicit RSTextBrowser(QWidget *parent = 0);

	void append(const QString &text);

	void setPlaceholderText(const QString &text);
	void setImageBlockWidget(RSImageBlockWidget *widget);
	void resetImagesStatus(bool load);
	QPixmap getBlockedImage();
	bool checkImage(QPoint pos, QString &imageStr);
	bool checkImage(QPoint pos) {QString imageStr; return checkImage(pos, imageStr); }
	QString anchorForPosition(const QPoint &pos) const;

	// Add QAction to context menu (action won't be deleted)
	void addContextMenuAction(QAction *action);

	void activateLinkClick(bool active);

	virtual QVariant loadResource(int type, const QUrl &name);

	QColor textColorQuote() const { return highlighter->textColorQuote();}
	QVariant textColorQuotes() const { return highlighter->textColorQuotes();}
	bool getShowImages() const { return mShowImages; }

	QMenu *createStandardContextMenuFromPoint(const QPoint &widgetPos);

Q_SIGNALS:
	void calculateContextMenuActions();

public slots:
	void showImages();
	void setTextColorQuote(QColor textColorQuote) { highlighter->setTextColorQuote(textColorQuote);}
	void setTextColorQuotes(QVariant textColorQuotes) { highlighter->setTextColorQuotes(textColorQuotes);}

private slots:
	void linkClicked(const QUrl &url);
	void destroyImageBlockWidget();
	void viewSource();
	void saveImage();

protected:
	void paintEvent(QPaintEvent *event);
	virtual void contextMenuEvent(QContextMenuEvent *event);

private:
	// Hide method from QTextBrowser
	using QTextBrowser::createStandardContextMenu;

private:
	QString mPlaceholderText;
	bool mShowImages;
	RSImageBlockWidget *mImageBlockWidget;
	bool mLinkClickActive;
	RsSyntaxHighlighter *highlighter;
	QList<QAction*> mContextMenuActions;
#ifdef RSTEXTBROWSER_CHECKIMAGE_DEBUG
	QRect mCursorRectStart;
	QRect mCursorRectLeft;
	QRect mCursorRectRight;
	QRect mCursorRectEnd;
#endif
};

#endif // RSTEXTBROWSER_H
