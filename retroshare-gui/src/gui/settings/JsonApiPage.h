/*
 * RetroShare JSON API
 * Copyright (C) 2018  Gioacchino Mazzurco <gio@eigenlab.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include <retroshare-gui/configpage.h>
#include "ui_JsonApiPage.h"

class JsonApiPage : public ConfigPage
{
	Q_OBJECT

public:

	JsonApiPage(QWidget * parent = nullptr, Qt::WindowFlags flags = 0);
	~JsonApiPage() {}

	/** Loads the settings for this page */
	virtual void load();

	virtual QPixmap iconPixmap() const
	{ return QPixmap(":/icons/svg/empty-circle.svg"); }
	virtual QString pageName() const { return tr("JSON API"); }
	virtual QString helpText() const;

	/** Call this after start of libretroshare/Retroshare
	 *  checks the settings and starts JSON API if required */
	static bool checkStartJsonApi();

	/** call this before shutdown of libretroshare
	 *  it stops the JSON API if its running */
	static void checkShutdownJsonApi();

public slots:
	void onApplyClicked(bool);
	void addTokenClicked(bool);
	void removeTokenClicked(bool);
	void tokenClicked(const QModelIndex& index);

private:
	Ui::JsonApiPage ui; /// Qt Designer generated object

	bool updateParams(QString &errmsg);
};
