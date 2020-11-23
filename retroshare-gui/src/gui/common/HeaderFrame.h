/*******************************************************************************
 * gui/common/HeaderFrame.h                                                    *
 *                                                                             *
 * Copyright (C) 2011, Retroshare Team <retroshare.project@gmail.com>          *
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

#ifndef HEADERFRAME_H
#define HEADERFRAME_H

#include <QFrame>

namespace Ui {
class HeaderFrame;
}

class HeaderFrame : public QFrame
{
	Q_OBJECT
    
public:
	HeaderFrame(QWidget *parent = 0);
	~HeaderFrame();

	void setHeaderText(const QString &headerText);
	void setHeaderImage(const QPixmap &headerImage);

private:
	Ui::HeaderFrame *ui;
};

#endif // HEADERFRAME_H
