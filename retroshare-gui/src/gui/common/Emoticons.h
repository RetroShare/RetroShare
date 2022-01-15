/*******************************************************************************
 * gui/common/Emoticons.h                                                      *
 *                                                                             *
 * Copyright (C) 2010, Retroshare Team <retroshare.project@gmail.com>          *
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

#pragma once

#include <QHash>
#include <QVector>

class QPixmap;
class QString;
class QWidget;

class Emoticons
{
public:
	void load();
	void showSmileyWidget(QWidget *parent, QWidget *button, const char *slotAddMethod, bool above);
	void showStickerWidget(QWidget *parent, QWidget *button, const char *slotAddMethod, bool above);
	QString importedStickerPath();
	Emoticons *operator->() const;

private:
	void loadToolTips(QWidget *container);
	void loadSmiley();
	void refreshStickerTabs(QVector<QString>& stickerTabs, QString foldername);
	void refreshStickerTabs(QVector<QString>& stickerTabs);

private:
	QHash<QString, QPair<QVector<QString>, QHash<QString, QString> > > m_smileys;
	QVector<QString> m_grpOrdered;
	QStringList m_filters;
	QStringList m_stickerFolders;
	QHash<QString, QString> m_tooltipcache;
	QHash<QString, QPixmap> m_iconcache;
};
