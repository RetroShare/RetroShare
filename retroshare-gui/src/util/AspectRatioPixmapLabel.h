/*******************************************************************************
 * retroshare-gui/src/util/AspectRatioPixmapLabel.h                             *
 *                                                                             *
 * Copyright (C) 2019  Retroshare Team       <retroshare.project@gmail.com>    *
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

#ifndef ASPECTRATIOPIXMAPLABEL_H
#define ASPECTRATIOPIXMAPLABEL_H

#include <QLabel>
#include <QPixmap>
#include <QResizeEvent>

class AspectRatioPixmapLabel : public QLabel
{
    Q_OBJECT
public:
	explicit AspectRatioPixmapLabel(QWidget *parent = nullptr);
	virtual int heightForWidth( int width ) const override;
	virtual QSize sizeHint() const override;
    QPixmap scaledPixmap() const;
public slots:
    void setPixmap ( const QPixmap & );
protected:
	void resizeEvent(QResizeEvent *event) override;
private:
    QPixmap pix;
};

#endif // ASPECTRATIOPIXMAPLABEL_H
