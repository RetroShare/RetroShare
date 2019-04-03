/*******************************************************************************
 * gui/settings/PostedPage.h                                                   *
 *                                                                             *
 * Copyright 2014 Retroshare Team  <retroshare.project@gmail.com>              *
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

#ifndef POSTEDPAGE_H
#define POSTEDPAGE_H

#include <retroshare-gui/configpage.h>

namespace Ui {
class PostedPage;
}

class PostedPage : public ConfigPage
{
	Q_OBJECT

public:
	PostedPage(QWidget * parent = 0, Qt::WindowFlags flags = 0);
	~PostedPage();

	/** Loads the settings for this page */
	virtual void load();

	virtual QPixmap iconPixmap() const { return QPixmap(":/icons/settings/posted.svg") ; }
	virtual QString pageName() const { return tr("Links") ; }
	virtual QString helpText() const { return ""; }

private:
	Ui::PostedPage *ui;
};

#endif // !POSTEDPAGE_H

