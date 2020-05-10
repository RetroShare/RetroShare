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

	Q_PROPERTY(QColor textColorQuoteGreen READ textColorQuoteGreen WRITE setTextColorQuoteLevel1)
	Q_PROPERTY(QColor textColorQuoteBlue READ textColorQuoteBlue WRITE setTextColorQuoteLevel2)
	Q_PROPERTY(QColor textColorQuoteRed READ textColorQuoteRed WRITE setTextColorQuoteLevel3)
	Q_PROPERTY(QColor textColorQuoteMagenta READ textColorQuoteMagenta WRITE setTextColorQuoteLevel4)
	Q_PROPERTY(QColor textColorQuoteTurquoise READ textColorQuoteTurquoise WRITE setTextColorQuoteLevel5)
	Q_PROPERTY(QColor textColorQuotePurple READ textColorQuotePurple WRITE setTextColorQuoteLevel6)
	Q_PROPERTY(QColor textColorQuoteMaroon READ textColorQuoteMaroon WRITE setTextColorQuoteLevel7)
	Q_PROPERTY(QColor textColorQuoteOlive READ textColorQuoteOlive WRITE setTextColorQuoteLevel8)

public:
	explicit RSTextBrowser(QWidget *parent = 0);

	void setPlaceholderText(const QString &text);
	void setImageBlockWidget(RSImageBlockWidget *widget);
	void resetImagesStatus(bool load);
	QPixmap getBlockedImage();
	bool checkImage(QPoint pos, QString &imageStr);
	bool checkImage(QPoint pos) {QString imageStr; return checkImage(pos, imageStr); }
	QString anchorForPosition(const QPoint &pos) const;


	void activateLinkClick(bool active);

	virtual QVariant loadResource(int type, const QUrl &name);

	QColor textColorQuoteGreen() const { return highlighter->textColorQuoteGreen();}
	QColor textColorQuoteBlue() const { return highlighter->textColorQuoteBlue();}
	QColor textColorQuoteRed() const { return highlighter->textColorQuoteRed();}
	QColor textColorQuoteMagenta() const { return highlighter->textColorQuoteMagenta();}
	QColor textColorQuoteTurquoise() const { return highlighter->textColorQuoteTurquoise();}
	QColor textColorQuotePurple() const { return highlighter->textColorQuotePurple(); }
	QColor textColorQuoteMaroon() const { return highlighter->textColorQuoteMaroon(); }
	QColor textColorQuoteOlive() const { return highlighter->textColorQuoteOlive(); }

	bool getShowImages() const { return mShowImages; }

public slots:
	void showImages();
	void setTextColorQuoteLevel1(QColor textColorQuoteGreen){ highlighter->setTextColorQuoteLevel1(textColorQuoteGreen);}
	void setTextColorQuoteLevel2(QColor textColorQuoteBlue){ highlighter->setTextColorQuoteLevel2(textColorQuoteBlue);}
	void setTextColorQuoteLevel3(QColor textColorQuoteRed){ highlighter->setTextColorQuoteLevel3(textColorQuoteRed);}
	void setTextColorQuoteLevel4(QColor textColorQuoteMagenta){ highlighter->setTextColorQuoteLevel4(textColorQuoteMagenta);}
	void setTextColorQuoteLevel5(QColor textColorQuoteTurquoise){ highlighter->setTextColorQuoteLevel5(textColorQuoteTurquoise);}
	void setTextColorQuoteLevel6(QColor textColorQuotePurple){ highlighter->setTextColorQuoteLevel6(textColorQuotePurple);}
	void setTextColorQuoteLevel7(QColor textColorQuoteMaroon){ highlighter->setTextColorQuoteLevel7(textColorQuoteMaroon);}
	void setTextColorQuoteLevel8(QColor textColorQuoteOlive){ highlighter->setTextColorQuoteLevel8(textColorQuoteOlive);}

private slots:
	void linkClicked(const QUrl &url);
	void destroyImageBlockWidget();

protected:
	void paintEvent(QPaintEvent *event);

private:
	QString mPlaceholderText;
	bool mShowImages;
	RSImageBlockWidget *mImageBlockWidget;
	bool mLinkClickActive;
	RsSyntaxHighlighter *highlighter;
#ifdef RSTEXTBROWSER_CHECKIMAGE_DEBUG
	QRect mCursorRectStart;
	QRect mCursorRectLeft;
	QRect mCursorRectRight;
	QRect mCursorRectEnd;
#endif
};

#endif // RSTEXTBROWSER_H
